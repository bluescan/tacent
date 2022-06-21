// tStandard.cpp
//
// Tacent functions and types that are standard across all platforms. Includes global functions like itoa which are not
// available on some platforms, but are common enough that they should be.
//
// Copyright (c) 2004-2006, 2015 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <stdlib.h>
#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif
#include "Foundation/tStandard.h"
#include "Foundation/tString.h"
#pragma warning (disable: 4146)
#pragma warning (disable: 4018)


const char* tStd::SeparatorSubStr							= "\x1a";
const char* tStd::SeparatorFileStr							= "\x1c";
const char* tStd::SeparatorGroupStr							= "\x1d";
const char* tStd::SeparatorRecordStr						= "\x1e";
const char* tStd::SeparatorUnitStr							= "\x1f";
const char* tStd::SeparatorAStr								= tStd::SeparatorUnitStr;
const char* tStd::SeparatorBStr								= tStd::SeparatorRecordStr;
const char* tStd::SeparatorCStr								= tStd::SeparatorGroupStr;
const char* tStd::SeparatorDStr								= tStd::SeparatorFileStr;
const char* tStd::SeparatorEStr								= tStd::SeparatorSubStr;
const char8_t* tStd::u8SeparatorSubStr						= (const char8_t*)tStd::SeparatorSubStr;
const char8_t* tStd::u8SeparatorFileStr						= (const char8_t*)tStd::SeparatorFileStr;
const char8_t* tStd::u8SeparatorGroupStr					= (const char8_t*)tStd::SeparatorGroupStr;
const char8_t* tStd::u8SeparatorRecordStr					= (const char8_t*)tStd::SeparatorRecordStr;
const char8_t* tStd::u8SeparatorUnitStr						= (const char8_t*)tStd::SeparatorUnitStr;
const char8_t* tStd::u8SeparatorAStr						= (const char8_t*)tStd::SeparatorAStr;
const char8_t* tStd::u8SeparatorBStr						= (const char8_t*)tStd::SeparatorBStr;
const char8_t* tStd::u8SeparatorCStr						= (const char8_t*)tStd::SeparatorCStr;
const char8_t* tStd::u8SeparatorDStr						= (const char8_t*)tStd::SeparatorDStr;
const char8_t* tStd::u8SeparatorEStr						= (const char8_t*)tStd::SeparatorEStr;


void* tStd::tMemmem(void* haystack, int haystackNumBytes, void* needle, int needleNumBytes)
{
	if ((haystackNumBytes <= 0) || (needleNumBytes <= 0) || (haystackNumBytes < needleNumBytes))
		return nullptr;

	// Serach for the pattern from the first haystack byte (0) to numNeedleBytes from the end. For example, if we are
	// seraching for 4 bytes in 8, there will be 5 mem compares of 4 bytes each.
	for (int i = 0; i <= haystackNumBytes-needleNumBytes; i++)
	{
		if (tMemcmp((uint8*)haystack + i, needle, needleNumBytes) == 0)
			return (uint8*)haystack + i;
	}

	return nullptr;
}


bool tStd::tStrtob(const char* str)
{
	tString lower(str);
	lower.ToLower();

	if
	(
		(lower == "true") || (lower == "t") ||
		(lower == "yes") || (lower == "y") ||
		(lower == "on") || (lower == "1") || (lower == "+") ||
		(lower == "enable") || (lower == "enabled") || (tStrtoi(str) != 0)
	)
		return true;
	else
		return false;
}


float tStd::tStrtof(const char* s)
{
	char* hash = tStrchr(s, '#');
	if (hash && (tStrlen(hash+1) == 8))
	{
		uint32 bin = tStd::tStrtoui32(hash+1, 16);
		return *((float*)(&bin));
	}

	return float( tStrtod(s) );
}


double tStd::tStrtod(const char* s)
{
	int l = tStrlen(s);
	if (!l)
		return 0.0;

	char* hash = tStrchr(s, '#');
	if (hash && (tStrlen(hash+1) == 16))
	{
		uint64 bin = tStrtoui64(hash+1, 16);
		return *((double*)&bin);
	}

	// This error checking is essential. Sometimes NANs are written in text format to a string.
	// Like "nan(snan)". We want these to evaluate to 0.0, not -1 or something else. We allow
	// 'e' and 'E' for numbers in exponential form like 3.09E08.
	for (int i = 0; i < l; i++)
	{
		char ch = s[i];
		if
		(
			((ch >= 'a') && (ch <= 'z') && (ch != 'e')) ||
			((ch >= 'A') && (ch <= 'Z') && (ch != 'E'))
		)
			return 0.0;
	}

	// Will be 0.0 if there was a problem.
	return strtod(s, nullptr);
}


void tStd::tStrrev(char* begin, char* end)
{	
	char aux;
	while (end > begin)
		aux = *end, *end-- = *begin, *begin++ = aux;
}
