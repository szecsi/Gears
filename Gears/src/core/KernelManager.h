#pragma once

#include "RandomSequenceBuffer.hpp"
#include "TextureQueue.hpp"
#include "StimulusGrid.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Framebuffer.hpp"
#include "Pointgrid.hpp"
#include "Quad.hpp"

#include "SpatialFilter.h"
#include "ShaderManager.h"
#include "FFT.h"

#include <string>
#include <map>

class SequenceRenderer;

class KernelManager
{
	boost::shared_ptr<SequenceRenderer> sequenceRenderer;
	ShaderManager::P shaderManager;

	KernelManager(boost::shared_ptr<SequenceRenderer> sequenceRenderer, ShaderManager::P shaderManager);
public:
	GEARS_SHARED_CREATE(KernelManager);

	struct Kernel
	{
		FFT* fft;
		Framebuffer* buff;
		Shader* kernelShader;
	};

	using KernelMap = std::map<std::string, Kernel>;

	uint getKernel(SpatialFilter::CP spatialFilter);
	uint update(SpatialFilter::CP spatialFilter);

	void clear();
private:
	KernelMap kernels;
};