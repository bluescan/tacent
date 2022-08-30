// tStandard.h
//
// Tacent functions and types that are standard across all platforms. Includes global functions like itoa which are not
// available on some platforms, but are common enough that they should be.
//
// Copyright (c) 2004-2006, 2015, 2017, 2020, 2021, 2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "Foundation/tPlatform.h"
#include "Foundation/tAssert.h"
#pragma warning (disable: 4996)
#pragma warning (disable: 4146)
#pragma warning (disable: 4307)
#define tNumElements(arr) int(sizeof(arr)/sizeof(*arr))


namespace tStd
{

// The 3 XOR trick is slower in most cases so we'll use a standard swap.
template<typename T> inline void tSwap(T& a, T& b)																		{ T t = a; a = b; b = t; }
inline void* tMemcpy(void* dest, const void* src, int numBytes)															{ return memcpy(dest, src, numBytes); }
inline void* tMemmov(void* dest, const void* src, int numBytes)															{ return memmove(dest, src, numBytes); }
inline void* tMemset(void* dest, uint8 val, int numBytes)																{ return memset(dest, val, numBytes); }
inline void* tMemchr(void* data, uint8 val, int numBytes)																{ return memchr(data, val, numBytes); }
inline const void* tMemchr(const void* data, uint8 val, int numBytes)													{ return memchr(data, val, numBytes); }
inline int tMemcmp(const void* a, const void* b, int numBytes)															{ return memcmp(a, b, numBytes); }

// Memory-search. Searches for memory sequence needle of length needleNumBytes in haystack of length haystackNumBytes.
// Returns nullptr if whole needle not found or a pointer to the first found needle otherwise.
void* tMemsrch(void* haystack, int haystackNumBytes, void* needle, int needleNumBytes);
inline const void* tMemsrch(const void* haystack, int haystackNumBytes, const void* needle, int needleNumBytes)			{ return tMemsrch((void*)haystack, haystackNumBytes, (void*)needle, needleNumBytes); }

// For character strings we support ASCII and full unicode via UTF-8. We do not support either USC2 or UTF-16 except
// for providing conversion functions. The CT (Compile-Time) strlen variant below can compute the string length at
// compile-time for constant string literals.
// @todo Apparently in C++23 we will be getting char8_t variants for a lot of the string functions. Until then the
// ASCII versions work quite well in most cases for UTF-8 strings.
// For all these functions, char* represents an ASCII string while char8_t* a UTF-8 string. 
const int tCharInvalid																									= 0xFF;
inline int tStrcmp(const char* a, const char* b)																		{ tAssert(a && b); return strcmp(a, b); }
inline int tStrncmp(const char* a, const char* b, int n)																{ tAssert(a && b && n >= 0); return strncmp(a, b, n); }
inline int tStrcmp(const char8_t* a, const char8_t* b)																	{ tAssert(a && b); return strcmp((const char*)a, (const char*)b); }
inline int tStrncmp(const char8_t* a, const char8_t* b, int n)															{ tAssert(a && b && n >= 0); return strncmp((const char*)a, (const char*)b, n); }
#if defined(PLATFORM_WINDOWS)
inline int tStricmp(const char* a, const char* b)																		{ tAssert(a && b); return stricmp(a, b); }
inline int tStrnicmp(const char* a, const char* b, int n)																{ tAssert(a && b && n >= 0); return strnicmp(a, b, n); }
inline int tStricmp(const char8_t* a, const char8_t* b)																	{ tAssert(a && b); return stricmp((const char*)a, (const char*)b); }
inline int tStrnicmp(const char8_t* a, const char8_t* b, int n)															{ tAssert(a && b && n >= 0); return strnicmp((const char*)a, (const char*)b, n); }
#else
// @todo Why on non-windows did I need to use strcasecmp? strcasecmp is not part of the C standard.
inline int tStricmp(const char* a, const char* b)																		{ tAssert(a && b); return strcasecmp(a, b); }
inline int tStrnicmp(const char* a, const char* b, int n)																{ tAssert(a && b && n >= 0); return strncasecmp(a, b, n); }
inline int tStricmp(const char8_t* a, const char8_t* b)																	{ tAssert(a && b); return strcasecmp((const char*)a, (const char*)b); }
inline int tStrnicmp(const char8_t* a, const char8_t* b, int n)															{ tAssert(a && b && n >= 0); return strncasecmp((const char*)a, (const char*)b, n); }
#endif
inline int tStrlen(const char* s)																						{ tAssert(s); return int(strlen(s)); }
inline constexpr int tStrlenCT(const char* s)																			{ return *s ? 1 + tStrlenCT(s + 1) : 0; }
inline char* tStrcpy(char* dst, const char* src)																		{ tAssert(dst && src); return strcpy(dst, src); }
inline char* tStrncpy(char* dst, const char* src, int n)																{ tAssert(dst && src && n >= 0); return strncpy(dst, src, n); }
inline char* tStrchr(const char* s, int c)																				{ tAssert(s && c >= 0 && c < 0x100); return (char*)strchr(s, c); }
inline char* tStrstr(const char* s, const char* r)				/* Search s for r. */									{ tAssert(s && r); return (char*)strstr(s, r); }
inline char* tStrcat(char* s, const char* r)																			{ tAssert(s && r); return strcat(s, r); }

inline int tStrlen(const char8_t* s)																					{ tAssert(s); return int(strlen((const char*)s)); }
inline int tStrlen(const char16_t* s)																					{ tAssert(s); int c = 0; while (*s++) c++; return c; }
inline int tStrlen(const char32_t* s)																					{ tAssert(s); int c = 0; while (*s++) c++; return c; }
inline constexpr int tStrlenCT(const char8_t* s)																		{ return *s ? 1 + tStrlenCT(s + 1) : 0; }
inline char8_t* tStrcpy(char8_t* dst, const char8_t* src)																{ tAssert(dst && src); return (char8_t*)strcpy((char*)dst, (const char*)src); }
inline char8_t* tStrncpy(char8_t* dst, const char8_t* src, int n)														{ tAssert(dst && src && n >= 0); return (char8_t*)strncpy((char*)dst, (const char*)src, n); }
inline char8_t* tStrchr(const char8_t* s, int c)																		{ tAssert(s && c >= 0 && c < 0x100); return (char8_t*)strchr((const char*)s, c); }
inline char8_t* tStrstr(const char8_t* s, const char8_t* r)		/* Search s for r. */									{ tAssert(s && r); return (char8_t*)strstr((const char*)s, (const char*)r); }
inline char8_t* tStrcat(char8_t* s, const char8_t* r)																	{ tAssert(s && r); return (char8_t*)strcat((char*)s, (const char*)r); }

#if defined(PLATFORM_WINDOWS)
inline char* tStrupr(char* s)																							{ tAssert(s); return _strupr(s); }
inline char* tStrlwr(char* s)																							{ tAssert(s); return _strlwr(s); }
inline char8_t* tStrupr(char8_t* s)																						{ tAssert(s); return (char8_t*)_strupr((char*)s); }
inline char8_t* tStrlwr(char8_t* s)																						{ tAssert(s); return (char8_t*)_strlwr((char*)s); }
#else
inline char* tStrupr(char* s)																							{ tAssert(s); char* c = s; while (*c) { *c = toupper(*c); c++; } return s; }
inline char* tStrlwr(char* s)																							{ tAssert(s); char* c = s; while (*c) { *c = tolower(*c); c++; } return s; }
inline char8_t* tStrupr(char8_t* s)																						{ tAssert(s); char8_t* c = s; while (*c) { *c = toupper(*c); c++; } return s; }
inline char8_t* tStrlwr(char8_t* s)																						{ tAssert(s); char8_t* c = s; while (*c) { *c = tolower(*c); c++; } return s; }
#endif
inline char tToUpper(char c)																							{ return toupper(c); }
inline char tToLower(char c)																							{ return tolower(c); }

// For these conversion calls, unknown digit characters for the supplied base are ignored. If base is not E [2, 36], the
// base in which to interpret the string is determined by passing a prefix in the string. Base 10 is used if no specific
// prefix is found. Although these functions take in UTF-8 strings (chat8_t*), a well-formed source string will only
// include ASCII characters like digits, negative signs, prefix letters etc. Again, unknown characters are ignored, or,
// in the case of the 'strict' variants, cause the return value to be false.
// Base prefixes in use:
//
// Base 16 prefixes: x X 0x 0X #
// Base 10 prefixes: d D 0d 0D
// Base 8 prefixes: o O 0o 0O @
// Base 4  prefixes: n N 0n 0N (N for Nybble)
// Base 2  prefixes: b B 0b 0B !
//
// Negative/positive symbol may only be used with base 10 strings: eg. "d-769" or just "-769". The behaviour follows
// this recipe: If the base is invalid (not between 2 and 36) the first and maybe second characters are examined to
// determine base. Next, invalid characters are removed (implementation may just ignore them). Invalid characters
// include + and - for non base 10. Finally the conversion is performed. Valid digits for base 36 are 0-9, a-z, and A-Z.
// In all string-to-int functions, 0 is returned if there is no conversion. This can happen if all characters are
// invalid, the passed in string is null, or the passed in string is empty.
//
// If base is E (1, 36] AND a prefix is supplied, the supplied base is used. Using the prefix instead would be
// ambiguous. For example, "0x" is a valid base 36 number but "0x" is also a prefix. If you supply a prefix it must
// match the base or you will get undefined results. eg "0xff" or "xff" with base=36 interprets the 'x', correctly, as
// 33. "0xff" with base=16 works too because 'x' is an invalid hex digit and gets ignored. The '0' is leading so also
// does not interfere. The same behaviour holds for all prefixes and bases. Finally, do not use base=-1 on strings
// that are not either base 16, 10, 8, 4, or 2. eg. "0XF" is a valid base-34 number, and we wouldn't want it
// interpreted as base 16 is that's not what you intended.
template <typename IntegralType> IntegralType tStrtoiT(const char*, int base = -1);
inline int32 tStrtoi32(const char* s, int base = -1)																	{ return tStrtoiT<int32>(s, base); }
inline uint32 tStrtoui32(const char* s, int base = -1)																	{ return tStrtoiT<uint32>(s, base); }
inline int64 tStrtoi64(const char* s, int base = -1)																	{ return tStrtoiT<int64>(s, base); }
inline uint64 tStrtoui64(const char* s, int base = -1)																	{ return tStrtoiT<uint64>(s, base); }
inline int tStrtoui(const char* s, int base = -1)																		{ return tStrtoui32(s, base); }
inline int tStrtoi(const char* s, int base = -1)																		{ return tStrtoi32(s, base); }
inline int tAtoi(const char* s)									/* Base 10 only. Use tStrtoi for arbitrary base. */		{ return tStrtoi32(s, 10); }

template <typename IntegralType> IntegralType tStrtoiT(const char8_t*, int base = -1);
inline int32 tStrtoi32(const char8_t* s, int base = -1)																	{ return tStrtoiT<int32>(s, base); }
inline uint32 tStrtoui32(const char8_t* s, int base = -1)																{ return tStrtoiT<uint32>(s, base); }
inline int64 tStrtoi64(const char8_t* s, int base = -1)																	{ return tStrtoiT<int64>(s, base); }
inline uint64 tStrtoui64(const char8_t* s, int base = -1)																{ return tStrtoiT<uint64>(s, base); }
inline int tStrtoui(const char8_t* s, int base = -1)																	{ return tStrtoui32(s, base); }
inline int tStrtoi(const char8_t* s, int base = -1)																		{ return tStrtoi32(s, base); }
inline int tAtoi(const char8_t* s)								/* Base 10 only. Use tStrtoi for arbitrary base. */		{ return tStrtoi32(s, 10); }

// These are just variants of above that are strict. If the conversion encounters any parsing errors (all characters are
// invalid, the passed in string is null, or the passed in string is empty) instead of returning the default value
// IntegralType(0), false is returned.
template <typename IntegralType> bool tStrtoiT(IntegralType&, const char*, int base = -1);
inline bool tStrtoi32(int32& v, const char* s, int base = -1)															{ return tStrtoiT<int32>(v, s, base); }
inline bool tStrtoui32(uint32& v, const char* s, int base = -1)															{ return tStrtoiT<uint32>(v, s, base); }
inline bool tStrtoi64(int64& v, const char* s, int base = -1)															{ return tStrtoiT<int64>(v, s, base); }
inline bool tStrtoui64(uint64& v, const char* s, int base = -1)															{ return tStrtoiT<uint64>(v, s, base); }
inline bool tStrtoui(uint32& v, const char* s, int base = -1)															{ return tStrtoui32(v, s, base); }
inline bool tStrtoi(int& v, const char* s, int base = -1)																{ return tStrtoi32(v, s, base); }
inline bool tAtoi(int& v, const char* s)																				{ return tStrtoi32(v, s, 10); }

template <typename IntegralType> bool tStrtoiT(IntegralType&, const char8_t*, int base = -1);
inline bool tStrtoi32(int32& v, const char8_t* s, int base = -1)														{ return tStrtoiT<int32>(v, s, base); }
inline bool tStrtoui32(uint32& v, const char8_t* s, int base = -1)														{ return tStrtoiT<uint32>(v, s, base); }
inline bool tStrtoi64(int64& v, const char8_t* s, int base = -1)														{ return tStrtoiT<int64>(v, s, base); }
inline bool tStrtoui64(uint64& v, const char8_t* s, int base = -1)														{ return tStrtoiT<uint64>(v, s, base); }
inline bool tStrtoui(uint32& v, const char8_t* s, int base = -1)														{ return tStrtoui32(v, s, base); }
inline bool tStrtoi(int& v, const char8_t* s, int base = -1)															{ return tStrtoi32(v, s, base); }
inline bool tAtoi(int& v, const char8_t* s)																				{ return tStrtoi32(v, s, 10); }

// String to bool. Case insensitive. Interprets "true", "t", "yes", "y", "on", "enable", "enabled", "1", "+", and
// strings that represent non-zero integers as boolean true. Otherwise false.
bool tStrtob(const char*);
inline bool tStrtob(const char8_t* s)																					{ return tStrtob((const char*)s); }

// These are both base 10 only. They return 0.0f (or 0.0) if there is no conversion. They also handle converting an
// optional binary representation in the string -- if it contains a hash(#) and the next 8 (or 16) digits are valid
// hex digits, they are interpreted as the binary IEEE 754 floating point rep directly. This stops 'wobble' when
// serializing/deserializing from disk multiple times as would be present in the approximate base 10 representations.
// Valid tStrtof example input stings include: "45.838#FADD23BB", and "45.838".
float tStrtof(const char*);
inline float tStrtof(const char8_t* s)																					{ return tStrtof((const char*)s); }
double tStrtod(const char*);
inline double tStrtod(const char8_t* s)																					{ return tStrtod((const char*)s); }

// These are synonyms of the above two functions.
inline float tAtof(const char* s)																						{ return tStrtof(s); }
inline double tAtod(const char* s)																						{ return tStrtod(s); }
inline float tAtof(const char8_t* s)																					{ return tStrtof(s); }
inline double tAtod(const char8_t* s)																					{ return tStrtod(s); }

// Here are the functions for going from integral types to strings. strSize (as opposed to length) must include
// enough room for the terminating null. strSize should be the full size of the passed-in str buffer. The resulting
// string, if letter characters are called for (bases > 10) will be capital, not lower case. Base must be E [2, 36].
// Conversion problems boil down to passing null str, strSize being too small, and specifying an out-of-bounds base.
// Returns false in these cases, and true on success. The int-to-string functions are mainly available to handle
// arbitrary base E (2, 36] since tsPrintf only handles octal, decimal, and hex.
//
// Floating point conversions to strings should be handled by the tPrintf-style functions due to their superior
// formatting specifiers. There are helper functions that convert float/double to strings. Due to the complexity of
// converting to a string, these functions are found in the System module rather than foundation. Examples of these
// functions are: tFtoa, tDtoa, tFtostr, and tDtostr.
template <typename IntegralType> bool tItostrT(char* str, int strSize, IntegralType value, int base = 10);
inline bool tItostr(char* str, int strSize, int32 value, int base = 10)													{ return tItostrT<int32>(str, strSize, value, base); }
inline bool tItostr(char* str, int strSize, int64 value, int base = 10)													{ return tItostrT<int64>(str, strSize, value, base); }
inline bool tItostr(char* str, int strSize, uint32 value, int base = 10)												{ return tItostrT<uint32>(str, strSize, value, base); }
inline bool tItostr(char* str, int strSize, uint64 value, int base = 10)												{ return tItostrT<uint64>(str, strSize, value, base); }
inline bool tItoa(char* str, int strSize, int32 value, int base = 10)													{ return tItostr(str, strSize, value, base); }
inline bool tItoa(char* str, int strSize, int64 value, int base = 10)													{ return tItostr(str, strSize, value, base); }
inline bool tItoa(char* str, int strSize, uint32 value, int base = 10)													{ return tItostr(str, strSize, value, base); }
inline bool tItoa(char* str, int strSize, uint64 value, int base = 10)													{ return tItostr(str, strSize, value, base); }

template <typename IntegralType> bool tItostrT(char8_t* str, int strSize, IntegralType value, int base = 10);
inline bool tItostr(char8_t* str, int strSize, int32 value, int base = 10)												{ return tItostrT<int32>(str, strSize, value, base); }
inline bool tItostr(char8_t* str, int strSize, int64 value, int base = 10)												{ return tItostrT<int64>(str, strSize, value, base); }
inline bool tItostr(char8_t* str, int strSize, uint32 value, int base = 10)												{ return tItostrT<uint32>(str, strSize, value, base); }
inline bool tItostr(char8_t* str, int strSize, uint64 value, int base = 10)												{ return tItostrT<uint64>(str, strSize, value, base); }
inline bool tItoa(char8_t* str, int strSize, int32 value, int base = 10)												{ return tItostr(str, strSize, value, base); }
inline bool tItoa(char8_t* str, int strSize, int64 value, int base = 10)												{ return tItostr(str, strSize, value, base); }
inline bool tItoa(char8_t* str, int strSize, uint32 value, int base = 10)												{ return tItostr(str, strSize, value, base); }
inline bool tItoa(char8_t* str, int strSize, uint64 value, int base = 10)												{ return tItostr(str, strSize, value, base); }

// Unicode encoding/decoding.
//
// Some handy exposed UTF codepoints. Remember, const defaults to internal linkage in C++.
const char32_t cCodepoint_Replacement		= 0x0000FFFD;	// U+FFFD Used for unknown or invalid encodings.
const char32_t cCodepoint_SpecialNonCharA	= 0x0000FFFE;	// U+FFFE Guaranteed not a valid character.
const char32_t cCodepoint_SpecialNonCharB	= 0x0000FFFF;	// U+FFFF Guaranteed not a valid character.
const char32_t cCodepoint_BOM				= 0x0000FEFF;	// U+FEFF Bute order marker. If bytes reversed in file it's little endian and value will be cCodepoint_SpecialNonCharA.

// These functions convert to/from the 3 main unicode encodings. Note that most text in Tacent is
// assumed to be UTF-8. These are provided so external or OS-specific calls can be made when they expect non-UTF-8
// input, and when results are supplied, converted back to UTF-8.
//
// Null termination is not part of UTF encoding. These work on arrays of codeunits (charN types). Not null terminated.
// 1) If (only) dst is nullptr, returns the exact number of codeunits (charNs) needed for dst.
// 2) If src is nullptr, dst is ignored and length is used to return a fast, worst-case number of codeunits (charNs)
//    needed for dst assuming no overlong encoding. This second method is fast because the contents of src are not
//    inspected, but it often gives conservative (larger) results.
// 3) If all args are valid, converts the UTFn src data to the dst UTFn encoding. Returns the number of dst codeunits
//    (charNs) written.
// Caller is responsibe for making sure dst is big enough! Length is not the number of codepoints, it is the number of
// codeunits (charNs) provided by the src pointer.
int tUTF8 (char8_t*  dst, const char16_t* src, int length);		// UFT-16 to UTF-8.
int tUTF8 (char8_t*  dst, const char32_t* src, int length);		// UFT-32 to UTF-8.
int tUTF16(char16_t* dst, const char8_t*  src, int length);		// UFT-8  to UTF-16.
int tUTF16(char16_t* dst, const char32_t* src, int length);		// UFT-32 to UTF-16.
int tUTF32(char32_t* dst, const char8_t*  src, int length);		// UFT-8  to UTF-32.
int tUTF32(char32_t* dst, const char16_t* src, int length);		// UFT-16 to UTF-32.

// String termination is not part of UTF encoding. These functions assume null terminated strings are input as src.
// 1) If dst (only) is nullptr, computes and returns the exact number of dst codeunits (charNs) needed -- not including
//    the null terminator). If you are going to new the memory yourself, add one for the null terminator.
// 2) If both src and dst valid, converts the UTFn string in src to the dst UTFn encoding. Returns length the of dst
//    in codeunits not including the null. Note that the null terminator _is_ written to the dst.
// 3) If src is nullptr, returns 0 and _does not modify_ dest at all even to add a null terminator.
int tUTF8s (char8_t*  dst, const char16_t* src);				// UTF-16 to UTF-8.
int tUTF8s (char8_t*  dst, const char32_t* src);				// UFT-32 to UTF-8.
int tUTF16s(char16_t* dst, const char8_t*  src);				// UTF-8  to UTF-16.
int tUTF16s(char16_t* dst, const char32_t* src);				// UTF-32 to UTF-16.
int tUTF32s(char32_t* dst, const char8_t*  src);				// UTF-8  to UTF-32.
int tUTF32s(char32_t* dst, const char16_t* src);				// UTF-16 to UTF-32.

// Individual codepoint functions. If you want to convert to a single codepoint WITHOUT having a null terminator written
// the tUTFns functions above don't work (they write the terminator) and the tUTFn functions above are inconvenient
// since you'd also need to provide the length.
//
// These tUTFnc functions fill the gap. They can take either a UTF-8/16/32 codeunit array (null terminated or not) and
// return a single UTF-32 encoded codepoint (and do not need the length as input). They can also take a UTF32 codepoint
// and convert to UTF-8/16/32 codeunit array without writing the null terminator. You can compose the two styles.
//
// The versions that take the codepoint as input may be used with a 'C++ character literal' like U'𝒞'.
// Note that C++ character literals are constrained to what is representable by a single code unit. u8'𝒞' is not
// well-formed in C++ (even though it says u8, it does not mean you can put any unicode character in there). Basically
// u8'' must have an ASCII character, u'' must have a character in the BMP, and U can have pretty much anything. For
// this reason, there's not much point supporting u8 and u when U does it all. This is different than C++ STRING
// literals, where u8"wΔ𝒞", u"wΔ𝒞", and U"wΔ𝒞" are all fine. (FYI w is ASCII, Δ is in the BMP. 𝒞 is in an astral plane.)
//
// eg. tUTF8c(dst,  U'𝒞')			 will return 4 and write 4 char8s into dst.
// eg. tUTF32c(u8"𝒞")				 will return the UTF-32 codepoint for 𝒞.
// eg. tUTF32c(U"𝒞")				 will also return the UTF-32 codepoint for 𝒞.
// eg. tUTF8c(dst, tUTF32c(u"Δ"))	will convert the UTF-16 encoding of Δ to a UTF-8 (non-null-terminated) array
//									and return how many UTF-8 code units are in the array.
//
// These read codeunit arrays and return a single codepoint in UTF-32. Special replacement returned on error. An error
// is either an invalid encoding OR when src is nullptr. Length is not needed as it's implicit in the encoding.
char32_t tUTF32c(const char8_t*  src);				// Reads 1 to 4 char8 codeunits from src.
char32_t tUTF32c(const char16_t* src);				// Reads 1 or 2(surrogtate) char16 codeunits from src.
char32_t tUTF32c(const char32_t* src);				// Reads 1 char32 codeunit from src.

// These take a codepoint in UTF-32 (src) and write to the dst array without adding a null-terminator. If dst is nullptr
// returns 0. If src is invalid, dst receives the special replacement. Returns num charNs written. The size hints in the
// arrays are worst case amounts of room you may need. If you want a null terminated string after conversion (with the
// single codepoint in it), make dst 1 bigger than suggested and set the Nth charN to 0, where N is the value returned.
int tUTF8c (char8_t  dst[4], char32_t src);			// Reads src codepoint and returns [0,4].
int tUTF16c(char16_t dst[2], char32_t src);			// Reads src codepoint and returns [0,2].
int tUTF32c(char32_t dst[1], char32_t src);			// Reads src codepoint and returns [0,1].


// These are non UTF-8 functions that work on individual ASCII characters or ASCII strings. tStrrev, for example,
// simply reverses the chars, it is not aware of UFT-8 surrogates and would mess them up.
inline bool tIsspace(char c)																							{ return isspace(int(c)) ? true : false; }
inline bool tIsdigit(char c)																							{ return isdigit(int(c)) ? true : false; }
inline bool tIsbdigit(char c)																							{ return ((c == '0') || (c == '1')) ? true : false; }
inline bool tIsodigit(char c)																							{ return ((c >= '0') && (c <= '7')) ? true : false; }
inline bool tIsxdigit(char c)																							{ return isxdigit(int(c)) ? true : false; }
inline bool tIsalpha(char c)																							{ return isalpha(int(c)) ? true : false; }
inline bool tIscntrl(char c)																							{ return iscntrl(int(c)) ? true : false; }
inline bool tIsalnum(char c)																							{ return isalnum(int(c)) ? true : false; }
inline bool tIsprint(char c)																							{ return isprint(int(c)) ? true : false; }
inline bool tIspunct(char c)																							{ return ispunct(int(c)) ? true : false; }
inline bool tIslower(char c)																							{ return islower(int(c)) ? true : false; }
inline bool tIsupper(char c)																							{ return isupper(int(c)) ? true : false; }
inline bool tIsHexDigit(char d)																							{ return ((d >= 'a' && d <= 'f')||(d >= 'A' && d <= 'F')||(d >= '0' && d <= '9')); }
void tStrrev(char* begin, char* end);

// These functions return an unchanged character if the input is not an alphabetic character.
inline char tChrlwr(char c)																								{ return tIsupper(c) ? c + ('a' - 'A') : c; }
inline char tChrupr(char c)																								{ return tIslower(c) ? c - ('a' - 'A') : c; }

// NAN means not a number. P for positive. N for negative. I for indefinite. S for signaling. Q for quiet.
enum class tFloatType
{
	NORM,							// A normal floating point value (normalized or denormalized). Must be first type.
	FIRST_SPECIAL,
	FIRST_NAN		= FIRST_SPECIAL,
	PSNAN			= FIRST_NAN,	// Positive Signaling Not-A-Number. This must be the first NAN type.
	NSNAN,							// Negative Signaling Not-A-Number.
	PQNAN,							// Positive non-signaling (quiet) Not-A-Number (QNAN).
	NQNAN,							// Negative non-signaling (quiet) Not-A-Number (QNAN). Must be the last NAN type.
	IQNAN,							// Indefinite QNAN. For AMD64 processors only a single NQNAN is used for Indefinite. 
	LAST_NAN		= IQNAN,
	PINF,							// Positive INFinity.
	NINF,							// Negative INFinity.
	LAST_SPECIAL	= NINF
};
tFloatType tGetFloatType(float);	// Single precision get float type.
tFloatType tGetFloatType(double);	// Double precision get float type.
inline bool tIsNAN(float v)																								{ tFloatType t = tGetFloatType(v); return ((int(t) >= int(tFloatType::FIRST_NAN)) && (int(t) <= int(tFloatType::LAST_NAN))) ? true : false; }
inline bool tIsNAN(double v)																							{ tFloatType t = tGetFloatType(v); return ((int(t) >= int(tFloatType::FIRST_NAN)) && (int(t) <= int(tFloatType::LAST_NAN))) ? true : false; }
inline bool tIsSpecial(float v)																							{ tFloatType t = tGetFloatType(v); return ((int(t) >= int(tFloatType::FIRST_SPECIAL)) && (int(t) <= int(tFloatType::LAST_SPECIAL))) ? true : false; }
inline bool tIsSpecial(double v)																						{ tFloatType t = tGetFloatType(v); return ((int(t) >= int(tFloatType::FIRST_SPECIAL)) && (int(t) <= int(tFloatType::LAST_SPECIAL))) ? true : false; }
inline float tFrexp(float arg, int* exp)																				{ return frexpf(arg, exp); }

// Examples of non-NORM float types. These are only examples as in many cases there are multiple bitpatterns for the
// same tFloatType. For example a PSNAN can have any bitpattern between 0x7F800001 and 0x7FBFFFFF (inclusive).
// P for positive. N for negative. Q for quiet. S for signalling.
inline float tFloatPSNAN()																								{ union LU { float Nan; uint32 B; } v; v.B = 0x7F800001; return v.Nan; }
inline float tFloatNSNAN()																								{ union LU { float Nan; uint32 B; } v; v.B = 0xFF800001; return v.Nan; }
inline float tFloatPQNAN()																								{ union LU { float Nan; uint32 B; } v; v.B = 0x7FC00000; return v.Nan; }
inline float tFloatIQNAN()																								{ union LU { float Nan; uint32 B; } v; v.B = 0xFFC00000; return v.Nan; }
inline float tFloatNQNAN()																								{ union LU { float Nan; uint32 B; } v; v.B = 0xFFC00001; return v.Nan; }
inline float tFloatPINF()																								{ union LU { float Inf; uint32 B; } v; v.B = 0x7F800000; return v.Inf; }
inline float tFloatNINF()																								{ union LU { float Inf; uint32 B; } v; v.B = 0xFF800000; return v.Inf; }

inline double tDoublePSNAN()																							{ union LU { double Nan; uint64 B; } v; v.B = 0x7FF0000000000001ULL; return v.Nan; }
inline double tDoubleNSNAN()																							{ union LU { double Nan; uint64 B; } v; v.B = 0xFFF0000000000001ULL; return v.Nan; }
inline double tDoublePQNAN()																							{ union LU { double Nan; uint64 B; } v; v.B = 0x7FF8000000000000ULL; return v.Nan; }
inline double tDoubleIQNAN()																							{ union LU { double Nan; uint64 B; } v; v.B = 0xFFF8000000000000ULL; return v.Nan; }
inline double tDoubleNQNAN()																							{ union LU { double Nan; uint64 B; } v; v.B = 0xFFF8000000000001ULL; return v.Nan; }
inline double tDoublePINF()																								{ union LU { double Inf; uint64 B; } v; v.B = 0x7FF0000000000000ULL; return v.Inf; }
inline double tDoubleNINF()																								{ union LU { double Inf; uint64 B; } v; v.B = 0xFFF0000000000000ULL; return v.Inf; }

// These ASCII separators may be used for things like replacing characters in strings for subsequent manipulation.
const char SeparatorSub																									= 26;	// 0x1A
const char SeparatorFile																								= 28;	// 0x1C
const char SeparatorGroup																								= 29;	// 0x1D
const char SeparatorRecord																								= 30;	// 0x1E
const char SeparatorUnit																								= 31;	// 0x1F

const char SeparatorA																									= SeparatorUnit;
const char SeparatorB																									= SeparatorRecord;
const char SeparatorC																									= SeparatorGroup;
const char SeparatorD																									= SeparatorFile;
const char SeparatorE																									= SeparatorSub;

// The null-terminated string versions of the separators come as ASCII strings and UTF-8. Since all ASCII strings are
// valid UTF-8 strings, the values are the same, it's just the type (char8_t*) that's different.
extern const char* SeparatorSubStr;
extern const char* SeparatorFileStr;
extern const char* SeparatorGroupStr;
extern const char* SeparatorRecordStr;
extern const char* SeparatorUnitStr;
extern const char* SeparatorAStr;
extern const char* SeparatorBStr;
extern const char* SeparatorCStr;
extern const char* SeparatorDStr;
extern const char* SeparatorEStr;

extern const char8_t* u8SeparatorSubStr;
extern const char8_t* u8SeparatorFileStr;
extern const char8_t* u8SeparatorGroupStr;
extern const char8_t* u8SeparatorRecordStr;
extern const char8_t* u8SeparatorUnitStr;
extern const char8_t* u8SeparatorAStr;
extern const char8_t* u8SeparatorBStr;
extern const char8_t* u8SeparatorCStr;
extern const char8_t* u8SeparatorDStr;
extern const char8_t* u8SeparatorEStr;


}


// Implementation below this line.


template<typename IntegralType> inline IntegralType tStd::tStrtoiT(const char* str, int base)
{
	return tStd::tStrtoiT<IntegralType>((const char8_t*)str, base);
}


template<typename IntegralType> inline IntegralType tStd::tStrtoiT(const char8_t* str, int base)
{
	if (!str || (*str == '\0'))
		return IntegralType(0);

	int len = tStrlen(str);
	const char8_t* start = str;
	const char8_t* end = str + len - 1;

	if ((base < 2) || (base > 36))
		base = -1;

	if (base == -1)
	{
		// Base is -1. Need to determine based on prefix.
		if ((len > 1) && (*start == '0'))
			start++;

		if ((*start == 'x') || (*start == 'X') || (*start == '#'))
			base = 16;
		else if ((*start == 'd') || (*start == 'D'))
			base = 10;
		else if ((*start == 'o') || (*start == 'O') || (*start == '@'))
			base = 8;
		else if ((*start == 'n') || (*start == 'N'))
			base = 4;
		else if ((*start == 'b') || (*start == 'B') || (*start == '!'))
			base = 2;

		if (base == -1)
			base = 10;
		else
			start++;
	}

	IntegralType val = 0;
	IntegralType colVal = 1;
	for (const char8_t* curr = end; curr >= start; curr--)
	{
		if ((*curr == '-') && (base == 10))
		{
			val = -val;
			continue;
		}

		int digVal = -1;
		if (*curr >= '0' && *curr <= '9')
			digVal = *curr - '0';
		else if (*curr >= 'a' && *curr <= 'z')
			digVal = 10 + (*curr - 'a');
		else if (*curr >= 'A' && *curr <= 'Z')
			digVal = 10 + (*curr - 'A');

		if ((digVal == -1) || (digVal > base-1))
			continue;

		val += IntegralType(digVal)*colVal;
		colVal *= IntegralType(base);
	}

	return val;
}


template<typename IntegralType> inline bool tStd::tStrtoiT(IntegralType& val, const char8_t* str, int base)
{
	val = 0;
	if (!str || (*str == '\0'))
		return false;

	int len = tStrlen(str);
	const char8_t* start = str;
	const char8_t* end = str + len - 1;
	bool negate = false;

	// If the number starts with a '-', before the base modifier, it should be applied
	if((*start == '-'))
	{
		negate = true;
		start++;
	}

	if ((base < 2) || (base > 36))
		base = -1;

	if (base == -1)
	{
		// Base is -1. Need to determine based on prefix.
		if ((len > 1) && (*start == '0'))
			start++;

		if ((*start == 'x') || (*start == 'X') || (*start == '#'))
			base = 16;
		else if ((*start == 'd') || (*start == 'D'))
			base = 10;
		else if ((*start == 'o') || (*start == 'O') || (*start == '@'))
			base = 8;
		else if ((*start == 'n') || (*start == 'N'))
			base = 4;
		else if ((*start == 'b') || (*start == 'B') || (*start == '!'))
			base = 2;

		if (base == -1)
			base = 10;
		else
			start++;
	}

	IntegralType colVal = 1;
	for (const char8_t* curr = end; curr >= start; curr--)
	{
		if ((curr == start) && (*curr == '-'))
		{
			// a '-' after the base specifier or a double minus in base 10 is an error
			if(negate || base != 10)
			{
				val = 0;
				return false;
			}

			val = -val;
			continue;
		}

		int digVal = -1;
		if (*curr >= '0' && *curr <= '9')
			digVal = *curr - '0';
		else if (*curr >= 'a' && *curr <= 'z')
			digVal = 10 + (*curr - 'a');
		else if (*curr >= 'A' && *curr <= 'Z')
			digVal = 10 + (*curr - 'A');

		if ((digVal == -1) || (digVal > base-1))
		{
			val = 0;
			return false;
		}

		val += IntegralType(digVal)*colVal;
		colVal *= IntegralType(base);
	}

	if(negate)
		val = -val;

	return true;
}


template<typename IntegralType> inline bool tStd::tItostrT(char* str, int strSize, IntegralType value, int base)
{
	return tStd::tItostrT<IntegralType>((char8_t*)str, strSize, value, base);
}


template<typename IntegralType> inline bool tStd::tItostrT(char8_t* str, int strSize, IntegralType value, int base)
{
	if (!str || strSize <= 0)
		return false;

	static char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	// Validate base.
	if ((base < 2) || (base > sizeof(digits)/sizeof(*digits)))
		return false;

	// Take care of sign.
	IntegralType sign = value;
	if (value < 0)
		value = -value;

	// Conversion. Number is reversed.
	char8_t* s = str;
	int numWritten = 0;
	int remainder;
	IntegralType quotient;
	do
	{
		remainder = value % base;
		tAssert((remainder < base) && (remainder >= 0));
		quotient = value / base;
		if (numWritten < strSize)
			*s++ = digits[remainder];
		else
			return false;
		numWritten++;
	} while ((value = quotient));

	if (sign < 0)
	{
		if (numWritten < strSize)
			*s++='-';
		else
			return false;
	}

	if (numWritten < strSize)
		*s = '\0';
	else
		return false;

	// The input 
	tStrrev((char*)str, (char*)(s-1));
	return true;
}


inline tStd::tFloatType tStd::tGetFloatType(float v)
{
	uint32 b = *((uint32*)&v);
		
	if ((b >= 0x7F800001) && (b <= 0x7FBFFFFF))
		return tFloatType::PSNAN;

	if ((b >= 0xFF800001) && (b <= 0xFFBFFFFF))
		return tFloatType::NSNAN;

	if ((b >= 0x7FC00000) && (b <= 0x7FFFFFFF))
		return tFloatType::PQNAN;

	if (b == 0xFFC00000)
		return tFloatType::IQNAN;

	if ((b >= 0xFFC00001) && (b <= 0xFFFFFFFF))
		return tFloatType::NQNAN;

	if (b == 0x7F800000)
		return tFloatType::PINF;

	if (b == 0xFF800000)
		return tFloatType::NINF;

	return tFloatType::NORM;
}


inline tStd::tFloatType tStd::tGetFloatType(double v)
{
	uint64 b = *((uint64*)&v);

	if ((b >= 0x7FF0000000000001ULL) && (b <= 0x7FF7FFFFFFFFFFFFULL))
		return tFloatType::PSNAN;

	if ((b >= 0xFFF0000000000001ULL) && (b <= 0xFFF7FFFFFFFFFFFFULL))
		return tFloatType::NSNAN;

	if ((b >= 0x7FF8000000000000ULL) && (b <= 0x7FFFFFFFFFFFFFFFULL))
		return tFloatType::PQNAN;

	if (b == 0xFFF8000000000000ULL)
		return tFloatType::IQNAN;

	if ((b >= 0xFFF8000000000001ULL) && (b <= 0xFFFFFFFFFFFFFFFFULL))
		return tFloatType::NQNAN;

	if (b == 0x7FF0000000000000ULL)
		return tFloatType::PINF;

	if (b == 0xFFF0000000000000ULL)
		return tFloatType::NINF;

	return tFloatType::NORM;
}
