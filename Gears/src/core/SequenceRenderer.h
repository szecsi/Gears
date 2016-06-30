#pragma once

#include "RandomSequenceBuffer.hpp"
#include "TextureQueue.hpp"
#include "StimulusGrid.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "framebuffer.hpp"
#include "pointgrid.hpp"
#include "quad.hpp"
#include "Nothing.hpp"

#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <iostream>
#include "Sequence.h"
#include "StimulusRenderer.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "KernelManager.h"
#include "SpatialFilterRenderer.h"
#include "Ticker.h"
#include "FFT.h"
#include "FontManager.h"

//! Represents the currently active sequence. Manages resources for GPU computation.
class SequenceRenderer
{
	friend class StimulusRenderer;
	friend class PassRenderer;
	friend class SpatialFilterRenderer;

	//! Active sequence.
	Sequence::CP sequence;
	FFT* fft2FrequencyDomain;
	FFT* fft2SpatialDomain;
	Framebuffer* spatialDomainFilteringBuffers[2];

	bool paused;
	unsigned int iFrame;
	std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::high_resolution_clock::duration> firstFrameTimePoint;
	std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::high_resolution_clock::duration> previousFrameTimePoint;
	int cFrame;
	int totalFramesSkipped;
	std::vector<int> skippedFrames;
	
	using StimulusRendererMap = std::map<unsigned int, StimulusRenderer::P >;
	StimulusRendererMap stimulusRenderers;
	StimulusRendererMap::iterator selectedStimulusRenderer;

	using PortMap = std::map<std::string, HANDLE >;
	PortMap ports;

	Stimulus::SignalMap noTickSignal;

	using SpatialFilterRendererMap = std::map<std::string, SpatialFilterRenderer::P >;
	SpatialFilterRendererMap spatialFilterRenderers;

//	Quad* fullscreenQuad;
	Nothing* nothing;

	Shader* textShader;					//< renders text

	std::map<std::string, std::string> text;
	bool textVisible;

	//! Index of current slice in texture queue. Changes continuously during sequence.
	unsigned int currentSlice;

	RandomSequenceBuffer* randomSequenceBuffers[5];
	std::ofstream randomExportStream;
	RandomSequenceBuffer* particleBuffers[2];
	TextureQueue*	textureQueue;
	TextureQueue*	currentTemporalProcessingState;
	TextureQueue*	nextTemporalProcessingState;

	FontManager fontManager;

	bool calibrating;
	float histogramMin;
	float histogramMax;
	float histogramScale;
	uint histogramResolution;
	uint calibrationImageWidth;
	uint calibrationImageHeight;

	Framebuffer* forwardRenderedImage;

	Shader* histogramShader;
	Shader* histogramClearShader;
	Shader* histogramDisplayShader;
	Framebuffer* calibrationImage;
	Framebuffer* histogramBuffer;

	float measuredDynRangeMin;
	float measuredDynRangeMax;
	float measuredMean;
	float measuredVariance;

	void readCalibrationResults();

	//! Constructor. Sets some parameters to zero, but the sequence remains invalid until apply is called.
	SequenceRenderer();
public:
	GEARS_SHARED_CREATE_WITH_GETSHAREDPTR(SequenceRenderer);
	
	//! Creates GPU resources for the sequence, releasing earlier ones, if any.
	void apply(Sequence::P sequence, ShaderManager::P shaderManager, TextureManager::P textureManager, KernelManager::P kernelManager);

	void cleanup();
	void reset();
	//! Computes next stimulus and displays it on screen. Return true if not in a black phase between bar or spot stimuli.
	bool renderFrame(GLuint defaultFrameBuffer);
	//! Signals the rendering fo the frame without actually rendering it (for video display)
	bool renderFrameHidden();

	void renderRandoms(Shader* randomGeneratorShader, uint iStimulusFrame, uint randomSeed, uint freezeRandomsAfterFrame);
	void renderParticles(Shader* particleShader, uint iStimulusFrame, float time);
	void renderTimeline();
	void renderSelectedStimulusTimeline();
	void renderSelectedStimulusTemporalKernel();
	void renderSelectedStimulusSpatialKernel(float min, float max, float width, float height);
	void renderSelectedStimulusSpatialProfile(float min, float max, float width, float height);
	Stimulus::CP getSelectedStimulus(){
		if(selectedStimulusRenderer == stimulusRenderers.end() )
			return nullptr;
		return selectedStimulusRenderer->second->getStimulus();
	}

	Stimulus::CP getCurrentStimulus();
	void abort();

	unsigned int getCurrentFrame();

	Sequence::CP getSequence(){return sequence;}

//	Quad* getFullscreenQuad(){return fullscreenQuad;}
	Nothing* getNothing(){return nothing;}

	void pickStimulus(double x, double y);

	void raiseSignal(std::string channel);
	void clearSignal(std::string channel);

	Ticker::P startTicker();
	const Stimulus::SignalMap& tick(uint& iTick);

	void skip(int skipCount);

	void enableExport(std::string path);
	bool exporting() const;

	uint calibrationStartingFrame;
	uint calibrationDuration;

	void enableCalibration(uint startingFrame, uint duration, float histogramMin, float histogramMax);

	uint sequenceTimelineStartFrame;
	uint sequenceTimelineFrameCount;
	void setSequenceTimelineZoom(uint nFrames)
	{
		sequenceTimelineFrameCount = nFrames;
	}
	void setSequenceTimelineStart(uint iStartFrame)
	{
		sequenceTimelineStartFrame = iStartFrame;
	}

	uint stimulusTimelineStartFrame;
	uint stimulusTimelineFrameCount;
	void setStimulusTimelineZoom(uint nFrames)
	{
		stimulusTimelineFrameCount = nFrames;
	}
	void setStimulusTimelineStart(uint iStartFrame)
	{
		stimulusTimelineStartFrame = iStartFrame;
	}
	void pause()
	{
		paused = !paused;
	}

	void beginCalibrationFrame();
	void endCalibrationFrame();

	int getSkippedFrameCount(){
		for(int q : skippedFrames)
			std::cout << q << '\t';
		return totalFramesSkipped;
	}
	std::string getSequenceTimingReport();
	void setText(std::string tag, std::string text){if(text == "") this->text.erase(tag); else this->text[tag] = text;}
	void showText(){this->textVisible = !this->textVisible;}
};