#include "stdafx.h"
#include "StimulusWindow.h"
#include <ctime>
#include <chrono>

void StimulusWindow::render()
{
	makeCurrent();
	glViewport(0, 0, screenw, screenh);

	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	if (sequenceRenderer->exporting())
		setSwapInterval(0);
	else
		setSwapInterval(sequenceRenderer->getSequence()->frameRateDivisor);

	sequenceRenderer->setScreenResolution(screenw, screenh);

	// render 3 frame in one image if high frequence device used
	size_t channelNum = sequenceRenderer->getSequence()->useHighFreqRender ? 3 : 1;

	for (size_t channelIdx = 0; channelIdx < channelNum; channelIdx++)
	{
		auto start = std::chrono::system_clock::now();
		if (!sequenceRenderer->renderFrame(0, channelIdx))
		{
			quit = true;
			break;
		}
		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsedSeconds = end - start;
		std::cout << "Length of renderFrame for " << (sequenceRenderer->getSequence()->useOpenCL ? "cl" : "gl") << "fft: " << elapsedSeconds.count() * 1000 << "ms." << std::endl;
	}
	//TODO finish
	// jelek az eszközre, nem tudjuk mikor vált framet az eszköz
	// 2. és 3.-nál felesleges kiküldeni az elektronikus jelet, ezt is ki kell kapcsolni
	// elektronikus jelek -> nem tud 3 frame-en belül történni
	
	swapBuffers();
	glFinish();
	if (ticker)
		ticker->onBufferSwap();
}

void StimulusWindow::preRender()
{
	makeCurrent();
	glViewport(0, 0, screenw, screenh);

	if (sequenceRenderer->getSequence()->getUsesBusyWaitingThreadForSingals())
		ticker = sequenceRenderer->startTicker();
}

void StimulusWindow::postRender()
{
	if (sequenceRenderer->getSequence()->getUsesBusyWaitingThreadForSingals())
		ticker->stop();
	ticker.reset();
	sequenceRenderer->reset();

	if (onHideCallback)
		onHideCallback();
}