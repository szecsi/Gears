#pragma once

#include "RandomSequenceBuffer.hpp"
#include "TextureQueue.hpp"
#include "StimulusGrid.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "framebuffer.hpp"
#include "pointgrid.hpp"
#include "quad.hpp"

#include "SpatialFilter.h"
#include "ShaderManager.h"

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

	using KernelMap = std::map<std::string, unsigned int>;

	unsigned int getKernel(SpatialFilter::P spatialFilter);

	void clear();
private:
	KernelMap kernels;
};