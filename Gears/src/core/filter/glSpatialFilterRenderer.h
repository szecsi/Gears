#pragma once
#include "SpatialFilterRenderer.h"

class GLSpatialFilterRenderer: public SpatialFilterRenderer
{
	GLFFT fft_rg;
	GLFFT fft_ba;
	GLFFT ifft_rg;
	GLFFT ifft_ba;
	Shader* convolutionShader;
protected:
	void fftConvolution() override;
	void bindTexture(Shader* shader) override;
	GLSpatialFilterRenderer(boost::shared_ptr<SequenceRenderer> sequenceRenderer, ShaderManager::P shaderManager, KernelManager::P _kernelManager, unsigned int width, unsigned int height);
public:
	template<typename... Args>
	inline static boost::shared_ptr<SpatialFilterRenderer> create(Args... args)
	{
		return boost::shared_ptr<SpatialFilterRenderer>(new GLSpatialFilterRenderer(args...));
	}
};