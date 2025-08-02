// tString.h
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
// Length		:	Returns how many code-units are used by the string. This is NOT like a strlen call as it does not
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
// Copyright (c) 2004-2006, 2015, 2017, 2019-2025 Tristan Grimmer.
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


struct tString
{
	tString()																											{ UpdateCapacity(0, false); }
	tString(const tString& src)																							{ Set(src); }

	// Construct a string of length null characters.
	explicit tString(int length)																						{ Set(length); }

	// Creates a tString with a single character, Note the char type here. A char8_t can't be guaranteed to store a
	// unicode codepoint if the codepoint requires continuations in the UTF-8 encoding. So, here we support char only
	// which we use for ASCII characters since ASCII chars are guaranteed to _not_ need continuation units in UFT-8.
	tString(char c)																										{ Set(c); }

	// The constructors that don't take in a length expect the string pointers to be null-terminated.
	// The constructors that do take a length may contain multiple nulls in the src string.
	// You can create a UTF-8 tString from an ASCII string (char*) since all ASCII strings are valid UTF-8.
	// Constructors taking char8_t, char16_t, or chat32_t pointers assume the src is UTF encoded.
	tString(const char*				src)																				{ Set(src); }
	tString(const char8_t*			src)																				{ Set(src); }
	tString(const char16_t*			src)																				{ Set(src); }
	tString(const char32_t*			src)																				{ Set(src); }
	tString(const char*				src, int srcLen)																	{ Set(src, srcLen); }
	tString(const char8_t*			src, int srcLen)																	{ Set(src, srcLen); }
	tString(const char16_t*			src, int srcLen)																	{ Set(src, srcLen); }
	tString(const char32_t*			src, int srcLen)																	{ Set(src, srcLen); }

	// The tStringUTF constructors allow the src strings to have multiple nulls in them.
	tString(const tStringUTF16&		src)																				{ Set(src); }
	tString(const tStringUTF32&		src)																				{ Set(src); }
	virtual ~tString()																									{ delete[] CodeUnits; }

	// The set functions always clear the current string and set it to the supplied src.
	void Set(const tString&			src);
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
	void Set(const tStringUTF16&	src);
	void Set(const tStringUTF32&	src);

	// Some external functions write directly into the CodeUnits and need to manually set the StringLength first. This
	// function allows you to do that. It also writes the internal null terminator. If you call this with a length >
	// capacity, it updates the capacity to be >= the requested length. This function preserves all existing characters
	// if it can. Calling with a number < the current length will cut off the end characters. If you call with a value >
	// current length, it sets the extra characters (up to length) with zeros. In all cases the string length will be
	// left at what you called this with. It is illegal to call with a negative value.
	// For efficiency you can opt to set preserve to false. In this case the characters may be completely uninitialized
	// and it is your responsibility to populate them with something valid. Internal null still written.
	void SetLength(int length, bool preserve = true);

	// Does not release memory. Simply sets the string to empty. Fast.
	void Clear()																										{ StringLength = 0; CodeUnits[0] = '\0'; }

	// The length in char8_t's (code-units), not the display length (which is not that useful).
	// This length has nothing to do with how many null characters are in the string or where they are.
	int Length() const																									{ return StringLength; }

	// Treats the tString as a null-terminated (C++) string and returns the length. Generally a tString would not treat
	// '\0' any differently that other characters. This circumvents this if you know you are dealing with a NT string.
	int LengthNullTerminated() const																					{ return tStd::tStrlen(CodeUnits); }

	// This is the internal capacity of the string. Appending more characters than this will cause more memory to be
	// needed.
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

	// If you know you are dealing with a null-terminated string, this function will reduce the length to the number of
	// characters before the null terminator and can reduce the memory used. Returns the new length.
	int ShrinkNullTerminated()																							{ SetLength(LengthNullTerminated()); Shrink(); return Length(); }

	// This is like Reserve except it takes in the number of _extra_ code-units you want. It will attempt to add or
	// subtract from the current capacity. Putting in a negative to shrink is supported. Again, it cannot shrink below
	// the current string length or lower than the minimum capacity. Returns the new capacity.
	int Grow(int numUnits)																								{ return Reserve(CurrCapacity + numUnits); }

	bool IsEmpty() const																								{ return (StringLength <= 0); }
	bool IsValid() const			/* Returns true if string is not empty. */											{ return !IsEmpty(); }

	tString& operator=(const tString&);

	// The IsEqual variants taking (only) pointers assume null-terminated inputs. Two empty strings are considered
	// equal. If the input is nullptr (for functions taking pointers) it is not considered equal to an empty string.
	// For variants taking pointers and a length, all characters are checked (multiple null chars supported).
	bool IsEqual(const tString&		str) const																			{ return IsEqual(str.CodeUnits, str.Length()); }
	bool IsEqual(const char*		str) const																			{ return IsEqual(str, str ? tStd::tStrlen(str) : 0); }
	bool IsEqual(const char8_t*		str) const																			{ return IsEqual(str, str ? tStd::tStrlen(str) : 0); }
	bool IsEqual(const char*		str, int strLen) const																{ return IsEqual((const char8_t*)str, strLen); }
	bool IsEqual(const char8_t*		str, int strLen) const;
	bool IsEqualCI(const tString&	str) const																			{ return IsEqualCI(str.CodeUnits, str.Length()); }
	bool IsEqualCI(const char*		str) const																			{ return IsEqualCI(str, str ? tStd::tStrlen(str) : 0); }
	bool IsEqualCI(const char8_t*	str) const																			{ return IsEqualCI(str, str ? tStd::tStrlen(str) : 0); }
	bool IsEqualCI(const char*		str, int strLen) const																{ return IsEqualCI((const char8_t*)str, strLen); }
	bool IsEqualCI(const char8_t*	str, int strLen) const;

	// Appends supplied suffix string to this string.
	tString& Append(const tString& suffix);

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

	friend tString operator+(const tString& prefix, const tString& suffix);
	tString& operator+=(const tString& suffix)																			{ return Append(suffix); }

	// All non-null characters must meet the criteria for these functions to return true.
	bool IsAlphabetic(bool includeUnderscore = true) const;
	bool IsNumeric(bool includeDecimal = false) const;
	bool IsAlphaNumeric(bool includeUnderscore = true, bool includeDecimal = false) const;

	// These only work well for ASCII strings as vars like 'count' are indexes into the text data and are not
	// 'continuation-aware'. This comment applies to all below functions with the words 'Left', 'Right', and 'Mid' in
	// them except for functions that take in a char8_t* or char* prefix or suffix. Those work for ASCII and UTF-8.
	//
	// Returns a tString of the characters before the first marker. Returns an empty string if marker was not found.
	tString Left(const char marker = ' ') const;
	tString Right(const char marker = ' ') const;			// Same as Left but chars after last marker.

	tString Left(int count) const;							// Returns a tString of the first count chars. Return what's available if count > length.
	tString Mid(int start, int count) const;				// Returns count chars from start (inclusive), or what's available if start+count > length.
	tString Right(int count) const;							// Same as Left but returns last count chars.

	// Extracts first word up to and not including first divider encountered. The tString is left with the remainder,
	// not including the divider. If divider isn't found an empty string is returned and the tString is left unmodified.
	tString ExtractLeft(const char divider = ' ');

	// Extracts last word after and not including last divider encountered. The tString is left with the remainder,
	// not including the divider. If divider isn't found an empty string is returned and the tString is left unmodified.
	tString ExtractRight(const char divider = ' ');

	// Returns a tString of the first count chars. Removes these from the current string. If count > length then what's
	// available is extracted.
	tString ExtractLeft(int count);

	// Returns chars from start to count, but also removes that from the tString.  If start + count > length then what's
	// available is extracted.
	tString ExtractMid(int start, int count);

	// Returns a tString of the last count chars. Removes these from the current string. If count > length then what's
	// available is extracted.
	tString ExtractRight(int count);

	// If this string starts with prefix, removes and returns it. If not, returns empty string and no modification.
	// Prefix is assumed to be null-terminated.
	tString ExtractLeft(const char* prefix)																				{ return ExtractLeft((const char8_t*)prefix); }
	tString ExtractLeft(const char8_t* prefix);

	// If this string ends with suffix, removes and returns it. If not, returns empty string and no modification.
	// Suffix is assumed to be null-terminated.
	tString ExtractRight(const char* suffix)																			{ return ExtractRight((const char8_t*)suffix); }
	tString ExtractRight(const char8_t* suffix);

	// Accesses the raw UTF-8 codeunits represented by the 'official' unsigned UTF-8 character datatype char8_t.
	// Except for Charz nullptr is not returned although they may return a pointer to an empty string.
	char8_t* Text()																										{ return CodeUnits; }
	const char8_t* Chars() const																						{ return CodeUnits; }
	const char8_t* Charz() const	/* Like Chars() but returns nullptr if the string is empty, not a pointer to "". */	{ return IsEmpty() ? nullptr : CodeUnits; }
	char8_t* Units() const			/* Unicode naming. Code 'units'. */													{ return CodeUnits; }

	// Many other functions and libraries that are UTF-8 compliant do not yet (and may never) use the proper char8_t
	// type and use char* and const char*. These functions allow you to retrieve the tString using the char type.
	// You can also use these with tPrintf and %s. These are synonyms of the above 4 calls.
	char* Txt()																											{ return (char*)CodeUnits; }
	const char* Chr() const																								{ return (const char*)CodeUnits; }
	const char* Chz() const			/* Like Chr() but returns nullptr if the string is empty, not a pointer to "". */	{ return IsEmpty() ? nullptr : (const char*)CodeUnits; }
	char8_t* Pod() const			/* Plain Old Data */																{ return CodeUnits; }

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

	// Returns the index of the first character in the tString that is also somewhere in the null-terminated string
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

	// Removes all leading characters in this string that match any of the characters in the null-terminated theseChars
	// eg. Calling RemoveLeading on "cbbabZINGabc" with "abc" yields "ZINGabc". Returns the number of characters removed.
	// Note that theseChars are ASCII.
	int RemoveLeading(const char* theseChars);

	// Removes all trailing characters in this string that match any of the characters in the null-terminated theseChars
	// eg. Calling RemoveTrailing on "abcZINGabcaab" with "abc" yields "abcZING". Returns the number of characters removed.
	// Note that theseChars are ASCII.
	int RemoveTrailing(const char* theseChars);

	// Removes the first ASCII character. Length will be one less after. Returns num removed. Will be 0 for an empty
	// string. Internally this one needs to do a mem-move.
	int RemoveFirst();

	// Removes the last ASCII character. Length will be one less after. Returns num removed.
	int RemoveLast();

	// Same as the above two but removes _any_ occurrences of characters specified in the null-terminated theseChars.
	// eg. Calling RemoveAny on "abcZaIbNcGabcaab" with "abc" yields "ZING". Returns the number of characters removed.
	// Note that theseChars are ASCII.
	int RemoveAny(const char* theseChars);

	// Removes any characters that are _not_ occurrences of characters specified in the null-terminated theseChars.
	// eg. Calling RemoveAnyNot on "abcZaIbNcGabcaab" with "abc" yields "abcabcabcaab". Returns the number of
	// characters removed. Note that theseChars are ASCII.
	int RemoveAnyNot(const char* theseChars);

	// ToUpper and ToLower both modify the object as well as return a reference to it. Returning a reference makes it
	// easy to string together expressions such as: if (name.ToLower() == "ah")
	tString& ToUpper()																									{ for (int n = 0; n < StringLength; n++) CodeUnits[n] = tStd::tToUpper(CodeUnits[n]); return *this; }
	tString& ToLower()																									{ for (int n = 0; n < StringLength; n++) CodeUnits[n] = tStd::tToLower(CodeUnits[n]); return *this; }

	// These do not modify the string. They return a new one.
	tString Upper() const																								{ tString s(*this); s.ToUpper(); return s; }
	tString Lower() const																								{ tString s(*this); s.ToLower(); return s; }

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

	// tString UTF encoding/decoding functions. tString is encoded in UTF-8. These functions allow you to convert from
	// tString to UTF-16/32 arrays. If dst is nullptr returns the number of charN codeunits needed. If incNullTerminator
	// is false the number needed will be one fower. If dst is valid, writes the codeunits to dst and returns number
	// of charN codeunits written.
	int GetUTF16(char16_t* dst, bool incNullTerminator = true) const;
	int GetUTF32(char32_t* dst, bool incNullTerminator = true) const;

	// Sets the tString from a UTF codeunit array. If srcLen is -1 assumes supplied array is null-terminated, otherwise
	// specify how long it is. Returns new length (not including null terminator) of the tString.
	int SetUTF16(const char16_t* src, int srcLen = -1);
	int SetUTF32(const char32_t* src, int srcLen = -1);

protected:
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

	// This could be made to be dynamic. Just didn't want to waste 4 bytes for every tString instance.
	const int MinCapacity			= 15;

	// If GrowParam is positive, it represents how many extra code-units to grow by when out of capacity.
	// If GrowParam is negative, its absolute value represents how many times bigger the capacity should be
	// compared to the required lenght of the string.
	// If GrowParam is zero, everthing still works, you just don't get the extra code-units so it's less efficient.
	int GrowParam					= 64;

	// The length of the tString currently used in code-units.
	int StringLength				= 0;

	// The capacity. The number of allocated CodeUnuts is always one more than this.
	int CurrCapacity				= 0;

	// By using the char8_t we are indicating the data is stored in UTF-8 encoding. Note that unlike char, a char8_t
	// is guaranteed to be unsigned, as well as a distinct type. In unicode spec for UTFn, these are called code-units.
	// With tStrings the CodeUnits pointer is never nullptr after construction. There is always some capacity.
	char8_t* CodeUnits				= nullptr;
};


// tStringUTF16 and tStringUTF32 are not intended to be full-fledged string classes, but they are handy to marshall data
// to and from OS calls that take or return these encodings. Primarily these abstract away the memory management for the
// different encodings, since the encoding size depends on the string contents. You may construct a tStringUTFn from a
// tString, and you may construct a tString from a tStringUTFn string.
//
// These helper classes do not support Capacity like tString, so they are not the most efficient. They _do_ support not
// requiring null-termination -- they are arrays of codeunits and length is stored explicitly. This allows multiple
// nulls to be placed in the string if desired. Internally we always store a terminating null.
struct tStringUTF16
{
	tStringUTF16()																										{ }
	explicit tStringUTF16(int length);	// Reserves length+1 char16_t code units (+1 for the inernal terminator).
	tStringUTF16(const tString& src)																					{ Set(src); }
	tStringUTF16(const tStringUTF16& src)																				{ Set(src); }
	tStringUTF16(const tStringUTF32& src)																				{ Set(src); }

	// These constructors expect null-termination of the input arrays.
	tStringUTF16(const char8_t*  src)																					{ Set(src); }
	tStringUTF16(const char16_t* src)																					{ Set(src); }
	tStringUTF16(const char32_t* src)																					{ Set(src); }

	// These constructors do not require null-termination of the input arrays.
	tStringUTF16(const char8_t*  src, int length)																		{ Set(src, length); }
	tStringUTF16(const char16_t* src, int length)																		{ Set(src, length); }
	tStringUTF16(const char32_t* src, int length)																		{ Set(src, length); }

	~tStringUTF16()																										{ delete[] CodeUnits; }

	void Set(const tString& src);
	void Set(const tStringUTF16& src);
	void Set(const tStringUTF32& src);
	void Set(const char8_t*  src);				// Assumes src is null-terminated.
	void Set(const char16_t* src);				// Assumes src is null-terminated.
	void Set(const char32_t* src);				// Assumes src is null-terminated.
	void Set(const char8_t*  src, int length);	// As meny nulls as you like.
	void Set(const char16_t* src, int length);	// As many nulls as you like.
	void Set(const char32_t* src, int length);	// As many nulls as you like.

	// Some external functions write directly into the CodeUnits and need to manually set the StringLength first. This
	// function allows you to do that. It also writes the internal null terminator. This function preserves all existing
	// code-units if it can. Calling with a number < the current length will cut off the end units. If you call with a
	// value > current length it sets the extra characters (up to length) with zeros. In all cases the string length
	// will be left at what you called this with. It is illegal to call with a negative value.
	// For efficiency you can opt to set preserve to false. In this case the characters may be completely uninitialized
	// and it is your responsibility to populate them with something valid. Internal null still written.
	void SetLength(int length, bool preserve = true);

	void Clear()																										{ delete[] CodeUnits; CodeUnits = nullptr; StringLength = 0; }
	bool IsValid() const																								{ return (Length() > 0); }
	bool IsEmpty() const																								{ return !IsValid(); }
	int Length() const																									{ return StringLength; }

	// Accesses the raw UTF-16 codeunits represented by the 'official' unsigned UTF-16 character datatype char16_t.
	char16_t* Text()				/* Unlike tString, will be nullptr if empty. */										{ return CodeUnits; }
	const char16_t* Chars() const	/* Unlike tString, will be nullptr if empty. */										{ return CodeUnits; }
	char16_t* Units() const			/* Unicode naming. Code 'units'. */													{ return CodeUnits; }

	// Shorter synonyms of the above.
	char16_t* Txt()																										{ return CodeUnits; }
	const char16_t* Chr() const																							{ return CodeUnits; }
	char16_t* Pod() const			/* Plain Old Data */																{ return CodeUnits; }

	#if defined(PLATFORM_WINDOWS)
	wchar_t* GetLPWSTR() const																							{ return (wchar_t*)CodeUnits; }
	#endif

private:
	int StringLength	= 0;					// In char16_t codeunits, not including terminating null.
	char16_t* CodeUnits	= nullptr;
};


struct tStringUTF32
{
	tStringUTF32()																										{ }
	explicit tStringUTF32(int length);	// Reserves length char32_t+1 code units (+1 for terminator).
	tStringUTF32(const tString& src)																					{ Set(src); }
	tStringUTF32(const tStringUTF16& src)																				{ Set(src); }
	tStringUTF32(const tStringUTF32& src)																				{ Set(src); }

	// These constructors expect null-termination of the input arrays.
	tStringUTF32(const char8_t*  src)																					{ Set(src); }
	tStringUTF32(const char16_t* src)																					{ Set(src); }
	tStringUTF32(const char32_t* src)																					{ Set(src); }

	// These constructors do not require null-termination of the input arrays.
	tStringUTF32(const char8_t*  src, int length)																		{ Set(src, length); }
	tStringUTF32(const char16_t* src, int length)																		{ Set(src, length); }
	tStringUTF32(const char32_t* src, int length)																		{ Set(src, length); }

	~tStringUTF32()																										{ delete[] CodeUnits; }

	void Set(const tString& src);
	void Set(const tStringUTF16& src);
	void Set(const tStringUTF32& src);
	void Set(const char8_t* src);				// Assumes src is null-terminated.
	void Set(const char16_t* src);				// Assumes src is null-terminated.
	void Set(const char32_t* src);				// Assumes src is null-terminated.
	void Set(const char8_t*  src, int length);	// As meny nulls as you like.
	void Set(const char16_t* src, int length);	// As many nulls as you like.
	void Set(const char32_t* src, int length);	// As many nulls as you like.

	// Some external functions write directly into the CodeUnits and need to manually set the StringLength first. This
	// function allows you to do that. It also writes the internal null terminator. This function preserves all existing
	// code-units if it can. Calling with a number < the current length will cut off the end units. If you call with a
	// value > current length it sets the extra characters (up to length) with zeros. In all cases the string length
	// will be left at what you called this with. It is illegal to call with a negative value.
	// For efficiency you can opt to set preserve to false. In this case the characters may be completely uninitialized
	// and it is your responsibility to populate them with something valid. Internal null still written.
	void SetLength(int length, bool preserve = true);

	void Clear()																										{ delete[] CodeUnits; CodeUnits = nullptr; StringLength = 0; }
	bool IsValid() const																								{ return (Length() > 0); }
	bool IsEmpty() const																								{ return !IsValid(); }
	int Length() const																									{ return StringLength; }

	// Accesses the raw UTF-32 codeunits represented by the 'official' unsigned UTF-32 character datatype char32_t.
	char32_t* Text()				/* Unlike tString, will be nullptr if empty. */										{ return CodeUnits; }
	const char32_t* Chars() const	/* Unlike tString, will be nullptr if empty. */										{ return CodeUnits; }
	char32_t* Units() const			/* Unicode naming. Code 'units'. */													{ return CodeUnits; }

	// Shorter synonyms of the above.
	char32_t* Txt()																										{ return CodeUnits; }
	const char32_t* Chr() const																							{ return CodeUnits; }
	char32_t* Pod() const			/* Plain Old Data */																{ return CodeUnits; }

private:
	int StringLength	= 0;		// In char32_t codeunits, not including terminating null.
	char32_t* CodeUnits	= nullptr;
};


// Binary operator overloads should be outside the class so we can do things like if ("a" == b) where b is a tString.
// Operators below that take char or char8_t pointers assume they are null-terminated.
inline bool operator==(const tString& a, const tString& b)																{ return a.IsEqual(b); }
inline bool operator!=(const tString& a, const tString& b)																{ return !a.IsEqual(b); }
inline bool operator==(const tString& a, const char8_t* b)																{ return a.IsEqual(b); }
inline bool operator!=(const tString& a, const char8_t* b)																{ return !a.IsEqual(b); }
inline bool operator==(const char8_t* a, const tString& b)																{ return b.IsEqual(a); }
inline bool operator!=(const char8_t* a, const tString& b)																{ return !b.IsEqual(a); }
inline bool operator==(const char* a, const tString& b)																	{ return b.IsEqual(a); }
inline bool operator!=(const char* a, const tString& b)																	{ return !b.IsEqual(a); }


// The tStringItem class is just the tString class except they can be placed on tLists.
struct tStringItem : public tLink<tStringItem>, public tString
{
public:
	tStringItem()																										: tString() { }

	// The tStringItem copy cons is missing, because as a list item can only be on one list at a time.
	tStringItem(const tString& s)																						: tString(s) { }
	tStringItem(const tStringUTF16& s)																					: tString(s) { }
	tStringItem(const tStringUTF32& s)																					: tString(s) { }
	tStringItem(int length)																								: tString(length) { }
	tStringItem(const char8_t* c)																						: tString(c) { }
	tStringItem(char c)																									: tString(c) { }

	// This call does NOT change the list that the tStringItem is on. The link remains unmodified.
	tStringItem& operator=(const tStringItem&);
};


// Some utility functions that act on strings.
namespace tStd
{
	// Separates the src string into components based on the divider. If src was "abc_def_ghi", components will get
	// "abc", "def", and "ghi" appended to it. Returns the number of components appended to the components list. The
	// version that takes the string divider allows for multicharacter dividers. Note that "abc__def_ghi" will explode
	// to "abc", "", "def", and "ghi". Empty strings are preserved allowing things like exploding text files based on
	// linefeeds. You'll get one item per line even if the line only contains a linefeed.
	int tExplode(tList<tStringItem>& components, const tString& src, char divider = '_');
	int tExplode(tList<tStringItem>& components, const tString& src, const tString& divider);
}


// Implementation below this line.


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


inline int tString::Reserve(int numUnits)
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


inline int tString::Shrink()
{
	if ((StringLength == CurrCapacity) || (CurrCapacity == MinCapacity))
		return CurrCapacity;

	tAssert(StringLength < CurrCapacity);
	return Reserve(StringLength);
}


inline tString& tString::operator=(const tString& src)
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


inline tString& tString::Append(const tString& suffix)
{
	if (suffix.IsEmpty())
		return *this;

	int oldLen = Length();
	int newLen = oldLen + suffix.Length();
	UpdateCapacity(newLen, true);

	// The plus one is so we get the terminating null with the memcpy.
	tStd::tMemcpy(CodeUnits + oldLen, suffix.CodeUnits, suffix.Length()+1);
	StringLength = newLen;
	return *this;
}


inline tString operator+(const tString& prefix, const tString& suffix)
{
	tString buf( prefix.Length() + suffix.Length() );

	tStd::tMemcpy(buf.CodeUnits, prefix.CodeUnits, prefix.Length());
	tStd::tMemcpy(buf.CodeUnits + prefix.Length(), suffix.CodeUnits, suffix.Length());
	return buf;
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
