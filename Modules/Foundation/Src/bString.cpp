// bString.cpp
//
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

#include "Foundation/bString.h"
#include "Foundation/tStandard.h"
#include "Foundation/tHash.h"


bString::operator uint32()
{
	return tHash::tHashStringFast32(CodeUnits);

}


bString::operator uint32() const
{
	return tHash::tHashStringFast32(CodeUnits);
}


bString bString::Left(const char marker) const
{
	int pos = FindChar(marker);
	if (pos == -1)
		return *this;
	if (pos == 0)
		return bString();

	bString buf(pos);
	tStd::tMemcpy(buf.CodeUnits, CodeUnits, pos);
	return buf;
}


bString bString::Right(const char marker) const
{
	int pos = FindChar(marker, true);
	if (pos == -1)
		return *this;

	int length = StringLength;
	if (pos == (length-1))
		return bString();

	bString buf(length - 1 - pos);
	tStd::tMemcpy(buf.CodeUnits, CodeUnits + pos + 1, length - 1 - pos);
	return buf;
}


bString bString::Left(int count) const
{
	if (count <= 0)
		return bString();

	int length = StringLength;
	if (count > length)
		count = length;

	bString buf(count);
	tStd::tMemcpy(buf.CodeUnits, CodeUnits, count);
	return buf;
}


bString bString::Right(int count) const
{
	if (count <= 0)
		return bString();

	int length = StringLength;
	int start = length - count;
	if (start < 0)
	{
		start = 0;
		count = length;
	}

	bString buf(count);
	tStd::tMemcpy(buf.CodeUnits, CodeUnits + start, count);
	return buf;
}


bString bString::Mid(int start, int count) const
{
	int length = StringLength;
	if ((start < 0) || (start >= length) || (count <= 0))
		return bString();

	if ((start + count) > length)
		count = length - start;

	bString buf(count);
	tStd::tMemcpy(buf.CodeUnits, CodeUnits + start, count);
	return buf;
}


///// WIP These should just call the versions taking the count.
bString bString::ExtractLeft(const char divider)
{
	int pos = FindChar(divider);
	if (pos == -1)
	{
		bString left(*this);
		Clear();
		return left;
	}

	int count = pos;
	bString left(count);
	tStd::tMemcpy(left.CodeUnits, CodeUnits, count);

	// We don't need to reallocate memory for this string. We can just do a memcpy
	// and adjust the StringLength. Capacity can stay the same.
	StringLength -= count+1;
	if (StringLength > 0)
		tStd::tMemcpy(CodeUnits, CodeUnits+pos+1, StringLength);
	CodeUnits[StringLength] = '\0';

	return left;
}


bString bString::ExtractRight(const char divider)
{
	int pos = FindChar(divider, true);
	if (pos == -1)
	{
		bString right(*this);
		Clear();
		return right;
	}

	int count = StringLength - pos - 1;
	bString right(count);
	tStd::tMemcpy(right.CodeUnits, CodeUnits+pos+1, count);

	// We don't need to reallocate memory for this string. We can just adjust the StringLength. Capacity can stay the same.
	StringLength -= count+1;
	CodeUnits[StringLength] = '\0';

	return right;
}


#if 0
tString tString::ExtractLeft(int count)
{
	int length = Length();
	if (count > length)
		count = length;

	if (count <= 0)
		return tString();

	tString left(count);
	tStd::tStrncpy(left.CodeUnits, CodeUnits, count);
	
	// Source string is known not to be empty now
	int newLength = length - count;
	if (newLength == 0)
	{
		delete CodeUnits;
		CodeUnits = &EmptyChar;
		return left;
	}

	char8_t* newText = new char8_t[newLength+1];
	tStd::tStrcpy(newText, CodeUnits+count);

	delete[] CodeUnits;
	CodeUnits = newText;

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
	tStd::tStrncpy(right.CodeUnits, CodeUnits + start, count);

	// Source string is known not to be empty now
	int newLength = length - count;
	if (newLength == 0)
	{
		delete CodeUnits;
		CodeUnits = &EmptyChar;
		return right;
	}

	char8_t* newText = new char8_t[newLength+1];
	CodeUnits[length - count] = '\0';

	tStd::tStrcpy(newText, CodeUnits);

	delete[] CodeUnits;
	CodeUnits = newText;

	return right;
}


tString tString::ExtractLeft(const char* prefix)
{
	if (!CodeUnits || (CodeUnits == &EmptyChar) || !prefix)
		return tString();

	int len = tStd::tStrlen(prefix);
	if ((len <= 0) || (Length() < len))
		return tString();

	if (tStd::tStrncmp(CodeUnits, (const char8_t*)prefix, len) == 0)
	{
		int oldlen = Length();
		char8_t* newtext = new char8_t[oldlen-len+1];
		tStd::tStrcpy(newtext, &CodeUnits[len]);
		delete[] CodeUnits;
		CodeUnits = newtext;
		return tString(prefix);
	}

	return tString();
}


tString tString::ExtractRight(const char* suffix)
{
	if (!CodeUnits || (CodeUnits == &EmptyChar) || !suffix)
		return tString();

	int len = tStd::tStrlen(suffix);
	if ((len <= 0) || (Length() < len))
		return tString();

	if (tStd::tStrncmp(&CodeUnits[Length()-len], (const char8_t*)suffix, len) == 0)
	{
		CodeUnits[Length()-len] = '\0';
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
	tStd::tStrncpy(mid.CodeUnits, CodeUnits + start, count);

	int newLength = length - count;
	if(newLength == 0)
	{
		delete CodeUnits;
		CodeUnits = &EmptyChar;
		return mid;
	}

	char8_t* newText = new char8_t[newLength+1];
	newText[newLength] = '\0';

	tStd::tStrncpy(newText, CodeUnits, start);
	tStd::tStrncpy(newText+start, CodeUnits+start+count, newLength-start);

	delete[] CodeUnits;
	CodeUnits = newText;

	return mid;
}


int tString::Replace(const char8_t* s, const char8_t* r)
{
	if (!s || (s[0] == '\0'))
		return 0;

	int origTextLength = tStd::tStrlen(CodeUnits);
	int searchStringLength = tStd::tStrlen(s);
	int replaceStringLength = r ? tStd::tStrlen(r) : 0;
	int replaceCount = 0;

	if (searchStringLength != replaceStringLength)
	{
		// Since the replacement string is a different size, we'll need to reallocate
		// out memory. We start by finding out how many replacements we will need to do.
		char8_t* searchStart = CodeUnits;

		while (searchStart < (CodeUnits + origTextLength))
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
			if (CodeUnits != &EmptyChar)
				delete[] CodeUnits;
			CodeUnits = &EmptyChar;
			return replaceCount;
		}

		char8_t* newText = new char8_t[newTextLength + 16];
		newText[newTextLength] = '\0';

		tStd::tMemset( newText, 0, newTextLength + 16 );

		int newTextWritePos = 0;

		searchStart = CodeUnits;
		while (searchStart < (CodeUnits + origTextLength))
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

		if (CodeUnits != &EmptyChar)
			delete[] CodeUnits;
		CodeUnits = newText;
	}
	else
	{
		// In this case the replacement string is exactly the same length at the search string.
		// Much easier to deal with and no need for memory allocation.
		char8_t* searchStart = CodeUnits;

		while (searchStart < (CodeUnits + origTextLength))
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
		if (CodeUnits[i] != c)
		{
			CodeUnits[destIndex] = CodeUnits[i];
			destIndex++;
		}
		else
		{
			numRemoved++;
		}
	}
	CodeUnits[destIndex] = '\0';

	return numRemoved;
}


int tString::RemoveLeading(const char* removeThese)
{
	if (!CodeUnits || (CodeUnits == &EmptyChar) || !removeThese)
		return 0;

	int cnt = 0;
	while (CodeUnits[cnt])
	{
		bool matches = false;
		int j = 0;
		while (removeThese[j] && !matches)
		{
			if (removeThese[j] == CodeUnits[cnt])
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
		tStd::tStrcpy(newtext, &CodeUnits[cnt]);
		delete[] CodeUnits;
		CodeUnits = newtext;
	}

	return cnt;
}


int tString::RemoveTrailing(const char* removeThese)
{
	if (!CodeUnits || (CodeUnits == &EmptyChar) || !removeThese)
		return 0;

	int oldlen = Length();
			
	int i = oldlen - 1;
	while (i > -1)
	{
		bool matches = false;
		int j = 0;
		while (removeThese[j] && !matches)
		{
			if (removeThese[j] == CodeUnits[i])
				matches = true;
			j++;
		}
		if (matches) 		
			i--;
		else
			break;
	}
	int numRemoved = oldlen - i;
	CodeUnits[i+1] = '\0';

	return numRemoved;
}
#endif

int bString::GetUTF16(char16_t* dst, bool incNullTerminator) const
{
	if (IsEmpty())
		return 0;

	if (!dst)
		return tStd::tUTF16(nullptr, CodeUnits, StringLength) + (incNullTerminator ? 1 : 0);

	int numUnitsWritten = tStd::tUTF16(dst, CodeUnits, StringLength);
	if (incNullTerminator)
	{
		dst[numUnitsWritten] = 0;
		numUnitsWritten++;
	}

	return numUnitsWritten;
}


int bString::GetUTF32(char32_t* dst, bool incNullTerminator) const
{
	if (IsEmpty())
		return 0;

	if (!dst)
		return tStd::tUTF32(nullptr, CodeUnits, StringLength) + (incNullTerminator ? 1 : 0);

	int numUnitsWritten = tStd::tUTF32(dst, CodeUnits, StringLength);
	if (incNullTerminator)
	{
		dst[numUnitsWritten] = 0;
		numUnitsWritten++;
	}

	return numUnitsWritten;
}


int bString::SetUTF16(const char16_t* src, int srcLen)
{
	if (!src || (srcLen == 0))
	{
		Clear();
		return 0;
	}

	// If srcLen < 0 it means ignore srcLen and assume src is null-terminated.
	if (srcLen < 0)
	{
		int len = tStd::tUTF8s(nullptr, src);
		UpdateCapacity(len, false);
		StringLength = tStd::tUTF8s(CodeUnits, src);
	}
	else
	{
		int len = tStd::tUTF8(nullptr, src, srcLen);
		UpdateCapacity(len, false);
		tStd::tUTF8(CodeUnits, src, srcLen);
		CodeUnits[len] = '\0';
		StringLength = len;
	}

	return StringLength;
}


int bString::SetUTF32(const char32_t* src, int srcLen)
{
	if (!src || (srcLen == 0))
	{
		Clear();
		return 0;
	}

	// If srcLen < 0 it means ignore srcLen and assume src is null-terminated.
	if (srcLen < 0)
	{
		int len = tStd::tUTF8s(nullptr, src);
		UpdateCapacity(len, false);
		StringLength = tStd::tUTF8s(CodeUnits, src);
	}
	else
	{
		int len = tStd::tUTF8(nullptr, src, srcLen);
		UpdateCapacity(len, false);
		tStd::tUTF8(CodeUnits, src, srcLen);
		CodeUnits[len] = '\0';
		StringLength = len;
	}

	return StringLength;
}

#if 0


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
#endif


void bString::UpdateCapacity(int capNeeded, bool preserve)
{
	int grow = 0;
	if (capNeeded > 0)
		grow = (GrowParam >= 0) ? GrowParam : capNeeded*(-GrowParam);

	capNeeded += grow;
	if (capNeeded < MinCapacity)
		capNeeded = MinCapacity;

	if (CurrCapacity >= capNeeded)
	{
		if (!preserve)
		{
			StringLength = 0;
			CodeUnits[0] = '\0';
		}
		return;
	}

	char8_t* newUnits = new char8_t[capNeeded+1];
	if (preserve)
	{
		tAssert(capNeeded >= StringLength);
		if (StringLength > 0)
			tStd::tMemcpy(newUnits, CodeUnits, StringLength);
	}
	else
	{
		StringLength = 0;
	}
	newUnits[StringLength] = '\0';

	// CodeUnits mey be nullptr the first time.
	delete[] CodeUnits;
	CodeUnits = newUnits;
	CurrCapacity = capNeeded;
}
