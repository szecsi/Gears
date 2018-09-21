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
	CLSpatialFilterRenderer(boost::shared_ptr<SequenceRenderer> sequenceRenderer, ShaderManager::P shaderManager, KernelManager::P _kernelManager, unsigned int width, unsigned int height);
public:
	template<typename... Args>
	inline static boost::shared_ptr<SpatialFilterRenderer> create(Args... args)
	{
		return boost::shared_ptr<SpatialFilterRenderer>(new CLSpatialFilterRenderer(args...));
	}
};