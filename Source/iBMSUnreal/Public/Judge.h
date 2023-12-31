// ReSharper disable StringLiteralTypo
// ReSharper disable IdentifierTypo
#pragma once

#include "CoreMinimal.h"
#include "BMSNote.h"


enum EJudgement
{
	
	PGreat,
	Great,
	Good,
	Bad,
	Kpoor,
	Poor,
	None,
	EJudgementCount
};
class FJudgeResult
{
public:
	FJudgeResult(EJudgement Judgement, long long Diff): Judgement(Judgement), Diff(Diff) {}
	EJudgement Judgement = None;
	long long Diff;
	bool IsComboBreak() const
	{
		return Judgement == Bad || Judgement == Poor;
	}

	bool IsNotePlayed() const
	{
		return Judgement != Kpoor && Judgement != None;
	}


	FString ToString() const
	{
		switch(Judgement)
		{
		case PGreat:
			return "PGREAT";
		case Great:
			return "GREAT";
		case Good:
			return "GOOD";
		case Bad:
			return "BAD";
		case Kpoor:
			return "KPOOR";
		case Poor:
			return "POOR";
		case None:
			return "NONE";
		default:
			return "NONE";
		}
	}
};
class IBMSUNREAL_API FJudge
{
private:
	// dictionary for timing windows. JudgeRank -> {Judgement -> (early, late)}
	inline static const TMap<EJudgement, TPair<long long, long long>> TimingWindowsByRank[4] = 
	{
		TMap<EJudgement, TPair<long long, long long>>
		{
				{PGreat, TPair<long long, long long>(-5000, 5000)},
				{Great, TPair<long long, long long>(-15000, 15000)},
				{Good, TPair<long long, long long>(-37500, 37500)},
				{Bad, TPair<long long, long long>(-385000, 490000)},
				{Kpoor, TPair<long long, long long>(-500000, 150000)}
		},
		TMap<EJudgement, TPair<long long, long long>>
		{
				{PGreat, TPair<long long, long long>(-10000, 10000)},
				{Great, TPair<long long, long long>(-30000, 30000)},
				{Good, TPair<long long, long long>(-75000, 75000)},
				{Bad, TPair<long long, long long>(-330000, 420000)},
				{Kpoor, TPair<long long, long long>(-500000, 150000)}
		},
		TMap<EJudgement, TPair<long long, long long>>
		{
				{PGreat, TPair<long long, long long>(-15000, 15000)},
				{Great, TPair<long long, long long>(-45000, 45000)},
				{Good, TPair<long long, long long>(-112500, 112500)},
				{Bad, TPair<long long, long long>(-275000, 350000)},
				{Kpoor, TPair<long long, long long>(-500000, 150000)}
		},
		TMap<EJudgement, TPair<long long, long long>>
		{
				{PGreat, TPair<long long, long long>(-20000, 20000)},
				{Great, TPair<long long, long long>(-60000, 60000)},
				{Good, TPair<long long, long long>(-150000, 150000)},
				{Bad, TPair<long long, long long>(-220000, 280000)},
				{Kpoor, TPair<long long, long long>(-500000, 150000)}
		}
	};


	TMap<EJudgement, TPair<long long, long long>> TimingWindows;

public:
	explicit FJudge(int Rank);
	static bool CheckRange(long long Diff, long long Early, long long Late);
	FJudgeResult JudgeNow(const FBMSNote* Note, long long InputTime);
	static int ClampRank(int Rank);
	static FString GetRankDescription(int Rank);	
};