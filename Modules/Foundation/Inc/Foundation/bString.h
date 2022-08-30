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
// (like Reserve) do not change the behaviour of a bString and are always safe, they simply affect the efficiency.
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
// Shrink		:	Shrinks the bString to the least amount of memory used possible. Like calling Reserver(Length());
// SetGrowMethod:	Controls how much extra space (Capacity - Length) to reserve when performing a memory operation.
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
#include "Foundation/tString.h"
struct tStringUTF16;
struct tStringUTF32;


// THIS CLASS IS WIP.
struct bString
{
	bString()																											{ UpdateCapacity(0, false); }
	bString(const bString& src)																							{ Set(src); }

	// Construct a string of length null characters.
	explicit bString(int length)																						{ Set(length); }

	// Creates a bString with a single character, Note the char type here. A char8_t can't be guaranteed to store a
	// unicode codepoint if the codepoint requires continuations in the UTF-8 encoding. So, here we support char only
	// which we use for ASCII characters since ASCII chars are guaranteed to _not_ need continuation units in UFT-8.
	bString(char c)																										{ Set(c); }

	// The constructors that don't take in a length expect the string pointers to be null-terminated.
	// The constructors that do take a length may contain multiple nulls in the src string.
	// You can create a UTF-8 bString from an ASCII string (char*) since all ASCII strings are valid UTF-8.
	// Constructors taking char8_t, char16_t, or chat32_t pointers assume the src is UTF encoded.
	bString(const char*				src)																				{ Set(src); }
	bString(const char8_t*			src)																				{ Set(src); }
	bString(const char16_t*			src)																				{ Set(src); }
	bString(const char32_t*			src)																				{ Set(src); }
	bString(const char*				src, int srcLen)																	{ Set(src, srcLen); }
	bString(const char8_t*			src, int srcLen)																	{ Set(src, srcLen); }
	bString(const char16_t*			src, int srcLen);
	bString(const char32_t*			src, int srcLen);

	// The tStringUTF constructors allow the src strings to have multiple nulls in them.
	bString(const tStringUTF16&		src)																				{ Set(src); }
	bString(const tStringUTF32&		src)																				{ Set(src); }
	virtual ~bString()																									{ delete[] CodeUnits; }

	void Set(const bString&			src);
	void Set(int length);
	void Set(char);
	void Set(const char*			src)																				{ Set((const char8_t*)src); }
	void Set(const char8_t*			src);
	void Set(const char16_t*		src)																				{ SetUTF16(src); }
	void Set(const char32_t*		src)																				{ SetUTF32(src); }
	void Set(const char*			src, int srcLen)																	{ Set((const char8_t*)src, srcLen); }
	void Set(const char8_t*			src, int srcLen);
	void Set(const char16_t*		src, int srcLen);
	void Set(const char32_t*		src, int srcLen);
	void Set(const tStringUTF16&	src)																				{ SetUTF16(src.Units(), src.Length()); }
	void Set(const tStringUTF32&	src)																				{ SetUTF32(src.Units(), src.Length()); }

	// Does not release memory. Simply sets the string to empty. Fast.
	void Clear()																										{ StringLength = 0; CodeUnits[0] = '\0'; }

	// The length in char8_t's (code-units), not the display length (which is not that useful).
	// This length has nothing to do with how many null characters are in the string or where the are.
	int Length() const																									{ return StringLength; }
	int Capacity() const																								{ return CurrCapacity; }

	// This function is for efficiency only. It does not modify the string contents. It simply makes sure the capacity
	// of the string is big enough to hold numUnits units (total). It returns the new capacity after any required
	// adjustments are made. The returned value will be >= Length(). You can also use this function to shrink the mem
	// used -- just call it with a value less than the current capacity. It won't be able to reduce it lower than the
	// current StringLength (or the MinCapacity) however.
	int Reserve(int numUnits);

	// Shrink releases as much memory as possible. It returns the new capacity after shrinking. Note that the new
	// capacity will be at least MinCapacity big. This basically calls Reserve(Length());
	int Shrink();

	// This is like Reserve except it takes in the number of _extra_ code-units you want. It will attempt to add or
	// subtract from the current capacity. Putting in a negative to shrink is supported. Again, it cannot shrink below
	// the current string length or lower than the minimum capacity. Returns the new capacity.
	int Grow(int numUnits)																								{ return Reserve(CurrCapacity + numUnits); }

	bool IsEmpty() const																								{ return (StringLength <= 0); }
	bool IsValid() const			/* Returns true is string is not empty. */											{ return !IsEmpty(); }

	bString& operator=(const bString&);

	// The IsEqual variants taking (only) pointers assume null-terminated inputs. Two empty strings are considered
	// equal. If the input is nullptr (for functions taking pointers) it is not considered equal to an empty string.
	// For variants taking pointers and a length, all characters are checked (multiple null chars supported).
	bool IsEqual(const bString&		str) const																			{ return IsEqual(str.CodeUnits, str.Length()); }
	bool IsEqual(const char*		str) const																			{ return IsEqual(str, str ? tStd::tStrlen(str) : 0); }
	bool IsEqual(const char8_t*		str) const																			{ return IsEqual(str, str ? tStd::tStrlen(str) : 0); }
	bool IsEqual(const char*		str, int strLen) const																{ return IsEqual((const char8_t*)str, strLen); }
	bool IsEqual(const char8_t*		str, int strLen) const;
	bool IsEqualCI(const bString&	str) const																			{ return IsEqualCI(str.CodeUnits, str.Length()); }
	bool IsEqualCI(const char*		str) const																			{ return IsEqualCI(str, str ? tStd::tStrlen(str) : 0); }
	bool IsEqualCI(const char8_t*	str) const																			{ return IsEqualCI(str, str ? tStd::tStrlen(str) : 0); }
	bool IsEqualCI(const char*		str, int strLen) const																{ return IsEqualCI((const char8_t*)str, strLen); }
	bool IsEqualCI(const char8_t*	str, int strLen) const;

	// These allow for implicit conversion to a UTF-8 code-unit pointer. By not including implicit casts to const char*
	// we are encouraging further proper use of char8_t. You can either make the function you are calling take the
	// proper UTF-* type, or explicitly call Chr() or Txt() to get an old char-based pointer.
	operator const char8_t*()																							{ return CodeUnits; }
	operator const char8_t*() const																						{ return CodeUnits; }
	char8_t& operator[](int i)		/* This may be somewhat meaningless if continuations needed at the index. */		{ return CodeUnits[i]; }

	explicit operator uint32();
	explicit operator uint32() const;

	friend bString operator+(const bString& prefix, const bString& suffix);
	bString& operator+=(const bString&);

	// All non-null characters must meet the criteria for these functions to return true.
	bool IsAlphabetic(bool includeUnderscore = true) const;
	bool IsNumeric(bool includeDecimal = false) const;
	bool IsAlphaNumeric(bool includeUnderscore = true, bool includeDecimal = false) const;

	// These only work well for ASCII strings as vars like 'count' are indexes into the text data and are not
	// 'continuation-aware'. This comment applies to all below functions with the words 'Left', 'Right', and 'Mid' in
	// them except for functions that take in a char8_t* or char* prefix or suffix. Those work for ASCII and UTF-8.
	//
	// Returns a bString of the characters before the first marker. Returns the entire string if marker was not found.
	// Think of left as excluding the marker and characters to the right, then returning the whole string makes sense.
	bString Left(const char marker = ' ') const;
	bString Right(const char marker = ' ') const;			// Same as Left but chars after last marker.

	bString Left(int count) const;							// Returns a bString of the first count chars. Return what's available if count > length.
	bString Mid(int start, int count) const;				// Returns count chars from start (inclusive), or what's available if start+count > length.
	bString Right(int count) const;							// Same as Left but returns last count chars.

	// Extracts first word up to and not including first divider encountered. The bString is left with the remainder,
	// not including the divider. If divider isn't found, the entire string is returned and the bString is left empty.
	bString ExtractLeft(const char divider = ' ');

	// Extracts word after last divider. The bString is left with the remainder, not including the divider. If the
	// divider isn't found, the entire string is returned and the bString is left empty.
	bString ExtractRight(const char divider = ' ');

	// Returns a bString of the first count chars. Removes these from the current string. If count > length then what's
	// available is extracted.
	bString ExtractLeft(int count);

	// Returns chars from start to count, but also removes that from the bString.  If start + count > length then what's
	// available is extracted.
	bString ExtractMid(int start, int count);

	// Returns a bString of the last count chars. Removes these from the current string. If count > length then what's
	// available is extracted.
	bString ExtractRight(int count);

	// If this string starts with prefix, removes and returns it. If not, returns empty string and no modification.
	// Prefix is assumed to be null-terminated.
	bString ExtractLeft(const char* prefix)																				{ return ExtractLeft((const char8_t*)prefix); }
	bString ExtractLeft(const char8_t* prefix);

	// If this string ends with suffix, removes and returns it. If not, returns empty string and no modification.
	// Suffix is assumed to be null-terminated.
	bString ExtractRight(const char* suffix)																			{ return ExtractRight((const char8_t*)suffix); }
	bString ExtractRight(const char8_t* suffix);

	// Accesses the raw UTF-8 codeunits represented by the 'official' unsigned UTF-8 character datatype char8_t.
	// Except for Charz nullptr is not returned although they may return a pointer to an empty string.
	char8_t* Text()																										{ return CodeUnits; }
	const char8_t* Chars() const																						{ return CodeUnits; }
	const char8_t* Charz() const	/* Like Chars() but returns nullptr if the string is empty, not a pointer to "". */	{ return IsEmpty() ? nullptr : CodeUnits; }
	char8_t* Units() const			/* Same as Text but uses unicode naming, Code Units (that make the Code Points. */	{ return CodeUnits; }

	// Many other functions and libraries that are UTF-8 compliant do not yet (and may never) use the proper char8_t
	// type and use char* and const char*. These functions allow you to retrieve the tString using the char type.
	// You can also use these with tPrintf and %s.
	char* Txt()																											{ return (char*)CodeUnits; }
	const char* Chr() const																								{ return (const char*)CodeUnits; }
	const char* Chz() const			/* Like Chr() but returns nullptr if the string is empty, not a pointer to "". */	{ return IsEmpty() ? nullptr : (const char*)CodeUnits; }
	char* Pod() const				/* Plain Old Data */																{ return (char*)CodeUnits; }

	// Counts the number of occurrences of c. Does not stop at first null. Iterates over the full StringLength.
	int CountChar(char c) const;

	// Returns index of first/last occurrence of char in the string. -1 if not found. Finds last if backwards flag is
	// set. The starting point may be specified. If backwards is false, the search proceeds forwards from the starting
	// point. If backwards is true, it proceeds backwards. If startIndex is -1, 0 is the starting point for a forward
	// search and length-1 is the starting point for a backwards search. Here is where UTF-8 is really cool, since
	// ASCII bytes do not occur when encoding non-ASCII code-points into UTF-8, this function can still just do a linear
	// search of all the characters. Pretty neat. What you can't do with this function is search for a codepoint that
	// requires continuation bytes in UTF-8. i.e. Since the input is a const char, char must be ASCII.
	//
	// @todo I like the idea of supporting UTF searches for particular codepoints etc by inputting the UTF-32
	// representation (using a char32_t) where necessary -- we'd just need to decode each codepoint (plus possible
	// continuations) in UTF-8 to the proper char32_t and use that. It would all just work (but it's a big-ish task).
	int FindChar(const char, bool backwards = false, int startIndex = -1) const;

	// Returns the index of the first character in the bString that is also somewhere in the null-terminated string
	// searchChars. Returns -1 if none of them match.
	int FindAny(const char* searchChars) const;

	// Returns index of first character of the string str in the string. Returns -1 if not found.
	// It is valid to perform this for ASCII strings as well so the function is overridden for const char*.
	// For both versions of FindString the src input is assumed to be null-terminated.
	int FindString(const char8_t* str, int startIndex = 0) const;
	int FindString(const char* str, int startIndex = 0) const															{ return FindString((const char8_t*)str, startIndex); }

	// Replace all occurrences of character 'search' with character 'replace'. Returns number of characters replaced.
	// ASCII-only. @todo Could make a variant that works with char8_t[4] and deals with UTF-8 continuations.
	int Replace(const char search, const char replace);

	// Replace all occurrences of string search with string replace. Returns the number of replacements. The replacement
	// is done in a forward direction. If replace is a larger size than search, memory may need to be managed to
	// accomadate the larger size if the capacity isn't big enough. If they are the same size, the function is faster
	// and doesn't need to mess with memory. If replace is "" or nullptr, all occurrences of search will be removed
	// (replaced by nothing). It is valid to perform this for ASCII strings as well as UTF-8 so the function is
	// overridden for const char*. The input strings are assumed to be null-terminated. Returns nomber of replacements.
	int Replace(const char8_t* search, const char8_t* replace);
	int Replace(const char* search, const char* replace)																{ return Replace((const char8_t*)search, (const char8_t*)replace); }

	// Remove all occurrences of the character rem. Returns the number of characters removed.
	int Remove(char rem);

	// Removing a string simply calls Replace with a null second string. Returns how many rem strings were removed.
	int Remove(const char8_t* rem)																						{ return Replace(rem, nullptr); }
	int Remove(const char* rem)																							{ return Remove((const char8_t*)rem); }

	////////////// WIP
	#if 0
	// Removes all leading characters in this string that match any of the characters in the null-erminated theseChars
	// eg. Calling RemoveLeading on "cbbabZING" with "abc" yields "ZING". Returns the number of characters removed.
	// Note that theseChars are ASCII.
	int RemoveLeading(const char* theseChars);

	// Removes all trailing characters in this string that match any of the characters in the null-erminated theseChars
	// eg. Calling RemoveTrailing on "ZINGabcaab" with "abc" yields "ZING". Returns the number of characters removed.
	// Note that theseChars are ASCII.
	int RemoveTrailing(const char* theseChars);
	#endif

	// ToUpper and ToLower both modify the object as well as return a reference to it. Returning a reference makes it
	// easy to string together expressions such as: if (name.ToLower() == "ah")
	bString& ToUpper()																									{ for (int n = 0; n < StringLength; n++) CodeUnits[n] = tStd::tToUpper(CodeUnits[n]); return *this; }
	bString& ToLower()																									{ for (int n = 0; n < StringLength; n++) CodeUnits[n] = tStd::tToLower(CodeUnits[n]); return *this; }

	// These do not modify the string. They return a new one.
	bString Upper() const																								{ bString s(*this); s.ToUpper(); return s; }
	bString Lower() const																								{ bString s(*this); s.ToLower(); return s; }

	// The GetAs functions consider the contents of the current bSstring up to the first null encountered. See comment
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

	// tString UTF encoding/decoding functions. tString is encoded in UTF-8. These functions allow you to convert from
	// tString to UTF-16/32 arrays. If dst is nullptr returns the number of charN codeunits needed. If incNullTerminator
	// is false the number needed will be one fower. If dst is valid, writes the codeunits to dst and returns number
	// of charN codeunits written.
	int GetUTF16(char16_t* dst, bool incNullTerminator = true) const;
	int GetUTF32(char32_t* dst, bool incNullTerminator = true) const;

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
	// This function never shrinks the capacity. Use Reserve, Shrink, or Grow (with negative input) for that.
	void UpdateCapacity(int capNeeded, bool preserve);
};


// Implementation below this line.


inline void bString::Set(const bString& src)
{
	int srcLen = src.Length();
	UpdateCapacity(srcLen, false);

	StringLength = srcLen;
	tStd::tMemcpy(CodeUnits, src.CodeUnits, StringLength);
	CodeUnits[StringLength] = '\0';
}


inline void bString::Set(int length)
{
	tAssert(length >= 0);
	UpdateCapacity(length, false);
	tStd::tMemset(CodeUnits, 0, StringLength+1);
	StringLength = length;
}


inline void bString::Set(char c)
{
	UpdateCapacity(1, false);
	CodeUnits[0] = c;
	CodeUnits[1] = '\0';
	StringLength = 1;
}


inline void bString::Set(const char8_t* src)
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


inline void bString::Set(const char8_t* src, int srcLen)
{
	if (!src || (srcLen < 0))
		srcLen = 0;
	UpdateCapacity(srcLen, false);
	if (srcLen > 0)
		tStd::tMemcpy(CodeUnits, src, srcLen);
	CodeUnits[srcLen] = '\0';
	StringLength = srcLen;
}


inline void bString::Set(const char16_t* src, int srcLen)
{
	if (srcLen <= 0)
	{
		Clear();
		return;
	}
	SetUTF16(src, srcLen);
}


inline void bString::Set(const char32_t* src, int srcLen)
{
	if (srcLen <= 0)
	{
		Clear();
		return;
	}
	SetUTF32(src, srcLen);
}


inline int bString::Reserve(int numUnits)
{
	if (numUnits < StringLength)
		numUnits = StringLength;
	if (numUnits < MinCapacity)
		numUnits = MinCapacity;
	if (numUnits == CurrCapacity)
		return CurrCapacity;

	// The plus one is so we can do the null-terminator in the memcpy. It also allows it to work if the string length is 0.
	char8_t* newUnits = new char8_t[numUnits+1];
	tStd::tMemcpy(newUnits, CodeUnits, StringLength+1);
	delete[] CodeUnits;
	CodeUnits = newUnits;
	CurrCapacity = numUnits;

	return CurrCapacity;
}


inline int bString::Shrink()
{
	if ((StringLength == CurrCapacity) || (CurrCapacity == MinCapacity))
		return CurrCapacity;

	tAssert(StringLength < CurrCapacity);
	return Reserve(StringLength);
}


inline bString& bString::operator=(const bString& src)
{
	if (this == &src)
		return *this;

	int srcLen = src.Length();
	UpdateCapacity(srcLen, false);
	if (srcLen > 0)
		tStd::tMemcpy(CodeUnits, src.CodeUnits, srcLen);
	CodeUnits[srcLen] = '\0';
	StringLength = srcLen;
	return *this;
}


inline bool bString::IsEqual(const char8_t* str, int strLen) const
{
	if (!str || (Length() != strLen))
		return false;

	// We also compare the null so that we can compare strings of length 0.
	return !tStd::tMemcmp(CodeUnits, str, strLen+1);
}


inline bool bString::IsEqualCI(const char8_t* str, int strLen) const
{
	if (!str || (Length() != strLen))
		return false;

	for (int n = 0; n < strLen; n++)
		if (tStd::tToLower(CodeUnits[n]) != tStd::tToLower(str[n]))
			return false;

	return true;
}


inline bString operator+(const bString& preStr, const bString& sufStr)
{
	bString buf( preStr.Length() + sufStr.Length() );

	tStd::tMemcpy(buf.CodeUnits, preStr.CodeUnits, preStr.Length());
	tStd::tMemcpy(buf.CodeUnits + preStr.Length(), sufStr.CodeUnits, sufStr.Length());
	return buf;
}


inline bString& bString::operator+=(const bString& sufStr)
{
	if (sufStr.IsEmpty())
		return *this;

	int oldLen = Length();
	int newLen = oldLen + sufStr.Length();
	UpdateCapacity(newLen, true);

	// The plus one is so we get the terminating null with the memcpy.
	tStd::tMemcpy(CodeUnits + oldLen, sufStr.CodeUnits, sufStr.Length()+1);
	StringLength = newLen;
	return *this;
}


inline bool bString::IsAlphabetic(bool includeUnderscore) const 
{
	for (int n = 0; n < StringLength; n++)
	{
		char c = char(CodeUnits[n]);
		if ( !(tStd::tIsalpha(c) || (includeUnderscore && (c == '_'))) )
			return false;
	}

	return true;
}


inline bool bString::IsNumeric(bool includeDecimal) const 
{
	for (int n = 0; n < StringLength; n++)
	{
		char c = char(CodeUnits[n]);
		if ( !(tStd::tIsdigit(c) || (includeDecimal && (c == '.'))) )
			return false;
	}

	return true;
}


inline bool bString::IsAlphaNumeric(bool includeUnderscore, bool includeDecimal) const
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


inline int bString::CountChar(char c) const
{
	int count = 0;
	for (int i = 0; i < StringLength; i++)
		if (char(CodeUnits[i]) == c)
			count++;
	return count;
}


inline int bString::FindChar(const char c, bool reverse, int start) const
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


inline int bString::FindAny(const char* chars) const
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


inline int bString::FindString(const char8_t* str, int start) const
{
	if (IsEmpty())
		return -1;

	tAssert((start >= 0) && (start < StringLength));
	const char8_t* found = tStd::tStrstr(&CodeUnits[start], str);
	if (found)
		return int(found - CodeUnits);

	return -1;
}


inline int bString::Replace(const char search, const char replace)
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
