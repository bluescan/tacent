// tName.h
//
// tName is similar to a tString but much simpler. It supports (currently) no string manipulation functions but it
// is much faster for other operations -- in particular comparisons. Internally it stores both a UTF-8 code-unit array,
// and a 64 bit hash. The hash allows the tName to be treated like an ID and gives it fast checks on equality operators.
// A hash table is NOT used by this class -- with a 64 bit hash and a universe of 1000000 strings, the probability of a
// collision is miniscule at around 2.7e-8 (assuming the hash function is good).
//
// The text in a tName is considered to be UTF-8 encoded. With UTF-8 encoding each character (code-point) may be encoded
// by 1 or more code-units (a code-unit is 8 bits). The char8_t is used to repreresent a code-unit (as the C++ standard
// encourages).
//
// Unlike tString, tName does NOT maintain a buffer with higher capacity than the string length or manage growing or
// shrinking the buffer. What it does is allocate a buffer for the precise size required (number of code-units plus one
// for the terminating null). If the string is very small, it uses what would be reserved for the code-unit array
// pointer as the data itself and avoids a heap allocation. When compiling as x64, you get up to 7 code-units in length
// (the eighth byte is for the null). When compiling for x86 or any target with 4 byte pointers this feature is
// disabled. This feature makes tName well-suited to store FourCCs. tName also maintains the length as a separate 4 byte
// integer. In total:
//  * 8 bytes of hash.
//  * 8 bytes (either pointer-to-data or data).
//  * 4 bytes length.
//
// A tName is _always_ null-terminated internally however you may store a string with more than one null ('\0') in it.
//
// Copyright (c) 2025 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include "Foundation/tStandard.h"
#include "Foundation/tString.h"
#include "Foundation/tHash.h"


struct tName
{
	// Constructs an initially invalid tName. Invalid is considered different to the empty string.
	tName()																												{ }

	// Copy cons.
	tName(const tName& src)																								{ Set(src); }

	// Construct from tString.
	tName(const tString& src)																							{ Set(src); }

	// Creates a tName with a single character, Note the char type here. A char8_t can't be guaranteed to store a
	// unicode codepoint if the codepoint requires continuations in the UTF-8 encoding. So, here we support char only
	// which we use for ASCII characters since ASCII chars are guaranteed to _not_ need continuation units in UFT-8.
	tName(char c)																										{ Set(c); }

	// These constructors expect the string pointers to be null-terminated. You can create a UTF-8 tName from an ASCII
	// string (char*) since all ASCII strings are valid UTF-8. Constructors taking char8_t, char16_t, or chat32_t
	// pointers assume the src is UTF encoded.
	tName(const char*				src)																				{ Set(src); }
	tName(const char8_t*			src)																				{ Set(src); }
	tName(const char16_t*			src)																				{ Set(src); }
	tName(const char32_t*			src)																				{ Set(src); }

	// These constructors, that specify the input length, allow you to have more than one null be present in the tName.
	// The internal hash is computed on the full number of code-units stored minus the internal terminating null.
	// For example, with 0 meaning '\0'.
	//
	// AB0CD0 with srcLen = 5 will be stored in the tName as AB0CD0
	// AB0CD0 with srcLen = 6 will be stored in the tName as AB0CD00
	// AB0CD  with srcLen = 5 will be stored in the tName as AB0CD0
	// 0      with srcLen = 0 will be stored in the tName as 0. This is the empty string.
	// AB0CD0 with srcLen = 2 will be stored in the tName as AB0
	// AB0CD0 with srcLen = 3 will be stored in the tName as AB00
	//
	// In all cases the length returned by Length() will match the supplied srcLen.
	tName(const char*				src, int srcLen)																	{ Set(src, srcLen); }
	tName(const char8_t*			src, int srcLen)																	{ Set(src, srcLen); }
	tName(const char16_t*			src, int srcLen)																	{ Set(src, srcLen); }
	tName(const char32_t*			src, int srcLen)																	{ Set(src, srcLen); }

	// The tStringUTF constructors allow the src strings to have multiple nulls in them.
	tName(const tStringUTF16&		src)																				{ Set(src); }
	tName(const tStringUTF32&		src)																				{ Set(src); }
	virtual ~tName()																									{ Clear(); }

	// The set functions always clear the current string and set it to the supplied src.
	void Set(const tName&			src);
	void Set(const tString&			src);
	void Set(char);
	void Set(const char*			src)																				{ Set((const char8_t*)src); }
	void Set(const char8_t*			src);
	void Set(const char16_t*		src)																				{ SetUTF16(src); }
	void Set(const char32_t*		src)																				{ SetUTF32(src); }
	void Set(const char*			src, int srcLen)																	{ Set((const char8_t*)src, srcLen); }
	void Set(const char8_t*			src, int srcLen);
	void Set(const char16_t*		src, int srcLen);
	void Set(const char32_t*		src, int srcLen);
	void Set(const tStringUTF16&	src);
	void Set(const tStringUTF32&	src);

	void Clear()					/* Makes the string invalid. Frees any heap memory used. */							{ CodeUnitsSize = 0; delete[] CodeUnits; CodeUnits = nullptr; Hash = 0; }
	void Empty()					/* Makes the string a valid empty string. */										{ Clear(); CodeUnitsSize = 1; CodeUnits = new char8_t[CodeUnitsSize]; CodeUnits[0] = '\0'; Hash = ComputeHash(); }

	// The length in char8_t's (code-units), not the display length (which is not that useful). Returns -1 if the tName
	// is not set (invalid).
	int Length() const																									{ return CodeUnitsSize - 1; }

	bool IsEmpty() const			/* Returns true for the empty name (length 0). This is a valid name. */				{ return (CodeUnitsSize == 1); }		// The 1 accounts for the internal null terminator.
	bool IsValid() const			/* Returns true for empty name "" (length 0) or any string with length >= 1 */		{ return (CodeUnitsSize != 0); }
	bool IsInvalid() const			/* Returns true for an invalid tName (length -1). */								{ return (CodeUnitsSize == 0); }

	bool IsSet() const				/* Synonym for IsValid. Also returns true for the empty name. */					{ return IsValid(); }
	bool IsNotSet() const			/* Synonym for IsInvalid. Only returns true for tNames that haven't been set. */	{ return IsInvalid(); }

	tName& operator=(const tName&);

	// The IsEqual variants taking (only) pointers assume null-terminated inputs. Two empty names are considered equal.
	// If the input is nullptr (for functions taking pointers) it is not considered equal to an empty name. A nullptr is
	// treated as an unset/invalid name and is not equal to anything (including another invalid name). For variants
	// taking pointers and a length, all characters are checked (multiple null chars supported).
	// ********WIP********
	bool IsEqual(const tName&		nam) const																			{ return IsEqual(nam.CodeUnits, nam.Length()); }
	bool IsEqual(const char*		str) const																			{ return IsEqual(str, str ? tStd::tStrlen(str) : 0); }
	bool IsEqual(const char8_t*		str) const																			{ return IsEqual(str, str ? tStd::tStrlen(str) : 0); }
	bool IsEqual(const char*		str, int strLen) const																{ return IsEqual((const char8_t*)str, strLen); }
	bool IsEqual(const char8_t*		str, int strLen) const;
	bool IsEqualCI(const tName&		nam) const																			{ return IsEqualCI(nam.CodeUnits, nam.Length()); }
	bool IsEqualCI(const char*		str) const																			{ return IsEqualCI(str, str ? tStd::tStrlen(str) : 0); }
	bool IsEqualCI(const char8_t*	str) const																			{ return IsEqualCI(str, str ? tStd::tStrlen(str) : 0); }
	bool IsEqualCI(const char*		str, int strLen) const																{ return IsEqualCI((const char8_t*)str, strLen); }
	bool IsEqualCI(const char8_t*	str, int strLen) const;

	// These allow for implicit conversion to a UTF-8 code-unit pointer. By not including implicit casts to const char*
	// we are encouraging further proper use of char8_t. You can either make the function you are calling take the
	// proper UTF-* type, or explicitly call Chr() or Txt() to get an old char-based pointer.
	operator const char8_t*()																							{ return CodeUnits; }
	operator const char8_t*() const																						{ return CodeUnits; }

	// The array index operator may be somewhat meaningless if there is a continuation at the index. It is assumed you
	// know what you're doing. The returned type of char is meant to emphasize that the returned value should be
	// interpreted as an ASCII char since char8_t are what is used in UTF-8 continuations. This also allows the result
	// to be used with the char-constructor of another string if desired.
	char& operator[](int i)																								{ return ((char*)CodeUnits)[i]; }

	// These return the fast 32bit hash of the string data (code units). They take into account the full represented
	// string -- not just up to the first null. That is, they use StringLength as the data-set size.
	explicit operator uint32();
	explicit operator uint32() const;

	// Accesses the raw UTF-8 codeunits represented by the 'official' unsigned UTF-8 character datatype char8_t. An
	// unset (invalid) tName will return nullptr. Charz, additionally, will return nullptr for tNames that are the empty
	// string "".
	char8_t* Text()																										{ return CodeUnits; }
	const char8_t* Chars() const																						{ return CodeUnits; }
	const char8_t* Charz() const	/* Like Chars() but returns nullptr if the name is empty, not a pointer to "". */	{ return IsEmpty() ? nullptr : CodeUnits; }
	char8_t* Units() const			/* Unicode naming. Code 'units'. */													{ return CodeUnits; }

	// Many other functions and libraries that are UTF-8 compliant do not yet (and may never) use the proper char8_t
	// type and use char* and const char*. These functions allow you to retrieve the tName using the char type. You can
	// also use these with tPrintf and %s. These are synonyms of the above 4 calls.
	char* Txt()																											{ return (char*)CodeUnits; }
	const char* Chr() const																								{ return (const char*)CodeUnits; }
	const char* Chz() const			/* Like Chr() but returns nullptr if the string is empty, not a pointer to "". */	{ return IsEmpty() ? nullptr : (const char*)CodeUnits; }
	char8_t* Pod() const			/* Plain Old Data */																{ return CodeUnits; }

	// The GetAs functions consider the contents of the current tString up to the first null encountered. See comment
	// for tStrtoiT in tStandard.h for format requirements. The summary is that if base is -1, the function looks one of
	// the following prefixes in the string, defaulting to base 10 if none found.
	//
	// Base 16 prefixes: x X 0x 0X #
	// Base 10 prefixes: d D 0d 0D
	// Base 8  prefixes: o O 0o 0O @
	// Base 2  prefixes: b B 0b 0B
	int GetAsInt(int base = -1) const																					{ return GetAsInt32(base); }
	int32 GetAsInt32(int base = -1) const																				{ return tStd::tStrtoi32(CodeUnits, base); }
	int64 GetAsInt64(int base = -1) const																				{ return tStd::tStrtoi64(CodeUnits, base); }
	uint GetAsUInt(int base = -1) const																					{ return GetAsUInt32(base); }
	uint32 GetAsUInt32(int base = -1) const																				{ return tStd::tStrtoui32(CodeUnits, base); }
	uint64 GetAsUInt64(int base = -1) const																				{ return tStd::tStrtoui64(CodeUnits, base); }

	// Case insensitive. Interprets "true", "t", "yes", "y", "on", "enable", "enabled", "1", "+", and strings that
	// represent non-zero integers as boolean true. Otherwise false.
	bool GetAsBool() const																								{ return tStd::tStrtob(CodeUnits); }

	// Base 10 interpretation only.
	float GetAsFloat() const																							{ return tStd::tStrtof(CodeUnits); }
	double GetAsDouble() const																							{ return tStd::tStrtod(CodeUnits); }

	// Shorter synonyms.
	int AsInt(int base = -1) const																						{ return GetAsInt(base); }
	int AsInt32(int base = -1) const																					{ return GetAsInt32(base); }
	int64 AsInt64(int base = -1) const																					{ return GetAsInt64(base); }
	uint AsUInt(int base = -1) const																					{ return GetAsUInt(base); }
	uint AsUInt32(int base = -1) const																					{ return GetAsUInt32(base); }
	uint64 AsUInt64(int base = -1) const																				{ return GetAsUInt64(base); }
	bool AsBool() const																									{ return GetAsBool(); }
	float AsFloat() const																								{ return GetAsFloat(); }
	double AsDouble() const																								{ return GetAsDouble(); }

	// Same as above but return false on any parse error instead of just returning 0.
	// @todo Float and double versions.
	bool ToInt(int& v, int base = -1) const																				{ return ToInt32(v, base); }
	bool ToInt32(int32& v, int base = -1) const																			{ return tStd::tStrtoi32(v, CodeUnits, base); }
	bool ToInt64(int64& v, int base = -1) const																			{ return tStd::tStrtoi64(v, CodeUnits, base); }
	bool ToUInt(uint& v, int base = -1) const																			{ return ToUInt32(v, base); }
	bool ToUInt32(uint32& v, int base = -1) const																		{ return tStd::tStrtoui32(v, CodeUnits, base); }
	bool ToUInt64(uint64& v, int base = -1) const																		{ return tStd::tStrtoui64(v, CodeUnits, base); }

	// tName UTF encoding/decoding functions. tName is encoded in UTF-8. These functions allow you to convert from tName
	// to UTF-16/32 arrays. If dst is nullptr returns the number of charN codeunits needed. If incNullTerminator is
	// false the number needed will be one fower. If dst is valid, writes the codeunits to dst and returns number of
	// charN codeunits written.
	int GetUTF16(char16_t* dst, bool incNullTerminator = true) const;
	int GetUTF32(char32_t* dst, bool incNullTerminator = true) const;

	// Sets the tName from a UTF codeunit array. If srcLen is -1 assumes supplied array is null-terminated, otherwise
	// specify how long it is. Returns new length (not including null terminator) of the tName.
	int SetUTF16(const char16_t* src, int srcLen = -1);
	int SetUTF32(const char32_t* src, int srcLen = -1);

protected:
	// Assumes CodeUnits and CodeUnitsSize are set appropriately. If CodeUnits is nullptr or CodeunitsSize is 0,
	// the tName is invalid and returns 0 for the hash. Note that the empty string does NOT get a 0 hash.
	uint64 ComputeHash() const;

	// By using the char8_t we are indicating the data is stored with UTF-8 encoding. Note that unlike char, a char8_t
	// is guaranteed to be unsigned, as well as a distinct type. In unicode spec for UTFn, these are called code-units.
	// With tNames the CodeUnits pointer (8 bytes) is directly used to store names of up to 7 code-units. The size
	// depends on whether you are compiling for 32 or 64 bit -- the pointer sizes are either 4 or 8 bytes.
	char8_t* CodeUnits				= nullptr;

	uint64 Hash						= 0;

	// The length of the CodeUnits array (not including the null terminator). For an invalid tName the Length is -1.
	int32 CodeUnitsSize				= 0;
};


// Binary operator overloads should be outside the class so we can do things like if ("a" == b) where b is a tString.
// Operators below that take char or char8_t pointers assume they are null-terminated.
inline bool operator==(const tName& a, const tName& b)																	{ return a.IsEqual(b); }
inline bool operator!=(const tName& a, const tName& b)																	{ return !a.IsEqual(b); }
inline bool operator==(const tName& a, const char8_t* b)																{ return a.IsEqual(b); }
inline bool operator!=(const tName& a, const char8_t* b)																{ return !a.IsEqual(b); }
inline bool operator==(const char8_t* a, const tName& b)																{ return b.IsEqual(a); }
inline bool operator!=(const char8_t* a, const tName& b)																{ return !b.IsEqual(a); }
inline bool operator==(const char* a, const tName& b)																	{ return b.IsEqual(a); }
inline bool operator!=(const char* a, const tName& b)																	{ return !b.IsEqual(a); }


// The tNameItem class is just the tName class except they can be placed on tLists.
struct tNameItem : public tLink<tNameItem>, public tName
{
public:
	tNameItem()																											: tName() { }

	// The tNameItem copy cons is missing, because as a list item can only be on one list at a time.
	tNameItem(const tName& s)																							: tName(s) { }
	tNameItem(const tStringUTF16& s)																					: tName(s) { }
	tNameItem(const tStringUTF32& s)																					: tName(s) { }
	tNameItem(int length)																								: tName(length) { }
	tNameItem(const char8_t* c)																							: tName(c) { }
	tNameItem(char c)																									: tName(c) { }

	// This call does NOT change the list that the tStringItem is on. The link remains unmodified.
	tNameItem& operator=(const tNameItem&);
};


// Implementation below this line.


inline uint64 tName::ComputeHash() const
{
	if (!CodeUnits || !CodeUnitsSize)
		return 0;

	return tHash::tHashData64((const uint8*)CodeUnits, CodeUnitsSize);
}


inline tName& tName::operator=(const tName& src)
{
	if (this == &src)
		return *this;

	Clear();
	if (src.IsInvalid())
		return *this;

	CodeUnitsSize = src.CodeUnitsSize;
	CodeUnits = new char8_t[CodeUnitsSize];
	tStd::tMemcpy(CodeUnits, src.CodeUnits, CodeUnitsSize);
	Hash = src.Hash;
	return *this;
}

#if 0
inline void tString::Set(const tString& src)
{
	int srcLen = src.Length();
	UpdateCapacity(srcLen, false);

	StringLength = srcLen;
	tStd::tMemcpy(CodeUnits, src.CodeUnits, StringLength);
	CodeUnits[StringLength] = '\0';
}


inline void tString::Set(int length)
{
	tAssert(length >= 0);
	UpdateCapacity(length, false);
	tStd::tMemset(CodeUnits, 0, length+1);
	StringLength = length;
}


inline void tString::Set(char c)
{
	UpdateCapacity(1, false);
	CodeUnits[0] = c;
	CodeUnits[1] = '\0';
	StringLength = 1;
}


inline void tString::Set(const char8_t* src)
{
	int srcLen = src ? tStd::tStrlen(src) : 0;
	UpdateCapacity(srcLen, false);
	if (srcLen > 0)
	{
		tStd::tMemcpy(CodeUnits, src, srcLen);
		CodeUnits[srcLen] = '\0';
		StringLength = srcLen;
	}
}


inline void tString::Set(const char8_t* src, int srcLen)
{
	if (!src || (srcLen < 0))
		srcLen = 0;
	UpdateCapacity(srcLen, false);
	if (srcLen > 0)
		tStd::tMemcpy(CodeUnits, src, srcLen);
	CodeUnits[srcLen] = '\0';
	StringLength = srcLen;
}


inline void tString::Set(const char16_t* src, int srcLen)
{
	if (srcLen <= 0)
	{
		Clear();
		return;
	}
	SetUTF16(src, srcLen);
}


inline void tString::Set(const char32_t* src, int srcLen)
{
	if (srcLen <= 0)
	{
		Clear();
		return;
	}
	SetUTF32(src, srcLen);
}


inline void tString::Set(const tStringUTF16& src)
{
	SetUTF16(src.Units(), src.Length());
}


inline void tString::Set(const tStringUTF32& src)
{
	SetUTF32(src.Units(), src.Length());
}


inline bool tString::IsEqual(const char8_t* str, int strLen) const
{
	if (!str || (Length() != strLen))
		return false;

	// We also compare the null so that we can compare strings of length 0.
	return !tStd::tMemcmp(CodeUnits, str, strLen+1);
}


inline bool tString::IsEqualCI(const char8_t* str, int strLen) const
{
	if (!str || (Length() != strLen))
		return false;

	for (int n = 0; n < strLen; n++)
		if (tStd::tToLower(CodeUnits[n]) != tStd::tToLower(str[n]))
			return false;

	return true;
}


inline bool tString::IsAlphabetic(bool includeUnderscore) const 
{
	for (int n = 0; n < StringLength; n++)
	{
		char c = char(CodeUnits[n]);
		if ( !(tStd::tIsalpha(c) || (includeUnderscore && (c == '_'))) )
			return false;
	}

	return true;
}


inline bool tString::IsNumeric(bool includeDecimal) const 
{
	for (int n = 0; n < StringLength; n++)
	{
		char c = char(CodeUnits[n]);
		if ( !(tStd::tIsdigit(c) || (includeDecimal && (c == '.'))) )
			return false;
	}

	return true;
}


inline bool tString::IsAlphaNumeric(bool includeUnderscore, bool includeDecimal) const
{
	// Doing them both in one loop.
	for (int n = 0; n < StringLength; n++)
	{
		char c = char(CodeUnits[n]);
		if ( !(tStd::tIsalnum(c) || (includeUnderscore && (c == '_')) || (includeDecimal && (c == '.'))) )
			return false;
	}

	return true;
}


inline int tString::CountChar(char c) const
{
	int count = 0;
	for (int i = 0; i < StringLength; i++)
		if (char(CodeUnits[i]) == c)
			count++;
	return count;
}


inline int tString::FindChar(const char c, bool reverse, int start) const
{
	const char8_t* pc = nullptr;

	if (start == -1)
	{
		if (reverse)
			start = Length() - 1;
		else
			start = 0;
	}

	if (reverse)
	{
		for (int i = start; i >= 0; i--)
			if (CodeUnits[i] == c)
			{
				pc = CodeUnits + i;
				break;
			}
	}
	else
	{
		for (int i = start; i < StringLength; i++)
			if (CodeUnits[i] == c)
			{
				pc = CodeUnits + i;
				break;
			}
	}

	if (!pc)
		return -1;

	// Returns the index.
	return int(pc - CodeUnits);
}


inline int tString::FindAny(const char* chars) const
{
	if (StringLength == 0)
		return -1;
	
	for (int i = 0; i < StringLength; i++)
	{
		char t = char(CodeUnits[i]);
		int j = 0;
		while (chars[j])
		{
			if (chars[j] == t)
				return i;
			j++;
		}
	}
	return -1;
}


inline int tString::FindString(const char8_t* str, int start) const
{
	if (IsEmpty())
		return -1;

	tAssert((start >= 0) && (start < StringLength));
	const char8_t* found = tStd::tStrstr(&CodeUnits[start], str);
	if (found)
		return int(found - CodeUnits);

	return -1;
}


inline int tString::Replace(const char search, const char replace)
{
	int numReplaced = 0;
	for (int i = 0; i < StringLength; i++)
	{
		if (CodeUnits[i] == search)
		{
			numReplaced++;
			CodeUnits[i] = replace;
		}
	}

	return numReplaced;
}


inline void tString::SetLength(int length, bool preserve)
{
	tAssert(length >= 0);		
	if (length > CurrCapacity)
		UpdateCapacity(length, preserve);
	
	// If new length is bigger, pad with zeros. The UpdateCapacity call will NOT modify the string length when preserve is true.
	if (preserve && (length > StringLength))
		tStd::tMemset(CodeUnits+StringLength, 0, length - StringLength);
	StringLength = length;
	CodeUnits[StringLength] = '\0';
}


inline tStringUTF16::tStringUTF16(int length)
{
	if (length <= 0)
		return;

	CodeUnits = new char16_t[1+length];
	tStd::tMemset(CodeUnits, 0, 2*(1+length));
	StringLength = length;
}


inline void tStringUTF16::Set(const tString& src)
{
	Clear();
	if (src.IsValid())
		Set(src.Units(), src.Length());
}


inline void tStringUTF16::Set(const tStringUTF16& src)
{
	Clear();
	if (src.IsValid())
		Set(src.CodeUnits, src.StringLength);
}


inline void tStringUTF16::Set(const tStringUTF32& src)
{
	Clear();
	if (src.IsValid())
		Set(src.Units(), src.Length());
}


inline void tStringUTF16::Set(const char8_t* src)
{
	int lenSrc = src ? tStd::tStrlen(src) : 0;
	Set(src, lenSrc);
}


inline void tStringUTF16::Set(const char16_t* src)
{
	int lenSrc = src ? tStd::tStrlen(src) : 0;
	Set(src, lenSrc);
}


inline void tStringUTF16::Set(const char32_t* src)
{
	int lenSrc = src ? tStd::tStrlen(src) : 0;
	Set(src, lenSrc);
}


inline void tStringUTF16::Set(const char8_t* src, int lenSrc)
{
	Clear();
	if (!src || (lenSrc <= 0))
		return;

	int len16 = tStd::tUTF16(nullptr, src, lenSrc);
	CodeUnits = new char16_t[len16+1];
	StringLength = tStd::tUTF16(CodeUnits, src, lenSrc);
	tAssert(StringLength == len16);
	CodeUnits[StringLength] = 0;
}


inline void tStringUTF16::Set(const char16_t* src, int lenSrc)
{
	Clear();
	if (!src || (lenSrc <= 0))
		return;

	CodeUnits = new char16_t[lenSrc+1];
	for (int cu = 0; cu < lenSrc; cu++)
		CodeUnits[cu] = src[cu];
	StringLength = lenSrc;
	CodeUnits[StringLength] = 0;
}


inline void tStringUTF16::Set(const char32_t* src, int lenSrc)
{
	Clear();
	if (!src || (lenSrc <= 0))
		return;

	int len16 = tStd::tUTF16(nullptr, src, lenSrc);
	CodeUnits = new char16_t[len16+1];
	StringLength = tStd::tUTF16(CodeUnits, src, lenSrc);
	tAssert(StringLength == len16);
	CodeUnits[StringLength] = 0;
}


inline void tStringUTF16::SetLength(int length, bool preserve)
{
	tAssert(length >= 0);
	if (length == StringLength)
		return;

	if (length == 0)
	{
		delete[] CodeUnits;
		CodeUnits = nullptr;
		StringLength = 0;
		return;
	}
	
	// If new length is bigger, pad with zeros. The UpdateCapacity call will NOT modify the string length when preserve is true.
	if (length > StringLength)
	{
		char16_t* newUnits = new char16_t[length+1];
		if (preserve)
		{
			tStd::tMemcpy(newUnits, CodeUnits, StringLength*sizeof(char16_t));
			tStd::tMemset(newUnits + StringLength, 0, (length - StringLength)*sizeof(char16_t));
		}
		delete[] CodeUnits;
		CodeUnits = newUnits;
	}
	StringLength = length;
	CodeUnits[StringLength] = 0;
}


inline tStringUTF32::tStringUTF32(int length)
{
	if (length <= 0)
		return;

	CodeUnits = new char32_t[1+length];
	tStd::tMemset(CodeUnits, 0, 4*(1+length));
	StringLength = length;
}


inline void tStringUTF32::Set(const tString& src)
{
	Clear();
	if (src.IsValid())
		Set(src.Units(), src.Length());
}


inline void tStringUTF32::Set(const tStringUTF16& src)
{
	Clear();
	if (src.IsValid())
		Set(src.Units(), src.Length());
}


inline void tStringUTF32::Set(const tStringUTF32& src)
{
	Clear();
	if (src.IsValid())
		Set(CodeUnits, StringLength);
}


inline void tStringUTF32::Set(const char8_t* src)
{
	int lenSrc = src ? tStd::tStrlen(src) : 0;
	Set(src, lenSrc);
}


inline void tStringUTF32::Set(const char16_t* src)
{
	int lenSrc = src ? tStd::tStrlen(src) : 0;
	Set(src, lenSrc);
}


inline void tStringUTF32::Set(const char32_t* src)
{
	int lenSrc = src ? tStd::tStrlen(src) : 0;
	Set(src, lenSrc);
}


inline void tStringUTF32::Set(const char8_t* src, int lenSrc)
{
	Clear();
	if (!src || (lenSrc <= 0))
		return;

	int len32 = tStd::tUTF32(nullptr, src, lenSrc);
	CodeUnits = new char32_t[len32+1];
	StringLength = tStd::tUTF32(CodeUnits, src, lenSrc);
	CodeUnits[lenSrc] = 0;
	StringLength = len32;
	tAssert(StringLength == len32);
	CodeUnits[StringLength] = 0;
}


inline void tStringUTF32::Set(const char16_t* src, int lenSrc)
{
	Clear();
	if (!src || (lenSrc <= 0))
		return;

	int len32 = tStd::tUTF32(nullptr, src, lenSrc);
	CodeUnits = new char32_t[len32+1];
	StringLength = tStd::tUTF32(CodeUnits, src, lenSrc);
	CodeUnits[lenSrc] = 0;
	StringLength = len32;
	tAssert(StringLength == len32);
	CodeUnits[StringLength] = 0;
}


inline void tStringUTF32::Set(const char32_t* src, int lenSrc)
{
	Clear();
	if (!src || (lenSrc <= 0))
		return;

	CodeUnits = new char32_t[lenSrc+1];
	for (int cu = 0; cu < lenSrc; cu++)
		CodeUnits[cu] = src[cu];
	StringLength = lenSrc;
	CodeUnits[StringLength] = 0;
}


inline void tStringUTF32::SetLength(int length, bool preserve)
{
	tAssert(length >= 0);
	if (length == StringLength)
		return;

	if (length == 0)
	{
		delete[] CodeUnits;
		CodeUnits = nullptr;
		StringLength = 0;
		return;
	}
	
	// If new length is bigger, pad with zeros. The UpdateCapacity call will NOT modify the string length when preserve is true.
	if (length > StringLength)
	{
		char32_t* newUnits = new char32_t[length+1];
		if (preserve)
		{
			tStd::tMemcpy(newUnits, CodeUnits, StringLength*sizeof(char32_t));
			tStd::tMemset(newUnits + StringLength, 0, (length - StringLength)*sizeof(char32_t));
		}
		delete[] CodeUnits;
		CodeUnits = newUnits;
	}
	StringLength = length;
	CodeUnits[StringLength] = 0;
}


inline tStringItem& tStringItem::operator=(const tStringItem& src)
{
	if (this == &src)
		return *this;

	UpdateCapacity(src.Length(), false);
	StringLength = src.Length();

	// The +1 gets us the internal null terminator in a single memcpy and also works with a StringLength of 0.
	tStd::tMemcpy(CodeUnits, src.CodeUnits, StringLength+1);
	return *this;
}


#endif