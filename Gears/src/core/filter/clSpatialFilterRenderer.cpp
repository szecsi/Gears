#include <stdafx.h>
#include "clSpatialFilterRenderer.h"

CLSpatialFilterRenderer::CLSpatialFilterRenderer(boost::shared_ptr<SequenceRenderer> sequenceRenderer, ShaderManager::P shaderManager, KernelManager::P _kernelManager, unsigned int width, unsigned int height)
	: SpatialFilterRenderer(sequenceRenderer, shaderManager, _kernelManager)
	, fft(width, height)
	, width(width)
	, height(height)
{
	copyShader = shaderManager->loadShader(R"GLSLC0D3(
			#version 150 compatibility
	    	#extension GL_ARB_texture_rectangle : enable
			precision highp float;
			uniform ivec2 offset;
			uniform ivec2 fftSize;
			uniform vec2 pixelRatio;
			uniform float pixelArea;
			uniform sampler2DRect srcrg;
			uniform sampler2DRect srcba;
			uniform bool clFFT;
			in vec2 fTexCoord;
			out vec4 outcolor;
			void main() {
				vec2 uv = gl_FragCoord.xy * pixelRatio;
				uv = mod(uv + offset, vec2(fftSize)) + vec2(1, 1);
				outcolor = texture2DRect(srcrg, uv) * pixelArea;
			}
		)GLSLC0D3"
	);
}

void CLSpatialFilterRenderer::fftConvolution()
{

	// Fourier transformation
	fft.set_input(renderStim);
	fft.redraw_input();
	if(!spatialFilter->stimulusGivenInFrequencyDomain)
		fft.do_fft(channelMode);

	// Multiply in frequency domain
	cl_mem filterr;
	cl_mem filterg;
	cl_mem filterb;
	cl_mem imager;
	cl_mem imageg;
	cl_mem imageb;

	if(kernelManager->getKernelChannels(spatialFilter, filterr, filterg, filterb))
	{
		fft.get_channels(imager, imageg, imageb);
		size_t size[1] = {width * height};
		if(channelMode == FFTChannelMode::Monochrome)
			OpenCLCore::Get()->MultiplyFFT(imager, filterr, size);
		else
			OpenCLCore::Get()->MultiplyFFT(imager, imageg, imageb, filterr, filterr, filterr, size); // Filter is monochrome, stored in red
	}

	// Do inverse fft
	if(!spatialFilter->showFft)
	{
		fft.do_inverse_fft();
	}
}

void CLSpatialFilterRenderer::bindTexture(Shader* shader)
{
	copyShader->bindUniformTextureRect("srcrg", fft.get_fullTex(), 0);
}