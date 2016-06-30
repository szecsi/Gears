#pragma once

#include "RandomSequenceBuffer.hpp"
#include "TextureQueue.hpp"
#include "StimulusGrid.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "framebuffer.hpp"
#include "pointgrid.hpp"
#include "quad.hpp"

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
	SpatialFilterRenderer(boost::shared_ptr<SequenceRenderer> sequenceRenderer, SpatialFilter::P spatialFilter, ShaderManager::P shaderManager,  KernelManager::P kernelManager);
public:
	GEARS_SHARED_CREATE(SpatialFilterRenderer);
	
	void renderFrame(std::function<void()> renderStimulus, bool useFft = true);
};