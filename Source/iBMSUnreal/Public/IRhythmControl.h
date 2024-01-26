#pragma once

class IRhythmControl
{
public:
	virtual ~IRhythmControl() = default;
	virtual int PressLane(int MainLane, int CompensateLane, double InputDelay = 0) = 0;
	virtual int PressLane(int Lane, double InputDelay = 0) = 0;
	virtual void ReleaseLane(int Lane, double InputDelay = 0) = 0;
	virtual UWorld* GetContextWorld() = 0;
};
