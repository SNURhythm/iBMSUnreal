// Fill out your copyright notice in the Description page of Project Settings.


#include "BMSParser.h"
#include <random>
#include "BMSLongNote.h"
#include "BMSNote.h"
#include "BMSLandmineNote.h"
#include "TimeLine.h"
#include "Measure.h"
#include "ShiftJISConverter.h"
#include "SHA256.h"
enum Channel {
	LaneAutoplay = 1,
	SectionRate = 2,
	BpmChange = 3,
	BgaPlay = 4,
	PoorPlay = 6,
	LayerPlay = 7,
	BpmChangeExtend = 8,
	Stop = 9,

	P1KeyBase = 1 * 36 + 1,
	P2KeyBase = 2 * 36 + 1,
	P1InvisibleKeyBase = 3 * 36 + 1,
	P2InvisibleKeyBase = 4 * 36 + 1,
	P1LongKeyBase = 5 * 36 + 1,
	P2LongKeyBase = 6 * 36 + 1,
	P1MineKeyBase = 13 * 36 + 1,
	P2MineKeyBase = 14 * 36 + 1
};
namespace KeyAssign {
	int Beat7[] = { 0, 1, 2, 3, 4, 7, -1, 5, 6, 8, 9, 10, 11, 12, 15, -1, 13, 14 };
	int PopN[] = { 0, 1, 2, 3, 4, -1, -1, -1, -1, -1, 5, 6, 7, 8, -1, -1, -1, -1 };
};

const int TempKey = 16;
const int Scroll = 1020;

FBMSParser::FBMSParser(): BpmTable{}, StopLengthTable{}
{
	std::random_device seeder;
	Seed = seeder();
}

void FBMSParser::SetRandomSeed(int RandomSeed)
{
	Seed = RandomSeed;
}

int FBMSParser::NoWav = -1;
int FBMSParser::MetronomeWav = -2;

void FBMSParser::Parse(FString path, FChart** chart, bool addReadyMeasure, bool metaOnly, std::atomic_bool& bCancelled)
{
	auto Chart = new FChart();
	*chart = Chart;
	Chart->Meta = new FChartMeta();
	Chart->Meta->BmsPath = path;
	Chart->Meta->Folder = FPaths::GetPath(path);
	FRegexPattern headerRegex = FRegexPattern(TEXT("^#([A-Za-z]+?)(\\d\\d)? +?(.+)?"));

	// implement the same thing as BMSParser.cs
	auto measures = TMap<int, TArray<TPair<int, FString>>>();
	TArray<uint8> bytes;
	FFileHelper::LoadFileToArray(bytes, *path);
	if(bCancelled) return;
	
	Chart->Meta->MD5 = FMD5::HashBytes(bytes.GetData(), bytes.Num());
	Chart->Meta->SHA256 = sha256(bytes);
	// bytes to FString
	FString content;
	ShiftJISConverter::BytesToUTF8(content, bytes.GetData(), bytes.Num());
	TArray<int> RandomStack;
	TArray<bool> SkipStack;
	// init prng with seed
	std::mt19937_64 Prng(Seed);
	
	
	auto lines = TArray<FString>();
	content.ParseIntoArrayLines(lines);
	auto lastMeasure = -1;

	for (auto& line : lines)
	{
		if(bCancelled) return;
		if (!line.StartsWith("#")) continue;
		auto upperLine = line.ToUpper();
		if(upperLine.StartsWith("#IF")) // #IF n
		{
			if(RandomStack.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("RandomStack is empty!"));
				continue;
			}
			int CurrentRandom = RandomStack.Last();
			int n = FCString::Atoi(*line.Mid(4));
			SkipStack.Push(CurrentRandom != n);
			continue;
		}
		if(upperLine.StartsWith("#ELSE"))
		{
			
			if(SkipStack.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("SkipStack is empty!"));
				continue;
			}
			bool CurrentSkip = SkipStack.Pop();
			SkipStack.Push(!CurrentSkip);
			continue;
		}
		if(upperLine.StartsWith("#ELSEIF"))
		{
			if(SkipStack.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("SkipStack is empty!"));
				continue;
			}
			bool CurrentSkip = SkipStack.Pop();
			int CurrentRandom = RandomStack.Last();
			int n = FCString::Atoi(*line.Mid(8));
			SkipStack.Push(CurrentSkip && CurrentRandom != n);
			continue;
		}
		if(upperLine.StartsWith("#ENDIF") || upperLine.StartsWith("#END IF"))
		{
			if(SkipStack.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("SkipStack is empty!"));
				continue;
			}
			SkipStack.Pop();
			continue;
		}
		if(!SkipStack.IsEmpty() && SkipStack.Last()) continue;
		if(upperLine.StartsWith("#RANDOM") || upperLine.StartsWith("#RONDAM")) // #RANDOM n
		{
			int n = FCString::Atoi(*line.Mid(7));
			std::uniform_int_distribution<int> dist(1, n);
			RandomStack.Push(dist(Prng));
			continue;
		}
		if(upperLine.StartsWith("#ENDRANDOM"))
		{
			if(RandomStack.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("RandomStack is empty!"));
				continue;
			}
			RandomStack.Pop();
			continue;
		}
		if (line.Len() >= 7 && FChar::IsDigit(line[1]) && FChar::IsDigit(line[2]) && FChar::IsDigit(line[3]) && line[6] == ':')
		{
			auto measure = FCString::Atoi(*line.Mid(1, 3));
			lastMeasure = FMath::Max(lastMeasure, measure);
			FString ch = line.Mid(4, 2);
			int channel;
			FString value;
			channel = DecodeBase36(ch);
			value = line.Mid(7);
			if (!measures.Contains(measure))
			{
				measures.Add(measure, TArray<TPair<int, FString>>());
			}
			measures[measure].Add(TPair<int, FString>(channel, value));
		}
		else
		{
			
			if (upperLine.StartsWith("#WAV") || upperLine.StartsWith("#BMP"))
			{
				if (line.Len() < 7) continue;
				auto xx = line.Mid(4, 2);
				auto value = line.Mid(7);
				FString cmd = upperLine.Mid(1, 3);
				ParseHeader(Chart, cmd, xx, value);
			}
			else if (upperLine.StartsWith("#STOP"))
			{
				if (line.Len() < 8) continue;
				auto xx = line.Mid(5, 2);
				auto value = line.Mid(8);
				FString cmd = "STOP";
				ParseHeader(Chart, cmd, xx, value);
			}
			else if (upperLine.StartsWith("#BPM"))
			{
				if (line.Mid(4).StartsWith(" "))
				{
					auto value = line.Mid(5);
					FString cmd = "BPM";
					FString xx = "";
					ParseHeader(Chart, cmd, xx, value);
				}
				else
				{
					if (line.Len() < 7) continue;
					auto xx = line.Mid(4, 2);
					auto value = line.Mid(7);
					FString cmd = "BPM";
					ParseHeader(Chart, cmd, xx, value);
				}
			}
			else
			{
				FString copy = line;
				auto matcher = FRegexMatcher(headerRegex, copy);

				if (matcher.FindNext())
				{
					auto cmd = matcher.GetCaptureGroup(1);
					/*auto xx = match.Groups.Num() > 3 ? match.Groups[2].ToString() : nullptr;*/
					auto xx = matcher.GetCaptureGroup(2);
					auto value = matcher.GetCaptureGroup(3);
					if (value.IsEmpty())
					{
						value = xx;
						xx = "";
					}
					ParseHeader(Chart, cmd, xx, value);
				}
			}
		}
	}

	if (addReadyMeasure)
	{
		measures.Add(0, TArray<TPair<int, FString>>());
		measures[0].Add(TPair<int, FString>(Channel::LaneAutoplay, "********"));
	}

	double timePassed = 0;
	int totalNotes = 0;
	int totalLongNotes = 0;
	int totalScratchNotes = 0;
	int totalBackSpinNotes = 0;
	int totalLandmineNotes = 0;
	auto currentBpm = Chart->Meta->Bpm;
	auto minBpm = Chart->Meta->Bpm;
	auto maxBpm = Chart->Meta->Bpm;
	auto lastNote = TArray<FBMSNote*>();
	lastNote.Init(nullptr, TempKey);
	auto lnStart = TArray<FBMSLongNote*>();
	lnStart.Init(nullptr, TempKey);

	for (auto i = 0; i <= lastMeasure; ++i)
	{
		if(bCancelled) return;
		if (!measures.Contains(i))
		{
			measures.Add(i, TArray<TPair<int, FString>>());
		}

		// gcd (int, int)
		auto measure = new FMeasure();
		auto timelines = TSortedMap<double, FTimeLine*>();

		for (auto& pair : measures[i])
		{
			if(bCancelled) break;
			auto channel = pair.Key;
			auto& data = pair.Value;
			if (channel == Channel::SectionRate)
			{
				measure->Scale = FCString::Atod(*data);
				continue;
			}

			auto laneNumber = 0; // NOTE: This is intentionally set to 0, not -1!
			if (channel >= Channel::P1KeyBase && channel < Channel::P1KeyBase + 9)
			{
				laneNumber = KeyAssign::Beat7[channel - Channel::P1KeyBase];
				channel = Channel::P1KeyBase;
			}
			else if (channel >= Channel::P2KeyBase && channel < Channel::P2KeyBase + 9)
			{
				laneNumber = KeyAssign::Beat7[channel - Channel::P2KeyBase + 9];
				channel = Channel::P1KeyBase;
			}
			else if (channel >= Channel::P1InvisibleKeyBase && channel < Channel::P1InvisibleKeyBase + 9)
			{
				laneNumber = KeyAssign::Beat7[channel - Channel::P1InvisibleKeyBase];
				channel = Channel::P1InvisibleKeyBase;
			}
			else if (channel >= Channel::P2InvisibleKeyBase && channel < Channel::P2InvisibleKeyBase + 9)
			{
				laneNumber = KeyAssign::Beat7[channel - Channel::P2InvisibleKeyBase + 9];
				channel = Channel::P1InvisibleKeyBase;
			}
			else if (channel >= Channel::P1LongKeyBase && channel < Channel::P1LongKeyBase + 9)
			{
				laneNumber = KeyAssign::Beat7[channel - Channel::P1LongKeyBase];
				channel = Channel::P1LongKeyBase;
			}
			else if (channel >= Channel::P2LongKeyBase && channel < Channel::P2LongKeyBase + 9)
			{
				laneNumber = KeyAssign::Beat7[channel - Channel::P2LongKeyBase + 9];
				channel = Channel::P1LongKeyBase;
			}
			else if (channel >= Channel::P1MineKeyBase && channel < Channel::P1MineKeyBase + 9)
			{
				laneNumber = KeyAssign::Beat7[channel - Channel::P1MineKeyBase];
				channel = Channel::P1MineKeyBase;
			}
			else if (channel >= Channel::P2MineKeyBase && channel < Channel::P2MineKeyBase + 9)
			{
				laneNumber = KeyAssign::Beat7[channel - Channel::P2MineKeyBase + 9];
				channel = Channel::P1MineKeyBase;
			}

			if (laneNumber == -1) continue;
			auto isScratch = laneNumber == 7 || laneNumber == 15;
			if (laneNumber == 5 || laneNumber == 6 || laneNumber == 13 || laneNumber == 14)
			{
				if(Chart->Meta->KeyMode == 5) Chart->Meta->KeyMode = 7;
				else if(Chart->Meta->KeyMode == 10) Chart->Meta->KeyMode = 14;
			}
			if (laneNumber >= 8) {
				if(Chart->Meta->KeyMode == 7) Chart->Meta->KeyMode = 14;
				else if(Chart->Meta->KeyMode == 5)Chart->Meta->KeyMode = 10;
				Chart->Meta->IsDP = true;
			}

			auto dataCount = data.Len() / 2;
			for (auto j = 0; j < dataCount; ++j)
			{
				if(bCancelled) break;
				auto val = data.Mid(j * 2, 2);
				if (val == "00")
				{
					if (timelines.Num() == 0 && j == 0)
					{
						auto timeline = new FTimeLine(TempKey, metaOnly);
						timelines.Add(0, timeline); // add ghost timeline
					}

					continue;
				}

				auto g = Gcd(j, dataCount);
				// ReSharper disable PossibleLossOfFraction
				auto position = static_cast<double>(j / g) / (dataCount / g);

				if (!timelines.Contains(position)) {
					auto timeline = new FTimeLine(TempKey, metaOnly);
					timelines.Add(position, timeline);
				}

				auto timeline = timelines[position];
				if (channel == Channel::LaneAutoplay || channel == Channel::P1InvisibleKeyBase)
				{
					if (metaOnly) break;
				}
				switch (channel)
				{
				case Channel::LaneAutoplay:
					if (metaOnly) break;
					if (val == "**")
					{
						timeline->AddBackgroundNote(new FBMSNote{ MetronomeWav });
						break;
					}
					if (DecodeBase36(val) != 0)
					{
						auto bgNote = new FBMSNote{ ToWaveId(Chart, val) };
						timeline->AddBackgroundNote(bgNote);
					}

					break;
				case Channel::BpmChange:
					timeline->Bpm = FParse::HexNumber(*val);
					// Debug.Log($"BPM_CHANGE: {timeline.Bpm}, on measure {i}");
					timeline->BpmChange = true;
					break;
				case Channel::BgaPlay:
					timeline->BgaBase = DecodeBase36(val);
					break;
				case Channel::PoorPlay:
					timeline->BgaPoor = DecodeBase36(val);
					break;
				case Channel::LayerPlay:
					timeline->BgaLayer = DecodeBase36(val);
					break;
				case Channel::BpmChangeExtend:
					{
						auto id = DecodeBase36(val);
						if (!CheckResourceIdRange(id))
						{
							UE_LOG(LogTemp, Warning, TEXT("Invalid BPM id: %s"), *val);
							break;
						}
						if(BpmTable.Contains(id))
							timeline->Bpm = BpmTable[id];
						else
							timeline->Bpm = 0;
						// Debug.Log($"BPM_CHANGE_EXTEND: {timeline.Bpm}, on measure {i}, {val}");
						timeline->BpmChange = true;
						break;
					}
				case Channel::Stop:
					{
						auto id = DecodeBase36(val);
						if (!CheckResourceIdRange(id))
						{
							UE_LOG(LogTemp, Warning, TEXT("Invalid StopLength id: %s"), *val);
							break;
						}
						if (StopLengthTable.Contains(id))
							timeline->StopLength = StopLengthTable[id];
						else
							timeline->StopLength = 0;
						// Debug.Log($"STOP: {timeline.StopLength}, on measure {i}");
						break;
					}
				case Channel::P1KeyBase: {
					auto ch = DecodeBase36(val);
					if (ch == Lnobj && lastNote[laneNumber] != nullptr) {
						if (isScratch)
						{
							++totalBackSpinNotes;
						}
						else
						{
							++totalLongNotes;
						}

						auto last = lastNote[laneNumber];
						lastNote[laneNumber] = nullptr;
						if (metaOnly) break;

						auto lastTimeline = last->Timeline;
						auto ln = new FBMSLongNote{ last->Wav };
						delete last;
						ln->Tail = new FBMSLongNote{ NoWav };
						ln->Tail->Head = ln;
						lastTimeline->SetNote(
							laneNumber, ln
						);
						timeline->SetNote(
							laneNumber, ln->Tail
						);
					}
					else {
						auto note = new FBMSNote{ ToWaveId(Chart, val) };
						lastNote[laneNumber] = note;
						++totalNotes;
						if (isScratch) ++totalScratchNotes;
						if (metaOnly) {
							delete note; // this is intended
							break;
						}
						timeline->SetNote(
							laneNumber, note
						);
					}

				}
				break;
				case Channel::P1InvisibleKeyBase:
				{
					if (metaOnly) break;
					auto invNote = new FBMSNote{ ToWaveId(Chart, val) };
					timeline->SetInvisibleNote(
						laneNumber, invNote
					);
					break;
				}
												
				case Channel::P1LongKeyBase:
					if (Lntype == 1)
					{
						if (lnStart[laneNumber] == nullptr)
						{
							++totalNotes;
							if (isScratch)
							{
								++totalBackSpinNotes;
							}
							else
							{
								++totalLongNotes;
							}

							auto ln = new FBMSLongNote{ ToWaveId(Chart, val) };
							lnStart[laneNumber] = ln;

							if (metaOnly) {
								delete ln; // this is intended
								break;
							}

							timeline->SetNote(
								laneNumber, ln
							);

						}
						else
						{
							if (!metaOnly) {
								auto tail = new FBMSLongNote{ NoWav };
								tail->Head = lnStart[laneNumber];
								lnStart[laneNumber]->Tail = tail;
								timeline->SetNote(
									laneNumber, tail
								);
							}
							lnStart[laneNumber] = nullptr;
						}
					}

					break;
				case Channel::P1MineKeyBase:
					// landmine
					++totalLandmineNotes;
					if (metaOnly) break;
					auto damage = DecodeBase36(val) / 2.0f;
					timeline->SetNote(
						laneNumber, new FBMSLandmineNote{ damage }
					);
					break;
				}
			}
		}

		Chart->Meta->TotalNotes = totalNotes;
		Chart->Meta->TotalLongNotes = totalLongNotes;
		Chart->Meta->TotalScratchNotes = totalScratchNotes;
		Chart->Meta->TotalBackSpinNotes = totalBackSpinNotes;

		auto lastPosition = 0.0;

		measure->Timing = static_cast<long long>(timePassed);

		for (auto& pair : timelines)
		{
			if(bCancelled) break;
			auto position = pair.Key;
			auto timeline = pair.Value;

			// Debug.Log($"measure: {i}, position: {position}, lastPosition: {lastPosition} bpm: {bpm} scale: {measure.scale} interval: {240 * 1000 * 1000 * (position - lastPosition) * measure.scale / bpm}");
			auto interval = 240000000.0 * (position - lastPosition) * measure->Scale / currentBpm;
			timePassed += interval;
			timeline->Timing = static_cast<long long>(timePassed);
			if (timeline->BpmChange)
			{
				currentBpm = timeline->Bpm;
				minBpm = FMath::Min(minBpm, timeline->Bpm);
				maxBpm = FMath::Max(maxBpm, timeline->Bpm);
			}
			else timeline->Bpm = currentBpm;

			// Debug.Log($"measure: {i}, position: {position}, lastPosition: {lastPosition}, bpm: {currentBpm} scale: {measure.Scale} interval: {interval} stop: {timeline.GetStopDuration()}");

			
			timePassed += timeline->GetStopDuration();
			if (!metaOnly) measure->TimeLines.Add(timeline);
			Chart->Meta->PlayLength = static_cast<long long>(timePassed);

			lastPosition = position;
		}

		if(metaOnly)
		{
			for(auto& timeline : timelines)
			{
				delete timeline.Value;
			}
			timelines.Empty();
		}

		if (!metaOnly && measure->TimeLines.Num() == 0) {
			auto timeline = new FTimeLine(TempKey, metaOnly);
			timeline->Timing = static_cast<long long>(timePassed);
			timeline->Bpm = currentBpm;
			measure->TimeLines.Add(timeline);
		}
		timePassed += 240000000.0 * (1 - lastPosition) * measure->Scale / currentBpm;
		if (!metaOnly) Chart->Measures.Add(measure);
		else delete measure;
	}

	Chart->Meta->TotalLength = static_cast<long long>(timePassed);
	Chart->Meta->MinBpm = minBpm;
	Chart->Meta->MaxBpm = maxBpm;
	if(Chart->Meta->Difficulty == 0)
	{
		FString FullTitle = (Chart->Meta->Title  + Chart->Meta->SubTitle).ToLower();
		if(FullTitle.Contains("beginner"))
		{
			Chart->Meta->Difficulty = 1;
		} else if (FullTitle.Contains("normal"))
		{
			Chart->Meta->Difficulty = 2;
		} else if (FullTitle.Contains("hyper"))
		{
			Chart->Meta->Difficulty = 3;
		} else if (FullTitle.Contains("another"))
		{
			Chart->Meta->Difficulty = 4;
		} else if (FullTitle.Contains("insane"))
		{
			Chart->Meta->Difficulty = 5;
		} else
		{
			if(totalNotes < 250)
			{
				Chart->Meta->Difficulty = 1;
			} else if (totalNotes < 600)
			{
				Chart->Meta->Difficulty = 2;
			} else if (totalNotes < 1000)
			{
				Chart->Meta->Difficulty = 3;
			} else if(totalNotes < 2000)
			{
				Chart->Meta->Difficulty = 4;
			} else
			{
				Chart->Meta->Difficulty = 5;
			}
		}
		
	}
}

void FBMSParser::ParseHeader(FChart* Chart, const FString& Cmd, const FString& Xx, FString Value) {
	// Debug.Log($"cmd: {cmd}, xx: {xx} isXXNull: {xx == null}, value: {value}");
	const FString CmdUpper = Cmd.ToUpper();
	if (CmdUpper == "PLAYER")
	{
		Chart->Meta->Player = FCString::Atoi(*Value);
	}
	else if (CmdUpper == "GENRE")
	{
		Chart->Meta->Genre = Value;
	}
	else if (CmdUpper == "TITLE")
	{
		Chart->Meta->Title = Value;
	}
	else if (CmdUpper == "SUBTITLE")
	{
		Chart->Meta->SubTitle = Value;
	}
	else if (CmdUpper == "ARTIST")
	{
		Chart->Meta->Artist = Value;
	}
	else if (CmdUpper == "SUBARTIST")
	{
		Chart->Meta->SubArtist = Value;
	}
	else if (CmdUpper == "DIFFICULTY")
	{
		Chart->Meta->Difficulty = FCString::Atoi(*Value);
	}
	else if (CmdUpper == "BPM")
	{
		if (Value.IsEmpty()) return; // TODO: handle this
		if (Xx.IsEmpty())
		{
			// chart initial bpm
			Chart->Meta->Bpm = FCString::Atod(*Value);
		}
		else
		{
			// Debug.Log($"BPM: {DecodeBase36(xx)} = {double.Parse(value)}");
			int id = DecodeBase36(Xx);
			if (!CheckResourceIdRange(id))
			{
				UE_LOG(LogTemp, Warning, TEXT("Invalid BPM id: %s"), *Xx);
				return;
			}
			BpmTable.Add(id, FCString::Atod(*Value));
		}
	}
	else if (CmdUpper == "STOP")
	{
		if (Value.IsEmpty() || Xx.IsEmpty() || Xx.Len() == 0) return;  // TODO: handle this
		int id = DecodeBase36(Xx);
		if (!CheckResourceIdRange(id))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid STOP id: %s"), *Xx);
			return;
		}
		StopLengthTable.Add(id, FCString::Atod(*Value));
	}
	else if (CmdUpper == "MIDIFILE")
	{
	}
	else if (CmdUpper == "VIDEOFILE")
	{
	}
	else if (CmdUpper == "PLAYLEVEL")
	{
		Chart->Meta->PlayLevel = FCString::Atod(*Value); // TODO: handle error
	}
	else if (CmdUpper == "RANK")
	{
		Chart->Meta->Rank = FCString::Atoi(*Value);
	}
	else if (CmdUpper == "TOTAL")
	{
		auto total = FCString::Atod(*Value);
		if (total > 0)
		{
			Chart->Meta->Total = total;	
		}
	}
	else if (CmdUpper == "VOLWAV") {

	}
	else if (CmdUpper == "STAGEFILE") {
		Chart->Meta->StageFile = Value;
	}
	else if (CmdUpper == "BANNER") {
		Chart->Meta->Banner = Value;
	}
	else if (CmdUpper == "BACKBMP") {
		Chart->Meta->BackBmp = Value;
	}
	else if (CmdUpper == "PREVIEW") {
		Chart->Meta->Preview = Value;
	}
	else if (CmdUpper == "WAV") {
		if (Xx.IsEmpty() || Value.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("WAV command requires two arguments"));
			return;
		}
		int id = DecodeBase36(Xx);
		if (!CheckResourceIdRange(id))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid WAV id: %s"), *Xx);
			return;
		}
		Chart->WavTable.Add(id, Value);
	}
	else if (CmdUpper == "BMP") {
		if (Xx.IsEmpty() || Value.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("BMP command requires two arguments"));
			return;
		}
		int id = DecodeBase36(Xx);
		if (!CheckResourceIdRange(id))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid BMP id: %s"), *Xx);
			return;
		}
		Chart->BmpTable.Add(id, Value);
		if (Xx == "00")
		{
			Chart->Meta->BgaPoorDefault = true;
		}
	}
	else if (CmdUpper == "RANDOM") {

	}
	else if (CmdUpper == "IF") {

	}
	else if (CmdUpper == "ENDIF") {

	}
	else if (CmdUpper == "LNOBJ") {
		Lnobj = DecodeBase36(Value);
	}
	else if (CmdUpper == "LNTYPE") {
		Lntype = FCString::Atoi(*Value);
	}
	else if (CmdUpper == "LNMODE") {
		Chart->Meta->LnMode = FCString::Atoi(*Value);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Unknown command: %s"), *CmdUpper);
	}	
}

int FBMSParser::Gcd(int A, int B) {
	while (true)
	{
		if (B == 0) return A;
		auto a1 = A;
		A = B;
		B = a1 % B;
	}
}

bool FBMSParser::CheckResourceIdRange(int Id)
{
	return Id >= 0 && Id < 36 * 36;
}

int FBMSParser::ToWaveId(FChart* Chart, const FString& Wav) {
	auto decoded = DecodeBase36(Wav);
	// check range
	if (!CheckResourceIdRange(decoded))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid wav id: %s"), *Wav);
		return NoWav;
	}

	return Chart->WavTable.Contains(decoded) ? decoded : NoWav;
}

int FBMSParser::DecodeBase36(const FString& Str) {
	int result = 0;
	const FString& StrUpper = Str.ToUpper();
	for (auto c : StrUpper)
	{
		result *= 36;
		if (FChar::IsDigit(c))
		{
			result += c - '0';
		}
		else if (FChar::IsAlpha(c))
		{
			result += c - 'A' + 10;
		}
		else
		{
			return -1;
		}
	}
	return result;
}
FBMSParser::~FBMSParser()
{
}
