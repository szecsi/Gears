#include "stdafx.h"
#include "SequenceRenderer.h"
#pragma region OpenGL
#include <GL/glew.h>
#pragma endregion includes for GLEW
#include <fstream>
#include <sstream>

#include <ctime>
#include <limits>
#include "SpatialFilterRenderer.h"


SpatialFilterRenderer::SpatialFilterRenderer(SequenceRenderer::P sequenceRenderer, SpatialFilter::P spatialFilter, ShaderManager::P shaderManager, KernelManager::P kernelManager):
	sequenceRenderer(sequenceRenderer), useFft(false),
	spatialFilter(spatialFilter)
{
	if(spatialFilter)
	{
		spatialKernelId = kernelManager->getKernel(spatialFilter);
		useFft = spatialFilter->useFft;
	}

	convolutionShader = shaderManager->loadShader( R"GLSLC0D3(
			#version 150 compatibility
			#extension GL_ARB_texture_rectangle : enable
			precision highp float;
			uniform sampler2DRect kernel;
			uniform sampler2DRect stim;
			uniform ivec2 fftSize;
			uniform bool showFft;
			in vec2 fTexCoord;
			out vec4 outcolor;
//			void main() { vec4 k =  texture2DRect(kernel, (ivec2(gl_FragCoord.xy) + fftSize/2)%fftSize);
	void main() {
				vec4 k =  texture2DRect(kernel, gl_FragCoord.xy);
//				vec4 k =  vec4(1, 0, 1, 0);
//				vec4 s =  vec4(1, 0, 1, 0);
				vec4 s =  texture2DRect(stim, gl_FragCoord.xy );
				outcolor = vec4(k.x*s.x - k.y*s.y, k.x*s.y + k.y*s.x, k.z*s.z - k.w*s.w, k.z*s.w + k.w*s.z);
//				int cq = (int(gl_FragCoord.x) % 2) ^ (int(gl_FragCoord.y) % 2);
//				if(showFft && (cq == 1))
//					outcolor = -outcolor;

//		 		outcolor = vec4(s.x, s.y, s.z, s.w);
//		 		outcolor = vec4(k.x, k.y, k.z, k.w);
//				outcolor = vec4(1, 1, 0, 0);
			}
		)GLSLC0D3"
		);

	spatialDomainConvolutionShader = shaderManager->loadShader(spatialFilter->spatialDomainConvolutionShaderSource);

	copyShader = shaderManager->loadShader( R"GLSLC0D3(
			#version 150 compatibility
	    	#extension GL_ARB_texture_rectangle : enable
			precision highp float;
			uniform ivec2 offset;
			uniform ivec2 fftSize;
			uniform vec2 pixelRatio;
			uniform float pixelArea;
			uniform sampler2DRect srcrg;
			uniform sampler2DRect srcba;
			in vec2 fTexCoord;
			out vec4 outcolor;
			void main() { vec2 uv = gl_FragCoord.xy * pixelRatio;
				uv = mod(uv + offset, vec2(fftSize)) + vec2(1, 1) ;
				outcolor = vec4(texture2DRect(srcrg, uv).xz, texture2DRect(srcba, uv).xz) * pixelArea;
//				vec2 tc = (uv + vec2(2, 2)) * textureSize(src) ;
//				int cq = (int(tc.x) % 2) ^ (int(tc.y) % 2);
//				if(cq == 0) outcolor = -outcolor;
	}
		)GLSLC0D3"
		);
}

void SpatialFilterRenderer::renderFrame(std::function<void()> renderStimulus)
{
	if(spatialFilter->useFft)
	{

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		spatialFilter->fftSwizzleMask = 0x00020000;
		sequenceRenderer->fft2FrequencyDomain[0]->set_input( renderStimulus );
		sequenceRenderer->fft2FrequencyDomain[0]->redraw_input();
		if(!spatialFilter->stimulusGivenInFrequencyDomain)
			sequenceRenderer->fft2FrequencyDomain[0]->do_fft();

		unsigned int freqTexId[2];
		freqTexId[0] = sequenceRenderer->fft2FrequencyDomain[0]->get_output();

		sequenceRenderer->fft2SpatialDomain[0]->set_input( [&] () {
			convolutionShader->enable();
			convolutionShader->bindUniformBool("showFft", spatialFilter->showFft);
			convolutionShader->bindUniformTextureRect("kernel", spatialKernelId, 0);
			convolutionShader->bindUniformTextureRect("stim", freqTexId[0], 1);
			convolutionShader->bindUniformInt2("fftSize", 
				sequenceRenderer->sequence->fftWidth_px,
				sequenceRenderer->sequence->fftHeight_px);
			sequenceRenderer->getNothing()->renderQuad();
			convolutionShader->disable();
			});

		if(!sequenceRenderer->getSequence()->isMonochrome())
		{
			spatialFilter->fftSwizzleMask = 0x00000406;
			sequenceRenderer->fft2FrequencyDomain[1]->set_input( renderStimulus );
			sequenceRenderer->fft2FrequencyDomain[1]->redraw_input();
			if(!spatialFilter->stimulusGivenInFrequencyDomain)
				sequenceRenderer->fft2FrequencyDomain[1]->do_fft();

			freqTexId[1] = sequenceRenderer->fft2FrequencyDomain[1]->get_output();

			sequenceRenderer->fft2SpatialDomain[1]->set_input( [&] () {
				convolutionShader->enable();
				convolutionShader->bindUniformBool("showFft", spatialFilter->showFft);
				convolutionShader->bindUniformTextureRect("kernel", spatialKernelId, 0);
				convolutionShader->bindUniformTextureRect("stim", freqTexId[1], 1);
				convolutionShader->bindUniformInt2("fftSize", 
					sequenceRenderer->sequence->fftWidth_px,
					sequenceRenderer->sequence->fftHeight_px);
				sequenceRenderer->getNothing()->renderQuad();
				convolutionShader->disable();
				});
		}

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		sequenceRenderer->fft2SpatialDomain[0]->redraw_input();
		if(!spatialFilter->showFft)
			sequenceRenderer->fft2SpatialDomain[0]->do_fft();

		if(!sequenceRenderer->getSequence()->isMonochrome())
		{
			sequenceRenderer->fft2SpatialDomain[1]->redraw_input();
			if(!spatialFilter->showFft)
				sequenceRenderer->fft2SpatialDomain[1]->do_fft();
		}

		if(sequenceRenderer->sequence->getMaxTemporalProcessingStateCount() > 0)
		{
			sequenceRenderer->nextTemporalProcessingState->setRenderTargets( );
			glViewport(
						0,
						0,
						sequenceRenderer->sequence->fieldWidth_px,
						sequenceRenderer->sequence->fieldHeight_px);

		}
		else if(sequenceRenderer->sequence->getMaxMemoryLength() > 0)
		{
			sequenceRenderer->textureQueue->setRenderTarget( sequenceRenderer->currentSlice );
			glViewport(
						0,
						0,
						sequenceRenderer->sequence->fieldWidth_px,
						sequenceRenderer->sequence->fieldHeight_px);

		}
		else
			glViewport( sequenceRenderer->sequence->fieldLeft_px, sequenceRenderer->sequence->fieldBottom_px, sequenceRenderer->sequence->fieldWidth_px, sequenceRenderer->sequence->fieldHeight_px);

		copyShader->enable();

//		copyShader->bindUniformInt2("offset", 
////			sequenceRenderer->sequence->fftWidth_px * sequenceRenderer->sequence->getMaxKernelWidth_um() / sequenceRenderer->sequence->getSpatialFilteredFieldWidth_um() / 2,
////			sequenceRenderer->sequence->fftHeight_px * sequenceRenderer->sequence->getMaxKernelHeight_um() / sequenceRenderer->sequence->getSpatialFilteredFieldHeight_um() / 2 );
////			-512,
////			-384 
//			-700,
//			-1000
//			);

		copyShader->bindUniformInt2("fftSize", 
			sequenceRenderer->sequence->fftWidth_px,
			sequenceRenderer->sequence->fftHeight_px);
		copyShader->bindUniformInt2("offset", 
			(int) (sequenceRenderer->sequence->fftWidth_px * (1.0 - sequenceRenderer->sequence->fieldWidth_um / sequenceRenderer->sequence->getSpatialFilteredFieldWidth_um() / 2)),
			(int) (sequenceRenderer->sequence->fftHeight_px * (1.0 - sequenceRenderer->sequence->fieldHeight_um / sequenceRenderer->sequence->getSpatialFilteredFieldHeight_um() / 2))
			);

		copyShader->bindUniformFloat("pixelArea", sequenceRenderer->sequence->getSpatialFilteredFieldWidth_um() * sequenceRenderer->sequence->getSpatialFilteredFieldHeight_um() 
			/ (float)sequenceRenderer->sequence->fftWidth_px / (float)sequenceRenderer->sequence->fftHeight_px);

		double que = (double)sequenceRenderer->sequence->fftWidth_px * sequenceRenderer->sequence->fieldWidth_um / 
			(sequenceRenderer->sequence->fieldWidth_px * sequenceRenderer->sequence->getSpatialFilteredFieldWidth_um());
		double vad = 
			(double)sequenceRenderer->sequence->fftHeight_px * sequenceRenderer->sequence->fieldHeight_um
			/ (sequenceRenderer->sequence->getSpatialFilteredFieldHeight_um() * sequenceRenderer->sequence->fieldHeight_px);
		copyShader->bindUniformFloat2("pixelRatio", 
			(double)sequenceRenderer->sequence->fftWidth_px * sequenceRenderer->sequence->fieldWidth_um / 
			(sequenceRenderer->sequence->fieldWidth_px * sequenceRenderer->sequence->getSpatialFilteredFieldWidth_um()) ,
			(double)sequenceRenderer->sequence->fftHeight_px * sequenceRenderer->sequence->fieldHeight_um
			/ (sequenceRenderer->sequence->getSpatialFilteredFieldHeight_um() * sequenceRenderer->sequence->fieldHeight_px) );

		copyShader->bindUniformTextureRect("srcrg", sequenceRenderer->fft2SpatialDomain[0]->get_output(), 0);
		if(!sequenceRenderer->getSequence()->isMonochrome())
			copyShader->bindUniformTextureRect("srcba", sequenceRenderer->fft2SpatialDomain[1]->get_output(), 1);

		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		sequenceRenderer->getNothing()->renderQuad();

		copyShader->disable();

		if(sequenceRenderer->sequence->getMaxTemporalProcessingStateCount() > 0)
		{
			sequenceRenderer->nextTemporalProcessingState->disableRenderTargets();
		}
		else if(sequenceRenderer->sequence->getMaxMemoryLength() > 0)
		{
			sequenceRenderer->textureQueue->disableRenderTarget();
		}

		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}
	else
	{
		sequenceRenderer->spatialDomainFilteringBuffers[0]->setRenderTarget(0);
		renderStimulus();
		sequenceRenderer->spatialDomainFilteringBuffers[0]->disableRenderTarget();

		///////////////
		glViewport(
			0,
			0,
			sequenceRenderer->sequence->fieldWidth_px,
			sequenceRenderer->sequence->fieldHeight_px);

		if (spatialFilter->separable)
		{
			sequenceRenderer->spatialDomainFilteringBuffers[1]->setRenderTarget(0);
			spatialDomainConvolutionShader->enable();

			

			spatialDomainConvolutionShader->bindUniformTexture("original", sequenceRenderer->spatialDomainFilteringBuffers[0]->getColorBuffer(0), 0);
			spatialDomainConvolutionShader->bindUniformTexture("kernel", spatialKernelId, 1);
			spatialDomainConvolutionShader->bindUniformFloat2("patternSizeOnRetina", sequenceRenderer->sequence->fieldWidth_um, sequenceRenderer->sequence->fieldHeight_um);
			spatialDomainConvolutionShader->bindUniformFloat2("kernelSizeOnRetina", spatialFilter->width_um, spatialFilter->height_um);
			spatialDomainConvolutionShader->bindUniformFloat2("step", 0, spatialFilter->height_um / 17.0f);
			spatialDomainConvolutionShader->bindUniformBool("combine", false);

			sequenceRenderer->getNothing()->renderQuad();
			spatialDomainConvolutionShader->disable();

			sequenceRenderer->spatialDomainFilteringBuffers[1]->disableRenderTarget();
		}

		///////////////

		if(sequenceRenderer->sequence->getMaxTemporalProcessingStateCount() > 0)
		{
			sequenceRenderer->nextTemporalProcessingState->setRenderTargets( );
		}
		else if(sequenceRenderer->sequence->getMaxMemoryLength() > 0)
		{
			sequenceRenderer->textureQueue->setRenderTarget( sequenceRenderer->currentSlice );
		}

		spatialDomainConvolutionShader->enable();

		glViewport(
			0,
			0,
			sequenceRenderer->sequence->fieldWidth_px,
			sequenceRenderer->sequence->fieldHeight_px);
		//glViewport( sequenceRenderer->sequence->fieldLeft_px, sequenceRenderer->sequence->fieldBottom_px, sequenceRenderer->sequence->fieldWidth_px, sequenceRenderer->sequence->fieldHeight_px);

		if (spatialFilter->separable)
			spatialDomainConvolutionShader->bindUniformTexture("original", sequenceRenderer->spatialDomainFilteringBuffers[1]->getColorBuffer(0), 0);
		else
			spatialDomainConvolutionShader->bindUniformTexture("original", sequenceRenderer->spatialDomainFilteringBuffers[0]->getColorBuffer(0), 0);
		spatialDomainConvolutionShader->bindUniformTexture("kernel", spatialKernelId, 1);
		spatialDomainConvolutionShader->bindUniformFloat2("patternSizeOnRetina", sequenceRenderer->sequence->fieldWidth_um, sequenceRenderer->sequence->fieldHeight_um);
		spatialDomainConvolutionShader->bindUniformFloat2("kernelSizeOnRetina", spatialFilter->width_um, spatialFilter->height_um);
		spatialDomainConvolutionShader->bindUniformFloat2("step", spatialFilter->width_um / 17.0f, 0.f);
		spatialDomainConvolutionShader->bindUniformBool("combine", true);
		sequenceRenderer->getNothing()->renderQuad();
		spatialDomainConvolutionShader->disable();

		if(sequenceRenderer->sequence->getMaxTemporalProcessingStateCount() > 0)
		{
			sequenceRenderer->nextTemporalProcessingState->disableRenderTargets();
		}
		else if(sequenceRenderer->sequence->getMaxMemoryLength() > 0)
		{
			sequenceRenderer->textureQueue->disableRenderTarget();
		}
	}

}

