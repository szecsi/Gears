#pragma once

#include "SpatialFilterRenderer.h"

class CLSpatialFilterRenderer: public SpatialFilterRenderer
{
	OPENCLFFT fft;
	OPENCLFFT fft_prepare;
	OPENCLFFT* current_fft = nullptr;
	OPENCLFFT* other_fft = nullptr;
	unsigned width;
	unsigned height;
	bool has_current_fft = false;
protected:
	void fftConvolution() override;
	void bindTexture(Shader* shader) override;
public:
	CLSpatialFilterRenderer(boost::shared_ptr<SequenceRenderer> sequenceRenderer, ShaderManager::P shaderManager, KernelManager::P _kernelManager, SpatialFilter::P _spatialFilter, unsigned int width, unsigned int height);
};