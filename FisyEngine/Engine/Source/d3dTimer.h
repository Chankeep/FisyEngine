#pragma once

#include<windows.h>

class d3dTimer
{
public:
	d3dTimer();

	float GetTotalTime() const;
	float GetDeltaTime() const { return static_cast<float>(deltaTime); }

	void Reset();
	void Start();
	void Stop();
	void Tick();

	bool isStopped() const { return stopping; }

private:
	double secondsPerCount = 0.0;

	//unit: second
	double deltaTime = -1.0;

	__int64 baseTime = 0;
	__int64 pausedTime = 0;
	__int64 stopTime = 0;
	__int64 lastTime = 0;
	__int64 currentTime = 0;

	bool stopping;
};
