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
#include "PassRenderer.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "KernelManager.h"
#include "SpatialFilterRenderer.h"

class SequenceRenderer;
class PassRenderer;

//! Represents the currently active sequence. Manages resources for GPU computation.
class StimulusRenderer
{
	friend class PassRenderer;
	boost::shared_ptr<SequenceRenderer> sequenceRenderer;

	unsigned int iFrame;
	unsigned int iTick;
	Stimulus::CP stimulus;
	SpatialFilterRenderer::P spatialFilterRenderer;

	Shader*	randomGeneratorShader;
	Shader*	particleShader;
	Shader*	stimulusGeneratorShader;
	Shader* temporalFilteringShader;
	Shader* dynamicToneShader;
	Shader* timelineShader;
	Shader* spikeShader;
	Shader* kernelShader;
	Shader* profileShader;
	Shader* temporalProfileShader;

	Texture1D*		gammaTexture;
	Texture2D*		measuredHistogramTexture;

	std::vector<PassRenderer::P> passRenderers;

	StimulusRenderer(boost::shared_ptr<SequenceRenderer> sequenceRenderer, Stimulus::CP stimulus, ShaderManager::P shaderManager, TextureManager::P textureManager, KernelManager::P kernelManager);
public:
	GEARS_SHARED_CREATE_WITH_GETSHAREDPTR(StimulusRenderer);
	~StimulusRenderer();
	void apply( ShaderManager::P shaderManager, TextureManager::P textureManager);
	
	void renderStimulus(GLuint defaultFrameBuffer, int skippedFrames);
	void renderSample(uint sFrame, int left, int top, int width, int height);
	void renderTimeline(bool* signals, uint startFrame, uint frameCount);
	void renderSpatialKernel(float min, float max, float width, float height);
	void renderSpatialProfile(float min, float max, float width, float height);
	void renderTemporalKernel();

	unsigned int getCurrentFrame(){ return iFrame;}
	unsigned int tick(){ return iTick++;}

	Stimulus::CP getStimulus() const {return stimulus;}

	void reset();
	void skipFrames(uint nFramesToSkip);

	bool hasSpatialFilter() const {return spatialFilterRenderer != nullptr;}
	boost::shared_ptr<SequenceRenderer> getSequenceRenderer() const { return sequenceRenderer;}

};