// Fill out your copyright notice in the Description page of Project Settings.


#include "FBMSParser.h"
#include "FBMSLongNote.h"
#include "FBMSNote.h"
#include "FBMSLandmineNote.h"
#include "FTimeLine.h"
#include "FMeasure.h"

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

const int TempKey = 8;
constexpr int NoWav = -1;
constexpr int MetronomeWav = -2;
const int Scroll = 1020;
FRegexPattern headerRegex = FRegexPattern(TEXT("^#([A-Za-z]+?)(\\d\\d)? +?(.+)?"));
FBMSParser::FBMSParser(): BpmTable{}, WavTable{}, BmpTable{}, StopLengthTable{}
{
	Chart = new FChart();
}


void FBMSParser::Parse(FString& path, bool addReadyMeasure, bool metaOnly)
{

	// implement the same thing as BMSParser.cs
	auto measures = TMap<int, TArray<TPair<int, FString>>>();
	auto bytes = TArray<uint8>();
	FFileHelper::LoadFileToArray(bytes, *path);
	auto md5 = FMD5::HashBytes(bytes.GetData(), bytes.Num());

	// bytes to FString
	auto bytesString = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(bytes.GetData())));
	auto lines = TArray<FString>();
	bytesString.ParseIntoArrayLines(lines);
	auto lastMeasure = -1;

	for (auto& line : lines)
	{
		if (!line.StartsWith("#")) continue;
		if (line.Len() < 7) continue;
		if (FChar::IsDigit(line[1]) && FChar::IsDigit(line[2]) && FChar::IsDigit(line[3]) && line[6] == ':')
		{
			auto measure = FCString::Atoi(*line.Mid(1, 3));
			lastMeasure = FMath::Max(lastMeasure, measure);
			FString ch = line.Mid(4, 2);
			auto channel = DecodeBase36(ch);
			auto value = line.Mid(7);
			if (!measures.Contains(measure))
			{
				measures.Add(measure, TArray<TPair<int, FString>>());
			}
			measures[measure].Add(TPair<int, FString>(channel, value));
		}
		else
		{
			auto upperLine = line.ToUpper();
			if (upperLine.StartsWith("#WAV") || upperLine.StartsWith("#BMP"))
			{
				if (line.Len() < 7) continue;
				auto xx = line.Mid(4, 2);
				auto value = line.Mid(7);
				FString cmd = upperLine.Mid(1, 3);
				ParseHeader(cmd, xx, value);
			}
			else if (upperLine.StartsWith("#STOP"))
			{
				if (line.Len() < 8) continue;
				auto xx = line.Mid(5, 2);
				auto value = line.Mid(8);
				FString cmd = "STOP";
				ParseHeader(cmd, xx, value);
			}
			else if (upperLine.StartsWith("#BPM"))
			{
				if (line.Mid(4).StartsWith(" "))
				{
					auto value = line.Mid(5);
					FString cmd = "BPM";
					FString xx = "";
					ParseHeader(cmd, xx, value);
				}
				else
				{
					if (line.Len() < 7) continue;
					auto xx = line.Mid(4, 2);
					auto value = line.Mid(7);
					FString cmd = "BPM";
					ParseHeader(cmd, xx, value);
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
					ParseHeader(cmd, xx, value);
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
	auto currentBpm = Chart->Meta.Bpm;
	auto minBpm = Chart->Meta.Bpm;
	auto maxBpm = Chart->Meta.Bpm;
	auto lastNote = TArray<FBMSNote*>();
	lastNote.Init(nullptr, TempKey);
	auto lnStart = TArray<FBMSLongNote*>();
	lnStart.Init(nullptr, TempKey);

	for (auto i = 0; i <= lastMeasure; ++i)
	{
		if (!measures.Contains(i))
		{
			measures.Add(i, TArray<TPair<int, FString>>());
		}

		// gcd (int, int)
		auto measure = new FMeasure();
		auto timelines = TMap<double, FTimeLine*>();

		for (auto& pair : measures[i])
		{
			auto channel = pair.Key;
			auto& data = pair.Value;
			if (channel == Channel::SectionRate)
			{
				measure->Scale = FCString::Atod(*data);
				continue;
			}

			auto laneNumber = 0;
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
				Chart->Meta.KeyMode = 7;
			}
			if (laneNumber >= TempKey) {
				// skip DP/PMS for now
				continue;
			}

			auto dataCount = data.Len() / 2;
			for (auto j = 0; j < dataCount; ++j)
			{
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
				auto position = (double)(j / g) / (dataCount / g);

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
					if (val == "**")
					{
						timeline->AddBackgroundNote(new FBMSNote{ MetronomeWav });
						break;
					}
					if (DecodeBase36(val) != 0)
					{
						auto bgNote = new FBMSNote{ ToWaveId(val) };
						timeline->AddBackgroundNote(bgNote);
					}

					break;
				case Channel::BpmChange:
					timeline->Bpm = FCString::Atod(*val);
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
					timeline->Bpm = BpmTable[DecodeBase36(val)];
					// Debug.Log($"BPM_CHANGE_EXTEND: {timeline.Bpm}, on measure {i}, {val}");
					timeline->BpmChange = true;
					break;
				case Channel::Stop:
					timeline->StopLength = StopLengthTable[DecodeBase36(val)];
					// Debug.Log($"STOP: {timeline.StopLength}, on measure {i}");
					break;
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
						auto note = new FBMSNote{ ToWaveId(val) };
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
				case Channel::P1InvisibleKeyBase: {
					auto invNote = new FBMSNote{ ToWaveId(val) };
					timeline->SetInvisibleNote(
						laneNumber, invNote
					);

				}
												break;
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

							auto ln = new FBMSLongNote{ ToWaveId(val) };
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

		Chart->Meta.TotalNotes = totalNotes;
		Chart->Meta.TotalLongNotes = totalLongNotes;
		Chart->Meta.TotalScratchNotes = totalScratchNotes;
		Chart->Meta.TotalBackSpinNotes = totalBackSpinNotes;

		auto lastPosition = 0.0;

		measure->Timing = static_cast<long>(timePassed);

		for (auto& pair : timelines)
		{
			auto position = pair.Key;
			auto timeline = pair.Value;

			// Debug.Log($"measure: {i}, position: {position}, lastPosition: {lastPosition} bpm: {bpm} scale: {measure.scale} interval: {240 * 1000 * 1000 * (position - lastPosition) * measure.scale / bpm}");
			auto interval = 240000000.0 * (position - lastPosition) * measure->Scale / currentBpm;
			timePassed += interval;
			timeline->Timing = static_cast<long>(timePassed);
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
			else delete timeline;
			Chart->Meta.PlayLength = static_cast<long>(timePassed);

			lastPosition = position;
		}

		if (!metaOnly && measure->TimeLines.Num() == 0) {
			auto timeline = new FTimeLine(TempKey, metaOnly);
			timeline->Timing = static_cast<long>(timePassed);
			timeline->Bpm = currentBpm;
			measure->TimeLines.Add(timeline);
		}
		timePassed += 240000000.0 * (1 - lastPosition) * measure->Scale / currentBpm;
		if (!metaOnly) Chart->Measures.Add(measure);
		else delete measure;
	}

	Chart->Meta.TotalLength = static_cast<long>(timePassed);
	Chart->Meta.MinBpm = minBpm;
	Chart->Meta.MaxBpm = maxBpm;
}

void FBMSParser::ParseHeader(FString& Cmd, FString& Xx, FString& Value) {
	// Debug.Log($"cmd: {cmd}, xx: {xx} isXXNull: {xx == null}, value: {value}");
	const FString CmdUpper = Cmd.ToUpper();
	if (CmdUpper == "PLAYER")
	{
		Chart->Meta.Player = FCString::Atoi(*Value);
	}
	else if (CmdUpper == "GENRE")
	{
		Chart->Meta.Genre = Value;
	}
	else if (CmdUpper == "TITLE")
	{
		Chart->Meta.Title = Value;
	}
	else if (CmdUpper == "SUBTITLE")
	{
		Chart->Meta.SubTitle = Value;
	}
	else if (CmdUpper == "ARTIST")
	{
		Chart->Meta.Artist = Value;
	}
	else if (CmdUpper == "SUBARTIST")
	{
		Chart->Meta.SubArtist = Value;
	}
	else if (CmdUpper == "DIFFICULTY")
	{
		Chart->Meta.Difficulty = FCString::Atoi(*Value);
	}
	else if (CmdUpper == "BPM")
	{
		if (Value.IsEmpty()) return; // TODO: handle this
		if (Xx.IsEmpty())
		{
			// chart initial bpm
			Chart->Meta.Bpm = FCString::Atod(*Value);
		}
		else
		{
			// Debug.Log($"BPM: {DecodeBase36(xx)} = {double.Parse(value)}");
			BpmTable[DecodeBase36(Xx)] = FCString::Atod(*Value);
		}
	}
	else if (CmdUpper == "STOP")
	{
		if (Value.IsEmpty() || Xx.IsEmpty() || Xx.Len() == 0) return;  // TODO: handle this
		StopLengthTable[DecodeBase36(Xx)] = FCString::Atod(*Value);
	}
	else if (CmdUpper == "MIDIFILE")
	{
	}
	else if (CmdUpper == "VIDEOFILE")
	{
	}
	else if (CmdUpper == "PLAYLEVEL")
	{
		Chart->Meta.PlayLevel = FCString::Atod(*Value); // TODO: handle error
	}
	else if (CmdUpper == "RANK")
	{
		Chart->Meta.Rank = FCString::Atoi(*Value);
	}
	else if (CmdUpper == "TOTAL")
	{
		auto total = FCString::Atod(*Value);
		if (total > 0)
		{
			Chart->Meta.Total = total;	
		}
	}
	else if (CmdUpper == "VOLWAV") {

	}
	else if (CmdUpper == "STAGEFILE") {
		Chart->Meta.StageFile = Value;
	}
	else if (CmdUpper == "BANNER") {
		Chart->Meta.Banner = Value;
	}
	else if (CmdUpper == "BACKBMP") {
		Chart->Meta.BackBmp = Value;
	}
	else if (CmdUpper == "PREVIEW") {
		Chart->Meta.Preview = Value;
	}
	else if (CmdUpper == "WAV") {
		if (Xx.IsEmpty() || Value.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("WAV command requires two arguments"));
			return;
		}
		WavTable[DecodeBase36(Xx)] = Value;
	}
	else if (CmdUpper == "BMP") {
		if (Xx.IsEmpty() || Value.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("WAV command requires two arguments"));
			return;
		}
		BmpTable[DecodeBase36(Xx)] = Value;
		if (Xx == "00")
		{
			Chart->Meta.BgaPoorDefault = true;
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
		Chart->Meta.LnMode = FCString::Atoi(*Value);
	}
	else {
		//UE_LOG(LogTemp, Warning, TEXT("Unknown command: %s"), *CmdUpper);
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

int FBMSParser::ToWaveId(FString& Wav) {
	auto decoded = DecodeBase36(Wav);
	// check range
	if (decoded < 0 || decoded > 36 * 36 - 1)
	{
		return NoWav;
	}

	return WavTable[decoded].IsEmpty() ? NoWav : decoded;
}

int FBMSParser::DecodeBase36(FString& Str) {
	int result = 0;
	for (auto c : Str)
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
