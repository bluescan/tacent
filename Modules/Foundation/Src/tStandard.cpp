// tStandard.cpp
//
// Tacent functions and types that are standard across all platforms. Includes global functions like itoa which are not
// available on some platforms, but are common enough that they should be.
//
// Copyright (c) 2004-2006, 2015, 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <stdlib.h>
#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif
#include "Foundation/tStandard.h"
#include "Foundation/tString.h"
#pragma warning (disable: 4146)
#pragma warning (disable: 4018)


const char* tStd::SeparatorSubStr							= "\x1a";
const char* tStd::SeparatorFileStr							= "\x1c";
const char* tStd::SeparatorGroupStr							= "\x1d";
const char* tStd::SeparatorRecordStr						= "\x1e";
const char* tStd::SeparatorUnitStr							= "\x1f";
const char* tStd::SeparatorAStr								= tStd::SeparatorUnitStr;
const char* tStd::SeparatorBStr								= tStd::SeparatorRecordStr;
const char* tStd::SeparatorCStr								= tStd::SeparatorGroupStr;
const char* tStd::SeparatorDStr								= tStd::SeparatorFileStr;
const char* tStd::SeparatorEStr								= tStd::SeparatorSubStr;
const char8_t* tStd::u8SeparatorSubStr						= (const char8_t*)tStd::SeparatorSubStr;
const char8_t* tStd::u8SeparatorFileStr						= (const char8_t*)tStd::SeparatorFileStr;
const char8_t* tStd::u8SeparatorGroupStr					= (const char8_t*)tStd::SeparatorGroupStr;
const char8_t* tStd::u8SeparatorRecordStr					= (const char8_t*)tStd::SeparatorRecordStr;
const char8_t* tStd::u8SeparatorUnitStr						= (const char8_t*)tStd::SeparatorUnitStr;
const char8_t* tStd::u8SeparatorAStr						= (const char8_t*)tStd::SeparatorAStr;
const char8_t* tStd::u8SeparatorBStr						= (const char8_t*)tStd::SeparatorBStr;
const char8_t* tStd::u8SeparatorCStr						= (const char8_t*)tStd::SeparatorCStr;
const char8_t* tStd::u8SeparatorDStr						= (const char8_t*)tStd::SeparatorDStr;
const char8_t* tStd::u8SeparatorEStr						= (const char8_t*)tStd::SeparatorEStr;


void* tStd::tMemsrch(void* haystack, int haystackNumBytes, void* needle, int needleNumBytes)
{
	if ((haystackNumBytes <= 0) || (needleNumBytes <= 0) || (haystackNumBytes < needleNumBytes))
		return nullptr;

	// Serach for the pattern from the first haystack byte (0) to numNeedleBytes from the end. For example, if we are
	// seraching for 4 bytes in 8, there will be 5 mem compares of 4 bytes each.
	for (int i = 0; i <= haystackNumBytes-needleNumBytes; i++)
	{
		if (tMemcmp((uint8*)haystack + i, needle, needleNumBytes) == 0)
			return (uint8*)haystack + i;
	}

	return nullptr;
}


bool tStd::tStrtob(const char* str)
{
	tString lower(str);
	lower.ToLower();

	if
	(
		(lower == "true") || (lower == "t") ||
		(lower == "yes") || (lower == "y") ||
		(lower == "on") || (lower == "1") || (lower == "+") ||
		(lower == "enable") || (lower == "enabled") || (tStrtoi(str) != 0)
	)
		return true;
	else
		return false;
}


float tStd::tStrtof(const char* s)
{
	char* hash = tStrchr(s, '#');
	if (hash && (tStrlen(hash+1) == 8))
	{
		uint32 bin = tStd::tStrtoui32(hash+1, 16);
		return *((float*)(&bin));
	}

	return float( tStrtod(s) );
}


double tStd::tStrtod(const char* s)
{
	int l = tStrlen(s);
	if (!l)
		return 0.0;

	char* hash = tStrchr(s, '#');
	if (hash && (tStrlen(hash+1) == 16))
	{
		uint64 bin = tStrtoui64(hash+1, 16);
		return *((double*)&bin);
	}

	// This error checking is essential. Sometimes NANs are written in text format to a string.
	// Like "nan(snan)". We want these to evaluate to 0.0, not -1 or something else. We allow
	// 'e' and 'E' for numbers in exponential form like 3.09E08.
	for (int i = 0; i < l; i++)
	{
		char ch = s[i];
		if
		(
			((ch >= 'a') && (ch <= 'z') && (ch != 'e')) ||
			((ch >= 'A') && (ch <= 'Z') && (ch != 'E'))
		)
			return 0.0;
	}

	// Will be 0.0 if there was a problem.
	return strtod(s, nullptr);
}


void tStd::tStrrev(char* begin, char* end)
{	
	char aux;
	while (end > begin)
		aux = *end, *end-- = *begin, *begin++ = aux;
}


// The UTF 8 <-> 16 conversion code below was based on https://github.com/Davipb/utf8-utf16-converter
// under the MIT licence. See Docs/Licence_Utf8Utf16.txt
namespace tUTF
{
	// BMP = Basic Multilingual Plane.
	// CP = Unicode codepoint.
	const char32_t cCodepoint_LastValidBMP		= 0x0000FFFD;	// Last valid codepoint. Note that U+FFFF and U+FFFE are guaranteed 'non-characters'. They do not appear if codepoint is valid.
	const char32_t cCodepoint_UnicodeMax		= 0x0010FFFF;	// The highest valid Unicode codepoint.
	const char32_t cCodepoint_UTF8Max1			= 0x0000007F;	// The highest codepoint that can be encoded with 1 byte  in UTF-8.
	const char32_t cCodepoint_UTF8Max2			= 0x000007FF;	// The highest codepoint that can be encoded with 2 bytes in UTF-8.
	const char32_t cCodepoint_UTF8Max3			= 0x0000FFFF;	// The highest codepoint that can be encoded with 3 bytes in UTF-8.

	const char16_t cSurrogate_GenericMask16		= 0xF800;		// The mask to apply before testing it against cSurrogate_GenericVal16
	const char16_t cSurrogate_GenericVal16		= 0xD800;		// If masked with cSurrogate_GenericMask16, matches this value, it is a surrogate.
	const char32_t cSurrogate_GenericMask32		= 0x0000F800;	// The mask to apply before testing it against cSurrogate_GenericVal32
	const char32_t cSurrogate_GenericVal32		= 0x0000D800;	// If masked with cSurrogate_GenericMask32, matches this value, it is a surrogate.

	const char16_t cSurrogate_Mask16			= 0xFC00;		// The mask to apply to a character before testing it against cSurrogate_HighVal16 or cSurrogate_LowVal16.
	const char16_t cSurrogate_HighVal16			= 0xD800;		// If a character, masked with cSurrogate_Mask16, matches this value, it is a high surrogate.
	const char16_t cSurrogate_LowVal16			= 0xDC00;		// If a character, masked with cSurrogate_Mask16, matches this value, it is a low surrogate.

	const char16_t cSurrogate_CodepointMask16	= 0x03FF;		// A mask that can be applied to a surrogate to extract the codepoint value contained in it.
	const char32_t cSurrogate_CodepointMask32	= 0x000003FF;	// A mask that can be applied to a surrogate to extract the codepoint value contained in it.
	const int      cSurrogate_CodepointBits		= 10;			// The number of LS bits of cSurrogate_CodepointMask that are set.
	const char32_t cSurrogate_CodepointOffset	= 0x00010000;	// The value that is subtracted from a codepoint before encoding it in a surrogate pair.

	const char8_t  cContinuation_UTF8Mask		= 0xC0;			// The mask to a apply to a character before testing it against cContinuation_UTF8Val
	const char8_t  cContinuation_UTF8Val		= 0x80;			// If a character, masked with cContinuation_UTF8Mask, matches this value, it is a UTF-8 continuation byte.
	const int      cContinuation_CodepointBits	= 6;			// The number of bits of a codepoint that are contained in a UTF-8 continuation byte.

	// A UTF-8 bit-pattern that can be set or verified.
	struct UTF8Pattern
	{
		char8_t Mask;		// The mask that should be applied to the character before testing it.
		char8_t Value;		// The value that the character should be tested against after applying the mask.
	};

	// Bit-patterns for leading bytes in a UTF-8 codepoint encoding. Each pattern represents the leading byte for a
	// character encoded with N UTF-8 bytes where N is the index + 1.
	static const UTF8Pattern UTF8LeadingBytes[] =
	{
		{ 0x80, 0x00 },		// 0xxxxxxx
		{ 0xE0, 0xC0 },		// 110xxxxx
		{ 0xF0, 0xE0 },		// 1110xxxx
		{ 0xF8, 0xF0 } 		// 11110xxx
	};
	const int UTF8LeadingBytes_NumElements = tNumElements(UTF8LeadingBytes);

	// Calculates the number of UTF-16 16-bit characters it would take to encode a codepoint. The codepoint is not
	// checked for validity. That should be done beforehand.
	int CalculateUtf16Length(char32_t codepoint);

	// Gets a single codepoint from a UTF-16 string (string does not need null-termination). Returns how many char16s
	// were read to generate the codepoint. If 2 char16s (surrogate pairs) were read, returns 2. Otherwise returns 1.
	// For invalid encodings, the codepoint is set to the special 'replacement' (from the BMP) and 1 is returned.
	int DecodeUtf16(char32_t& codepoint, const char16_t* src);

	// Encodes a 32-bit codepoint into a UTF-16 string. The codepoint is not checked for validity by this function. You
	// must ensure the dst buffer is big enough -- 2 is always big enough, but you can call CalculateUtf16Length to get
	// an exact size. Returns the number of char16s written to dst [0,2]. Returns 0 is dst is nullptr.
	int EncodeUtf16(char16_t* dst, char32_t codepoint);

	// Calculates the number of UTF-8 8-bit chars it would take to encode a codepoint. The codepoint is not checked
	// for validity. That should be done beforehand.
	int CalculateUtf8Length(char32_t codepoint);

	// Gets a single codepoint from a UTF-8 string (string does not need null-termination). Returns how many char8s
	// were read to generate the codepoint. eg. If 3 char8s (surrogates) were read, returns 3.
	// For invalid encodings, the codepoint is set to the special 'replacement' num bytes read from src is returned.
	int DecodeUtf8(char32_t& codepoint, const char8_t* src);

	// Encodes a 32-bit codepoint into a UTF-8 string. The codepoint is not checked for validity by this function. You
	// must ensure the dst buffer is big enough -- 4 is always big enough, but you can call CalculateUtf8Length to get
	// an exact size. Returns the number of char8s written to dst [0,4]. Returns 0 is dst is nullptr.
	int EncodeUtf8(char8_t* dst, char32_t codepoint);
};


int tStd::tUTF8(char8_t* dst, const char16_t* src, int srcLen)
{
	// Compute fast worst-case size needed.
	// UTF-8 can use up to 3 bytes to encode some codepoints in the BMP (Basic Multilingual Plane). This has
	// implications for how much room UTF-8 encoded text could take up from src data that's UTF-16. Eg. 2 char16s could be
	// either 2 codepoints in the BMP (6 bytes in UTF-8) or a single codepoint if the second char16 is a surrogate (4 bytes
	// in UTF-8). Therefore worst case without inspecting data is 3*numChar16s.
	if (!src)
		return srcLen * 3;

	int total = 0;
	while (srcLen > 0)
	{
		char32_t codepoint;
		int read = tUTF::DecodeUtf16(codepoint, src);
		srcLen -= read;
		src += read;

		int written = 0;
		if (dst)
		{
			written = tUTF::EncodeUtf8(dst, codepoint);
			dst += written;
		}
		else
		{
			// No encoding. Just compute length.
			written = tUTF::CalculateUtf8Length(codepoint);
		}
		total += written;
	}

	return total;
}


int tStd::tUTF8(char8_t* dst, const char32_t* src, int srcLen)
{
	// Compute fast worst-case size needed.
	// Worst case is every char32 needing 4 char8s.
	if (!src)
		return srcLen * 4;

	int total = 0;
	for (int i = 0; i < srcLen; i++)
	{
		char32_t codepoint = src[i];

		int written = 0;
		if (dst)
		{
			written = tUTF::EncodeUtf8(dst, codepoint);
			dst += written;
		}
		else
		{
			// No encoding. Just compute length.
			written = tUTF::CalculateUtf8Length(codepoint);
		}
		total += written;
	}

	return total;
}


int tStd::tUTF16(char16_t* dst, const char8_t* src, int srcLen)
{
	// Compute fast worst-case size needed.
	// 1 char8					-> 1 char16.
	// 2 char8s (surrogates)	-> 1 char16.
	// 3 char8s (surrogates)	-> also guaranteed 1 char16.
	// 4 char8s (surrogates)	-> 2 char16s.
	// So worst-case is every byte needing 1 whole char16.
	if (!src)
		return srcLen;

	int total = 0;
	while (srcLen > 0)
	{
		char32_t codepoint;
		int read = tUTF::DecodeUtf8(codepoint, src);
		srcLen -= read;
		src += read;

		int written = 0;
		if (dst)
		{
			written = tUTF::EncodeUtf16(dst, codepoint);
			dst += written;
		}
		else
		{
			// No encoding. Just compute length.
			written = tUTF::CalculateUtf16Length(codepoint);
		}
		total += written;
	}

	return total;
}


int tStd::tUTF16(char16_t* dst, const char32_t* src, int srcLen)
{
	// Compute fast worst-case size needed.
	// Worst case is every char32 needing 2 char16s.
	if (!src)
		return srcLen * 2;

	int total = 0;
	for (int i = 0; i < srcLen; i++)
	{
		char32_t codepoint = src[i];

		int written = 0;
		if (dst)
		{
			written = tUTF::EncodeUtf16(dst, codepoint);
			dst += written;
		}
		else
		{
			// No encoding. Just compute length.
			written = tUTF::CalculateUtf16Length(codepoint);
		}
		total += written;
	}

	return total;
}


int tStd::tUTF32(char32_t* dst, const char8_t* src, int srcLen)
{
	// Compute fast worst-case size needed.
	// Worst-case is every char8 needing 1 whole char32.
	if (!src)
		return srcLen;

	int total = 0;
	while (srcLen > 0)
	{
		char32_t codepoint;
		int read = tUTF::DecodeUtf8(codepoint, src);
		srcLen -= read;
		src += read;

		if (dst)
		{
			dst[0] = codepoint;
			dst++;
		}
		total++;
	}

	return total;
}


int tStd::tUTF32(char32_t* dst, const char16_t* src, int srcLen)
{
	// Compute fast worst-case size needed.
	// Worst-case is every char16 needing 1 whole char32.
	if (!src)
		return srcLen;

	int total = 0;
	while (srcLen > 0)
	{
		char32_t codepoint;
		int read = tUTF::DecodeUtf16(codepoint, src);
		srcLen -= read;
		src += read;

		if (dst)
		{
			dst[0] = codepoint;
			dst++;
		}
		total++;
	}

	return total;
}


int tStd::tUTF8s(char8_t* dst, const char16_t* src)
{
	if (!src)
		return 0;

	int length = tUTF8(dst, src, tStrlen(src));
	if (dst)
		dst[length] = u8'\0';

	return length;
}


int tStd::tUTF8s(char8_t* dst, const char32_t* src)
{
	if (!src)
		return 0;

	int length = tUTF8(dst, src, tStrlen(src));
	if (dst)
		dst[length] = u8'\0';

	return length;
}


int tStd::tUTF16s(char16_t* dst, const char8_t* src)
{
	if (!src)
		return 0;

	int length = tUTF16(dst, src, tStrlen(src));
	if (dst)
		dst[length] = u'\0';

	return length;
}


int tStd::tUTF16s(char16_t* dst, const char32_t* src)
{
	if (!src)
		return 0;

	int length = tUTF16(dst, src, tStrlen(src));
	if (dst)
		dst[length] = u'\0';

	return length;
}


int tStd::tUTF32s(char32_t* dst, const char8_t* src)
{
	if (!src)
		return 0;

	int length = tUTF32(dst, src, tStrlen(src));
	if (dst)
		dst[length] = U'\0';

	return length;
}


int tStd::tUTF32s(char32_t* dst, const char16_t* src)
{
	if (!src)
		return 0;

	int length = tUTF32(dst, src, tStrlen(src));
	if (dst)
		dst[length] = U'\0';

	return length;
}


char32_t tStd::tUTF32c(const char8_t* srcPoint)
{
	char32_t codepoint = cCodepoint_Replacement;
	if (!srcPoint)
		return codepoint;

	tUTF::DecodeUtf8(codepoint, srcPoint);
	return codepoint;
}


char32_t tStd::tUTF32c(const char16_t* srcPoint)
{
	char32_t codepoint = cCodepoint_Replacement;
	if (!srcPoint)
		return codepoint;

	tUTF::DecodeUtf16(codepoint, srcPoint);
	return codepoint;
}


char32_t tStd::tUTF32c(const char32_t* srcPoint)
{
	char32_t codepoint = cCodepoint_Replacement;
	if (!srcPoint)
		return codepoint;

	if (*srcPoint > tUTF::cCodepoint_UnicodeMax)
		codepoint = cCodepoint_Replacement;
	else
		codepoint = *srcPoint;

	return codepoint;
}


int tStd::tUTF32c(char32_t dst[1], char8_t* srcPoint)
{
	char32_t codepoint = cCodepoint_Replacement;
	if (!srcPoint)
	{
		if (dst) dst[0] = codepoint;
		return 0;
	}

	// Decode is a low-level function. It expects srcPoint to be valid.
	int unitCount = tUTF::DecodeUtf8(codepoint, srcPoint);
	if (dst) dst[0] = codepoint;
	return unitCount;
}


int tStd::tUTF32c(char32_t dst[1], char16_t* srcPoint)
{
	char32_t codepoint = cCodepoint_Replacement;
	if (!srcPoint)
	{
		if (dst) dst[0] = codepoint;
		return 0;
	}

	// Decode is a low-level function. It expects srcPoint to be valid.
	int unitCount = tUTF::DecodeUtf16(codepoint, srcPoint);
	if (dst) dst[0] = codepoint;
	return unitCount;
}


int tStd::tUTF32c(char32_t dst[1], char32_t* srcPoint)
{
	if (!srcPoint)
	{
		if (dst) dst[0] = cCodepoint_Replacement;
		return 0;
	}

	if (dst) dst[0] = *srcPoint;
	return 1;
}


int tStd::tUTF8c(char8_t dst[4], char32_t srcPoint)
{
	return tUTF::EncodeUtf8(dst, srcPoint);
}


int tStd::tUTF16c(char16_t dst[2], char32_t srcPoint)
{
	return tUTF::EncodeUtf16(dst, srcPoint);
}


int tStd::tUTF32c(char32_t dst[1], char32_t srcPoint)
{
	if (!dst)
		return 0;

	if (srcPoint > tUTF::cCodepoint_UnicodeMax)
		dst[0] = cCodepoint_Replacement;
	else
		dst[0] = srcPoint;
	return 1;
}


int tUTF::CalculateUtf16Length(char32_t codepoint)
{
	if (codepoint <= cCodepoint_LastValidBMP)
		return 1;

	return 2;
}


int tUTF::DecodeUtf16(char32_t& codepoint, const char16_t* src)
{
	tAssert(src);
	char16_t high = src[0];

	// If BMP character, we're done.
	if ((high & cSurrogate_GenericMask16) != cSurrogate_GenericVal16)
	{
		codepoint = high;
		return 1;
	}

	// If unmatched low surrogate it's invalid. Return replacement.
	if ((high & cSurrogate_Mask16) != cSurrogate_HighVal16)
	{
		codepoint = tStd::cCodepoint_Replacement;
		return 1;
	}
	
	char16_t low = src[1];

	// If unmatched high surrogate it's invalid. Return replacement.
	if ((low & cSurrogate_Mask16) != cSurrogate_LowVal16)
	{
		codepoint = tStd::cCodepoint_Replacement;
		return 1;
	}

	// Two correctly matched surrogates if we ade it this far.
	// The high bits of the codepoint are the value bits of the high surrogate.
	// The low bits of the codepoint are the value bits of the low surrogate.
	codepoint = high & cSurrogate_CodepointMask16;
	codepoint <<= cSurrogate_CodepointBits;
	codepoint |= low & cSurrogate_CodepointMask16;
	codepoint += cSurrogate_CodepointOffset;	
	return 2;
}


int tUTF::EncodeUtf16(char16_t* dst, char32_t codepoint)
{
	if (!dst)
		return 0;

	// If codepoint in the BMP just write the single char16.
	if (codepoint <= cCodepoint_LastValidBMP)
	{
		dst[0] = codepoint;
		return 1;
	}

	codepoint -= cSurrogate_CodepointOffset;
	char16_t low = cSurrogate_LowVal16;
	low |= codepoint & cSurrogate_CodepointMask32;

	codepoint >>= cSurrogate_CodepointBits;
	char16_t high = cSurrogate_HighVal16;
	high |= codepoint & cSurrogate_CodepointMask32;

	dst[0] = high;
	dst[1] = low;
	return 2;
}


int tUTF::CalculateUtf8Length(char32_t codepoint)
{
	if (codepoint <= cCodepoint_UTF8Max1)
		return 1;

	if (codepoint <= cCodepoint_UTF8Max2)
		return 2;

	if (codepoint <= cCodepoint_UTF8Max3)
		return 3;

	if (codepoint <= cCodepoint_UnicodeMax)
		return 4;

	// Return max 4 in case the UTF-8 standard ever increases cCodepoint_UnicodeMax. What they won't
	// break is that UTF-8 can encode all codepoints, so checking UnicodeMax is still valid.
	return 4;
}


int tUTF::DecodeUtf8(char32_t& codepoint, const char8_t* src)
{
	tAssert(src);
	char8_t leading = src[0];
	int encodingLength = 0;
	UTF8Pattern leadingPattern;

	bool matches = false;	// True if the leading byte matches the current leading pattern.
	do
	{
		encodingLength++;
		leadingPattern = UTF8LeadingBytes[encodingLength - 1];
		matches = (leading & leadingPattern.Mask) == leadingPattern.Value;

	} while (!matches && (encodingLength < UTF8LeadingBytes_NumElements));

	// If leading byte doesn't match any known pattern it is invalid and we return replacement.
	if (!matches)
	{
		codepoint = tStd::cCodepoint_Replacement;
		return encodingLength;
	}

	codepoint = leading & ~leadingPattern.Mask;

	// This loop only ends up running if continuation codeunits found (not ASCII).
	for (int i = 1; i < encodingLength; i++)
	{
		char8_t continuation = src[i];

		// If number of continuation bytes is not the same as advertised on the leading byte it's an invalid encoding
		// so we return the replacement.
		if ((continuation & cContinuation_UTF8Mask) != cContinuation_UTF8Val)
		{
			codepoint = tStd::cCodepoint_Replacement;

			// I think the best behaviour here is to return how much we processed b4 running into a problem.
			// If we returned encodingLength we might skip some input when an invalid is encountered. Hard to say.
			return 1+i;
		}

		codepoint <<= cContinuation_CodepointBits;
		codepoint |= continuation & ~cContinuation_UTF8Mask;
	}

	if
	(
		// These are guaranteed to be non-characters by the standard and reuire the replacement.
		((codepoint == tStd::cCodepoint_SpecialNonCharA) || (codepoint == tStd::cCodepoint_SpecialNonCharB)) ||

		// Surrogates are invalid Unicode codepoints and should only be used in UTF-16. Invalid encoding so return replacement.
		((codepoint <= cCodepoint_LastValidBMP) && ((codepoint & cSurrogate_GenericMask32) == cSurrogate_GenericVal32)) ||

		// UTF-8 can encode codepoints larger than the Unicode standard allows. If it does it's an invalid encoding and we return the replacement codepoint.
		(codepoint > cCodepoint_UnicodeMax) ||

		// Overlong encodings are considered invalid so we return the replacement codepoint and return the actual number read so we skip the overlong completely.
		// We do this last cuz of short-circuit expression evaluation in C++ (calc only called if necessary).
		(CalculateUtf8Length(codepoint) != encodingLength)
	)
	{
		codepoint = tStd::cCodepoint_Replacement;
	}

	return encodingLength;
}


int tUTF::EncodeUtf8(char8_t* dst, char32_t codepoint)
{
	if (!dst)
		return 0;

	// Write the continuation bytes in reverse order.
	int encodeLength = CalculateUtf8Length(codepoint);
	for (int contIndex = encodeLength - 1; contIndex > 0; contIndex--)
	{
		char8_t cont = codepoint & ~cContinuation_UTF8Mask;
		cont |= cContinuation_UTF8Val;
		dst[contIndex] = cont;
		codepoint >>= cContinuation_CodepointBits;
	}

	// Write the leading byte.
	UTF8Pattern pattern = UTF8LeadingBytes[encodeLength - 1];
	char8_t lead = codepoint & ~(pattern.Mask);
	lead |= pattern.Value;
	dst[0] = lead;

	return encodeLength;
}
