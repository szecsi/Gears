#include "stdafx.h"
#include "SequenceRenderer.h"
#pragma region OpenGL
#include <GL/glew.h>
#include "wglext.h"
#pragma endregion includes for GLEW and WGL
#include <fstream>
#include <sstream>
#include <time.h>
#include <limits>
#include <iostream>
#include <boost/filesystem.hpp>
#include "core/pythonerr.h"

extern PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT;
extern PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT;

SequenceRenderer::SequenceRenderer()
{
	selectedStimulusRenderer = stimulusRenderers.end();

//	fullscreenQuad = new Quad();
	nothing = new Nothing();

	sequence = nullptr;

	textureQueue = nullptr;
	currentTemporalProcessingState = nullptr;
	nextTemporalProcessingState = nullptr;

	fft2FrequencyDomain = nullptr;
	fft2SpatialDomain = nullptr;

	spatialDomainFilteringBuffers[0] = nullptr;
	spatialDomainFilteringBuffers[1] = nullptr;

	randomSequenceBuffers[0] = 0;
	randomSequenceBuffers[1] = 0;
	randomSequenceBuffers[2] = 0;
	randomSequenceBuffers[3] = 0;
	randomSequenceBuffers[4] = 0;

	particleBuffers[0] = 0;
	particleBuffers[1] = 0;

	sequenceTimelineStartFrame = 0;
	sequenceTimelineFrameCount = 100;
	stimulusTimelineStartFrame = 0;
	stimulusTimelineFrameCount = 100;

	paused = false;

	calibrating = false;

	histogramMin = -1;
	histogramMax = 2;
	histogramScale = 50000;
	histogramResolution = 256;
	calibrationImageWidth = 768;
	calibrationImageHeight = 768;

	histogramShader = 0;
	histogramClearShader = 0;
	histogramDisplayShader = 0;
	calibrationImage = 0;
	histogramBuffer = 0;
	textShader = 0;

	textVisible = false;

	measuredDynRangeMin = std::numeric_limits<double>::quiet_NaN();
	measuredDynRangeMax = std::numeric_limits<double>::quiet_NaN();
	measuredMean  = std::numeric_limits<double>::quiet_NaN();
	measuredVariance  = std::numeric_limits<double>::quiet_NaN();

	forwardRenderedImage = nullptr;
	cFrame = 1;
}

void SequenceRenderer::apply(Sequence::P sequence, ShaderManager::P shaderManager, TextureManager::P textureManager, KernelManager::P kernelManager)
{
	this->sequence = sequence;
	iFrame = 1;
	paused = false;
	currentSlice = 0;

	if(forwardRenderedImage)
		delete forwardRenderedImage;
	if(sequence->getUsesForwardRendering())
	{
		forwardRenderedImage = new Framebuffer(sequence->fieldWidth_px, sequence->fieldHeight_px, 1);
	}
	else
		forwardRenderedImage = nullptr;

	if(textureQueue) {delete textureQueue; textureQueue = nullptr;}
	if(sequence->getMaxMemoryLength() > 0)
	{
		textureQueue = new TextureQueue(sequence->fieldWidth_px, sequence->fieldHeight_px, sequence->getMaxMemoryLength(), sequence->isMonochrome(), false);
		textureQueue->clear();
	}
	if(sequence->getMaxTemporalProcessingStateCount() > 0)
	{
		unsigned int nSlices = sequence->getMaxTemporalProcessingStateCount() / 4 + 1;
		nSlices += 1; // output
		if(!sequence->isMonochrome())
			nSlices *= 3;
		currentTemporalProcessingState = new TextureQueue(sequence->fieldWidth_px, sequence->fieldHeight_px, nSlices, false, false);
		currentTemporalProcessingState->clear();
		nextTemporalProcessingState = new TextureQueue(sequence->fieldWidth_px, sequence->fieldHeight_px, nSlices, false, false);
		nextTemporalProcessingState->clear();
	}
	stimulusRenderers.clear();

	for(auto& s : sequence->getStimuli())
	{
		StimulusRenderer::P stimulusRenderer = StimulusRenderer::create(getSharedPtr(), s.second, shaderManager, textureManager, kernelManager);
		stimulusRenderer->apply(shaderManager, textureManager);
		stimulusRenderers[s.first] = stimulusRenderer;
	}
	selectedStimulusRenderer = stimulusRenderers.end();

	if(sequence->hasFft)
	{
		fft2FrequencyDomain = new FFT(sequence->fftWidth_px, sequence->fftHeight_px);
		fft2SpatialDomain = new FFT(sequence->fftWidth_px, sequence->fftHeight_px, 0, true, true);
	}
	if(sequence->hasSpatialDomainConvolution)
	{
		spatialDomainFilteringBuffers[0] = new Framebuffer(sequence->fieldWidth_px, sequence->fieldHeight_px, 1);
		spatialDomainFilteringBuffers[1] = new Framebuffer(sequence->fieldWidth_px, sequence->fieldHeight_px, 1);
	}

	if(sequence->maxRandomGridWidth > 0)
	{
		randomSequenceBuffers[0] = new RandomSequenceBuffer(sequence->maxRandomGridWidth, sequence->maxRandomGridHeight);
		randomSequenceBuffers[1] = new RandomSequenceBuffer(sequence->maxRandomGridWidth, sequence->maxRandomGridHeight);
		randomSequenceBuffers[2] = new RandomSequenceBuffer(sequence->maxRandomGridWidth, sequence->maxRandomGridHeight);
		randomSequenceBuffers[3] = new RandomSequenceBuffer(sequence->maxRandomGridWidth, sequence->maxRandomGridHeight);
		randomSequenceBuffers[4] = new RandomSequenceBuffer(sequence->maxRandomGridWidth, sequence->maxRandomGridHeight);
	}

	if(sequence->maxParticleGridWidth > 0)
	{
		particleBuffers[0] = new RandomSequenceBuffer(sequence->maxParticleGridWidth, sequence->maxParticleGridHeight);
		particleBuffers[1] = new RandomSequenceBuffer(sequence->maxParticleGridWidth, sequence->maxParticleGridHeight);
	}

	calibrating = false;

	if(histogramShader == 0)	delete histogramShader;
	if(histogramClearShader == 0)	delete histogramClearShader;
	if(histogramDisplayShader == 0)	delete histogramDisplayShader;
	if(calibrationImage == 0)	delete calibrationImage;
	if(histogramBuffer == 0)	delete histogramBuffer;

	histogramShader = shaderManager->loadShaderFromFile("./Project/Shaders/computeHistogram.vert", "./Project/Shaders/computeHistogram.frag");
	histogramClearShader = shaderManager->loadShaderFromFile("./Project/Shaders/quad.vert", "./Project/Shaders/clearHistogram.frag");
	histogramDisplayShader = shaderManager->loadShaderFromFile("./Project/Shaders/quad.vert", "./Project/Shaders/showHistogram.frag");
	calibrationImage = new Framebuffer(calibrationImageWidth, calibrationImageHeight, 1);
	histogramBuffer = new Framebuffer(histogramResolution, 1, 1, false, true, true);
	histogramBuffer->clear();

	if(textShader == 0) delete textShader;
	textShader = shaderManager->loadShaderFromFile("./Project/Shaders/text.vert", "./Project/Shaders/text.frag");

	for(auto& c : sequence->getChannels())
	{
		PortMap::iterator iPort = ports.find(c.second.portName);
		if(iPort == ports.end())
		{
			HANDLE hComm = CreateFile(c.second.portName.c_str(),
				GENERIC_READ | GENERIC_WRITE,
				0,
				0,
				OPEN_EXISTING,
				FILE_FLAG_OVERLAPPED,
				0);
			ports[c.second.portName] = hComm;
			EscapeCommFunction(hComm, c.second.clearFunc);
			if (hComm == INVALID_HANDLE_VALUE)
			{
				std::stringstream ss;
				ss << "No device on port " << c.second.portName << " !" << std::endl;
				PyErr_Warn(PyExc_Warning, ss.str().c_str());
				//boost::python::throw_error_already_set();
			}
		}
		else
			EscapeCommFunction(iPort->second, c.second.clearFunc);
	}

}

bool SequenceRenderer::renderFrame(GLuint defaultFrameBuffer)
{
	typedef std::chrono::high_resolution_clock Clock;
	typedef std::chrono::duration<float> Fsec;
	int nSkippedFrames = 0;

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	//if(iFrame == 310)
	//	paused = true;

	if(iFrame >= sequence->getMeasurementStart())
	{
		//std::cout << sequence->getMeasurementStart() << ':' << iFrame << '\n';
		if(iFrame == sequence->getMeasurementStart())
		{
			firstFrameTimePoint = Clock::now();
			previousFrameTimePoint = firstFrameTimePoint;
			cFrame = 1;
			totalFramesSkipped = 0;
			skippedFrames.clear();
		}
		else if(iFrame > sequence->getMeasurementEnd())
		{
		}
		else if(!calibrating && !randomExportStream.is_open())
		{
			auto now = Clock::now();
			Fsec  elapsed = now - firstFrameTimePoint;
			Fsec  elapsedSinceLastFrame = now - previousFrameTimePoint;
			previousFrameTimePoint = now;
			////auto  elapsedNano = std::chrono::duration_cast<std::chrono::nanoseconds>(now - firstFrameTimePoint);
			////std::cout << elapsed.count() << " : " << elapsedNano.count() << '\n';
			//std::cout << elapsedSinceLastFrame.count() << '\n';
			int vSyncPeriodsSinceLastFrame = elapsedSinceLastFrame.count() / sequence->getFrameInterval_s() + 0.5f;
			////int toFrame = elapsed.count() / sequence->getFrameInterval_s() + 1.5f;
			////std::cout << elapsed.count() - (iFrame + frameOffset - 1.5f) * sequence->getFrameInterval_s() << '\n';
			////toFrame -= frameOffset;
			Sequence::SignalMap::const_iterator iSignal = sequence->getSignals().lower_bound(iFrame-1);
			Sequence::SignalMap::const_iterator eSignal = sequence->getSignals().upper_bound(iFrame + vSyncPeriodsSinceLastFrame);
			while(iSignal != eSignal)
			{
				if(iSignal->second.clear)
					clearSignal(iSignal->second.channel);
				else
					raiseSignal(iSignal->second.channel);
				iSignal++;
			}
			nSkippedFrames = vSyncPeriodsSinceLastFrame - 1;
			if(nSkippedFrames < 0)
			{
				//if(!skippedFrames.empty() && skippedFrames.back() == iFrame)
				//{
				//	skippedFrames.pop_back();
				//					}
				//else
				skippedFrames.push_back(-(int)iFrame);
			}
			totalFramesSkipped += nSkippedFrames;
			for(uint q=iFrame; q<iFrame + nSkippedFrames; q++)
				skippedFrames.push_back(q);
			iFrame += nSkippedFrames;
			cFrame += vSyncPeriodsSinceLastFrame;
		}
	}
	else
		cFrame = 0;

	StimulusRendererMap::iterator iStimulusRenderer = stimulusRenderers.lower_bound(iFrame);
	if(calibrating && iFrame == calibrationStartingFrame)
		histogramBuffer->clear();

	if(calibrating && iFrame > calibrationStartingFrame + calibrationDuration)
	{
		readCalibrationResults();
		return false;
	}
	if(iStimulusRenderer == stimulusRenderers.end() )
		return false;
	StimulusRenderer::P stimulusRenderer = iStimulusRenderer->second;
	stimulusRenderer->renderStimulus(defaultFrameBuffer, nSkippedFrames);
	if(!paused)
	{
		iFrame++;
		if(sequence->getMaxMemoryLength() > 0)
			currentSlice = (currentSlice+1) % sequence->getMaxMemoryLength();
	}

	if(textVisible)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glColor3d(1, 0, 0);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	/*	glBegin(GL_QUADS);
			glVertex2d(1, 1);
			glVertex2d(1, 0);
			glVertex2d(0, 0);
			glVertex2d(0, 1);
		glEnd();*/

		textShader->enable();
		TexFont* font = fontManager.loadFont("Candara");
		textShader->bindUniformTexture("glyphTexture", font->getTextureId(), 0);
		glPushMatrix();
		glTranslated(-1, 0.9, 0);
		glScaled(0.002, 0.002, 0.002);
		bool first = true;
		for(auto label : text)
		{
			if(first)
				font->glRenderString(label.second + "\n", "Candara", false, false, false, false, 0xefffffff, TEXFONT_MODE_OPEN_ONLY);
			else
				font->glRenderString(label.second + "\n", "Candara", false, false, false, false, 0xefffffff, TEXFONT_MODE_CONTINUE);
			first = false;
		}
		if(!first)
			glPopMatrix();
		textShader->disable();
		glPopMatrix();
		glDisable(GL_BLEND);
	}
	if(calibrating)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glColor3d(0, 0, 0);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		textShader->enable();
		TexFont* font = fontManager.loadFont("Candara");
		textShader->bindUniformTexture("glyphTexture", font->getTextureId(), 0);
		for(int i=0; i<=histogramMax-histogramMin; i++)
		{
			int v = i + histogramMin;
			float p = (v - histogramMin)/ (histogramMax-histogramMin) * 2.0f / 1.1f - 0.909f;
			glPushMatrix();
			glTranslated(p, -0.8, 0);
			glScaled(0.004, 0.004, 0.004);
			font->glRenderString(std::to_string(v), "Candara", false, false, false, false, 0xefffffff, TEXFONT_MODE_OPEN_AND_CLOSE);

			glPopMatrix();
		}	
		textShader->disable();

		glDisable(GL_BLEND);


		glEnable(GL_LINE_STIPPLE);
		glLineWidth(2);
		glLineStipple(1, 0xcccc);
		glColor3d(0.6, 0.6, 0.6);
		for(int i=0; i<=histogramMax-histogramMin; i++)
		{
			int v = i + histogramMin;
			float p = (v - histogramMin)/ (histogramMax-histogramMin) * 2.0f / 1.1f - 0.909f;
			glPushMatrix();
			glTranslated(p, 0, 0);

			glBegin(GL_LINES);
				glVertex2d(0, -10000000);
				glVertex2d(0, +10000000);
			glEnd();
			glPopMatrix();
		}
		glDisable(GL_LINE_STIPPLE);

	}


	return true;
}

void SequenceRenderer::renderTimeline()
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/*glColor4d(0, 1, 0, 1);
	glBegin(GL_LINES);
		for(int u=0; u<8; u++)
		{
			glVertex2d(-1, u*0.25 - 1.0);
			glVertex2d(1, u*0.25 - 1.0);
		}
	glEnd();*/

	glTranslated(-1.0, 0.0, 0);
	glScaled(2.0 / sequenceTimelineFrameCount, 1.0, 1.0);
	glTranslated(-(double)sequenceTimelineStartFrame, 0.0, 0);

	if(selectedStimulusRenderer != stimulusRenderers.end())
	{
		auto stim = *selectedStimulusRenderer;
		glPushMatrix();
		glTranslated(stim.second->getStimulus()->getStartingFrame(), 0.0, 0);
		glColor4d(0.35, 0, 0, 1);
		glBegin(GL_QUADS);
			glVertex2d(0, -5);
			glVertex2d(0, 0.89);
			glVertex2d(stim.second->getStimulus()->getDuration(), 0.89);
			glVertex2d(stim.second->getStimulus()->getDuration(), -5);
		glEnd();
		glPopMatrix();
	}

	glColor3d(1, 0, 0);
	int cChannel = 0;
	for(auto iChannel : sequence->getChannels() )
	{
		glPushMatrix();
		std::string channelName = iChannel.first;

		glTranslated(0.0, -1, 0.0);
		glScaled(1.0, 1.0 / sequence->getChannels().size(), 1);
		glTranslated(0.0, 0.25 + (sequence->getChannels().size() - 1 - cChannel), 0.0);
		glScaled(1.0, 0.25, 1);

		auto& signals = sequence->getSignals();

		glColor4d(0.3, 0, 0, 1);
		glBegin(GL_LINES);
			glVertex2d(-100, 1);
			glVertex2d(+100000, 1);
			glVertex2d(+100000, 0);
			glVertex2d(-100, 0);
		glEnd();
		glColor4d(1, 0, 0, 1);

		bool channelHasSequenceSignal = false;
		bool lastHigh = false;
		glBegin(GL_LINE_STRIP);
			for(auto iSignal : signals)
			{
				if(iSignal.second.channel == iChannel.first)
				{
					if(channelHasSequenceSignal == false)
					{
						channelHasSequenceSignal = true;
						glVertex2d(1, 0);
					}
					if(iSignal.second.clear)
					{
						glVertex2d(iSignal.first, lastHigh?1.0:0.0);
						glVertex2d(iSignal.first, 0.0);
						lastHigh = false;
					}
					else
					{
						glVertex2d(iSignal.first, lastHigh?1.0:0.0);
						glVertex2d(iSignal.first, 1.0);
						lastHigh = true;
					}
				}
			}
			if(channelHasSequenceSignal)
				glVertex2d(sequence->getDuration(), lastHigh?1:0);
		glEnd();
		glPopMatrix();
		cChannel++;
	}

	glLineWidth(3.0);
	bool signalLevels[] = {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};
	for(auto stim : stimulusRenderers)
	{
		glPushMatrix();
		uint stimStartFrame = stim.second->getStimulus()->getStartingFrame();
		glTranslated(stimStartFrame, 0.0, 0.0);

		uint fromf = 0;
		if(sequenceTimelineStartFrame > stimStartFrame)
			fromf = sequenceTimelineStartFrame - stimStartFrame;
		uint dur = stim.second->getStimulus()->getDuration();
		uint tof = 0;
		if(sequenceTimelineStartFrame + sequenceTimelineFrameCount > stimStartFrame)
			tof = sequenceTimelineStartFrame + sequenceTimelineFrameCount - stimStartFrame;
		if(tof > dur)
			tof = dur;
		if(fromf < tof)
			stim.second->renderTimeline(signalLevels, fromf, tof - fromf );
		glPopMatrix();
	}
	/*
	int vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);

	// white blank for shots
//	glViewport(vp[0], 50, vp[2], 200);
	glViewport(0, 0, 1920, 400);
	glPushMatrix();
	glLoadIdentity();
	glColor3d(1, 1, 1);
	glBegin(GL_QUADS);
	glVertex2d(-1, -1);
	glVertex2d(-1, 1);
	glVertex2d(1, 1);
	glVertex2d(1, -1);
	glEnd();
	glPopMatrix();


	// render film strip
	glViewport(vp[0], 85, vp[2], 363);
	glPushMatrix();
	glLoadIdentity();
	glColor3d(0.2, 0.2, 0.0);
	glBegin(GL_QUADS);
	glVertex2d(-1, -1);
	glVertex2d(-1, 1);
	glVertex2d(1, 1);
	glVertex2d(1, -1);
	glColor3d(1, 1, 1);
//for(uint i=0; i<64; i++)
//{
//	glVertex2d((i+0.5)/32.0 - 1 + -0.004, -0.88 + -0.05);
//	glVertex2d((i+0.5)/32.0 - 1 + -0.004, -0.88 + 0.05);
//	glVertex2d((i+0.5)/32.0 - 1 + 0.004,  -0.88 + 0.05);
//	glVertex2d((i+0.5)/32.0 - 1 + 0.004,  -0.88 + -0.05);
//}
//for(uint i=0; i<64; i++)
//{
//	glVertex2d((i+0.5)/32.0 - 1 + -0.004, 0.88 + -0.05);
//	glVertex2d((i+0.5)/32.0 - 1 + -0.004, 0.88 + 0.05);
//	glVertex2d((i+0.5)/32.0 - 1 + 0.004,  0.88 + 0.05);
//	glVertex2d((i+0.5)/32.0 - 1 + 0.004,  0.88 + -0.05);
//}
	glEnd();
	glPopMatrix();

	uint iPic = 0;
	for(uint iSampleFrame=0; iSampleFrame<sequenceTimelineFrameCount; iSampleFrame += (sequenceTimelineFrameCount - 1) / 3 + 1 )
	{
		uint iFrameFrame = sequenceTimelineStartFrame + iSampleFrame;
		StimulusRendererMap::iterator iStimulusRenderer = stimulusRenderers.lower_bound(iFrameFrame);
		if(iStimulusRenderer != stimulusRenderers.end())
		{
			iStimulusRenderer->second->renderSample(
				iFrameFrame - iStimulusRenderer->second->getStimulus()->getStartingFrame(),
				80 + iPic * vp[2] / 3,
				100,
				vp[2] / 3,
				333
				);
			iPic++;
		}
	}
	glViewport(vp[0], vp[1], vp[2], vp[3]);

	glColor4d(1, 0, 0, 1);*/
}

void SequenceRenderer::renderSelectedStimulusTimeline()
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(selectedStimulusRenderer == stimulusRenderers.end() )
		return;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(-1.0, 0, 0);
	glScaled(2.0 / stimulusTimelineFrameCount, 2, 1.0);
	glTranslated(-(double)stimulusTimelineStartFrame, 0.0, 0.0);

	glLineWidth(3.0);
	selectedStimulusRenderer->second->renderTimeline(nullptr, stimulusTimelineStartFrame, stimulusTimelineFrameCount);
	glColor4d(1, 0, 0, 1);

}


void SequenceRenderer::renderSelectedStimulusSpatialKernel(float min, float max, float width, float height)
{
	if(selectedStimulusRenderer == stimulusRenderers.end() )
		return;
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	selectedStimulusRenderer->second->renderSpatialKernel( min,  max, width, height);
}

void SequenceRenderer::renderSelectedStimulusSpatialProfile(float min, float max, float width, float height)
{
	if(selectedStimulusRenderer == stimulusRenderers.end() )
		return;
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	selectedStimulusRenderer->second->renderSpatialProfile( min,  max, width, height);
}

void SequenceRenderer::renderSelectedStimulusTemporalKernel()
{
	if(selectedStimulusRenderer == stimulusRenderers.end() )
		return;
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	selectedStimulusRenderer->second->renderTemporalKernel( );
}

void SequenceRenderer::abort()
{
//			raise(CH0); // STOP EXPERIMENT
//			lower(CH0); // STOP EXPERIMENT
//			lower(CH1 | CH2 | CH3);
//			signal = SequenceDesc::Signal::NONE;
//			glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
}

void SequenceRenderer::pickStimulus(double x, double y)
{
	uint iPickedFrame = x * sequence->getDuration();
	selectedStimulusRenderer = stimulusRenderers.lower_bound(iPickedFrame);
	if(selectedStimulusRenderer == stimulusRenderers.end())
		selectedStimulusRenderer = stimulusRenderers.begin();

}

void SequenceRenderer::raiseSignal(std::string channel)
{
	Sequence::ChannelMap::const_iterator iChannel = sequence->getChannels().find(channel);
	if(iChannel == sequence->getChannels().end())
	{
		PySys_WriteStdout("\nUnknown channel: ");		//TODO
		PySys_WriteStdout(channel.c_str());		//TODO
		return;
	}
	PortMap::iterator iPort = ports.find(iChannel->second.portName);
	if(iPort == ports.end())
	{
		PySys_WriteStdout("\nChannel uses unknown port:");		//TODO
		PySys_WriteStdout(iChannel->second.portName.c_str());		//TODO
		return;
	}
	EscapeCommFunction(iPort->second, iChannel->second.raiseFunc);
}

void SequenceRenderer::clearSignal(std::string channel)
{
	Sequence::ChannelMap::const_iterator iChannel = sequence->getChannels().find(channel);
	if(iChannel == sequence->getChannels().end())
	{
		PySys_WriteStdout("Unknown channel.");		//TODO
		return;
	}
	PortMap::iterator iPort = ports.find(iChannel->second.portName);
	if(iPort == ports.end())
	{
		PySys_WriteStdout("Channel uses unknown port.");		//TODO
		return;
	}
	EscapeCommFunction(iPort->second, iChannel->second.clearFunc);
}

void SequenceRenderer::reset()
{
	calibrating = false;

	randomExportStream.close();
	iFrame = 1;
	paused = false;
	//selectedStimulusRenderer = stimulusRenderers.end();
	for(auto stim : stimulusRenderers)
		stim.second->reset();

	if(textureQueue)
		textureQueue->clear();
	if(currentTemporalProcessingState)
		currentTemporalProcessingState->clear();
	if(nextTemporalProcessingState)
		nextTemporalProcessingState->clear();

	for(auto& c : sequence->getChannels())
	{
		PortMap::iterator iPort = ports.find(c.second.portName);
		if(iPort != ports.end())
		{
			EscapeCommFunction(iPort->second, c.second.clearFunc);
		}
	}
	
	if(sequence->resetCallback)
		sequence->resetCallback();

}

void SequenceRenderer::cleanup()
{
	if(randomSequenceBuffers[0]) {	delete randomSequenceBuffers[0]; randomSequenceBuffers[0] = nullptr; }
	if(randomSequenceBuffers[1]) {	delete randomSequenceBuffers[1]; randomSequenceBuffers[1] = nullptr; }
	if(randomSequenceBuffers[2]) {	delete randomSequenceBuffers[2]; randomSequenceBuffers[2] = nullptr; }
	if(randomSequenceBuffers[3]) {	delete randomSequenceBuffers[3]; randomSequenceBuffers[3] = nullptr; }
	if(randomSequenceBuffers[4]) {	delete randomSequenceBuffers[4]; randomSequenceBuffers[4] = nullptr; }

	if(particleBuffers[0]) {	delete particleBuffers[0]; particleBuffers[0] = nullptr; }
	if(particleBuffers[1]) {	delete particleBuffers[1]; particleBuffers[1] = nullptr; }

	if(forwardRenderedImage)
		delete forwardRenderedImage;

	//if(fft2FrequencyDomain)
	//{
	//	delete fft2FrequencyDomain;
	//	fft2FrequencyDomain = nullptr;
	//}
	//if(fft2SpatialDomain)
	//{
	//	delete fft2SpatialDomain;
	//	fft2SpatialDomain = nullptr;
	//}
	//
	//stimulusRenderers.clear();
	//
	//delete nothing;
	//
	//if(textureQueue)
	//{
	//	delete textureQueue;
	//	textureQueue = nullptr;
	//}

	for(auto iPort : ports)
	{
		CloseHandle(iPort.second);
	}
	if(spatialDomainFilteringBuffers[0])
	{
		delete spatialDomainFilteringBuffers[0];
		spatialDomainFilteringBuffers[0] = nullptr;
	}
	if(spatialDomainFilteringBuffers[1])
	{
		delete spatialDomainFilteringBuffers[1];
		spatialDomainFilteringBuffers[1] = nullptr;
	}
}

void SequenceRenderer::renderParticles(Shader* particleShader, uint iStimulusFrame, float time)
{
 	particleBuffers[1]->setRenderTarget();
	particleShader->enable();
	particleShader->bindUniformTexture("previousParticles", particleBuffers[0]->getColorBuffer(), 0);
	particleShader->bindUniformTexture("randoms", randomSequenceBuffers[0]->getColorBuffer(), 1);
	particleShader->bindUniformInt("frame", iStimulusFrame);
	particleShader->bindUniformFloat("time", time);
	getNothing()->renderQuad();
	particleShader->disable();
	particleBuffers[1]->disableRenderTarget();

	std::swap(particleBuffers[0], particleBuffers[1]);
}

void SequenceRenderer::renderRandoms(Shader* randomGeneratorShader, uint iStimulusFrame, uint randomSeed, uint freezeRandomsAfterFrame)
{
	if(freezeRandomsAfterFrame != 0 && freezeRandomsAfterFrame < iStimulusFrame )
		return;
	randomSequenceBuffers[4]->setRenderTarget();
	randomGeneratorShader->enable();
	randomGeneratorShader->bindUniformTexture("previousSequenceElements0",
		randomSequenceBuffers[0]->getColorBuffer(), 0);
	randomGeneratorShader->bindUniformTexture("previousSequenceElements1",
		randomSequenceBuffers[1]->getColorBuffer(), 1);
	randomGeneratorShader->bindUniformTexture("previousSequenceElements2",
		randomSequenceBuffers[2]->getColorBuffer(), 2);
	randomGeneratorShader->bindUniformTexture("previousSequenceElements3",
		randomSequenceBuffers[3]->getColorBuffer(), 3);
	randomGeneratorShader->bindUniformInt("frame", iStimulusFrame);
	randomGeneratorShader->bindUniformUint("seed", randomSeed);
	getNothing()->renderQuad();
	randomGeneratorShader->disable();
	randomSequenceBuffers[4]->disableRenderTarget();

	std::swap(randomSequenceBuffers[3], randomSequenceBuffers[4]);
	std::swap(randomSequenceBuffers[2], randomSequenceBuffers[3]);
	std::swap(randomSequenceBuffers[1], randomSequenceBuffers[2]);
	std::swap(randomSequenceBuffers[0], randomSequenceBuffers[1]);

	if(randomExportStream.is_open())
	{
		glBindTexture(GL_TEXTURE_2D, randomSequenceBuffers[0]->getColorBuffer());
		
		uint* randoms = new uint[sequence->maxRandomGridWidth * sequence->maxRandomGridHeight * 4];
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, (void*)randoms);

		if(sequence->exportRandomsWithHashmark)
		{
			randomExportStream << "#Frame " << iStimulusFrame << " of stimulus, frame " << iFrame <<  " of sequence.\n";
			randomExportStream << "#Number of rows: " << sequence->maxRandomGridHeight << "\n";
			randomExportStream << "#Number of cells per row: " << sequence->maxRandomGridWidth << "\n";
			randomExportStream << "#Number of values per cell: " << sequence->exportRandomsChannelCount << "\n";
			if(sequence->exportRandomsAsReal)
				randomExportStream << "#Values are floating point in [0,1]\n";
			else if(sequence->exportRandomsAsBinary)
				randomExportStream << "#Values are either 0 or 1\n";
			else
				randomExportStream << "#Values are unsigned 32-bit integers";
		}
		uint i = 0;
		for(uint iRow = 0; iRow < sequence->maxRandomGridHeight; iRow++)
		{
			for(uint iPix = 0; iPix < sequence->maxRandomGridWidth ; iPix++)
			{
				for(uint iColor = 0; iColor < sequence->exportRandomsChannelCount; iColor++, i++)
				{
					if(sequence->exportRandomsAsReal)
						randomExportStream << ((double)randoms[i] / 0xffffffff) << "\t";
					else if(sequence->exportRandomsAsBinary)
						randomExportStream << (randoms[i] >> 31) << "\t";
					else
						randomExportStream << randoms[i] << "\t";
				}
				i += 4 - sequence->exportRandomsChannelCount;
				randomExportStream << "\t";
			}
			randomExportStream << "\n";
		}
		randomExportStream << "\n\n";
		delete randoms;
	}

}

void SequenceRenderer::enableExport(std::string path)
{
	boost::filesystem::path bpath(path);
	if( !boost::filesystem::exists( bpath.parent_path() ))
		boost::filesystem::create_directories( bpath.parent_path() );

	std::stringstream ss;
	std::time_t t = time(0);   // get time now
	struct std::tm  now;
	localtime_s( &now, &t );

	ss = std::stringstream();
	ss << path << "_" ;

	ss << (now.tm_year + 1900) << '-' 
			<< (now.tm_mon + 1) << '-'
			<<  now.tm_mday << "_" << now.tm_hour << "-" << now.tm_min << ".txt";

	randomExportStream.open(ss.str());

	if(sequence->exportRandomsWithHashmark)
	{
		randomExportStream << "# " << sequence->getDuration() << " frames to follow." << ".\n";
	}

}

Ticker::P SequenceRenderer::startTicker()
{
	Ticker::P ticker = Ticker::create(getSharedPtr());
	ticker->start(sequence->tickInterval, sequence->getFrameInterval_s());
	return ticker;
}


const Stimulus::SignalMap& SequenceRenderer::tick(uint& iTick)
{
	if(iFrame == 1)
		return noTickSignal;
	StimulusRendererMap::iterator iStimulusRenderer = stimulusRenderers.lower_bound(iFrame-1);
	if(iStimulusRenderer == stimulusRenderers.end())
		return noTickSignal;
	StimulusRenderer::P stimulusRenderer = iStimulusRenderer->second;
	iTick = stimulusRenderer->tick();
	return stimulusRenderer->getStimulus()->tickSignals;
}

void SequenceRenderer::skip(int skipCount)
{
	if(skipCount >= 100000000)
	{
		if(paused)
			paused = false;
	}
	bool measurementStarted = iFrame > sequence->getMeasurementStart();
	if(!measurementStarted && skipCount > 1 || iFrame > sequence->getMeasurementEnd() )
	{
		iFrame = sequence->getDuration();
		return;
	}
	StimulusRendererMap::iterator iStimulusRenderer = stimulusRenderers.lower_bound(iFrame);
	if(skipCount > 0)
		for(int i=0; i < skipCount-1 && iStimulusRenderer != stimulusRenderers.end(); i++)
		{
			iStimulusRenderer++;
		}
	if(skipCount < 0)
		for(int i=0; i < -skipCount+1 && iStimulusRenderer != stimulusRenderers.begin(); i++)
		{
			iStimulusRenderer--;
		}
	if(iStimulusRenderer != stimulusRenderers.end())
		iFrame = iStimulusRenderer->first+1;
	else
		iFrame = sequence->getDuration();
	if(measurementStarted && iFrame < sequence->getMeasurementStart())
		iFrame = sequence->getMeasurementStart();
	if(iFrame > sequence->getMeasurementEnd())
		iFrame = sequence->getMeasurementEnd();
}

bool SequenceRenderer::exporting() const
{
	return randomExportStream.is_open();
}


void SequenceRenderer::beginCalibrationFrame()
{
	if(calibrating) 
	{
		glViewport(
			0,
			0,
			calibrationImageWidth,
			calibrationImageHeight);

		calibrationImage->setRenderTarget();
	}
}

void SequenceRenderer::endCalibrationFrame()
{
	if(calibrating) // add to histogram
	{
		calibrationImage->disableRenderTarget();
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		histogramBuffer->setRenderTarget();
		histogramShader->enable();
		histogramShader->bindUniformTexture("inputBuffer", calibrationImage->getColorBuffer(0), 0);
		histogramShader->bindUniformFloat("histogramLevels", histogramResolution);
		histogramShader->bindUniformFloat("domainMin", histogramMin);
		histogramShader->bindUniformFloat("domainMax", histogramMax);
		nothing->renderPoints(calibrationImageWidth*calibrationImageHeight);
		histogramShader->disable();
		histogramBuffer->disableRenderTarget();
		glDisable(GL_BLEND);

		glFinish();

		glViewport(
			0,
			0,
			sequence->displayWidth_px,
			sequence->displayHeight_px);

		histogramDisplayShader->enable();
		histogramDisplayShader->bindUniformTexture("histogramBuffer", histogramBuffer->getColorBuffer(0), 0);
		histogramDisplayShader->bindUniformFloat("maxValue", histogramScale * (iFrame - calibrationStartingFrame));
		histogramDisplayShader->bindUniformFloat("domainMin", histogramMin);
		histogramDisplayShader->bindUniformFloat("domainMax", histogramMax);
		nothing->renderQuad();
		histogramDisplayShader->disable();
	}
}

void SequenceRenderer::enableCalibration(uint startingFrame, uint duration, float histogramMin, float histogramMax)
{
	calibrating = true;
	calibrationStartingFrame = startingFrame;
	calibrationDuration = duration;
	this->histogramMin = histogramMin;
	this->histogramMax = histogramMax;
	
//	histogramBuffer->clear();
//	histogramBuffer->setRenderTarget();
//	histogramClearShader->enable();
//	nothing->renderQuad();
//	histogramClearShader->disable();
//	histogramBuffer->disableRenderTarget();

	iFrame = calibrationStartingFrame;

}


void SequenceRenderer::readCalibrationResults()
{
	glBindTexture(GL_TEXTURE_2D, histogramBuffer->getColorBuffer(0));

	float histi[4096];
	histi[0] = 3;
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (void*)histi);

	float histogramTop = 0;
	double m = 0;
	double w = 0;
	for (int e = 0; e<histogramResolution; e++)
	{
		w += histi[e * 4] + histi[e * 4 + 1] + histi[e * 4 + 2];
		m += (histi[e * 4] + histi[e * 4 + 1] + histi[e * 4 + 2]) * (e / (float)histogramResolution * (histogramMax - histogramMin) + histogramMin);
		histogramTop = std::max(histogramTop, histi[e * 4]);
	}
	measuredMean = m / w;
	//TODO histogramScale = histogramTop / (float)(iFrame - calibrationStartingFrame);// * 0.004;

	double vari2=0;
	for (int e = 0; e<histogramResolution; e++)
	{
		double d = e / (float)histogramResolution * (histogramMax - histogramMin) + histogramMin - measuredMean;
		vari2 += (histi[e * 4] + histi[e * 4 + 1] + histi[e * 4 + 2]) * d*d;
	}
	measuredVariance = sqrt(vari2 / w);

	for (int e = 0; e<histogramResolution; e++)
	{
		if (histi[e * 4] + histi[e * 4 + 1] + histi[e * 4 + 2] > 10.5)
		{
			measuredDynRangeMin = e / (float)histogramResolution * (histogramMax - histogramMin) + histogramMin;
			break;
		}
	}
	for (int e = 0; e<histogramResolution-1; e++)
	{
		if (histi[(histogramResolution - 1 - e) * 4] + histi[(histogramResolution - 1 - e) * 4 + 1] + histi[(histogramResolution - 1 - e) * 4 + 2] > 10.5)
		{
			measuredDynRangeMax = (histogramResolution - e) / (float)histogramResolution * (histogramMax - histogramMin) + histogramMin;
			break;
		}
	}

	StimulusRendererMap::iterator iStimulusRenderer = stimulusRenderers.lower_bound(calibrationStartingFrame);
	if(iStimulusRenderer == stimulusRenderers.end())
		return;
	iStimulusRenderer->second->getStimulus()->setMeasuredDynamics(measuredDynRangeMin, measuredDynRangeMax, measuredMean, measuredVariance);


}

bool SequenceRenderer::renderFrameHidden()
{
	int nSkippedFrames = 0;

	raiseSignal("Tick");
	clearSignal("Tick");

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	if(!calibrating && !randomExportStream.is_open())
	{
		Sequence::SignalMap::const_iterator iSignal = sequence->getSignals().lower_bound(iFrame-1);
		Sequence::SignalMap::const_iterator eSignal = sequence->getSignals().upper_bound(iFrame);
		while(iSignal != eSignal)
		{
			if(iSignal->second.clear)
				clearSignal(iSignal->second.channel);
			else
				raiseSignal(iSignal->second.channel);
			iSignal++;
		}
		typedef std::chrono::high_resolution_clock Clock;
		typedef std::chrono::duration<float> Fsec;

		auto now = Clock::now();
		//Fsec  elapsed = now - firstFrameTimePoint;
		//Fsec  elapsedSinceLastFrame = now - previousFrameTimePoint;
		previousFrameTimePoint = now;
	}

	if(!paused)
		iFrame++;
	cFrame++;
	return true;
}

Stimulus::CP SequenceRenderer::getCurrentStimulus()
{
	StimulusRendererMap::iterator iStimulusRenderer = stimulusRenderers.lower_bound(iFrame);
	if(iStimulusRenderer == stimulusRenderers.end() )
		iStimulusRenderer = stimulusRenderers.begin();
	return iStimulusRenderer->second->getStimulus();
}

std::string SequenceRenderer::getSequenceTimingReport()
{
	typedef std::chrono::high_resolution_clock Clock;
	typedef std::chrono::duration<float> Fsec;

	std::stringstream ss;
	Fsec  elapsed = previousFrameTimePoint - firstFrameTimePoint;

	if(calibrating || randomExportStream.is_open())
		ss << "Frame rate not measured during tone calibration or random number export.<BR>";
	else if(cFrame == 0)
		ss << "Sequence aborted before measurement start.<BR>";
	else
	{
		ss << "Total measurement duration: " << elapsed.count() << "<BR>";
		ss << "Frames skipped/total: " << totalFramesSkipped << '/' << cFrame-1 << "<BR>";
		ss << "Measured system frame interval [s]: " << elapsed.count() / (cFrame-1.0) << "<BR>";
		ss << "Measured system frame rate [Hz]: " <<  (cFrame-1.0) / elapsed.count() << "<BR>";
	}
	return ss.str();
}