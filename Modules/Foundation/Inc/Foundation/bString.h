// bString.h
//
// bString is a simple and readable string class that implements sensible operators and implicit casts. The text in a
// bString is considerd to be UTF-8 encoded. With UTF-8 encoding each character (code-point) may be encoded by 1 or more
// code-units (a code-unit is 8 bits). The char8_t is used to repreresent a code-unit (as the C++ standard encourages).
//
// Externally a bString should be thought of as an array of code-units which may contain multiple null characters. A
// valid string of length 5 could be "ab\0\0e" for example. Internally a bString is null-terminated, but that is for
// implementational efficiency only -- many external functions require null-terminated strings, so it makes it easy to
// return one if the internal representation already has a null terminator. For example the length-5 string "abcde" is
// stored internally as 'a' 'b' 'c' 'd' 'e' '\0'.
//
// It can be inefficient (in time) to only maintain the exact amount of memory needed to store a particular string -- it
// would require a new memory allocation every time a string changes size. For this reason bStrings have a 'capacity'.
// The capacity of a bString is the number of code-units that can be stored without requiring additional memory
// management calls. For example, a bString with capacity 10 could be storing "abcde". If you were to add "fghij" to the
// string, it would be done without any delete[] or new calls. Note that internally a bString of capacity 10 actually
// has malloced an array of 11 code-units, the 11th one being for the terminating null. Functions that affect capacity
// do not change the behaviour of a bString and are always safe, they simply affect the efficiency.
//
// When the bString does need to grow its capacity (perhaps another string is being added/appended to it) there is the
// question of how much extra space to reserve. The SetGrowMethod may be used to set how much extra space is reserved
// when a memory-size-changing operation takes place. By default a constant amount of extra memory is reserved.
//
// A few of the salient functions related to the above are:
// Lenght		:	Returns how many code-units are used by the string. This is NOT like a strlen call as it does not
//					rely on nul-termination. It does not need to iterate as the length is stored explicitely.
// Capacity		:	Returns the current capacity of the bString in code-units.
// Reserve		:	This is instead of a SetCapacity call. There is no SetCapacity as we could not guarantee that a
//					requested capacity is non-destructive. Calling Reserve(5) on a string of Length 10 will not result
//					in a new capacity of 5 because it would require culling half of the code-units. Reserve can also be
//					used to shrink (release memory) if possible. See the comments before the function itself.
// SetGrowMethod:	Controls how much extra space (Capacity - Length) to reserve when performing a memory operation.
//
//

// For conversions of arbitrary types to bStrings, see tsPrint in the higher level System module.
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

#pragma once
#include "Foundation/tStandard.h"
#include "Foundation/tList.h"
struct tStringUTF16;
struct tStringUTF32;


// THIS CLASS IS WIP.
struct bString
{
	bString();

	// Expects src to be null-terminated.
	bString(const char8_t* src);

	// You can create a UTF-8 bString from a null-terminated ASCII string no problem. All ASCII strings are valid UTF-8.
	bString(const char* src);

	// Constructs from a code-unit (or ASCII char) array of size n. Src may have multiple nulls in it.
	bString(const char8_t* src, int n);
	bString(const char* src, int n);

	bString(const bString& src);
	bString(const char16_t* src);
	bString(const char32_t* src);
	bString(const tStringUTF16& src);
	bString(const tStringUTF32& src);

	// Construct a string of length null characters.
	explicit bString(int length);

	// Note the char here. A char8_t can't be guaranteed to store a unicode codepoint if the codepoint requires
	// continuations in the UTF-8 encoding. So, here we support char only which we use for ASCII characters. These chars
	// are guaranteed to _not_ need continuation units in UFT-8.
	bString(char);

	virtual ~bString();

	void Set(const char8_t* src);
	void Set(const char* src);//																								{ Set((const char8_t*)s); }
	void Set(const char8_t* src, int n);
	void Set(const char* src, int n);
	void Set(const bString& src);
	void Set(const char16_t* src)																						{ SetUTF16(src); }
	void Set(const char32_t* src)																						{ SetUTF32(src); }

	// WIP
	void Set(const tStringUTF16& src);
	void Set(const tStringUTF32& src);
	void Set(int length);
	void Set(char);

	// The length in char8_t's (code-units), not the display length (which is not that useful).
	// This length has nothing to do with how many null characters are in the string or where the are.
	int Length() const;

	// Does not release memory. Simply clears the string. Fast.
	void Clear()	{ StringLength = 0; CodeUnits[0] = '\0'; }

	int Capacity() const;

	#if 0

	tString& operator=(const tString&);

	bool IsEqual(const tString& s) const																				{ return !tStd::tStrcmp(CodeUnits, s.CodeUnits); }
	bool IsEqual(const char8_t* s) const																				{ if (!s) return false; return !tStd::tStrcmp(CodeUnits, s); }
	bool IsEqual(const char* s) const																					{ if (!s) return false; return !tStd::tStrcmp(CodeUnits, (char8_t*)s); }
	bool IsEqualCI(const tString& s) const																				{ return !tStd::tStricmp(CodeUnits, s.CodeUnits); }
	bool IsEqualCI(const char8_t* s) const																				{ if (!s) return false; return !tStd::tStricmp(CodeUnits, s); }
	bool IsEqualCI(const char* s) const																					{ if (!s) return false; return !tStd::tStricmp(CodeUnits, (char8_t*)s); }

	// These allow for implicit conversion to a UTF-8 code-unit pointer. By not including implicit casts to const char*
	// we are encouraging further proper use of char8_t. You can either make the function you are calling take the
	// proper UTF-* type, or explicitly call Chr() or Txt() to get an old char-based pointer.
	operator const char8_t*()																							{ return CodeUnits; }
	operator const char8_t*() const																						{ return CodeUnits; }

	explicit operator uint32();
	explicit operator uint32() const;

	char8_t& operator[](int i)		/* This may be somewhat meaningless if continuations needed at the index. */		{ return CodeUnits[i]; }
	friend tString operator+(const tString& prefix, const tString& suffix);
	tString& operator+=(const tString&);


	int Length() const				/* The length in char8_t's, not the display length (which is not that useful). */	{ return int(tStd::tStrlen(CodeUnits)); }
	bool IsEmpty() const																								{ return (CodeUnits == &EmptyChar) || !tStd::tStrlen(CodeUnits); }
	bool IsValid() const			/* returns true is string is not empty. */											{ return !IsEmpty(); }

	bool IsAlphabetic(bool includeUnderscore = true) const;
	bool IsNumeric(bool includeDecimal = false) const;
	bool IsAlphaNumeric(bool includeUnderscore = true, bool includeDecimal = false) const;

	// Current string data is lost and enough space is reserved for length characters. The reserved memory can be zeroed.
	void Reserve(int length, bool zeroMemory = true);

	// These only work well for ASCII strings as vars like 'count' are indexes into the text data and are not
	// 'continuation-aware'. This comment applies to all below functions with the words 'Left', 'Right', and 'Mid' in
	// them except for functions that take in a char8_t* or char* prefix or suffix. Those work for ASCII and UTF-8..
	tString Left(const char marker = ' ') const;			// Returns a tString of the characters before the first marker. Returns the entire string if marker was not found.
	tString Right(const char marker = ' ') const;			// Same as Left but chars after last marker.
	tString Left(int count) const;							// Returns a tString of the first count chars. Return what's available if count > length.
	tString Right(int count) const;							// Same as Left but returns last count chars.
	tString Mid(int start, int count) const;				// Returns count chars from start (inclusive), or what's available if start+count > length.

	// Extracts first word up to and not including first divider encountered. The tString is left with the remainder,
	// not including the divider. If divider isn't found, the entire string is returned and the tString is left empty.
	tString ExtractLeft(const char divider = ' ');

	// Extracts word after last divider. The tString is left with the remainder, not including the divider. If the
	// divider isn't found, the entire string is returned and the tString is left empty.
	tString ExtractRight(const char divider = ' ');

	// Returns a tString of the first count chars. Removes these from the current string. If count > length then what's
	// available is extracted.
	tString ExtractLeft(int count);

	// Returns a tString of the last count chars. Removes these from the current string. If count > length then what's
	// available is extracted.
	tString ExtractRight(int count);

	// If this string starts with prefix, removes and returns it. If not, returns empty string and no modification.
	tString ExtractLeft(const char* prefix);
	tString ExtractLeft(const char8_t* prefix)																			{ return ExtractLeft((const char*)prefix); }

	// If this string ends with (UTF-8) suffix, removes and returns it. If not, returns empty string and no modification.
	tString ExtractRight(const char* suffix);
	tString ExtractRight(const char8_t* suffix)																			{ return ExtractRight((const char*)suffix); }

	// Returns chars from start to count, but also removes that from the tString.  If start + count > length then what's
	// available is extracted.
	tString ExtractMid(int start, int count);

	// Accesses the raw UTF-8 codeunits represented by the 'official' unsigned UTF-8 character datatype char8_t.
	char8_t* Text()																										{ return CodeUnits; }
	const char8_t* Chars() const																						{ return CodeUnits; }
	const char8_t* Charz() const	/* Like Chars() but returns nullptr if the string is empty, not a pointer to "". */	{ return IsEmpty() ? nullptr : CodeUnits; }
	char8_t* Units() const			/* Same as Text but uses unicode naming, Code Units (that make the Code Points. */	{ return CodeUnits; }

	// Many other functions and libraries that are UTF-8 compliant do not yet (and may never) use the proper char8_t
	// type and use char* and const char*. These functions allow you to retrieve the tString using the char type.
	// Use these with tPrintf and %s.
	#endif
	char* Txt()																											{ return (char*)CodeUnits; }
	const char* Chr() const																								{ return (const char*)CodeUnits; }
	#if 0
	const char* Chz() const			/* Like Chr() but returns nullptr if the string is empty, not a pointer to "". */	{ return IsEmpty() ? nullptr : (const char*)CodeUnits; }
	char* Pod() const				/* Plain Old Data */																{ return (char*)CodeUnits; }

	// Returns index of first/last occurrence of char in the string. -1 if not found. Finds last if backwards flag is
	// set. The starting point may be specified. If backwards is false, the search proceeds forwards from the starting
	// point. If backwards is true, it proceeds backwards. If startIndex is -1, 0 is the starting point for a forward
	// search and length-1 is the starting point for a backwards search. Here is where UTF-8 is really cool, since
	// ASCII bytes do not occur when encoding non-ASCII code-points into UTF-8, this function can still just do a linear
	// search of all the characters. Pretty neat. What you can't do with this function is search for a codepoint that
	// requires continuation bytes in UTF-8. i.e. Since the input is a const char, char must be ASCII.
	//
	// @todo I like the idea of supporting UTF searches for particular codepoints etc by inputting the UTF-32
	// representation (using a char32_t) where necessary -- we'd just need to decode each codepoint in UTF-8 to the
	// proper char32_t and use that. It would all just work (but it's a big-ish task).
	int FindChar(const char, bool backwards = false, int startIndex = -1) const;

	// Returns the index of the first character in the tString that is also somewhere in the null-terminated string
	// searchChars. Returns -1 if none of them match.
	int FindAny(const char* searchChars) const;

	// Returns index of first character of the string s in the string. Returns -1 if not found.
	// It is valid to perform this for ASCII strings as well so the function is overridden for const char*.
	int FindString(const char8_t* s, int startIndex = 0) const;
	int FindString(const char* s, int startIndex = 0) const																{ return FindString((const char8_t*)s, startIndex); }

	// Replace all occurrences of character c with character r. Returns number of characters replaced. ASCII-only.
	int Replace(const char c, const char r);

	// Replace all occurrences of string search with string replace. Returns the number of replacements. The replacement
	// is done in a forward direction. If replace is a different size than search, memory will be managed to accomadate
	// the larger or smaller resulting string and keep the memory footprint as small as possible. If they are the same
	// size, the function is faster and doesn't need to mess with memory. If replace is "" or 0, all occurrences of
	// search will be removed (replaced by nothing).
	// It is valid to perform this for ASCII strings as well so the function is overridden for const char*.
	int Replace(const char8_t* search, const char8_t* replace);
	int Replace(const char* search, const char* replace)																{ return Replace((const char8_t*)search, (const char8_t*)replace); }

	// Remove all occurrences of the character rem. Returns the number of characters removed.
	int Remove(const char rem);

	// Removing a string simply calls Replace with a null second string. Returns how many rem strings were removed.
	int Remove(const char8_t* rem)																						{ return Replace(rem, nullptr); }
	int Remove(const char* rem)																							{ return Remove((const char8_t*)rem); }

	// Removes all leading characters in this string that match any of the characters in the null-erminated theseChars
	// eg. Calling RemoveLeading on "cbbabZING" with "abc" yields "ZING". Returns the number of characters removed.
	// Note that theseChars are ASCII.
	int RemoveLeading(const char* theseChars);

	// Removes all trailing characters in this string that match any of the characters in the null-erminated theseChars
	// eg. Calling RemoveTrailing on "ZINGabcaab" with "abc" yields "ZING". Returns the number of characters removed.
	// Note that theseChars are ASCII.
	int RemoveTrailing(const char* theseChars);

	int CountChar(char c) const;							// Counts the number of occurrences of c.

	// ToUpper and ToLower both modify the object as well as return a reference to it. Returning a reference makes it
	// easy to string together expressions such as: if (name.ToLower() == "ah")
	tString& ToUpper()																									{ tStd::tStrupr(CodeUnits); return *this; }
	tString& ToLower()																									{ tStd::tStrlwr(CodeUnits); return *this; }

	// These do not modify the string. They return a new one.
	tString Upper() const																								{ tString s(*this); s.ToUpper(); return s; }
	tString Lower() const																								{ tString s(*this); s.ToLower(); return s; }

	// See comment for tStrtoiT in tStandard.h for format requirements. The summary is that if base is -1, the function
	// looks one of the following prefixes in the string, defaulting to base 10 if none found.
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

	float GetAsFloat() const									/* Base 10 interpretation only. */						{ return tStd::tStrtof(CodeUnits); }
	double GetAsDouble() const									/* Base 10 interpretation only. */						{ return tStd::tStrtod(CodeUnits); }

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

	// tString UTF encoding/decoding functions. tString is encoded in UTF-8. These functions allow you to convert from
	// tString to UTF-16/32. If dst is nullptr returns the number of charNs needed. If incNullTerminator is false that
	// number needed will be one fewer. If dst is valid, writes the codeunits to dst and returns number charNs written.
	int GetUTF16(char16_t* dst, bool incNullTerminator = true) const;
	int GetUTF32(char32_t* dst, bool incNullTerminator = true) const;

#endif
	// Sets the bString from a UTF codeunit array. If srcLen is -1 assumes supplied array is null-terminated, otherwise
	// specify how long it is. Returns new length (not including null terminator) of the bString.
	int SetUTF16(const char16_t* src, int srcLen = -1);
	int SetUTF32(const char32_t* src, int srcLen = -1);

protected:
	// This could be made to be dynamic. Just didn't want to waste 4 bytes for every bString instance.
	const int MinCapacity			= 15;

	// If GrowParam is positive, it represents how many extra code-units to grow by when out of capacity.
	// If GrowParam is negative, its absolute value represents how many times bigger the capacity should be
	// compared to the required lenght of the string.
	// If GrowParam is zero, everthing still works, you just don't get the extra code-units so it's less efficient.
	int GrowParam					= 64;

	// The length of the bString currently used in code-units.
	int StringLength				= 0;

	// The capacity. The number of allocated CodeUnuts is always one more than this.
	int CurrCapacity				= 0;

	// By using the char8_t we are indicating the data is stored in UTF-8 encoding. Note that unlike char, a char8_t
	// is guaranteed to be unsigned, as well as a distinct type. In unicode spec for UTFn, these are called code-units.
	// With bStrings the CodeUnits pointer is never nullptr after construction. There is always some capacity.
	char8_t* CodeUnits				= nullptr;

private:

	// The basic idea behind this function is you ask it for a specific amount of room that you know you will need -- to
	// do say an append operaion. It guarantees that the capacity afterwards will be at least as big as what you requested.
	//
	// It makes sure CurrCapacity is at least as big as capNeeded. If it already is, it does nothing. If it
	// is not, it updates CurrCapacity (and the CodeUnits) to have enough room plus whatever extra is dictated by the
	// GrowParam.
	//
	// In some cases you care if the original string is preserved (eg. For an append) and in some cases you do not
	// (eg. For a Set call, the old contents are cleared). If you don't need the string preserved, call this with
	// preserve = false. It will save a memcpy (and the string length will be 0 afterwards).
	// If the string is empty, it doesn't really matter what preserve is set to.
	//
	// If called with preserve true the function is nondestructive and expects StringLength to be set correctly and be
	// the current length, not the potential future length (do not modify StringLength first). It is illegal to call
	// with preserve true and a capNeeded that is less than the StringLength.
	//
	// This function respects MinCapactity. If capNeeded + (possible grow amount) is under MinCapacity,
	// MinCapacity will be used instead. Calling with capNeeded = 0 is special, it will not add any extra grow amount.
	// This results in MinCapacity being used. When calling with 0 you still need to meet the StringLenghth requirement if
	// preserve is true (i.e. StringLength would need to be 0).
	//
	// This function never shrinks the capacity.
	void UpdateCapacity(int capNeeded, bool preserve);
};


// Implementation below this line.


inline bString::bString()
{
	UpdateCapacity(0, false);
}


inline bString::bString(const char8_t* src)
{
	Set(src);
}


inline bString::bString(const char* src)
{
	Set(src);
}


inline bString::bString(const char8_t* src, int n)
{
	Set(src, n);
}


inline bString::bString(const char* src, int n)
{
	Set(src, n);
}


inline bString::bString(const bString& src)
{
	Set(src);
}


inline bString::bString(const char16_t* src)
{
	Set(src);
}


inline bString::bString(const char32_t* src)
{
	Set(src);
}


inline bString::bString(const tStringUTF16& src)
{
	// WIP Requires changes to tStringUTF16 to support multiple nulls.
///////	Set(src);
}


inline bString::bString(const tStringUTF32& src)
{
	// WIP Requires changes to tStringUTF32 to support multiple nulls.
////////	Set(src);
}


inline bString::bString(int length)
{
	tAssert(length >= 0);
	StringLength = length;
	UpdateCapacity(StringLength, false);
	tStd::tMemset(CodeUnits, 0, StringLength+1);
}


inline bString::bString(char c)
{
	UpdateCapacity(1, false);
	StringLength = 1;
	CodeUnits[0] = c;
	CodeUnits[1] = '\0';
}


inline bString::~bString()
{
	delete[] CodeUnits;
}


inline void bString::Set(const char8_t* src)
{
	int n = src ? tStd::tStrlen(src) : 0;
	UpdateCapacity(n, false);
	if (n > 0)
	{
		tStd::tMemcpy(CodeUnits, src, n);
		CodeUnits[n] = '\0';
		StringLength = n;
	}
}


inline void bString::Set(const char* src)
{
	Set((const char8_t*)src);
}


inline void bString::Set(const char8_t* src, int n)
{
	if (!src || (n < 0))
		n = 0;
	UpdateCapacity(n, false);
	if (n > 0)
		tStd::tMemcpy(CodeUnits, src, n);
	CodeUnits[n] = '\0';
	StringLength = n;
}


inline void bString::Set(const char* src, int n)
{
	Set((const char8_t*)src, n);
}


inline void bString::Set(const bString& src)
{
	int len = src.Length();
	UpdateCapacity(len, false);

	StringLength = len;
	tStd::tMemcpy(CodeUnits, src.CodeUnits, StringLength);
	CodeUnits[StringLength] = '\0';
}


inline int bString::Length() const
{
	return StringLength;
}


inline int bString::Capacity() const
{
	return CurrCapacity;
}


inline void bString::UpdateCapacity(int capNeeded, bool preserve)
{
	int grow = 0;
	if (capNeeded > 0)
		grow = (GrowParam >= 0) ? GrowParam : capNeeded*(-GrowParam);

	capNeeded += grow;
	if (capNeeded < MinCapacity)
		capNeeded = MinCapacity;

	if (CurrCapacity >= capNeeded)
	{
		StringLength = 0;
		CodeUnits[0] = '\0';
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

#if 0

inline void tString::Reserve(int length, bool zeroMemory)
{
	if (CodeUnits != &EmptyChar)
		delete[] CodeUnits;

	if (length <= 0)
	{
		CodeUnits = &EmptyChar;
		return;
	}

	CodeUnits = new char8_t[length+1];
	if (zeroMemory)
		tStd::tMemset(CodeUnits, 0, length+1);
}


inline int tString::CountChar(char c) const
{
	char8_t* i = CodeUnits;
	int count = 0;
	while (*i != '\0')
		count += (*i++ == c) ? 1 : 0;

	return count;
}


inline void tString::Set(const tStringUTF16& src)
{
	Set(src.Units());
}


inline void tString::Set(const tStringUTF32& src)
{
	Set(src.Units());
}


inline tString& tString::operator=(const tString& src)
{
	if (this == &src)
		return *this;

	if (CodeUnits != &EmptyChar)
		delete[] CodeUnits;

	CodeUnits = new char8_t[1 + src.Length()];
	tStd::tStrcpy(CodeUnits, src.CodeUnits);
	return *this;
}


inline tString operator+(const tString& preStr, const tString& sufStr)
{
	tString buf( preStr.Length() + sufStr.Length() );
	tStd::tStrcpy(buf.CodeUnits, preStr.CodeUnits);
	tStd::tStrcpy(buf.CodeUnits + preStr.Length(), sufStr.CodeUnits);

	return buf;
}


inline tString& tString::operator+=(const tString& sufStr)
{
	if (sufStr.IsEmpty())
		return *this;
	else
	{
		char8_t* newCodeUnits = new char8_t[ Length() + sufStr.Length() + 1 ];
		tStd::tStrcpy(newCodeUnits, CodeUnits);
		tStd::tStrcpy(newCodeUnits + Length(), sufStr.CodeUnits);

		if (CodeUnits != &EmptyChar)
			delete[] CodeUnits;

		CodeUnits = newCodeUnits;
		return *this;
	}
}


inline bool tString::IsAlphabetic(bool includeUnderscore) const 
{
	if (CodeUnits == &EmptyChar)
		return false;

	const char8_t* c = CodeUnits;
	while (*c)
	{
		if ( !((*c >= 'A' && *c <= 'Z') || (*c >= 'a' && *c <= 'z') || (includeUnderscore && *c == '_')) )
			return false;
		c++;
	}

	return true;
}


inline bool tString::IsNumeric(bool includeDecimal) const 
{
	if (CodeUnits == &EmptyChar)
		return false;

	const char8_t* c = CodeUnits;
	while (*c)
	{
		if ( !((*c >= '0' && *c <= '9') || (includeDecimal && *c == '.')) )
			return false;
		c++;
	}

	return true;
}


inline bool tString::IsAlphaNumeric(bool includeUnderscore, bool includeDecimal) const
{
	return (IsAlphabetic(includeUnderscore) || IsNumeric(includeDecimal));
}


inline int tString::FindAny(const char* chars) const
{
	if (CodeUnits == &EmptyChar)
		return -1;
	
	int i = 0;
	while (CodeUnits[i])
	{
		int j = 0;
		while (chars[j])
		{
			if (chars[j] == CodeUnits[i])
				return i;
			j++;
		}
		i++;
	}
	return -1;
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
		pc = tStd::tStrchr(&CodeUnits[start], c);

	if (!pc)
		return -1;

	// Returns the index.
	return int(pc - CodeUnits);
}


inline int tString::FindString(const char8_t* s, int start) const
{
	int len = Length();
	if (!len)
		return -1;

	tAssert((start >= 0) && (start < Length()));
	const char8_t* found = tStd::tStrstr(&CodeUnits[start], s);
	if (found)
		return int(found - CodeUnits);

	return -1;
}


inline int tString::Replace(const char c, const char r)
{
	int numReplaced = 0;
	for (int i = 0; i < Length(); i++)
	{
		if (CodeUnits[i] == c)
		{
			numReplaced++;
			CodeUnits[i] = r;
		}
	}

	return numReplaced;
}


inline tStringUTF16::tStringUTF16(int length)
{
	if (!length)
	{
		CodeUnits = nullptr;
	}
	else
	{
		CodeUnits = new char16_t[1+length];
		tStd::tMemset(CodeUnits, 0, 2*(1+length));
	}
}


inline void tStringUTF16::Set(const char16_t* src)
{
	Clear();
	int len = src ? tStd::tStrlen(src) : 0;
	if (len)
	{
		CodeUnits = new char16_t[len+1];
		for (int cu = 0; cu < len; cu++)
			CodeUnits[cu] = src[cu];
		CodeUnits[len] = 0;
	}
}


inline void tStringUTF16::Set(const char8_t* src)
{
	Clear();
	int len = src ? tStd::tStrlen(src) : 0;
	if (len)
	{
		int len16 = tStd::tUTF16s(nullptr, src);
		CodeUnits = new char16_t[len16+1];
		tStd::tUTF16s(CodeUnits, src);
	}
}


inline void tStringUTF16::Set(const tStringUTF16& src)
{
	Clear();
	if (src.IsValid())
		Set(src.Chars());
}


inline void tStringUTF16::Set(const tString& src)
{
	Clear();
	if (src.IsValid())
		Set(src.Chars());
}


inline tStringUTF32::tStringUTF32(int length)
{
	if (!length)
	{
		CodeUnits = nullptr;
	}
	else
	{
		CodeUnits = new char32_t[1+length];
		tStd::tMemset(CodeUnits, 0, 4*(1+length));
	}
}


inline void tStringUTF32::Set(const char32_t* src)
{
	Clear();
	int len = src ? tStd::tStrlen(src) : 0;
	if (len)
	{
		CodeUnits = new char32_t[len+1];
		for (int cu = 0; cu < len; cu++)
			CodeUnits[cu] = src[cu];
		CodeUnits[len] = 0;
	}
}


inline void tStringUTF32::Set(const char8_t* src)
{
	Clear();
	int len = src ? tStd::tStrlen(src) : 0;
	if (len)
	{
		int len32 = tStd::tUTF32s(nullptr, src);
		CodeUnits = new char32_t[len32+1];
		tStd::tUTF32s(CodeUnits, src);
	}
}


inline void tStringUTF32::Set(const tStringUTF32& src)
{
	Clear();
	if (src.IsValid())
		Set(src.Chars());
}


inline void tStringUTF32::Set(const tString& src)
{
	Clear();
	if (src.IsValid())
		Set(src.Chars());
}


inline tStringItem& tStringItem::operator=(const tStringItem& src)
{
	if (this == &src)
		return *this;

	if (CodeUnits != &EmptyChar)
		delete[] CodeUnits;

	CodeUnits = new char8_t[1 + src.Length()];
	tStd::tStrcpy(CodeUnits, src.CodeUnits);
	return *this;
}
#endif
