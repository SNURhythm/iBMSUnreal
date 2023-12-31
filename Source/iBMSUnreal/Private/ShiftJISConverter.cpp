// Fill out your copyright notice in the Description page of Project Settings.


#include "ShiftJISConverter.h"


void ShiftJISConverter::BytesToUTF8(FString& outString, const uint8* input, int32 size)
{
	//ShiftJis won't give 4byte UTF8, so max. 3 byte per input char are needed
	TArray<uint8> result;
	result.SetNumUninitialized(size * 3);
	size_t indexInput = 0, indexOutput = 0;

	while (indexInput < size)
	{
		char arraySection = (input[indexInput]) >> 4;

		size_t arrayOffset;
		if (arraySection == 0x8) arrayOffset = 0x100; //these are two-byte shiftjis
		else if (arraySection == 0x9) arrayOffset = 0x1100;
		else if (arraySection == 0xE) arrayOffset = 0x2100;
		else arrayOffset = 0; //this is one byte shiftjis

		//determining real array offset
		if (arrayOffset)
		{
			arrayOffset += ((input[indexInput]) & 0xf) << 8;
			indexInput++;
			if (indexInput >= size) break;
		}
		arrayOffset += input[indexInput++];
		arrayOffset <<= 1;

		//unicode number is...
		uint16_t unicodeValue = (shiftJIS_convTable[arrayOffset] << 8) | shiftJIS_convTable[arrayOffset + 1];

		//converting to UTF8
		if (unicodeValue < 0x80)
		{
			result[indexOutput++] = unicodeValue;
		}
		else if (unicodeValue < 0x800)
		{
			result[indexOutput++] = 0xC0 | (unicodeValue >> 6);
			result[indexOutput++] = 0x80 | (unicodeValue & 0x3f);
		}
		else
		{
			result[indexOutput++] = 0xE0 | (unicodeValue >> 12);
			result[indexOutput++] = 0x80 | ((unicodeValue & 0xfff) >> 6);
			result[indexOutput++] = 0x80 | (unicodeValue & 0x3f);
		}
	}

	result.SetNum(indexOutput);

	FFileHelper::BufferToString(outString, result.GetData(), result.Num());
	
}
