// tString.cpp
//
// tString is a simple and readable string class that implements sensible operators, including implicit casts. There is
// no UCS2 or UTF-16 support. The text in a tString is considerd to be UTF-8 and terminated with a nul. You cannot
// stream (from cin etc) more than 512 chars into a string. This restriction is only for wacky << streaming. For
// conversions of arbitrary types to tStrings, see tsPrint in the higher level System module.
//
// Copyright (c) 2004-2006, 2015, 2017, 2020, 2021 Tristan Grimmer.
// Copyright (c) 2020 Stefan Wessels.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include "Foundation/tString.h"
#include "Foundation/tStandard.h"
#include "Foundation/tHash.h"


char8_t tString::EmptyChar = '\0';


tString::operator uint32()
{
	return tHash::tHashStringFast32(TextData);

}


tString::operator uint32() const
{
	return tHash::tHashStringFast32(TextData);
}


tString tString::Left(const char c) const
{
	int pos = FindChar(c);
	if (pos == -1)
		return *this;
	if (pos == 0)
		return tString();

	// Remember, this zeros the memory, so tStrncpy not dealing with the terminating null is ok.
	tString buf(pos);
	tStd::tStrncpy(buf.TextData, TextData, pos);
	return buf;
}


tString tString::Right(const char c) const
{
	int pos = FindChar(c, true);
	if (pos == -1)
		return *this;

	int length = Length();
	if (pos == (length-1))
		return tString();

	// Remember, this zeros the memory, so tStrncpy not dealing with the terminating null is ok.
	tString buf(length - 1 - pos);
	tStd::tStrncpy(buf.TextData, TextData + pos + 1, length - 1 - pos);
	return buf;
}


tString tString::Left(int count) const
{
	if(count <= 0)
		return tString();

	int length = Length();
	if (count > length)
		count = length;

	tString buf(count);
	tStd::tStrncpy(buf.TextData, TextData, count);
	return buf;
}


tString tString::Right(int count) const
{
	if (count <= 0)
		return tString();
		
	int length = Length();
	int start = length - count;
	if (start < 0)
	{
		start = 0;
		count = length;
	}

	tString buf(count);
	tStd::tStrncpy(buf.TextData, TextData + start, count);
	return buf;
}


tString tString::Mid(int start, int count) const
{
	int length = Length();
	if(start < 0 || start >= length || count <= 0)
		return tString();

	if(start + count > length)
		count = length - start;

	tString buf(count);
	tStd::tStrncpy(buf.TextData, TextData + start, count);
	return buf;
}


tString tString::ExtractLeft(const char divider)
{
	int pos = FindChar(divider);
	if (pos == -1)
	{
		tString buf(Text());
		Clear();
		return buf;
	}

	// Remember, this constructor zeros the memory, so strncpy not dealing with the terminating null is ok.
	tString buf(pos);
	tStd::tStrncpy(buf.TextData, TextData, pos);

	int length = Length();
	char8_t* newText = new char8_t[length-pos];

	// This will append the null.
	tStd::tStrncpy(newText, TextData+pos+1, length-pos);

	if (TextData != &EmptyChar)
		delete[] TextData;
	TextData = newText;

	return buf;
}


tString tString::ExtractRight(const char divider)
{
	int pos = FindChar(divider, true);
	if (pos == -1)
	{
		tString buf(Text());
		Clear();
		return buf;
	}

	int wordLength = Length() - pos - 1;

	// Remember, this constructor zeros the memory, so strncpy not dealing with the terminating null is ok.
	tString buf(wordLength);
	tStd::tStrncpy(buf.TextData, TextData+pos+1, wordLength);

	char8_t* newText = new char8_t[pos+1];
	tStd::tStrncpy(newText, TextData, pos);
	newText[pos] = '\0';

	if (TextData != &EmptyChar)
		delete[] TextData;
	TextData = newText;

	return buf;
}


tString tString::ExtractLeft(int count)
{
	int length = Length();
	if (count > length)
		count = length;

	if (count <= 0)
		return tString();

	tString left(count);
	tStd::tStrncpy(left.TextData, TextData, count);
	
	// Source string is known not to be empty now
	int newLength = length - count;
	if (newLength == 0)
	{
		delete TextData;
		TextData = &EmptyChar;
		return left;
	}

	char8_t* newText = new char8_t[newLength+1];
	tStd::tStrcpy(newText, TextData+count);

	delete[] TextData;
	TextData = newText;

	return left;
}


tString tString::ExtractRight(int count)
{
	int length = Length();
	int start = length - count;
	if(start < 0)
	{
		start = 0;
		count = length;
	}

	if(count <= 0)
		return tString();

	tString right(count);
	tStd::tStrncpy(right.TextData, TextData + start, count);

	// Source string is known not to be empty now
	int newLength = length - count;
	if (newLength == 0)
	{
		delete TextData;
		TextData = &EmptyChar;
		return right;
	}

	char8_t* newText = new char8_t[newLength+1];
	TextData[length - count] = '\0';

	tStd::tStrcpy(newText, TextData);

	delete[] TextData;
	TextData = newText;

	return right;
}


tString tString::ExtractLeft(const char* prefix)
{
	if (!TextData || (TextData == &EmptyChar) || !prefix)
		return tString();

	int len = tStd::tStrlen(prefix);
	if ((len <= 0) || (Length() < len))
		return tString();

	if (tStd::tStrncmp(TextData, (const char8_t*)prefix, len) == 0)
	{
		int oldlen = Length();
		char8_t* newtext = new char8_t[oldlen-len+1];
		tStd::tStrcpy(newtext, &TextData[len]);
		delete[] TextData;
		TextData = newtext;
		return tString(prefix);
	}

	return tString();
}


tString tString::ExtractRight(const char* suffix)
{
	if (!TextData || (TextData == &EmptyChar) || !suffix)
		return tString();

	int len = tStd::tStrlen(suffix);
	if ((len <= 0) || (Length() < len))
		return tString();

	if (tStd::tStrncmp(&TextData[Length()-len], (const char8_t*)suffix, len) == 0)
	{
		TextData[Length()-len] = '\0';
		return tString(suffix);
	}

	return tString();
}


tString tString::ExtractMid(int start, int count)
{
	int length = Length();
	if(start < 0 || start >= length || count <= 0)
		return tString();

	if(start + count > length)
		count = length - start;

	tString mid(count);
	tStd::tStrncpy(mid.TextData, TextData + start, count);

	int newLength = length - count;
	if(newLength == 0)
	{
		delete TextData;
		TextData = &EmptyChar;
		return mid;
	}

	char8_t* newText = new char8_t[newLength+1];
	newText[newLength] = '\0';

	tStd::tStrncpy(newText, TextData, start);
	tStd::tStrncpy(newText+start, TextData+start+count, newLength-start);

	delete[] TextData;
	TextData = newText;

	return mid;
}


int tString::Replace(const char8_t* s, const char8_t* r)
{
	if (!s || (s[0] == '\0'))
		return 0;

	int origTextLength = tStd::tStrlen(TextData);
	int searchStringLength = tStd::tStrlen(s);
	int replaceStringLength = r ? tStd::tStrlen(r) : 0;
	int replaceCount = 0;

	if (searchStringLength != replaceStringLength)
	{
		// Since the replacement string is a different size, we'll need to reallocate
		// out memory. We start by finding out how many replacements we will need to do.
		char8_t* searchStart = TextData;

		while (searchStart < (TextData + origTextLength))
		{
			char8_t* foundString = tStd::tStrstr(searchStart, s);
			if (!foundString)
				break;

			replaceCount++;
			searchStart = foundString + searchStringLength;
		}

		// The new length may be bigger or smaller than the original. If the newlength is precisely
		// 0, it means that the entire string is being replaced with nothing, so we can exit early.
		// eg. Replace "abcd" in "abcdabcd" with ""
		int newTextLength = origTextLength + replaceCount*(replaceStringLength - searchStringLength);
		if (!newTextLength)
		{
			if (TextData != &EmptyChar)
				delete[] TextData;
			TextData = &EmptyChar;
			return replaceCount;
		}

		char8_t* newText = new char8_t[newTextLength + 16];
		newText[newTextLength] = '\0';

		tStd::tMemset( newText, 0, newTextLength + 16 );

		int newTextWritePos = 0;

		searchStart = TextData;
		while (searchStart < (TextData + origTextLength))
		{
			char8_t* foundString = tStd::tStrstr(searchStart, s);

			if (foundString)
			{
				tStd::tMemcpy(newText+newTextWritePos, searchStart, int(foundString-searchStart));
				newTextWritePos += int(foundString-searchStart);

				tStd::tMemcpy(newText+newTextWritePos, r, replaceStringLength);
				newTextWritePos += replaceStringLength;
			}
			else
			{
				tStd::tStrcpy(newText+newTextWritePos, searchStart);
				break;
			}

			searchStart = foundString + searchStringLength;
		}

		if (TextData != &EmptyChar)
			delete[] TextData;
		TextData = newText;
	}
	else
	{
		// In this case the replacement string is exactly the same length at the search string.
		// Much easier to deal with and no need for memory allocation.
		char8_t* searchStart = TextData;

		while (searchStart < (TextData + origTextLength))
		{
			char8_t* foundString = tStd::tStrstr(searchStart, s);
			if (foundString)
			{
				tStd::tMemcpy(foundString, r, replaceStringLength);
				replaceCount++;
			}
			else
			{
				break;
			}

			searchStart = foundString + searchStringLength;
		}
	}

	return replaceCount;
}


int tString::Remove(const char c)
{
	int destIndex = 0;
	int numRemoved = 0;

	// This operation can be done in place.
	for (int i = 0; i < Length(); i++)
	{
		if (TextData[i] != c)
		{
			TextData[destIndex] = TextData[i];
			destIndex++;
		}
		else
		{
			numRemoved++;
		}
	}
	TextData[destIndex] = '\0';

	return numRemoved;
}


int tString::RemoveLeading(const char* removeThese)
{
	if (!TextData || (TextData == &EmptyChar) || !removeThese)
		return 0;

	int cnt = 0;
	while (TextData[cnt])
	{
		bool matches = false;
		int j = 0;
		while (removeThese[j] && !matches)
		{
			if (removeThese[j] == TextData[cnt])
				matches = true;
			j++;
		}
		if (matches) 		
			cnt++;
		else
			break;
	}

	if (cnt > 0)
	{
		int oldlen = Length();
		char8_t* newtext = new char8_t[oldlen-cnt+1];
		tStd::tStrcpy(newtext, &TextData[cnt]);
		delete[] TextData;
		TextData = newtext;
	}

	return cnt;
}


int tString::RemoveTrailing(const char* removeThese)
{
	if (!TextData || (TextData == &EmptyChar) || !removeThese)
		return 0;

	int oldlen = Length();
			
	int i = oldlen - 1;
	while (i > -1)
	{
		bool matches = false;
		int j = 0;
		while (removeThese[j] && !matches)
		{
			if (removeThese[j] == TextData[i])
				matches = true;
			j++;
		}
		if (matches) 		
			i--;
		else
			break;
	}
	int numRemoved = oldlen - i;
	TextData[i+1] = '\0';

	return numRemoved;
}


int tString::GetUTF(char16_t* dst, bool incNullTerminator)
{
	if (!dst)
		return tStd::tUTF16s(nullptr, TextData) + (incNullTerminator ? 1 : 0);

	if (incNullTerminator)
		return tStd::tUTF16s(dst, TextData);

	return tStd::tUTF16(dst, TextData, Length());
}


int tString::GetUTF(char32_t* dst, bool incNullTerminator)
{
	if (!dst)
		return tStd::tUTF32s(nullptr, TextData) + (incNullTerminator ? 1 : 0);

	if (incNullTerminator)
		return tStd::tUTF32s(dst, TextData);

	return tStd::tUTF32(dst, TextData, Length());
}


int tString::SetUTF(const char16_t* src, int srcLen)
{
	if (!src || (srcLen == 0))
	{
		Clear();
		return 0;
	}
	if (srcLen < 0)
	{
		int lenNeeded = tStd::tUTF8s(nullptr, src);
		Reserve(lenNeeded, false);
		return tStd::tUTF8s(TextData, src);
	}
	int lenNeeded = tStd::tUTF8(nullptr, src, srcLen);
	Reserve(lenNeeded, false);
	tStd::tUTF8(TextData, src, srcLen);
	TextData[lenNeeded] = '\0';
	return lenNeeded;
}


int tString::SetUTF(const char32_t* src, int srcLen)
{
	if (!src || (srcLen == 0))
	{
		Clear();
		return 0;
	}
	if (srcLen < 0)
	{
		int lenNeeded = tStd::tUTF8s(nullptr, src);
		Reserve(lenNeeded, false);
		return tStd::tUTF8s(TextData, src);
	}
	int lenNeeded = tStd::tUTF8(nullptr, src, srcLen);
	Reserve(lenNeeded, false);
	tStd::tUTF8(TextData, src, srcLen);
	TextData[lenNeeded] = '\0';
	return lenNeeded;
}


int tStd::tExplode(tList<tStringItem>& components, const tString& src, char divider)
{
	tString source = src;
	int startCount = components.GetNumItems();
	while (source.FindChar(divider) != -1)
	{
		tString component = source.ExtractLeft(divider);
		components.Append(new tStringItem(component));
	}

	// If there's anything left in source we need to add it.
	if (!source.IsEmpty())
		components.Append(new tStringItem(source));

	return components.GetNumItems() - startCount;
}


int tStd::tExplode(tList<tStringItem>& components, const tString& src, const tString& divider)
{
	// Well, this is a bit of a cheezy way of doing this.  We just assume that ASCII character 31,
	// the "unit separator", is meant for this kind of thing and not otherwise present in the src string.
	tString source = src;
	char8_t sep[2];
	sep[0] = 31;
	sep[1] = 0;
	source.Replace(divider, sep);
	return tExplode(components, source, 31);
}
