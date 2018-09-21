#include <stdafx.h>
#include <chrono>
#include "clSpatialFilterRenderer.h"

CLSpatialFilterRenderer::CLSpatialFilterRenderer(boost::shared_ptr<SequenceRenderer> sequenceRenderer, ShaderManager::P shaderManager, KernelManager::P _kernelManager, SpatialFilter::P _spatialFilter, unsigned int width, unsigned int height)
	: SpatialFilterRenderer(sequenceRenderer, shaderManager, _kernelManager, _spatialFilter)
	, fft(width, height)
	, fft_prepare(width, height)
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
	current_fft = &fft;
	other_fft = &fft_prepare;
}

void CLSpatialFilterRenderer::fftConvolution()
{
	auto start = std::chrono::system_clock::now();
	// Fourier transformation
	other_fft->set_input(renderStim);
	other_fft->redraw_input();
	glFinish();

	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> drawelapsedSeconds = end - start;
	

	start = std::chrono::system_clock::now();

	if(!spatialFilter->stimulusGivenInFrequencyDomain)
		other_fft->do_fft(channelMode);

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> fftelapsedSeconds = end - start;

	start = std::chrono::system_clock::now();

	// Multiply in frequency domain
	cl_mem filterr;
	cl_mem filterg;
	cl_mem filterb;
	cl_mem imager;
	cl_mem imageg;
	cl_mem imageb;

	if(kernelManager->getKernelChannels(spatialFilter, filterr, filterg, filterb))
	{
		other_fft->get_channels(imager, imageg, imageb);
		size_t size[1] = {width * height};
		if(channelMode == FFTChannelMode::Monochrome)
			OpenCLCore::Get()->MultiplyFFT(other_fft->getQueue(), imager, filterr, size);
		else
			OpenCLCore::Get()->MultiplyFFT(other_fft->getQueue(), imager, imageg, imageb, filterr, filterr, filterr, size); // Filter is monochrome, stored in red
	}

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> melapsedSeconds = end - start;

	start = std::chrono::system_clock::now();

	// Do inverse fft
	if(!spatialFilter->showFft)
	{
		other_fft->do_inverse_fft();
		if(has_current_fft)
			current_fft->finishConv();
	}

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> ielapsedSeconds = end - start;
	std::cout << "   Length of draw for clfft: " << drawelapsedSeconds.count() * 1000 << "ms." << std::endl;
	std::cout << "   Length of fft for clfft: " << fftelapsedSeconds.count() * 1000 << "ms." << std::endl;
	std::cout << "   Length of multiply for clfft: " << melapsedSeconds.count() * 1000 << "ms." << std::endl;
	std::cout << "   Length of inverse fft for clfft: " << ielapsedSeconds.count() * 1000 << "ms." << std::endl;

}

void CLSpatialFilterRenderer::bindTexture(Shader* shader)
{
	copyShader->bindUniformTextureRect("srcrg", current_fft->get_fullTex(), 0);
	std::swap(current_fft, other_fft);
	has_current_fft = true;
}