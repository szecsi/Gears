#include "stdafx.h"
#include "Ticker.h"
#include "SequenceRenderer.h"
#include <iostream>

void Ticker::start(float tickInterval_s, float frameInterval_s)
{
	ticksPerFrame = (int)(frameInterval_s / tickInterval_s + 0.5);
	ticksToGoInCurrentFrame = 0;
	vsync = false;

	tickInterval = std::chrono::duration_cast< std::chrono::high_resolution_clock::duration > (std::chrono::nanoseconds((long long)(tickInterval_s * 1000000000.0)));

	tickerThread = new std::thread(&Ticker::run, this);
	SetThreadPriority( tickerThread->native_handle(), THREAD_PRIORITY_HIGHEST);
}

void Ticker::stop()
{
	live = false;
	tickerThread->join();
	delete tickerThread;
}

void Ticker::onBufferSwap()
{
//	typedef std::chrono::high_resolution_clock Clock;
//	ticksToGoInCurrentFrame = ticksPerFrame;
//	previousTickTimePoint = Clock::now() - tickInterval;

	vsync = true;
}

void Ticker::run() 
{
	typedef std::chrono::high_resolution_clock Clock;

	auto spikeDuration = std::chrono::high_resolution_clock::duration( (std::chrono::high_resolution_clock::rep)( 100.0) );

	auto previousTickTimePoint = Clock::now();
	while(live)
	{
		auto t = Clock::now();
		auto dt = t - previousTickTimePoint; 
		if(vsync)
		{
			vsync = false;
			if(dt < tickInterval / 2)
				previousTickTimePoint = Clock::now();
			else
				previousTickTimePoint = Clock::now() - tickInterval;
		}
		else if(dt > tickInterval)
		{
			previousTickTimePoint += tickInterval;
			uint iTick = 1;
			auto& signals = sequenceRenderer->tick(iTick);

			//for(auto& signal : signals)
			//{
			//	if( (signal.first != 0 && iTick % signal.first == signal.first-1) || (iTick==1 && signal.first == 0))
			//	{
			//		if(signal.second.clear)
			//			sequenceRenderer->clearSignal(signal.second.channel);
			//		else
			//			sequenceRenderer->raiseSignal(signal.second.channel);
			//	}
			//}
			Stimulus::SignalMap::const_iterator iSignal = signals.find(iTick);
			bool handled = false;
			while(iSignal != signals.end() && iSignal->first == iTick)
			{
				if(iSignal->second.clear)
					sequenceRenderer->clearSignal(iSignal->second.channel);
				else
					sequenceRenderer->raiseSignal(iSignal->second.channel);
				iSignal++;
				handled = true;
			}
			if(!handled)
			{
				Stimulus::SignalMap::const_iterator iSignal = signals.find(0);
				while(iSignal != signals.end() && iSignal->first == 0)
				{
					if(iSignal->second.clear)
						sequenceRenderer->clearSignal(iSignal->second.channel);
					else
						sequenceRenderer->raiseSignal(iSignal->second.channel);
					iSignal++;
				}
			}
		}
	}
}