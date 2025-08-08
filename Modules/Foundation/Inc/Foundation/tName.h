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
// for the terminating null).
//
// @todo If the string is very small, it could be modified to use what would be reserved for the code-unit array
// pointer as the data itself and avoids a heap allocation. When compiling as x64, you would get up to 7 code-units in
// length (the eighth byte is for the null). When compiling for x86 or any target with 4 byte pointers this feature
// would be disabled. It would make tName well-suited to store FourCCs.
//
// tName also maintains the length as a separate 4 byte integer. In total:
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

	// Construct from tString. tString has a different concept of IsValid (it is the empty string). For this reason
	// after this constructor the tName will always be valid (IsSet true). An empty tString generates a valid tName set
	// to the empty string.
	tName(const tString& src)																							{ Set(src); }

	// Creates a tName with a single character, Note the char type here. A char8_t can't be guaranteed to store a
	// unicode codepoint if the codepoint requires continuations in the UTF-8 encoding. So, here we support char only
	// which we use for ASCII characters since ASCII chars are guaranteed to _not_ need continuation units in UFT-8.
	tName(char c)																										{ Set(c); }

	// These constructors expect the string pointers to be null-terminated. You can create a UTF-8 tName from an ASCII
	// string (char*) since all ASCII strings are valid UTF-8. Constructors taking char8_t, char16_t, or chat32_t
	// pointers assume the src is UTF encoded. If src is nullptr, an invalid tName is created.
	tName(const char*				src)																				{ Set(src); }
	tName(const char8_t*			src)																				{ Set(src); }
	tName(const char16_t*			src)																				{ Set(src); }
	tName(const char32_t*			src)																				{ Set(src); }

	// These constructors, that specify the input length, allow you to have more than one null be present in the tName.
	// The internal hash is computed on the number of code-units stored minus the internal terminating null.
	// For example, with 0 meaning '\0'.
	//
	// AB0CD0 with srcLen = 5 will be stored in the tName as AB0CD0
	// AB0CD0 with srcLen = 6 will be stored in the tName as AB0CD00
	// AB0CD  with srcLen = 5 will be stored in the tName as AB0CD0
	// 0      with srcLen = 0 will be stored in the tName as 0. This is the empty string.
	// AB0CD0 with srcLen = 2 will be stored in the tName as AB0
	// AB0CD0 with srcLen = 3 will be stored in the tName as AB00
	//
	// In all cases the length returned by Length() will match the supplied srcLen. If src is nullptr or srcLen < 0, the
	// resulting tName is invalid.
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

	// For Set functions that take in a pointer, if the src pointer is nullptr, the resulting tName is invalid.
	void Set(const char*			src)																				{ Set((const char8_t*)src); }
	void Set(const char8_t*			src);
	void Set(const char16_t*		src)																				{ SetUTF16(src); }
	void Set(const char32_t*		src)																				{ SetUTF32(src); }

	// For Set functions that take in a pointer and a length (allowing multiple nulls), if src is nullptr or srcLen < 0,
	// the resulting tName is invalid. In all cases the length returned by Length() will match the supplied srcLen.
	void Set(const char*			src, int srcLen)																	{ Set((const char8_t*)src, srcLen); }
	void Set(const char8_t*			src, int srcLen);
	void Set(const char16_t*		src, int srcLen);
	void Set(const char32_t*		src, int srcLen);

	void Set(const tStringUTF16&	src);
	void Set(const tStringUTF32&	src);

	void Clear()					/* Makes the string invalid. Frees any heap memory used. */							{ CodeUnitsSize = 0; delete[] CodeUnits; CodeUnits = nullptr; Hash = 0; }
	void Empty()					/* Makes the string a valid empty string. */										{ Clear(); CodeUnitsSize = 1; CodeUnits = new char8_t[CodeUnitsSize]; CodeUnits[0] = '\0'; Hash = ComputeHash(); }

	// The length in char8_t's (code-units), not the display length (which is not that useful). Returns -1 if the tName
	// is not set (invalid). The tName may have multiple nulls in it. This is fine, it does not stop at the first one.
	int Length() const																									{ return CodeUnitsSize - 1; }

	uint64 GetHash() const																								{ return Hash; }
	uint64 GetID() const																								{ return GetHash(); }
	uint64 ID() const																									{ return GetHash(); }
	uint64 AsID() const																									{ return GetHash(); }

	bool IsEmpty() const			/* Returns true for the empty name (length 0). This is a valid name. */				{ return (CodeUnitsSize == 1); }		// The 1 accounts for the internal null terminator.
	bool IsValid() const			/* Returns true for empty name "" (length 0) or any string with length >= 1 */		{ return (CodeUnitsSize != 0); }
	bool IsInvalid() const			/* Returns true for an invalid tName (length -1). */								{ return (CodeUnitsSize == 0); }

	bool IsSet() const				/* Synonym for IsValid. Also returns true for the empty name. */					{ return IsValid(); }
	bool IsNotSet() const			/* Synonym for IsInvalid. Only returns true for tNames that haven't been set. */	{ return IsInvalid(); }

	// The IsEqual variants taking (only) pointers assume null-terminated inputs. Two empty names are considered equal.
	// If the input is nullptr (for functions taking pointers) it is not considered equal to an empty name. A nullptr is
	// treated as an unset/invalid name and is not equal to anything (including another invalid name). For variants
	// taking pointers and a length, all characters are checked (multiple null chars supported). If strLen < 0 the input
	// is treated as invalid and equality is guaranteed false. For variants taking no length, the name is considered
	// equal if the characters match up to the first null in the tName (even if there are more of them internally). For
	// the IsEqual that takes in another tName, the comparisons are very fast as only the hash is compared.
	bool IsEqual(const tName&		nam) const				/* Fast. Compares hashes. */								{ if (IsInvalid() || nam.IsInvalid()) return false; return (Hash == nam.Hash); }
	bool IsEqual(const char*		str) const				/* A nullptr str is treated as an invalid string. */ 		{ return IsEqual(str, str ? tStd::tStrlen(str) : -1); }
	bool IsEqual(const char8_t*		str) const																			{ return IsEqual(str, str ? tStd::tStrlen(str) : -1); }
	bool IsEqual(const char*		str, int strLen) const	/* strLen = 0 and non-null str is the empty string. */		{ return IsEqual((const char8_t*)str, strLen); }
	bool IsEqual(const char8_t*		str, int strLen) const;	/* Defined inline below. */

	// Appends supplied suffix name to this name. Handles the full length of suffix -- including multiple nulls if there
	// are any.
	tName& Append(const tName& suffix);
	tName& operator=(const tName& nam)																					{ Set(nam); return *this; }

	// These are not particulary fast, but they are useful if you just want to construct a concatenated tName and not
	// modify it afterwards.
	friend tName operator+(const tName& prefix, const tName& suffix);
	tName& operator+=(const tName& suffix)																				{ return Append(suffix); }

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

	// These return the fast 32 bit hash of the string data (code units). They take into account the full
	// represented string -- not just up to the first null. That is, they use StringLength as the data-set size.
	explicit operator uint32();
	explicit operator uint32() const;

	// Similar to above but return the 64 bit hash (not a fast version).
	explicit operator uint64()																							{ return GetHash(); }
	explicit operator uint64() const																					{ return GetHash(); }

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

	// The GetAs functions consider the contents of the current tName up to the first null encountered. See comment
	// for tStrtoiT in tStandard.h for format requirements. The summary is that if base is -1, the function looks one of
	// the following prefixes in the string, defaulting to base 10 if none found. For invalid names 0 is returned.
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

	// Same as above but return false on any parse error instead of just returning 0. These return false if the tName is
	// invalid. @todo Float and double versions.
	bool ToInt(int& v, int base = -1) const																				{ return ToInt32(v, base); }
	bool ToInt32(int32& v, int base = -1) const																			{ return tStd::tStrtoi32(v, CodeUnits, base); }
	bool ToInt64(int64& v, int base = -1) const																			{ return tStd::tStrtoi64(v, CodeUnits, base); }
	bool ToUInt(uint& v, int base = -1) const																			{ return ToUInt32(v, base); }
	bool ToUInt32(uint32& v, int base = -1) const																		{ return tStd::tStrtoui32(v, CodeUnits, base); }
	bool ToUInt64(uint64& v, int base = -1) const																		{ return tStd::tStrtoui64(v, CodeUnits, base); }

	// tName UTF encoding/decoding functions. tName is encoded in UTF-8. These functions allow you to convert from tName
	// to UTF-16/32 arrays. If dst is nullptr returns the number of charN codeunits needed. If incNullTerminator is
	// false the number needed will be one fewer. If dst is valid, writes the codeunits to dst and returns number of
	// charNN codeunits written. If tName is invalid OR empty, 0 is returned and dst (if provided) is not modified.
	int GetUTF16(char16_t* dst, bool incNullTerminator = true) const;
	int GetUTF32(char32_t* dst, bool incNullTerminator = true) const;

	// Sets the tName from a UTF codeunit array. If srcLen is -1 assumes supplied array is null-terminated, otherwise
	// specify how long it is. Returns new length (not including null terminator) of the tName. If either src is
	// nullptr or srcLen is 0, the result is an invalid tName and 0 is returned.
	int SetUTF16(const char16_t* src, int srcLen = -1);
	int SetUTF32(const char32_t* src, int srcLen = -1);

protected:
	// Assumes CodeUnits and CodeUnitsSize are set appropriately. If CodeUnitsSize is 0 (implying CodeUnits is nullptr)
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


// Binary operator overloads should be outside the class so we can do things like if ("a" == b) where b is a tName.
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

	// The tNameItem copy cons is missing because as an intrusive list-item it can only be on one list at a time.
	tNameItem(const tName& s)																							: tName(s) { }
	tNameItem(const tStringUTF16& s)																					: tName(s) { }
	tNameItem(const tStringUTF32& s)																					: tName(s) { }
	tNameItem(const char8_t* c)																							: tName(c) { }
	tNameItem(char c)																									: tName(c) { }

	// This call does NOT change the list that the tNameItem is on. The link remains unmodified.
	tNameItem& operator=(const tNameItem&);
};


// Implementation below this line.


inline uint64 tName::ComputeHash() const
{
	if (IsInvalid())
		return 0;

	// This call deals with Length=0 gracefully (empty string). It does not deref the data pointer in this case.
	uint64 hash = tHash::tHashData64((const uint8*)CodeUnits, Length());
	if (hash == 0)
		hash = 0xFFFFFFFFFFFFFFFF;

	return hash;
}


inline tName::operator uint32()
{
	if (IsInvalid())
		return 0;

	// This call deals with Length=0 gracefully (empty string). It does not deref the data pointer in this case.
	return tHash::tHashDataFast32((const uint8*)CodeUnits, Length());
}


inline tName::operator uint32() const
{
	if (IsInvalid())
		return 0;

	// This function deals with a Length of zero gracefully. It does not deref the data pointer in this case.
	return tHash::tHashDataFast32((const uint8*)CodeUnits, Length());
}


inline void tName::Set(const tName& src)
{
	if (this == &src)
		return;

	Clear();
	if (src.IsInvalid())
		return;

	CodeUnitsSize = src.CodeUnitsSize;
	CodeUnits = new char8_t[CodeUnitsSize];
	tStd::tMemcpy(CodeUnits, src.CodeUnits, CodeUnitsSize);
	Hash = src.Hash;
}


inline void tName::Set(const tString& src)
{
	// We happen to know that tString internally always has a trailing null so we can leverage that and add 1 to the
	// length.
	if (src.IsEmpty())
	{
		Empty();
		return;
	}

	Clear();
	CodeUnitsSize = src.Length() + 1;
	CodeUnits = new char8_t[CodeUnitsSize];
	tStd::tMemcpy(CodeUnits, src.Units(), CodeUnitsSize);
	Hash = ComputeHash();
}


inline void tName::Set(char c)
{
	Clear();
	CodeUnitsSize = 2;
	CodeUnits = new char8_t[CodeUnitsSize];
	CodeUnits[0] = c;
	CodeUnits[1] = '\0';
	Hash = ComputeHash();
}


inline void tName::Set(const char8_t* src)
{
	Clear();
	if (!src)
		return;

	CodeUnitsSize = tStd::tStrlen(src) + 1;
	CodeUnits = new char8_t[CodeUnitsSize];
	tStd::tMemcpy(CodeUnits, src, CodeUnitsSize);		// Includes the terminating null.
	Hash = ComputeHash();
}


inline void tName::Set(const char8_t* src, int srcLen)
{
	Clear();
	if (!src || (srcLen < 0))
		return;

	CodeUnitsSize = srcLen + 1;
	CodeUnits = new char8_t[CodeUnitsSize];
	if (srcLen > 0)
		tStd::tMemcpy(CodeUnits, src, srcLen);
	CodeUnits[srcLen] = '\0';
	Hash = ComputeHash();
}


inline void tName::Set(const char16_t* src, int srcLen)
{
	Clear();
	if (!src || (srcLen < 0))
		return;
	SetUTF16(src, srcLen);
}


inline void tName::Set(const char32_t* src, int srcLen)
{
	Clear();
	if (!src || (srcLen < 0))
		return;
	SetUTF32(src, srcLen);
}


inline void tName::Set(const tStringUTF16& src)
{
	SetUTF16(src.Units(), src.Length());
}


inline void tName::Set(const tStringUTF32& src)
{
	SetUTF32(src.Units(), src.Length());
}


inline bool tName::IsEqual(const char8_t* str, int strLen) const
{
	if (IsInvalid() || !str || (strLen < 0) || (Length() != strLen))
		return false;

	if ((strLen == 0) && IsEmpty())
		return true;

	return !tStd::tMemcmp(CodeUnits, str, strLen);
}


inline tName& tName::Append(const tName& suffix)
{
	// Empty is guaranteed to have CodeUnitsSize == 1, a single null character.
	if (suffix.IsInvalid() || suffix.IsEmpty())
		return *this;

	if (IsInvalid() || IsEmpty())
	{
		Set(suffix);
		return *this;
	}

	int thisSize = Length();
	int thatSize = suffix.Length();
	int newSize = thisSize + thatSize + 1;

	char8_t* newUnits = new char8_t[newSize];
	tStd::tMemcpy(newUnits, CodeUnits, thisSize);

	// The plus one is so we get the terminating null with the memcpy.
	tStd::tMemcpy(newUnits+thisSize, suffix.CodeUnits, thatSize+1);
	Clear();

	CodeUnits = newUnits;
	CodeUnitsSize = newSize;
	Hash = ComputeHash();
	return *this;
}


inline tName operator+(const tName& prefix, const tName& suffix)
{
	tName concatenated(prefix);
	return concatenated.Append(suffix);
}


inline int tName::GetUTF16(char16_t* dst, bool incNullTerminator) const
{
	if (IsInvalid() || IsEmpty())
		return 0;

	if (!dst)
		return tStd::tUTF16(nullptr, CodeUnits, Length()) + (incNullTerminator ? 1 : 0);

	int numUnitsWritten = tStd::tUTF16(dst, CodeUnits, Length());
	if (incNullTerminator)
	{
		dst[numUnitsWritten] = 0;
		numUnitsWritten++;
	}

	return numUnitsWritten;
}


inline int tName::GetUTF32(char32_t* dst, bool incNullTerminator) const
{
	if (IsInvalid() || IsEmpty())
		return 0;

	if (!dst)
		return tStd::tUTF32(nullptr, CodeUnits, Length()) + (incNullTerminator ? 1 : 0);

	int numUnitsWritten = tStd::tUTF32(dst, CodeUnits, Length());
	if (incNullTerminator)
	{
		dst[numUnitsWritten] = 0;
		numUnitsWritten++;
	}

	return numUnitsWritten;
}


inline int tName::SetUTF16(const char16_t* src, int srcLen)
{
	Clear();
	if (!src || (srcLen == 0))
		return 0;

	// If srcLen < 0 it means ignore srcLen and assume src is null-terminated.
	if (srcLen < 0)
	{
		CodeUnitsSize = tStd::tUTF8s(nullptr, src) + 1;	// +1 for the internal null termination.
		CodeUnits = new char8_t[CodeUnitsSize];
		tStd::tUTF8s(CodeUnits, src);					// Writes the null terminator.
	}
	else
	{
		int len = tStd::tUTF8(nullptr, src, srcLen);
		CodeUnitsSize = len + 1;						// +1 for the internal null termination.
		CodeUnits = new char8_t[CodeUnitsSize];
		tStd::tUTF8(CodeUnits, src, srcLen);
		CodeUnits[len] = '\0';
	}

	Hash = ComputeHash();
	return Length();
}


inline int tName::SetUTF32(const char32_t* src, int srcLen)
{
	Clear();
	if (!src || (srcLen == 0))
		return 0;

	// If srcLen < 0 it means ignore srcLen and assume src is null-terminated.
	if (srcLen < 0)
	{
		CodeUnitsSize = tStd::tUTF8s(nullptr, src) + 1;	// +1 for the internal null termination.
		CodeUnits = new char8_t[CodeUnitsSize];
		tStd::tUTF8s(CodeUnits, src);
	}
	else
	{
		int len = tStd::tUTF8(nullptr, src, srcLen);
		CodeUnitsSize = len + 1;						// +1 for the internal null termination.
		tStd::tUTF8(CodeUnits, src, srcLen);
		CodeUnits[len] = '\0';
	}

	Hash = ComputeHash();
	return Length();
}


inline tNameItem& tNameItem::operator=(const tNameItem& src)
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
