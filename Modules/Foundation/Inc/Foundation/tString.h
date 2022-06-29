// tString.h
//
// tString is a simple and readable string class that implements sensible operators, including implicit casts. There is
// no UCS2 or UTF-16 support. The text in a tString is considerd to be UTF-8 and terminated with a nul. You cannot
// stream (from cin etc) more than 512 chars into a string. This restriction is only for wacky << streaming. For
// conversions of arbitrary types to tStrings, see tsPrint in the higher level System module.
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


struct tString
{
	tString()																											{ CodeUnits = &EmptyChar; }
	tString(const tString&);
	tString(const char16_t* src)																						{ CodeUnits = &EmptyChar; Set(src); }
	tString(const char32_t* src)																						{ CodeUnits = &EmptyChar; Set(src); }
	tString(const tStringUTF16& src)																					{ CodeUnits = &EmptyChar; Set(src); }
	tString(const tStringUTF32& src)																					{ CodeUnits = &EmptyChar; Set(src); }

	// Construct a string with enough room for length characters. Length+1 characters are reserved to make room for the
	// null terminator. The reserved space is zeroed.
	explicit tString(int length);

	tString(const char8_t*);

	// You can create a UTF-8 tString from an ASCII string no problem. All ASCII strings are valid UTF-8.
	tString(const char* s)																								: tString((const char8_t*)s) { }

	// Note the difference here. A char8_t can't be guaranteed to store a unicode codepoint if the codepoint requires
	// continuations in the UTF-8 encoding. So, here we support char only which we use for ASCII characters (which are
	// guaranteed not to need continuation bytes in UFT-8).
	tString(char);
	virtual ~tString();

	tString& operator=(const tString&);

	bool IsEqual(const tString& s) const																				{ return !tStd::tStrcmp(CodeUnits, s.CodeUnits); }
	bool IsEqual(const char8_t* s) const																				{ if (!s) return false; return !tStd::tStrcmp(CodeUnits, s); }
	bool IsEqual(const char* s) const																					{ if (!s) return false; return !tStd::tStrcmp(CodeUnits, (char8_t*)s); }
	bool IsEqualCI(const tString& s) const																				{ return !tStd::tStricmp(CodeUnits, s.CodeUnits); }
	bool IsEqualCI(const char8_t* s) const																				{ if (!s) return false; return !tStd::tStricmp(CodeUnits, s); }
	bool IsEqualCI(const char* s) const																					{ if (!s) return false; return !tStd::tStricmp(CodeUnits, (char8_t*)s); }

	// These allow for implicit conversion to a UTF-8 character pointer. By not including implicit casts to const char*
	// we are encouraging further proper use of char8_t. You can either make the function you are calling take the
	// proper UTF-* type, or explicitly call Chs() or Txt() to get an old char-based pointer.
	operator const char8_t*()																							{ return CodeUnits; }
	operator const char8_t*() const																						{ return CodeUnits; }

	explicit operator uint32();
	explicit operator uint32() const;

	char8_t& operator[](int i)		/* This may be somewhat meaningless if continuations needed at the index. */		{ return CodeUnits[i]; }
	friend tString operator+(const tString& prefix, const tString& suffix);
	tString& operator+=(const tString&);

	void Set(const char8_t*);
	void Set(const char* s)																								{ Set((const char8_t*)s); }

	void Set(const char16_t* src)																						{ SetUTF16(src); }
	void Set(const char32_t* src)																						{ SetUTF32(src); }
	void Set(const tStringUTF16& src);
	void Set(const tStringUTF32& src);

	int Length() const				/* The length in char8_t's, not the display length (which is not that useful). */	{ return int(tStd::tStrlen(CodeUnits)); }
	bool IsEmpty() const																								{ return (CodeUnits == &EmptyChar) || !tStd::tStrlen(CodeUnits); }
	bool IsValid() const			/* returns true is string is not empty. */											{ return !IsEmpty(); }
	void Clear()																										{ if (CodeUnits != &EmptyChar) delete[] CodeUnits; CodeUnits = &EmptyChar; }

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

	// Many other functions and libraries that are UTF-8 compliant are not yet (and may never) use the proper char8_t
	// type and use char* and const char*. These functions allow you to retrieve the tString using the char type.
	// Use these with tPrintf and %s.
	char* Txt()																											{ return (char*)CodeUnits; }
	const char* Chs() const																								{ return (const char*)CodeUnits; }
	const char* Chz() const			/* Like Chs() but returns nullptr if the string is empty, not a pointer to "". */	{ return IsEmpty() ? nullptr : (const char*)CodeUnits; }
	const char* Pod() const			/* Plain Old Data */																{ return (const char*)CodeUnits; }

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

	// Sets the tString from a UTF codeunit array. If srcLen is -1 assumes supplied array is null-terminated, otherwise
	// specify how long it is. Returns new length (not including null terminator) of the tString.
	int SetUTF16(const char16_t* src, int srcLen = -1);
	int SetUTF32(const char32_t* src, int srcLen = -1);

protected:
	// By using the char8_t we are indicating the data is stored in UTF-8 encoding. Note that unlike char, a char8_t
	// is guaranteed to be unsigned, as well as a distinct type. In unicode, these are called code-units.
	char8_t* CodeUnits;
	static char8_t EmptyChar;										// All empty strings can use this.
};


// tStringUTF16 and tStringUTF32 are not intended to be full-fledged string classes, but they are handy to marshall data
// to and from OS calls that take or return these encodings. Primarily these abstract away the memory management for the
// different encodings, since the encoding size depends on the string contents. You make construct a tStringUTFn from a
// tString, and you may construct a tString from a tStringUTFn string.
struct tStringUTF16
{
	tStringUTF16()																										{ }
	explicit tStringUTF16(int length);	// Reserves length+1 char32_t code units (+1 for terminator).
	tStringUTF16(const char16_t* src)																					{ Set(src); }
	tStringUTF16(const char8_t* src)																					{ Set(src); }
	tStringUTF16(const tStringUTF16& src)																				{ Set(src); }
	tStringUTF16(const tString& src)																					{ Set(src); }
	~tStringUTF16()																										{ delete[] CodeUnits; }

	void Clear()																										{ delete[] CodeUnits; CodeUnits = nullptr; }
	bool IsValid() const																								{ return (Length() > 0); }
	int Length() const																									{ return CodeUnits ? tStd::tStrlen(CodeUnits) : 0; }
	const char16_t* Chars() const																						{ return CodeUnits; }
	char16_t* Units() const																								{ return CodeUnits; }
	#if defined(PLATFORM_WINDOWS)
	wchar_t* GetLPWSTR() const																							{ return (wchar_t*)CodeUnits; }
	#endif

	void Set(const char16_t* src);
	void Set(const char8_t* src);
	void Set(const tStringUTF16& src);
	void Set(const tString& src);

private:
	char16_t* CodeUnits = nullptr;
};


struct tStringUTF32
{
	tStringUTF32()																										{ }
	explicit tStringUTF32(int length);	// Reserves length char32_t+1 code units (+1 for terminator).
	tStringUTF32(const char32_t* src)																					{ Set(src); }
	tStringUTF32(const char8_t* src)																					{ Set(src); }
	tStringUTF32(const tStringUTF32& src)																				{ Set(src); }
	tStringUTF32(const tString& src)																					{ Set(src); }
	~tStringUTF32()																										{ delete[] CodeUnits; }

	void Clear()																										{ delete[] CodeUnits; CodeUnits = nullptr; }
	bool IsValid() const																								{ return (Length() > 0); }
	int Length() const																									{ return CodeUnits ? tStd::tStrlen(CodeUnits) : 0; }
	const char32_t* Chars() const																						{ return CodeUnits; }
	char32_t* Units() const																								{ return CodeUnits; }

	void Set(const char32_t* src);
	void Set(const char8_t* src);
	void Set(const tStringUTF32& src);
	void Set(const tString& src);

private:
	char32_t* CodeUnits = nullptr;
};


// Binary operator overloads should be outside the class so we can do things like if ("a" == b) where b is a tString.
inline bool operator==(const tString& a, const tString& b)																{ return !tStd::tStrcmp(a.Chars(), b.Chars()); }
inline bool operator!=(const tString& a, const tString& b)																{ return !!tStd::tStrcmp(a.Chars(), b.Chars()); }
inline bool operator==(const tString& a, const char8_t* b)																{ return !tStd::tStrcmp(a.Chars(), b); }
inline bool operator!=(const tString& a, const char8_t* b)																{ return !!tStd::tStrcmp(a.Chars(), b); }
inline bool operator==(const char8_t* a, const tString& b)																{ return !tStd::tStrcmp(a, b.Chars()); }
inline bool operator!=(const char8_t* a, const tString& b)																{ return !!tStd::tStrcmp(a, b.Chars()); }
inline bool operator==(const char* a, const tString& b)																	{ return !tStd::tStrcmp((const char8_t*)a, b.Chars()); }
inline bool operator!=(const char* a, const tString& b)																	{ return !!tStd::tStrcmp((const char8_t*)a, b.Chars()); }


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


inline tString::tString(const char8_t* t)
{
	if (t)
	{
		int len = int(tStd::tStrlen(t));
		if (len > 0)
		{
			CodeUnits = new char8_t[1 + len];
			tStd::tStrcpy(CodeUnits, t);
			return;
		}
	}

	CodeUnits = &EmptyChar;
}


inline tString::tString(const tString& s)
{
	CodeUnits = new char8_t[1 + tStd::tStrlen(s.CodeUnits)];
	tStd::tStrcpy(CodeUnits, s.CodeUnits);
}


inline tString::tString(char c)
{
	CodeUnits = new char8_t[2];
	CodeUnits[0] = c;
	CodeUnits[1] = '\0';
}


inline tString::tString(int length)
{
	if (!length)
	{
		CodeUnits = &EmptyChar;
	}
	else
	{
		CodeUnits = new char8_t[1+length];
		tStd::tMemset(CodeUnits, 0, 1+length);
	}
}


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


inline void tString::Set(const char8_t* s)
{
	Clear();
	if (!s)
		return;

	int len = tStd::tStrlen(s);
	if (len <= 0)
		return;

	CodeUnits = new char8_t[1 + len];
	tStd::tStrcpy(CodeUnits, s);
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


inline tString::~tString()
{
	if (CodeUnits != &EmptyChar)
		delete[] CodeUnits;
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
