// tString.cpp
//
// tString is a simple and readable string class that implements sensible operators and implicit casts. The text in a
// tString is considerd to be UTF-8 encoded. With UTF-8 encoding each character (code-point) may be encoded by 1 or more
// code-units (a code-unit is 8 bits). The char8_t is used to repreresent a code-unit (as the C++ standard encourages).
//
// Externally a tString should be thought of as an array of code-units which may contain multiple null characters. A
// valid string of length 5 could be "ab\0\0e" for example. Internally a tString is null-terminated, but that is for
// implementational efficiency only -- many external functions require null-terminated strings, so it makes it easy to
// return one if the internal representation already has a null terminator. For example the length-5 string "abcde" is
// stored internally as 'a' 'b' 'c' 'd' 'e' '\0'.
//
// It can be inefficient (in time) to only maintain the exact amount of memory needed to store a particular string -- it
// would require a new memory allocation every time a string changes size. For this reason tStrings have a 'capacity'.
// The capacity of a tString is the number of code-units that can be stored without requiring additional memory
// management calls. For example, a tString with capacity 10 could be storing "abcde". If you were to add "fghij" to the
// string, it would be done without any delete[] or new calls. Note that internally a tString of capacity 10 actually
// has malloced an array of 11 code-units, the 11th one being for the terminating null. Functions that affect capacity
// (like Reserve) do not change the behaviour of a tString and are always safe, they simply affect the efficiency.
//
// When the tString does need to grow its capacity (perhaps another string is being added/appended to it) there is the
// question of how much extra space to reserve. The SetGrowMethod may be used to set how much extra space is reserved
// when a memory-size-changing operation takes place. By default a constant amount of extra memory is reserved.
//
// A few of the salient functions related to the above are:
// Lenght		:	Returns how many code-units are used by the string. This is NOT like a strlen call as it does not
//					rely on nul-termination. It does not need to iterate as the length is stored explicitely.
// Capacity		:	Returns the current capacity of the tString in code-units.
// Reserve		:	This is instead of a SetCapacity call. There is no SetCapacity as we could not guarantee that a
//					requested capacity is non-destructive. Calling Reserve(5) on a string of Length 10 will not result
//					in a new capacity of 5 because it would require culling half of the code-units. Reserve can also be
//					used to shrink (release memory) if possible. See the comments before the function itself.
// Shrink		:	Shrinks the tString to the least amount of memory used possible. Like calling Reserver(Length());
// SetGrowMethod:	Controls how much extra space (Capacity - Length) to reserve when performing a memory operation.
//
// For conversions of arbitrary types to tStrings, see tsPrint in the higher level System module.
//
// Copyright (c) 2004-2006, 2015, 2017, 2019-2022 Tristan Grimmer.
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


tString::operator uint32()
{
	return tHash::tHashStringFast32(CodeUnits);
}


tString::operator uint32() const
{
	return tHash::tHashStringFast32(CodeUnits);
}


tString tString::Left(const char marker) const
{
	int pos = FindChar(marker);
	if (pos == -1)
		return *this;
	if (pos == 0)
		return tString();

	tString buf(pos);
	tStd::tMemcpy(buf.CodeUnits, CodeUnits, pos);
	return buf;
}


tString tString::Right(const char marker) const
{
	int pos = FindChar(marker, true);
	if (pos == -1)
		return *this;

	int length = StringLength;
	if (pos == (length-1))
		return tString();

	tString buf(length - 1 - pos);
	tStd::tMemcpy(buf.CodeUnits, CodeUnits + pos + 1, length - 1 - pos);
	return buf;
}


tString tString::Left(int count) const
{
	if (count <= 0)
		return tString();

	int length = StringLength;
	if (count > length)
		count = length;

	tString buf(count);
	tStd::tMemcpy(buf.CodeUnits, CodeUnits, count);
	return buf;
}


tString tString::Mid(int start, int count) const
{
	int length = StringLength;
	if ((start < 0) || (start >= length) || (count <= 0))
		return tString();

	if ((start + count) > length)
		count = length - start;

	tString buf(count);
	tStd::tMemcpy(buf.CodeUnits, CodeUnits + start, count);
	return buf;
}


tString tString::Right(int count) const
{
	if (count <= 0)
		return tString();

	int length = StringLength;
	int start = length - count;
	if (start < 0)
	{
		start = 0;
		count = length;
	}

	tString buf(count);
	tStd::tMemcpy(buf.CodeUnits, CodeUnits + start, count);
	return buf;
}


tString tString::ExtractLeft(const char divider)
{
	int pos = FindChar(divider);
	if (pos == -1)
	{
		tString left(*this);
		Clear();
		return left;
	}

	int count = pos;
	tString left(count);
	tStd::tMemcpy(left.CodeUnits, CodeUnits, count);

	// We don't need to reallocate memory for this string. We can just do a memmove and adjust the StringLength.
	// Memmove is needed since src and dest overlap. Capacity can stay the same.
	StringLength -= count+1;
	if (StringLength > 0)
		tStd::tMemmov(CodeUnits, CodeUnits+pos+1, StringLength);
	CodeUnits[StringLength] = '\0';

	return left;
}


tString tString::ExtractRight(const char divider)
{
	int pos = FindChar(divider, true);
	if (pos == -1)
	{
		tString right(*this);
		Clear();
		return right;
	}

	int count = StringLength - pos - 1;
	tString right(count);
	tStd::tMemcpy(right.CodeUnits, CodeUnits+pos+1, count);

	// We don't need to reallocate or move memory for this string. We can just adjust the StringLength.
	// Capacity can stay the same.
	StringLength -= count+1;
	CodeUnits[StringLength] = '\0';

	return right;
}


tString tString::ExtractLeft(int count)
{
	if (count >= StringLength)
	{
		tString left(*this);
		Clear();
		return left;
	}

	if (count <= 0)
		return tString();

	tString left(count);
	tStd::tMemcpy(left.CodeUnits, CodeUnits, count);

	// We don't need to reallocate memory for this string. We can just do a memmove and adjust the StringLength.
	// Memmove is needed since src and dest overlap. Capacity can stay the same.
	StringLength -= count;
	if (StringLength > 0)
		tStd::tMemmov(CodeUnits, CodeUnits+count, StringLength);
	CodeUnits[StringLength] = '\0';

	return left;
}


tString tString::ExtractMid(int start, int count)
{
	int length = StringLength;
	if ((start < 0) || (start >= length) || (count <= 0))
		return tString();

	if ((start + count) > length)
		count = length - start;

	tString mid(count);
	tStd::tMemcpy(mid.CodeUnits, CodeUnits + start, count);

	// We don't need to reallocate memory for this string. We can just do a memmove and adjust the StringLength.
	// Memmove is needed since src and dest overlap. Capacity can stay the same.
	int numMove = length - (start + count);
	if (numMove > 0)
		tStd::tMemcpy(CodeUnits + start, CodeUnits + start + count, numMove);
	StringLength -= count;
	CodeUnits[StringLength] = '\0';

	return mid;
}


tString tString::ExtractRight(int count)
{
	if (count >= StringLength)
	{
		tString right(*this);
		Clear();
		return right;
	}

	if (count <= 0)
		return tString();

	tString right(count);
	tStd::tMemcpy(right.CodeUnits, CodeUnits+StringLength-count, count);

	// We don't need to reallocate or move memory for this string. We can just adjust the StringLength.
	// Capacity can stay the same.
	StringLength -= count;
	CodeUnits[StringLength] = '\0';

	return right;
}


tString tString::ExtractLeft(const char8_t* prefix)
{
	if (IsEmpty() || !prefix)
		return tString();

	int len = tStd::tStrlen(prefix);
	if ((len <= 0) || (len > StringLength))
		return tString();

	if (tStd::tStrncmp(CodeUnits, prefix, len) == 0)
	{
		// We don't need to reallocate memory for this string. We can just do a memmove and adjust the StringLength.
		// Memmove is needed since src and dest overlap. Capacity can stay the same.
		if (StringLength > len)
			tStd::tMemmov(CodeUnits, CodeUnits+len, StringLength-len);
		StringLength -= len;
		CodeUnits[StringLength] = '\0';
		return tString(prefix);
	}

	return tString();
}


tString tString::ExtractRight(const char8_t* suffix)
{
	if (IsEmpty() || !suffix)
		return tString();

	int len = tStd::tStrlen(suffix);
	if ((len <= 0) || (len > StringLength))
		return tString();

	if (tStd::tStrncmp(&CodeUnits[StringLength-len], suffix, len) == 0)
	{
		// We don't need to reallocate or move memory for this string. We can just adjust the StringLength.
		// Capacity can stay the same.
		StringLength -= len;
		CodeUnits[StringLength] = '\0';
		return tString(suffix);
	}

	return tString();
}


int tString::Replace(const char8_t* search, const char8_t* replace)
{
	// Zeroth scenario (trivial) -- Search is empty. Definitely won't be able to find it.
	if (!search || (search[0] == '\0'))
		return 0;

	// First scenario (trivial) -- The search length is bigger than the string length. It simply can't be there.
	int searchLength = tStd::tStrlen(search);
	if (searchLength > StringLength)
		return 0;

	int replaceLength = replace ? tStd::tStrlen(replace) : 0;
	int replaceCount = 0;

	// Second scenario (easy) -- The search and replace string lengths are equal. We know in this case there will be no
	// need to mess with memory and we don't care how many replacements there will be. We can just go ahead and replace
	// them in one loop.
	if (replaceLength == searchLength)
	{
		char8_t* searchStart = CodeUnits;
		while (searchStart < (CodeUnits + StringLength))
		{
			char8_t* foundString = (char8_t*)tStd::tMemsrch(searchStart, StringLength-(searchStart-CodeUnits), search, searchLength);
			if (foundString)
			{
				tStd::tMemcpy(foundString, replace, replaceLength);
				replaceCount++;
			}
			else
			{
				break;
			}
			searchStart = foundString + searchLength;
		}
		return replaceCount;
	}

	// Third scenario (hard) -- Different search and replace sizes. Supports empty replace string as well.
	// The first step is to count how many replacements there are going to be so we can set the capacity properly.
	char8_t* searchStart = CodeUnits;
	while (searchStart < (CodeUnits + StringLength))
	{
		char8_t* foundString = (char8_t*)tStd::tMemsrch(searchStart, StringLength-(searchStart-CodeUnits), search, searchLength);
		if (!foundString)
			break;

		replaceCount++;
		searchStart = foundString + searchLength;
	}

	// The new length may be bigger or smaller than the original. If the capNeeded is precisely
	// 0, it means that the entire string is being replaced with nothing, so we can exit early.
	// eg. Replace "abcd" in "abcdabcd" with ""
	int newLength = StringLength - (replaceCount*searchLength) + (replaceCount*replaceLength);
	if (newLength == 0)
	{
		Clear();
		return replaceCount;
	}

	// The easiest way of doing this is to have a scratchpad we can write the new string into.
	char8_t* newText = new char8_t[newLength];
	int newWritePos = 0;

	searchStart = CodeUnits;
	while (searchStart < (CodeUnits + StringLength))
	{
		char8_t* foundString = (char8_t*)tStd::tMemsrch(searchStart, StringLength-(searchStart-CodeUnits), search, searchLength);
		if (foundString)
		{
			// Copy the stuff before the found string.
			int lenBeforeFound = int(foundString-searchStart);
			if (lenBeforeFound > 0)
				tStd::tMemcpy(newText+newWritePos, searchStart, lenBeforeFound);
			newWritePos += int(foundString-searchStart);

			// Copy the replacement in.
			if (replaceLength > 0)
				tStd::tMemcpy(newText+newWritePos, replace, replaceLength);
			newWritePos += replaceLength;
		}
		else
		{
			// Copy the remainder when nothing found.
			int numRemain = newLength-newWritePos;
			if (numRemain > 0)
				tStd::tMemcpy(newText+newWritePos, searchStart, numRemain);
			break;
		}
		searchStart = foundString + searchLength;
	}

	// Make sure there's enough capacity.
	UpdateCapacity(newLength, false);

	// Copy the scratchpad data over.
	if (newLength > 0)
		tStd::tMemcpy(CodeUnits, newText, newLength);
	CodeUnits[newLength] = '\0';
	StringLength = newLength;
	delete[] newText;

	return replaceCount;
}


int tString::Remove(char rem)
{
	int destIndex = 0;
	int numRemoved = 0;

	// This operation can be done in place.
	for (int i = 0; i < StringLength; i++)
	{
		if (CodeUnits[i] != rem)
			CodeUnits[destIndex++] = CodeUnits[i];
		else
			numRemoved++;
	}
	StringLength -= numRemoved;
	CodeUnits[StringLength] = '\0';

	return numRemoved;
}


int tString::RemoveLeading(const char* removeThese)
{
	if (IsEmpty() || !removeThese || !removeThese[0])
		return 0;

	// Since the StringLength can't get bigger, no need to do any memory management. We can do it in one pass.
	int writeIndex = 0;
	bool checkPresence = true;
	int numRemoved = 0;
	for (int readIndex = 0; readIndex < StringLength; readIndex++)
	{
		char8_t readChar = CodeUnits[readIndex];

		// Is readChar present in theseChars?
		bool present = false; int j = 0;
		if (checkPresence)
			while (removeThese[j] && !present)
				if (removeThese[j++] == readChar)
					present = true;

		if (present && checkPresence)
		{
			numRemoved++;
			continue;
		}

		// Stop checking after hit first char not found.
		checkPresence = false;
		CodeUnits[writeIndex++] = readChar;
	}

	StringLength -= numRemoved;
	CodeUnits[StringLength] = '\0';
	return numRemoved;
}


int tString::RemoveTrailing(const char* removeThese)
{
	if (IsEmpty() || !removeThese || !removeThese[0])
		return 0;

	// Since the StringLength can't get bigger, no need to do any memory management. We can do it in one pass.
	int writeIndex = StringLength-1;
	bool checkPresence = true;
	int numRemoved = 0;
	for (int readIndex = StringLength-1; readIndex >= 0; readIndex--)
	{
		char8_t readChar = CodeUnits[readIndex];

		// Is readChar present in theseChars?
		bool present = false; int j = 0;
		if (checkPresence)
			while (removeThese[j] && !present)
				if (removeThese[j++] == readChar)
					present = true;

		if (present && checkPresence)
		{
			numRemoved++;
			continue;
		}

		// Stop checking after hit first char (going backwards) not found.
		checkPresence = false;
		CodeUnits[writeIndex--] = readChar;
	}

	StringLength -= numRemoved;

	// Cuz we went backwards we now need to shift everything left to where Codeunits begins.
	// Important to use memory-move and not memory-copy because they overlap.
	if (numRemoved > 0)
		tStd::tMemmov(CodeUnits, CodeUnits+writeIndex+1, StringLength);
	CodeUnits[StringLength] = '\0';
	return numRemoved;
}

int tString::RemoveFirst()
{
	if (IsEmpty())
		return 0;
	
	// We don't have a -1 on the StringLength here so we get the internal null terminator for free.
	tStd::tMemmov(CodeUnits, CodeUnits+1, StringLength);
	StringLength--;
	return 1;
}


int tString::RemoveLast()
{
	if (IsEmpty())
		return 0;

	StringLength--;
	CodeUnits[StringLength] = '\0';
	return 1;
}


int tString::RemoveAny(const char* removeThese)
{
	if (IsEmpty() || !removeThese || !removeThese[0])
		return 0;

	// Since the StringLength can't get bigger, no need to do any memory management. We can do it in one pass.
	int writeIndex = 0;
	int numRemoved = 0;
	for (int readIndex = 0; readIndex < StringLength; readIndex++)
	{
		char8_t readChar = CodeUnits[readIndex];

		// Is readChar present in theseChars?
		bool present = false; int j = 0;
		while (removeThese[j] && !present)
			if (removeThese[j++] == readChar)
				present = true;

		if (present)
		{
			numRemoved++;
			continue;
		}

		CodeUnits[writeIndex++] = readChar;
	}

	StringLength -= numRemoved;
	CodeUnits[StringLength] = '\0';
	return numRemoved;
}


int tString::GetUTF16(char16_t* dst, bool incNullTerminator) const
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


int tString::GetUTF32(char32_t* dst, bool incNullTerminator) const
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


int tString::SetUTF16(const char16_t* src, int srcLen)
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


int tString::SetUTF32(const char32_t* src, int srcLen)
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


void tString::UpdateCapacity(int capNeeded, bool preserve)
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
