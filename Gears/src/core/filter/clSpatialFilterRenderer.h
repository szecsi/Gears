#pragma once

#include "SpatialFilterRenderer.h"

class CLSpatialFilterRenderer: public SpatialFilterRenderer
{
	OPENCLFFT fft;
	unsigned width;
	unsigned height;
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