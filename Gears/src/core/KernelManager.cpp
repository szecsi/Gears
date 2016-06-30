#include "stdafx.h"
#include "KernelManager.h"
#include "SequenceRenderer.h"


KernelManager::KernelManager(SequenceRenderer::P sequenceRenderer, ShaderManager::P shaderManager):
	sequenceRenderer(sequenceRenderer),
	shaderManager(shaderManager)
{
}

unsigned int KernelManager::getKernel(SpatialFilter::P spatialFilter)
{
	KernelMap::iterator i = kernels.find(spatialFilter->name);
	if (i != kernels.end())
		return i->second;

	std::string fragmentShaderSource = spatialFilter->getKernelGeneratorShaderSource();

	Shader* kernelShader = shaderManager->loadShader(fragmentShaderSource);

	Sequence::CP sequence = sequenceRenderer->getSequence();

	auto renderKernelLambda = 
		[&] () {
				kernelShader->enable();
				kernelShader->bindUniformBool("kernelGivenInFrequencyDomain", spatialFilter->kernelGivenInFrequencyDomain ); 
				if(spatialFilter->useFft)
				{
					if(spatialFilter->kernelGivenInFrequencyDomain)
						kernelShader->bindUniformFloat2("patternSizeOnRetina", 1000000.0 / sequence->getSpatialKernelWidth_um(), 1000000.0 / sequence->getSpatialKernelHeight_um());
					else
						kernelShader->bindUniformFloat2("patternSizeOnRetina", sequence->getSpatialKernelWidth_um(), sequence->getSpatialKernelHeight_um());
				}
				else
					kernelShader->bindUniformFloat2("patternSizeOnRetina", spatialFilter->width_um, spatialFilter->height_um);

				kernelShader->bindUniformFloat2("texelSize_um", spatialFilter->width_um / spatialFilter->horizontalSampleCount, spatialFilter->height_um / spatialFilter->verticalSampleCount);

				for(auto& setting : spatialFilter->shaderVariables)
				{
					kernelShader->bindUniformFloat(setting.first.c_str(), setting.second);
				}
				for(auto& setting : spatialFilter->shaderColors)
				{
					kernelShader->bindUniformFloat3(setting.first.c_str(), setting.second.x, setting.second.y, setting.second.z);
				}
				for(auto& setting : spatialFilter->shaderVectors)
				{
					kernelShader->bindUniformFloat2(setting.first.c_str(), setting.second.x, setting.second.y);
				}
			
				sequenceRenderer->getNothing()->renderQuad();
				kernelShader->disable();
		};

	if(spatialFilter->useFft)
	{
		FFT fft(sequence->fftWidth_px, sequence->fftHeight_px);
		fft.set_input( renderKernelLambda );

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		fft.redraw_input();
		if(!spatialFilter->kernelGivenInFrequencyDomain)
			fft.do_fft();
		unsigned int kid = fft.get_output();
		kernels[spatialFilter->name] = kid;
		return kid;
	}
	else
	{
		Framebuffer* bufi = new Framebuffer(spatialFilter->horizontalSampleCount, spatialFilter->verticalSampleCount, 1);
		bufi->setRenderTarget(0);
		renderKernelLambda();
		bufi->disableRenderTarget();
		unsigned int kid = bufi->getColorBuffer(0);
		kernels[spatialFilter->name] = kid;
		return kid;
	}
}

void KernelManager::clear()
{
	kernels.clear();
}