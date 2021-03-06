﻿#include "stdafx.h"
#include "PassRenderer.h"
#include "SequenceRenderer.h"
#pragma region OpenGL
#include <GL/glew.h>
#pragma endregion includes for GLEW
#include <fstream>
#include <sstream>
#include <ctime>
#include <limits>
#include <chrono>
#include <iostream>

PassRenderer::PassRenderer(boost::shared_ptr<StimulusRenderer> stimulusRenderer, Pass::CP pass, 
		ShaderManager::P shaderManager, TextureManager::P textureManager):
	pass(pass),
	stimulusRenderer(stimulusRenderer),
	polytex(nullptr)
{
	stimulusGeneratorShader = shaderManager->loadShader(
		pass->getStimulusGeneratorVertexShaderSource(pass->rasterizationMode),
		pass->getStimulusGeneratorGeometryShaderSource(pass->rasterizationMode),
		pass->getStimulusGeneratorShaderSource());

	iFrame = 1;

	for(auto& iImage : pass->shaderImages)
	{
		Texture2D* texture = textureManager->loadTexture(iImage.second);
		textureSettings[iImage.first] = texture;
	}

	timelineShader = shaderManager->loadShader(
		pass->getTimelineVertexShaderSource(), 
		pass->getTimelineFragmentShaderSource());

	videoFrameY = nullptr;
	videoFrameU = nullptr;
	videoFrameV = nullptr;
	if( pass->hasVideo() )
	{
		if( movieDecoder.initialize(pass->getVideo()) )
		{
			movieDecoder.stop();
			movieDecoder.start();
			for(int iBuffering=0; iBuffering<10; iBuffering++)
			{
				if(movieDecoder.decodeVideoFrame(videoFrame))
					break;
				Sleep(100);
			}
			movieDecoder.bufferPackets();

			videoFrameY = new Texture2D();
			videoFrameU = new Texture2D();
			videoFrameV = new Texture2D();

			videoFrameY->initialize8bit(videoFrame.getYLineSize(), movieDecoder.getFrameHeight(), 8);
			videoFrameU->initialize8bit(videoFrame.getULineSize(), movieDecoder.getFrameHeight()/2, 8);
			videoFrameV->initialize8bit(videoFrame.getVLineSize(), movieDecoder.getFrameHeight()/2, 8);

			videoClipFactorY = Gears::Math::float2((float)movieDecoder.getFrameWidth() / videoFrame.getYLineSize(), -1);
			videoClipFactorUV = Gears::Math::float2((float)movieDecoder.getFrameWidth() / 2.f / videoFrame.getULineSize(), -1);

			videoFrameY->setData8bit(videoFrame.getYPlane());
			videoFrameU->setData8bit(videoFrame.getUPlane());
			videoFrameV->setData8bit(videoFrame.getVPlane());
			//movieDecoder.stop();
		}
		else
		{
			movieDecoder.destroy();
			//TODO could not open video
		}
	}

	std::string polyname = "TODO_use_polygon_repo_textures";
	//TODO POLYGON
	polytex = textureManager->get(polyname);
	if(polytex == nullptr && !pass->polygonMask.empty())
	{
		polytex = new Texture2D();
		polytex->initializeRG(pass->polygonMask.size(), 1, 128);
		polytex->setDataRG((float*)&pass->polygonMask[0]);
		textureSettings["vertices"] = polytex;
	}
	if(polytex == nullptr && !pass->quads.empty())
	{
		polytex = new Texture2D();
		polytex->initializeRG(pass->quads.size()*3, 1, 128);
		polytex->setDataRG((float*)&pass->quads[0]);
		textureSettings["quads"] = polytex;
	}
}

PassRenderer::~PassRenderer()
{
	movieDecoder.destroy();

	if(videoFrameY) delete videoFrameY;
	if(videoFrameU) delete videoFrameU;
	if(videoFrameV) delete videoFrameV;
}

void PassRenderer::renderPass(int skippedFrames)
{
	auto stimulus = pass->getStimulus();
	auto sequenceRenderer = stimulusRenderer->getSequenceRenderer();

	//TODO manage skipped frames in some consistent manner. With PRNGs or videos just skipping a frame is not really tolerable
	//iFrame += skippedFrames;

	float time = getCurrentFrame() / stimulus->sequence->deviceFrameRate * stimulus->sequence->frameRateDivisor;

	stimulusGeneratorShader->enable();

	if( stimulusRenderer->hasSpatialFilter() && stimulus->spatialFilter->useFft )
		stimulusGeneratorShader->bindUniformFloat2("patternSizeOnRetina", stimulus->sequence->getSpatialFilteredFieldWidth_um(), stimulus->sequence->getSpatialFilteredFieldHeight_um());
	else
		stimulusGeneratorShader->bindUniformFloat2("patternSizeOnRetina", stimulus->sequence->fieldWidth_um, stimulus->sequence->fieldHeight_um);

	if(stimulus->spatialFilter != nullptr && stimulus->spatialFilter->useFft)
	{
		stimulusGeneratorShader->bindUniformInt("swizzleForFft", stimulus->spatialFilter->fftSwizzleMask);
	}
	else
		stimulusGeneratorShader->bindUniformInt("swizzleForFft", 0xffffffff);

	stimulusGeneratorShader->bindUniformInt("frame", getCurrentFrame());
	stimulusGeneratorShader->bindUniformFloat("time", time);

	for(auto& setting : pass->shaderVariables)
	{
		stimulusGeneratorShader->bindUniformFloat(setting.first.c_str(), setting.second);
	}
	for(auto& setting : pass->shaderColors)
	{
		stimulusGeneratorShader->bindUniformFloat3(setting.first.c_str(), setting.second.x, setting.second.y, setting.second.z);
	}
	for(auto& setting : pass->shaderVectors)
	{
		stimulusGeneratorShader->bindUniformFloat2(setting.first.c_str(), setting.second.x, setting.second.y);
	}

	uint slot=0;
	for(auto& setting : textureSettings)
	{
		stimulusGeneratorShader->bindUniformTexture(setting.first.c_str(), setting.second->getTextureHandle(), slot++);
	}
	if(stimulus->temporalProcessingStateCount > 3)
	{
		stimulusGeneratorShader->bindUniformTextureArray("temporalProcessingState", sequenceRenderer->currentTemporalProcessingState->getColorBuffer(), slot++);
		stimulusGeneratorShader->bindUniformMatrix("stateTransitionMatrix", (float*)stimulus->temporalProcessingStateTransitionMatrix, 4);
	}
	else if(stimulus->temporalProcessingStateCount > 0)
	{
		stimulusGeneratorShader->bindUniformTextureArray("temporalProcessingState", sequenceRenderer->currentTemporalProcessingState->getColorBuffer(), slot++);
		stimulusGeneratorShader->bindUniformMatrix("stateTransitionMatrix", (float*)stimulus->temporalProcessingStateTransitionMatrix, 1);
	}

	if(stimulus->usesForwardRendering)
		stimulusGeneratorShader->bindUniformTexture("forwardImage", sequenceRenderer->forwardRenderedImage->getColorBuffer(0), slot++);

	if(stimulus->randomSeed != 0)
	{
		stimulusGeneratorShader->bindUniformTexture("randoms", sequenceRenderer->randomSequenceBuffers[0]->getColorBuffer(), slot++);
		stimulusGeneratorShader->bindUniformFloat2("cellSize", (float)stimulus->sequence->fieldWidth_um / stimulus->randomGridWidth, (float)stimulus->sequence->fieldHeight_um / stimulus->randomGridHeight);
		stimulusGeneratorShader->bindUniformInt2("randomGridSize", stimulus->randomGridWidth, stimulus->randomGridHeight);
	}

	if(stimulus->particleGridWidth != 0)
	{
		stimulusGeneratorShader->bindUniformTexture("particles", sequenceRenderer->particleBuffers[0]->getColorBuffer(), slot++);
	}

	if(stimulus->doesToneMappingInStimulusGenerator)
	{
		stimulusGeneratorShader->bindUniformFloat("toneRangeMin", stimulus->toneRangeMin);
		stimulusGeneratorShader->bindUniformFloat("toneRangeMax", stimulus->toneRangeMax);
		if(stimulus->toneMappingMode == Stimulus::ToneMappingMode::ERF)
		{
			stimulusGeneratorShader->bindUniformFloat("toneRangeMean",stimulus->toneRangeMean);
			stimulusGeneratorShader->bindUniformFloat("toneRangeVar", stimulus->toneRangeVar);
		}
		else
		{
			stimulusGeneratorShader->bindUniformFloat("toneRangeMean",0.f);
			stimulusGeneratorShader->bindUniformFloat("toneRangeVar", -1.f);
		}
		stimulusGeneratorShader->bindUniformBool("doTone", !stimulus->doesDynamicToneMapping);
		stimulusGeneratorShader->bindUniformBool("doGamma", !sequenceRenderer->calibrating && !stimulus->doesDynamicToneMapping);
		stimulusGeneratorShader->bindUniformTexture1D("gamma", stimulusRenderer->gammaTexture->getTextureHandle(), slot++);
		if(stimulusRenderer->measuredHistogramTexture)
			stimulusGeneratorShader->bindUniformTexture("histogram", stimulusRenderer->measuredHistogramTexture->getTextureHandle(), slot++);
		stimulusGeneratorShader->bindUniformInt("gammaSampleCount", stimulus->gammaSamplesCount);
	}


	if(pass->hasVideo() && videoFrameY != nullptr )
	{
		stimulusGeneratorShader->bindUniformTexture("videoFrameY", videoFrameY->getTextureHandle(), slot++);
		stimulusGeneratorShader->bindUniformTexture("videoFrameU", videoFrameU->getTextureHandle(), slot++);
		stimulusGeneratorShader->bindUniformTexture("videoFrameV", videoFrameV->getTextureHandle(), slot++); 
		stimulusGeneratorShader->bindUniformFloat2("videoClipFactorY", videoClipFactorY.x, -videoClipFactorY.y);
		stimulusGeneratorShader->bindUniformFloat2("videoClipFactorUV", videoClipFactorUV.x, -videoClipFactorUV.y);
	}

	if(pass->rasterizationMode == Pass::RasterizationMode::fullscreen)
		sequenceRenderer->getNothing()->renderQuad();
	else if(pass->rasterizationMode == Pass::RasterizationMode::triangles)
		sequenceRenderer->getNothing()->renderTriangles(pass->polygonMask.size());
	else if(pass->rasterizationMode == Pass::RasterizationMode::quads)
	{
		glEnable(GL_BLEND);
		if(pass->transparent)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else
			glBlendFunc(GL_ONE, GL_ONE);
		sequenceRenderer->getNothing()->renderPoints(pass->quads.size());
		glDisable(GL_BLEND);
	}
	stimulusGeneratorShader->disable();

	if(!sequenceRenderer->paused)
	{
		iFrame++;
		if(pass->hasVideo() && videoFrameY != nullptr )
		{
			if(movieDecoder.decodeVideoFrame(videoFrame))
			{
				videoFrameY->setData8bit(videoFrame.getYPlane());
				videoFrameU->setData8bit(videoFrame.getUPlane());
				videoFrameV->setData8bit(videoFrame.getVPlane());
			}
		}
	}
}

void PassRenderer::renderSample(uint sFrame)
{
	uint cFrame = iFrame;
	auto cVideoFrameY = videoFrameY;
	videoFrameY = nullptr;
	iFrame = sFrame;
	renderPass(0);
	iFrame = cFrame;
	videoFrameY = cVideoFrameY;
}

void PassRenderer::renderTimeline(uint startFrame, uint frameCount)
{
	auto stimulus = pass->getStimulus();
	auto sequenceRenderer = stimulusRenderer->getSequenceRenderer();

	glPushMatrix();
	//if(signals != nullptr)
	{
		glScaled(1, 0.4, 1);
		glTranslated(0, 0.25, 0);
	}
	glColor4d(0.25, 0, 0, 1);
	glBegin(GL_LINES);
		glVertex2d(0, 0);
		glVertex2d(100000000, 0);
		glVertex2d(0, 1);
		glVertex2d(100000000, 1);
	glEnd();
	timelineShader->enable();
	timelineShader->bindUniformFloat("frameInterval", stimulus->sequence->getFrameInterval_s());
	uint stride = frameCount / 4000 + 1;
	timelineShader->bindUniformInt("startFrame", startFrame);
	timelineShader->bindUniformInt("stride", stride);
	for(auto& setting : pass->shaderVariables)
	{
		timelineShader->bindUniformFloat(setting.first.c_str(), setting.second);
	}
	for(auto& setting : pass->shaderColors)
	{
		timelineShader->bindUniformFloat3(setting.first.c_str(), setting.second.x, setting.second.y, setting.second.z);
	}
	for(auto& setting : pass->shaderVectors)
	{
		timelineShader->bindUniformFloat2(setting.first.c_str(), setting.second.x, setting.second.y);
	}
	//sequenceRenderer->getNothing()->renderLineStrip(stimulus->getDuration()+1);
	sequenceRenderer->getNothing()->renderLineStrip(frameCount * 2 / stride /*stimulus->getDuration()*2*/);
	timelineShader->disable();
	glColor4d(0.25, 0, 0, 1);
	glLineWidth(2.0);
	glEnable(GL_LINE_STIPPLE);
	glLineStipple(2, 0x5555);
	glBegin(GL_LINES);
		glVertex2d(0, 1.35);
		glVertex2d(0, -0.05);
	glEnd();
	glDisable(GL_LINE_STIPPLE);

	glPopMatrix();

}

void PassRenderer::reset()
{
	if( pass->hasVideo() && videoFrameY != nullptr)
	{
		movieDecoder.stop();
		movieDecoder.seek(0);
		movieDecoder.start();
		for(int iBuffering=0; iBuffering<10; iBuffering++)
		{
			if(movieDecoder.decodeVideoFrame(videoFrame))
				break;
			Sleep(100);
		}
		//movieDecoder.bufferPackets();

		videoFrameY->setData8bit(videoFrame.getYPlane());
		videoFrameU->setData8bit(videoFrame.getUPlane());
		videoFrameV->setData8bit(videoFrame.getVPlane());
	}
	iFrame = 1;
}

void PassRenderer::skipFrames(uint nFramesToSkip)
{
	iFrame += nFramesToSkip;
}