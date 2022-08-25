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


//char8_t tString::EmptyChar = '\0';

#if 0
tString::operator uint32()
{
	return tHash::tHashStringFast32(CodeUnits);

}


tString::operator uint32() const
{
	return tHash::tHashStringFast32(CodeUnits);
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
	tStd::tStrncpy(buf.CodeUnits, CodeUnits, pos);
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
	tStd::tStrncpy(buf.CodeUnits, CodeUnits + pos + 1, length - 1 - pos);
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
	tStd::tStrncpy(buf.CodeUnits, CodeUnits, count);
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
	tStd::tStrncpy(buf.CodeUnits, CodeUnits + start, count);
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
	tStd::tStrncpy(buf.CodeUnits, CodeUnits + start, count);
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
	tStd::tStrncpy(buf.CodeUnits, CodeUnits, pos);

	int length = Length();
	char8_t* newText = new char8_t[length-pos];

	// This will append the null.
	tStd::tStrncpy(newText, CodeUnits+pos+1, length-pos);

	if (CodeUnits != &EmptyChar)
		delete[] CodeUnits;
	CodeUnits = newText;

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
	tStd::tStrncpy(buf.CodeUnits, CodeUnits+pos+1, wordLength);

	char8_t* newText = new char8_t[pos+1];
	tStd::tStrncpy(newText, CodeUnits, pos);
	newText[pos] = '\0';

	if (CodeUnits != &EmptyChar)
		delete[] CodeUnits;
	CodeUnits = newText;

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


int tString::GetUTF16(char16_t* dst, bool incNullTerminator) const
{
	if (!dst)
		return tStd::tUTF16s(nullptr, CodeUnits) + (incNullTerminator ? 1 : 0);

	if (incNullTerminator)
		return tStd::tUTF16s(dst, CodeUnits);

	return tStd::tUTF16(dst, CodeUnits, Length());
}


int tString::GetUTF32(char32_t* dst, bool incNullTerminator) const
{
	if (!dst)
		return tStd::tUTF32s(nullptr, CodeUnits) + (incNullTerminator ? 1 : 0);

	if (incNullTerminator)
		return tStd::tUTF32s(dst, CodeUnits);

	return tStd::tUTF32(dst, CodeUnits, Length());
}

#endif
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
