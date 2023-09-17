// Fill out your copyright notice in the Description page of Project Settings.


#include "BMSParser.h"
#include "BMSLongNote.h"
#include "BMSNote.h"
#include "BMSLandmineNote.h"
#include "TimeLine.h"
#include "Measure.h"

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

const int TempKey = 18;
const int NoWav = - 1;
const int MetronomeWav = -2;
const int Scroll = 1020;
FRegexPattern headerRegex = FRegexPattern(TEXT("^#([A-Za-z]+?)(\\d\\d)? +?(.+)?"));
BMSParser::BMSParser()
{
	
}


void BMSParser::Parse(FString& path, bool addReadyMeasure, bool metaOnly)
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
	
	for (auto &line : lines)
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
					FString xx;
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
	auto currentBpm = chart.Meta.Bpm;
	auto minBpm = chart.Meta.Bpm;
	auto maxBpm = chart.Meta.Bpm;
	auto lastNote = TArray<BMSNote*>();
	lastNote.Init(nullptr, TempKey);
	auto lnStart = TArray<BMSLongNote*>();
	lnStart.Init(nullptr, TempKey);

	for (auto i = 0; i <= lastMeasure; ++i)
	{
		if (!measures.Contains(i))
		{
			measures.Add(i, TArray<TPair<int, FString>>());
		}

		// gcd (int, int)
		auto measure = Measure();
		auto timelines = TMap<double, TimeLine*>();

		for (auto& pair : measures[i])
		{
			auto channel = pair.Key;
			auto& data = pair.Value;
			if (channel == Channel::SectionRate)
			{
				measure.Scale = FCString::Atod(*data);
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
				chart.Meta.KeyMode = 7;
			}

			auto dataCount = data.Len() / 2;
			for (auto j = 0; j < dataCount; ++j)
			{
				auto val = data.Mid(j * 2, 2);
				if (val == "00")
				{
					if (timelines.Num() == 0 && j == 0)
					{
						timelines.Add(0, new TimeLine(TempKey)); // add ghost timeline
					}

					continue;
				}

				auto g = Gcd(j, dataCount);
				// ReSharper disable PossibleLossOfFraction
				auto position = (double)(j / g) / (dataCount / g);

				if (!timelines.Contains(position)) timelines.Add(position, new TimeLine(TempKey));

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
						timeline->AddBackgroundNote(new BMSNote{ MetronomeWav });
						break;
					}
					if (DecodeBase36(val) != 0)
					{
						auto bgNote = new BMSNote{ ToWaveId(val) };
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
					timeline->Bpm = bpmTable[DecodeBase36(val)];
					// Debug.Log($"BPM_CHANGE_EXTEND: {timeline.Bpm}, on measure {i}, {val}");
					timeline->BpmChange = true;
					break;
				case Channel::Stop:
					timeline->StopLength = StopLengthTable[DecodeBase36(val)];
					// Debug.Log($"STOP: {timeline.StopLength}, on measure {i}");
					break;
				case Channel::P1KeyBase: {
					auto ch = DecodeBase36(val);
					if (ch == lnobj && lastNote[laneNumber] != nullptr) {
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
						auto ln = new BMSLongNote{ last->Wav };

						ln->Tail = new BMSLongNote{ NoWav };
						ln->Tail->Head = ln;
						lastTimeline->SetNote(
							laneNumber, ln
						);
						timeline->SetNote(
							laneNumber, ln->Tail
						);
					}
					else {
						auto note = new BMSNote{ ToWaveId(val) };
						lastNote[laneNumber] = note;
						++totalNotes;
						if (isScratch) ++totalScratchNotes;
						if (metaOnly) break;
						timeline->SetNote(
							laneNumber, note
						);
					}
					
				}
				break;
				case Channel::P1InvisibleKeyBase: {
					auto invNote = new BMSNote{ ToWaveId(val) };
					timeline->SetInvisibleNote(
						laneNumber, invNote
					);
					
				}
				break;
				case Channel::P1LongKeyBase:
					if (lntype == 1)
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

							auto ln = new BMSLongNote{ ToWaveId(val) };
							lnStart[laneNumber] = ln;

							if (metaOnly) break;

							timeline->SetNote(
								laneNumber, ln
							);

						}
						else
						{
							auto tail = new BMSLongNote{ NoWav };
							tail->Head = lnStart[laneNumber];
							lnStart[laneNumber]->Tail = tail;
							lnStart[laneNumber] = nullptr;
							if (metaOnly) break;
							timeline->SetNote(
								laneNumber, tail
							);

						}
					}

					break;
				case Channel::P1MineKeyBase:
					// landmine
					++totalLandmineNotes;
					if (metaOnly) break;
					auto damage = DecodeBase36(val) / 2.0f;
					timeline->SetNote(
						laneNumber, new BMSLandmineNote{ damage }
					);
					break;
				}
			}
		}

		chart.Meta.TotalNotes = totalNotes;
		chart.Meta.TotalLongNotes = totalLongNotes;
		chart.Meta.TotalScratchNotes = totalScratchNotes;
		chart.Meta.TotalBackSpinNotes = totalBackSpinNotes;
		
		auto lastPosition = 0.0;

		measure.Timing = (long)timePassed;
		if (!metaOnly) chart.Measures.Add(&measure);
		for (auto& pair : timelines)
		{
			auto position = pair.Key;
			auto timeline = pair.Value;

			// Debug.Log($"measure: {i}, position: {position}, lastPosition: {lastPosition} bpm: {bpm} scale: {measure.scale} interval: {240 * 1000 * 1000 * (position - lastPosition) * measure.scale / bpm}");
			auto interval = 240000000.0 * (position - lastPosition) * measure.Scale / currentBpm;
			timePassed += interval;
			timeline->Timing = (long)timePassed;
			if (timeline->BpmChange)
			{
				currentBpm = timeline->Bpm;
				minBpm = FMath::Min(minBpm, timeline->Bpm);
				maxBpm = FMath::Max(maxBpm, timeline->Bpm);
			}
			else timeline->Bpm = currentBpm;

			// Debug.Log($"measure: {i}, position: {position}, lastPosition: {lastPosition}, bpm: {currentBpm} scale: {measure.Scale} interval: {interval} stop: {timeline.GetStopDuration()}");

			if (!metaOnly) measure.TimeLines.Add(timeline);
			timePassed += timeline->GetStopDuration();

			chart.Meta.PlayLength = (long)timePassed;

			lastPosition = position;
		}

		if (!metaOnly && measure.TimeLines.Num() == 0) {
			auto timeline = new TimeLine(TempKey);
			timeline->Timing = (long)timePassed;
			timeline->Bpm = currentBpm;
			measure.TimeLines.Add(timeline);
		}
		timePassed += 240000000.0 * (1 - lastPosition) * measure.Scale / currentBpm;
	}

	chart.Meta.TotalLength = (long)timePassed;
	chart.Meta.MinBpm = minBpm;
	chart.Meta.MaxBpm = maxBpm;
}

void BMSParser::ParseHeader(FString& cmd, FString& xx, FString& value) {
	// Debug.Log($"cmd: {cmd}, xx: {xx} isXXNull: {xx == null}, value: {value}");
	cmd = cmd.ToUpper();
	if (cmd == "PLAYER")
	{
		chart.Meta.Player = FCString::Atoi(*value);
	}
	else if (cmd == "GENRE")
	{
		chart.Meta.Genre = value;
	}
	else if (cmd == "TITLE")
	{
		chart.Meta.Title = value;
	}
	else if (cmd == "SUBTITLE")
	{
		chart.Meta.SubTitle = value;
	}
	else if (cmd == "ARTIST")
	{
		chart.Meta.Artist = value;
	}
	else if (cmd == "SUBARTIST")
	{
		chart.Meta.SubArtist = value;
	}
	else if (cmd == "DIFFICULTY")
	{
		chart.Meta.Difficulty = FCString::Atoi(*value);
	}
	else if (cmd == "BPM")
	{
		if (value.IsEmpty()) throw "invalid BPM value";
		if (xx.IsEmpty())
		{
			// chart initial bpm
			chart.Meta.Bpm = FCString::Atod(*value);
		}
		else
		{
			// Debug.Log($"BPM: {DecodeBase36(xx)} = {double.Parse(value)}");
			bpmTable[DecodeBase36(xx)] = FCString::Atod(*value);
		}
	}
	else if (cmd == "STOP")
	{
		if (value.IsEmpty() || xx.IsEmpty() || xx.Len() == 0) throw "invalid arguments in #STOP";
		StopLengthTable[DecodeBase36(xx)] = FCString::Atod(*value);
	}
	else if (cmd == "MIDIFILE")
	{
	}
	else if (cmd == "VIDEOFILE")
	{
	}
	else if (cmd == "PLAYLEVEL")
	{
		try
		{
			chart.Meta.PlayLevel = FCString::Atod(*value);
		}
		catch (...)
		{
			UE_LOG(LogTemp, Warning, TEXT("invalid playlevel: %s"), *value);
		}
	}
	else if (cmd == "RANK")
	{
		chart.Meta.Rank = FCString::Atoi(*value);
	}
	else if (cmd == "TOTAL")
	{
		auto total = FCString::Atod(*value);
		if (total > 0)
		{
			chart.Meta.Total = total;	
		}
	}
	else if (cmd == "VOLWAV") {

	}
	else if (cmd == "STAGEFILE") {
		chart.Meta.StageFile = value;
	}
	else if (cmd == "BANNER") {
		chart.Meta.Banner = value;
	}
	else if (cmd == "BACKBMP") {
		chart.Meta.BackBmp = value;
	}
	else if (cmd == "PREVIEW") {
		chart.Meta.Preview = value;
	}
	else if (cmd == "WAV") {
		if (xx.IsEmpty() || value.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("WAV command requires two arguments"));
			return;
		}
		wavTable[DecodeBase36(xx)] = value;
	}
	else if (cmd == "BMP") {
		if (xx.IsEmpty() || value.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("WAV command requires two arguments"));
			return;
		}
		bmpTable[DecodeBase36(xx)] = value;
		if (xx == "00")
		{
			chart.Meta.BgaPoorDefault = true;
		}
	}
	else if (cmd == "RANDOM") {

	}
	else if (cmd == "IF") {

	}
	else if (cmd == "ENDIF") {

	}
	else if (cmd == "LNOBJ") {
		lnobj = DecodeBase36(value);
	}
	else if (cmd == "LNTYPE") {
		lntype = FCString::Atoi(*value);
	}
	else if (cmd == "LNMODE") {
		chart.Meta.LnMode = FCString::Atoi(*value);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Unknown command: %s"), *cmd);
	}	
}

int BMSParser::Gcd(int a, int b) {
	while (true)
	{
		if (b == 0) return a;
		auto a1 = a;
		a = b;
		b = a1 % b;
	}
}

int BMSParser::ToWaveId(FString& wav) {
	auto decoded = DecodeBase36(wav);
	// check range
	if (decoded < 0 || decoded > 36 * 36 - 1)
	{
		return NoWav;
	}

	return wavTable[decoded].IsEmpty() ? NoWav : decoded;
}

int BMSParser::DecodeBase36(FString& str) {
	int result = 0;
	for (auto c : str)
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
BMSParser::~BMSParser()
{
}
