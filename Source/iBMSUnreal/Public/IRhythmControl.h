#pragma once

class IRhythmControl
{
public:
	virtual ~IRhythmControl() = default;
	virtual void PressLane(int Lane, double InputDelay=0) = 0;
	virtual void ReleaseLane(int Lane, double InputDelay=0) = 0;
	virtual UWorld* GetContextWorld() = 0;
};
