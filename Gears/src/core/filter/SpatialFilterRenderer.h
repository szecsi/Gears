#pragma once

#include "RandomSequenceBuffer.hpp"
#include "TextureQueue.hpp"
#include "StimulusGrid.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Framebuffer.hpp"
#include "Pointgrid.hpp"
#include "Quad.hpp"

#include <string>
#include <map>
#include <functional>
#include "Sequence.h"
#include "SpatialFilter.h"
#include "KernelManager.h"
#include "fft/FFT.h"

class SequenceRenderer;

//! Represents the currently active sequence. Manages resources for GPU computation.
class SpatialFilterRenderer
{
protected:
	boost::shared_ptr<SequenceRenderer> sequenceRenderer;
	unsigned int spatialKernelId = 0;
	Shader* spatialDomainConvolutionShader;
	Shader* copyShader;

	std::function<void(int)> renderStim;
	FFTChannelMode channelMode;
	std::function<void()> renderQuad;

	SpatialFilter::P spatialFilter;
	KernelManager::P kernelManager;
	ShaderManager::P shaderManager;
	SpatialFilterRenderer(boost::shared_ptr<SequenceRenderer> sequenceRenderer, ShaderManager::P shaderManager, KernelManager::P _kernelManager, SpatialFilter::P _spatialFilter);
public:
	void renderFrame(std::function<void(int)> renderStimulus);

	void updateKernel();
	virtual void initFirstFrames(std::function<void(int)> stim);
	virtual void prepareNext() {};

protected:
	virtual void fftConvolution() = 0;
	virtual void bindTexture(Shader* shader) = 0;
	void normalConvolution();
};