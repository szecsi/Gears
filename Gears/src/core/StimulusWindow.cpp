#include "stdafx.h"
#include "StimulusWindow.h"

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

	// render 3 frame in one image

	size_t channelNum = sequenceRenderer->getSequence()->useHighFreqRender ? 3 : 1;
	// sequenceRenderer->SetHighFrequenceRender(true);
	for (size_t channelIdx = 0; channelIdx < channelNum; channelIdx++)
	{
		if (!sequenceRenderer->renderFrame(0, channelIdx))
		{
			quit = true;
			break;
		}
			//swapBuffers();
	}
	//TODO finished 

	// jelek az eszközre, nem tudjuk mikor vált framet az eszköz
	// 2. és 3.-nál felesleges kiküldeni az elektronikus jelet, ezt is ki kell kapcsolni
	// szürke árnyalatosra áttranszformálni
	// .pyx fájl ->DefaultSequence-nek paraméter, vagy ennek leszármazott, ebből tudja, hogy így kell renderelni
	// nem 3 számmal osztható frameünk van
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