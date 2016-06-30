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

	convolutionShader = shaderManager->loadShader(
		"	#version 150 compatibility																		  \n"
		"	#extension GL_ARB_texture_rectangle : enable            \n"
		"	precision highp float;																  \n"
		"	uniform sampler2DRect kernel;															  \n"
		"	uniform sampler2DRect stim;																  \n"
		"	uniform ivec2 fftSize;																  \n"
		"	in vec2 fTexCoord;																	  \n"
		"	out vec4 outcolor;																	  \n"
//		"	void main() { vec4 k =  texture2DRect(kernel, (ivec2(gl_FragCoord.xy) + fftSize/2)%fftSize);							\n"
"	void main() { vec4 k =  texture2DRect(kernel, gl_FragCoord.xy);							\n"
//		"		vec4 s =  texture2DRect(stim, (ivec2(gl_FragCoord.xy) + fftSize/2)%fftSize );										\n"
		"		vec4 s =  texture2DRect(stim, gl_FragCoord.xy );										\n"
		"		outcolor = vec4(k.x*s.x - k.y*s.y, k.x*s.y + k.y*s.x, k.z*s.z - k.w*s.w, k.z*s.w + k.w*s.z); }					\n"
//		 "		outcolor = vec4(s.x, s.y, s.z, s.w); }					\n"
//		 "		outcolor = vec4(k.x, k.y, k.z, k.w); }					\n"
//		"		outcolor = vec4(1, 1, 0, 0); }					\n"

		);

	spatialDomainConvolutionShader = shaderManager->loadShader(
		"	#version 150 compatibility																		  \n"
		"	#extension GL_ARB_texture_rectangle : enable            \n"
		"	precision highp float;																  \n"
		"	uniform sampler2D original;																\n"
		"	uniform sampler2D kernel;															  \n"
		"	uniform vec2 patternSizeOnRetina;																\n"
		"	uniform vec2 kernelSizeOnRetina;																\n"
		"	uniform vec2 step;																\n"
		"	uniform bool combine;																\n"
		"	in vec2 fTexCoord;																	\n"
		"	out vec4 outcolor;																	\n"
		"	void main() {																		 \n"
//		"			outcolor = texture2D(kernel, fTexCoord) * vec4(1,1, 1, 1); 								\n"
		"		outcolor = vec4(0, 0, 0, 0);																		 \n"
		"		for(float i=-8; i<=8.01; i+=1.0)																		\n"
		"		{																		\n"
		"			vec2 sample = step * i;																		\n"
		"			if(combine)			\n"
		"				outcolor += texture2D(original, fTexCoord + sample / patternSizeOnRetina).rgba				\n"
		"					* texture2D(kernel, vec2(0.5, 0.5) - sample / kernelSizeOnRetina).xxyy; 								\n"
		"			else			\n"
		"				outcolor += texture2D(original, fTexCoord + sample / patternSizeOnRetina).gbgb				\n"
//		"				outcolor += vec4(1, 1, 1, 1)				\n"
		"					* texture2D(kernel, vec2(0.5, 0.5) - sample / kernelSizeOnRetina).xxyy; 								\n"
		"		}																		\n"
		"			if(combine)			\n"
		"				outcolor = vec4(-outcolor.rgrg  + outcolor.baba ) * 50;					\n"
//		"				outcolor = (outcolor.ba ).rrgg;					\n"
//		"				outcolor = (outcolor.rg).rrgg;					\n"
		"	}								\n"
		);

	copyShader = shaderManager->loadShader(
		"	#version 150 compatibility																		\n"
	    "	#extension GL_ARB_texture_rectangle : enable            \n"
		"	precision highp float;																  \n"
		"	uniform ivec2 offset;																\n"
		"	uniform ivec2 fftSize;																\n"
		"	uniform vec2 pixelRatio;																\n"
		"	uniform float pixelArea;																\n"
		"	uniform sampler2DRect src;																\n"
		"	in vec2 fTexCoord;																	\n"
		"	out vec4 outcolor;																	\n"
//		"	void main() { outcolor = abs(texture2DRect(src, ivec2((int(gl_FragCoord.x) + 256)%1024, (int(gl_FragCoord.y) + 256) % 1024) /*+ offset*/ )); }								\n"
		"	void main() { vec2 uv = gl_FragCoord.xy * pixelRatio; \n"
//		"		uv = vec2((ivec2(uv) + fftSize/2)%fftSize) + vec2(0.5, 0.5);																\n"
		"		uv = mod(uv + offset, vec2(fftSize)) + vec2(0.5, 0.5);																\n"
		"		outcolor = texture2DRect(src, uv).xxzz * pixelArea; }								\n"
//		"	void main() { outcolor = vec4(0.0, 0, 0, 0) + vec4(length(texture2DRect(src, gl_FragCoord.xy - vec2(750,750) ) ), 0, 0, 0); }								\n"
//		"	void main() { outcolor = texture2DRect(src, gl_FragCoord.xy*2); }								\n"
		);
}

void SpatialFilterRenderer::renderFrame(std::function<void()> renderStimulus, bool useFft)
{
	if(useFft)
	{
		sequenceRenderer->fft2FrequencyDomain->set_input( renderStimulus );

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		sequenceRenderer->fft2FrequencyDomain->redraw_input();
		sequenceRenderer->fft2FrequencyDomain->do_fft();

		unsigned int freqTexId = sequenceRenderer->fft2FrequencyDomain->get_output();

		sequenceRenderer->fft2SpatialDomain->set_input( [&] () {
			convolutionShader->enable();
			convolutionShader->bindUniformTextureRect("kernel", spatialKernelId, 0);
			convolutionShader->bindUniformTextureRect("stim", freqTexId, 1);
			convolutionShader->bindUniformInt2("fftSize", 
				sequenceRenderer->sequence->fftWidth_px,
				sequenceRenderer->sequence->fftHeight_px);

			//sequenceRenderer->fullscreenQuad->render(convolutionShader);
			sequenceRenderer->getNothing()->renderQuad();
			convolutionShader->disable();
		});


		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		sequenceRenderer->fft2SpatialDomain->redraw_input();
		sequenceRenderer->fft2SpatialDomain->do_fft();

		if(sequenceRenderer->sequence->getMaxMemoryLength() > 0)
		{
			sequenceRenderer->textureQueue->setRenderTarget( sequenceRenderer->currentSlice );
			glViewport(
						0,
						0,
						sequenceRenderer->sequence->fieldWidth_px,
						sequenceRenderer->sequence->fieldHeight_px);

		}
		else if(sequenceRenderer->sequence->getMaxTemporalProcessingStateCount() > 0)
		{
			sequenceRenderer->nextTemporalProcessingState->setRenderTargets( );
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
////			sequenceRenderer->sequence->fftWidth_px * sequenceRenderer->sequence->getMaxKernelWidth_um() / sequenceRenderer->sequence->getSpatialKernelWidth_um() / 2,
////			sequenceRenderer->sequence->fftHeight_px * sequenceRenderer->sequence->getMaxKernelHeight_um() / sequenceRenderer->sequence->getSpatialKernelHeight_um() / 2 );
////			-512,
////			-384 
//			-700,
//			-1000
//			);

		copyShader->bindUniformInt2("fftSize", 
			sequenceRenderer->sequence->fftWidth_px,
			sequenceRenderer->sequence->fftHeight_px);
		copyShader->bindUniformInt2("offset", 
			sequenceRenderer->sequence->fftWidth_px * (1.0 - sequenceRenderer->sequence->fieldWidth_um / sequenceRenderer->sequence->getSpatialKernelWidth_um() / 2),
			sequenceRenderer->sequence->fftHeight_px * (1.0 - sequenceRenderer->sequence->fieldHeight_um / sequenceRenderer->sequence->getSpatialKernelHeight_um() / 2) 
			);

		copyShader->bindUniformFloat("pixelArea", sequenceRenderer->sequence->getSpatialKernelWidth_um() * sequenceRenderer->sequence->getSpatialKernelHeight_um() 
			/ (float)sequenceRenderer->sequence->fftWidth_px / (float)sequenceRenderer->sequence->fftHeight_px);

		double que = (double)sequenceRenderer->sequence->fftWidth_px * sequenceRenderer->sequence->fieldWidth_um / 
			(sequenceRenderer->sequence->fieldWidth_px * sequenceRenderer->sequence->getSpatialKernelWidth_um());
		double vad = 
			(double)sequenceRenderer->sequence->fftHeight_px * sequenceRenderer->sequence->fieldHeight_um
			/ (sequenceRenderer->sequence->getSpatialKernelHeight_um() * sequenceRenderer->sequence->fieldHeight_px);
		copyShader->bindUniformFloat2("pixelRatio", 
			(double)sequenceRenderer->sequence->fftWidth_px * sequenceRenderer->sequence->fieldWidth_um / 
			(sequenceRenderer->sequence->fieldWidth_px * sequenceRenderer->sequence->getSpatialKernelWidth_um()) ,
			(double)sequenceRenderer->sequence->fftHeight_px * sequenceRenderer->sequence->fieldHeight_um
			/ (sequenceRenderer->sequence->getSpatialKernelHeight_um() * sequenceRenderer->sequence->fieldHeight_px) );

		

	//	copyShader->bindUniformTextureRect("src", spatialKernelId, 0);

		copyShader->bindUniformTextureRect("src", sequenceRenderer->fft2SpatialDomain->get_output(), 0);

		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		sequenceRenderer->getNothing()->renderQuad();

		copyShader->disable();

		if(sequenceRenderer->sequence->getMaxMemoryLength() > 0)
		{
			sequenceRenderer->textureQueue->disableRenderTarget();
		}
		else if(sequenceRenderer->sequence->getMaxTemporalProcessingStateCount() > 0)
		{
			sequenceRenderer->nextTemporalProcessingState->disableRenderTargets();
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

		sequenceRenderer->spatialDomainFilteringBuffers[1]->setRenderTarget(0);
		spatialDomainConvolutionShader->enable();

		glViewport(
			0,
			0,
			sequenceRenderer->sequence->fieldWidth_px,
			sequenceRenderer->sequence->fieldHeight_px);

		spatialDomainConvolutionShader->bindUniformTexture("original", sequenceRenderer->spatialDomainFilteringBuffers[0]->getColorBuffer(0), 0);
		spatialDomainConvolutionShader->bindUniformTexture("kernel", spatialKernelId, 1);
		spatialDomainConvolutionShader->bindUniformFloat2("patternSizeOnRetina", sequenceRenderer->sequence->fieldWidth_um, sequenceRenderer->sequence->fieldHeight_um);
		spatialDomainConvolutionShader->bindUniformFloat2("kernelSizeOnRetina", spatialFilter->width_um, spatialFilter->height_um);
		spatialDomainConvolutionShader->bindUniformFloat2("step", 0, spatialFilter->height_um / 17.0);
		spatialDomainConvolutionShader->bindUniformBool("combine", false);

		sequenceRenderer->getNothing()->renderQuad();
		spatialDomainConvolutionShader->disable();

		sequenceRenderer->spatialDomainFilteringBuffers[1]->disableRenderTarget();

		///////////////

		if(sequenceRenderer->sequence->getMaxMemoryLength() > 0)
		{
			sequenceRenderer->textureQueue->setRenderTarget( sequenceRenderer->currentSlice );
		}
		else if(sequenceRenderer->sequence->getMaxTemporalProcessingStateCount() > 0)
		{
			sequenceRenderer->nextTemporalProcessingState->setRenderTargets( );
		}

		spatialDomainConvolutionShader->enable();

		//glViewport(
		//	0,
		//	0,
		//	sequenceRenderer->sequence->fieldWidth_px,
		//	sequenceRenderer->sequence->fieldHeight_px);
		glViewport( sequenceRenderer->sequence->fieldLeft_px, sequenceRenderer->sequence->fieldBottom_px, sequenceRenderer->sequence->fieldWidth_px, sequenceRenderer->sequence->fieldHeight_px);

		spatialDomainConvolutionShader->bindUniformTexture("original", sequenceRenderer->spatialDomainFilteringBuffers[1]->getColorBuffer(0), 0);
		spatialDomainConvolutionShader->bindUniformTexture("kernel", spatialKernelId, 1);
		spatialDomainConvolutionShader->bindUniformFloat2("patternSizeOnRetina", sequenceRenderer->sequence->fieldWidth_um, sequenceRenderer->sequence->fieldHeight_um);
		spatialDomainConvolutionShader->bindUniformFloat2("kernelSizeOnRetina", spatialFilter->width_um, spatialFilter->height_um);
		spatialDomainConvolutionShader->bindUniformFloat2("step", spatialFilter->width_um / 17.0, 0);
		spatialDomainConvolutionShader->bindUniformBool("combine", true);
		sequenceRenderer->getNothing()->renderQuad();
		spatialDomainConvolutionShader->disable();

		if(sequenceRenderer->sequence->getMaxMemoryLength() > 0)
		{
			sequenceRenderer->textureQueue->disableRenderTarget();
		}
		else if(sequenceRenderer->sequence->getMaxTemporalProcessingStateCount() > 0)
		{
			sequenceRenderer->nextTemporalProcessingState->disableRenderTargets();
		}
	}

}

