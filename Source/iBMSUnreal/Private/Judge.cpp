#include "Judge.h"

#include "TimeLine.h"

FJudge::FJudge(const int Rank)
{
	const int Clamped = ClampRank(Rank);
	TimingWindows = TimingWindowsByRank[Clamped];
}

bool FJudge::CheckRange(const long long Diff, const long long Early, const long long Late)
{
	return Early <= Diff && Diff <= Late;
}

FJudgeResult FJudge::JudgeNow(const FBMSNote* Note, const long long InputTime)
{
	const auto& Timeline = Note->Timeline;
	const long long Diff = InputTime - Timeline->Timing;
	// check range for each judgement
	for(const auto& Pair : TimingWindows)
	{
		const auto& Judgement = Pair.Key;
		const auto& Range = Pair.Value;
		if(CheckRange(Diff, Range.Key, Range.Value))
		{
			return FJudgeResult{Judgement, Diff};
		}
	}
	
	return FJudgeResult{None, Diff};
}

int FJudge::ClampRank(const int Rank)
{
	return FMath::Clamp(Rank, 0, 3);
}

FString FJudge::GetRankDescription(const int Rank)
{
	switch(Rank)
	{
	case 0:
		return "VERY HARD";
	case 1:
		return "HARD";
	case 2:
		return "NORMAL";
	case 3:
		return "EASY";
	default:
		return "EASY";
	}
}