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

class SequenceRenderer;

//! Represents the currently active sequence. Manages resources for GPU computation.
class SpatialFilterRenderer
{
	bool useFft;
	boost::shared_ptr<SequenceRenderer> sequenceRenderer;
	unsigned int spatialKernelId;
	Shader* convolutionShader;
	Shader* spatialDomainConvolutionShader;
	Shader* copyShader;

	SpatialFilter::P spatialFilter;
	KernelManager::P kernelManager;
	SpatialFilterRenderer(boost::shared_ptr<SequenceRenderer> sequenceRenderer, SpatialFilter::P spatialFilter, ShaderManager::P shaderManager,  KernelManager::P kernelManager);
public:
	GEARS_SHARED_CREATE(SpatialFilterRenderer);
	
	void renderFrame(std::function<void()> renderStimulus);

	void updateKernel();
};