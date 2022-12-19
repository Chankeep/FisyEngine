#include "d3dTimer.h"
d3dTimer::d3dTimer()
{
	__int64 countsPerSec;
	//得到当前计数器的频率（frequency）
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSec));
	//计数器的频率的倒数就是每个计数所代表的秒数
	secondsPerCount = 1.0 / static_cast<double>(countsPerSec);
}

float d3dTimer::GetTotalTime() const
{
	if (stopping)
	{
		return static_cast<float>((stopTime - pausedTime - baseTime) * secondsPerCount);
	}
	else
	{
		return static_cast<float>((currentTime - pausedTime - baseTime) * secondsPerCount);
	}
}

void d3dTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

	baseTime = currTime;
	lastTime = currTime;
	stopTime = 0;
	stopping = false;
}

void d3dTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));

	if (stopping)
	{
		pausedTime += (startTime - stopTime);
		lastTime = startTime;

		stopTime = 0;
		stopping = false;
	}

}

void d3dTimer::Stop()
{
	if (!stopping)
	{
		__int64 currTime;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

		stopTime = currTime;
		stopping = true;
	}
}

void d3dTimer::Tick()
{
	if (stopping)
	{
		deltaTime = 0.0;
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));
	currentTime = currTime;

	deltaTime = (currentTime - lastTime) * secondsPerCount;
	lastTime = currentTime;

	if (deltaTime < 0.0)
	{
		deltaTime = 0.0;
	}
}
