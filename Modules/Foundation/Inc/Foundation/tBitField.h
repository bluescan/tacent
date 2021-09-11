// tBitField.h
//
// A tBitField is a fixed size array of bits. Similar to STL bitset class. An tBitField needs to know how many bits
// will be stored at compile time and there is no possibility to grow or dynamically change that number. All bitwise
// operators are overloaded appropriately. This class is perfect for flags where a uint32 or uint64 is not enough.
//
// Comparisons
// tBitArray - Use when you want to store a large number of bits and you don't know how many at compile-time.
//             This type os primatily for storage and access to a large number of bits. Not many bitwise or
//             mathematical operators.
// tBitField - Use when know how many bits at compile-time and you want bitwise logic opertors like and, or, xor,
//             shift, not, etc. Good for storing a fixed number of flags or channels etc.
// tFixInt   - Use when you want full mathematical operations like any built-in integral type. Size must be known at
//             compile time and must be a multiple of 32 bits. You get + - / * etc as well as all bitwise logic ops.
//
// Copyright (c) 2004-2006, 2015, 2017, 2020, 2021 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include "Foundation/tString.h"


// The tBitField class. NumBits represents the number of bits that can be stored by the instance. There are no
// conditions on the value of NumBits used and long as it is a whole number. If NumBits is a multiple of 32, the
// tBitField will be supported by tPrintf so just call something like: tsPrintf(s, "%032|128X", bitvar128) to convert it
// to a string. The memory image size taken up will always be a multiple of 4 bytes. ex: sizeof(tBitField<16>) = 4 and
// sizeof(tBitField<33>) = 8. You can still use tPrintf on a 33-bit bit-field, just be aware of the size. Any padding
// bits are guaranteed to be clear in the internal representation and when saved to a chunk (disk) format.
template<int NumBits> class tBitField
{
public:
	tBitField()												/* All bits cleared. */										{ Clear(); }

	// Disabled CopyCons so class remains a POD-type. Allows it to be passed to tPrintf for non MSVC compilers.
	// tBitField(const tBitField& src)																					{ for (int i = 0; i < NumElements; i++) ElemData[i] = src.ElemData[i]; }

	// Originally this constructor could take any base, but wanted to keep the tBitField fast and simple and did NOT
	// want to rely on tFixInt to make the arbitrary-base conversion code work. If you need bits from an arbitrary based
	// string, use tFixInt instead. Currently this function takes hex strings, with or without a leading 0x. Case insensitive.
	tBitField(const char* hexStr)																						{ Set(hexStr); }
	tBitField(int val)																									{ Set(val); }

	// Power-of-2 constructors from 8 to 256 bits.
	tBitField(uint8 val)																								{ Set(val); }
	tBitField(uint16 val)																								{ Set(val); }
	tBitField(uint32 val)																								{ Set(val); }
	tBitField(uint64 val)																								{ Set(val); }
	tBitField(uint64 msb, uint64 lsb)																					{ Set(msb, lsb); }
	tBitField(uint64 msb, uint64 hb, uint64 lb, uint64 lsb)																{ Set(msb, hb, lb, lsb); }

	// Array constructors.
	tBitField(const uint8* src, int len)					/* See Set(const uint8*, int) */							{ Set(src, len); }
	tBitField(const uint16* src, int len)					/* See Set(const uint8*, int) */							{ Set(src, len); }
	tBitField(const uint32* src, int len)					/* See Set(const uint8*, int) */							{ Set(src, len); }
	tBitField(const uint64* src, int len)					/* See Set(const uint8*, int) */							{ Set(src, len); }

	// Originally this Set could take any base, but wanted to keep the tBitField fast and simple and did NOT want to rey
	// on tFixInt to make the arbitrary-base conversion code work. If you need bits from an arbitrary based string, use
	// tFixInt instead. Currently this function takes hex strings, with or without a leading 0x. Case insensitive.
	void Set(const char* hexString);

	// Set from a binary string like "1011010100001".
	void SetBinary(const char* binaryString);

	// Sets the bitfield from the supplied data. Asserts if the bit-field is not big enough. The bitfield is allowed to
	// be bigger, in which case zeroes are set in the most-sig bits.
	void Set(int val)																									{ Set(uint32(val)); }
	void Set(uint8 val);
	void Set(uint16 val);
	void Set(uint32 val);
	void Set(uint64 val);
	void Set(uint64 msb, uint64 lsb);
	void Set(uint64 msb, uint64 hb, uint64 lb, uint64 lsb);
	void Set(const uint8* src, int len);
	void Set(const uint16* src, int len);
	void Set(const uint32* src, int len);
	void Set(const uint64* src, int len);

	operator int8() const																								{ int8 r; Extract(r); return r; }
	operator int16() const																								{ int16 r; Extract(r); return r; }
	operator int32() const																								{ int32 r; Extract(r); return r; }
	operator int64() const																								{ int64 r; Extract(r); return r; }
	operator uint8() const																								{ uint8 r; Extract(r); return r; }
	operator uint16() const																								{ uint16 r; Extract(r); return r; }
	operator uint32() const																								{ uint32 r; Extract(r); return r; }
	operator uint64() const																								{ uint64 r; Extract(r); return r; }
	operator bool() const;

	void Clear()																										{ for (int i = 0; i < NumElements; i++) ElemData[i] = 0x00000000; }
	bool GetBit(int n) const;								// Gets the n'th bit as a bool. Zero-based index where zero is the least significant binary digit.
	void SetBit(int n, bool val = true);					// Sets the n'th bit to val. Zero-based index where zero is least significant binary digit.
	void SetAll(bool val = true);
	void ClearAll()																										{ Clear(); }
	void InvertAll();
	bool AreAll(bool val) const;							/* Checks if all bits are set to val. */
	int GetNumBits() const									/* Returns the number of bits stored by the bit-field. */	{ return NumBits; }
	int CountBits(bool val) const;							/* Returns the number of bits that match val. */

	// Gets the bit-field as a string in base 16. Upper case and no leading 0x. You can also use tPrintf, if you need
	// leading zeroes or more control over formatting.
	tString GetAsHexString() const;

	// Ditto but for binary.
	tString GetAsBinaryString() const;

	// These deal with the internal uint32 elements that store the bit field. The elements are always least-significant
	// at the beginning, regardless of machine endianness.
	int GetNumElements() const								/* Returns how many uint32s are used for the bit array. */	{ return NumElements; }

	uint32 GetElement(int i) const																						{ return ElemData[i]; }
	void SetElement(int i, uint32 val)																					{ ElemData[i] = val; }

	void GetElements(uint32* dest) const					/* Least significant at the beginning. */					{ tAssert(dest); tMemcpy(dest, ElemData, NumElements*4); }
	void SetElements(const uint32* src)						/* Least sig at the beginning. Clears unused bits. */		{ tAssert(src); tMemcpy(ElemData, src, NumElements*4); ClearPadBits(); }

	uint32& Element(int i)																								{ return ElemData[i]; }
	uint32* Elements() const																							{ return (uint32*)ElemData; }

	// Gets the n'th byte as a uint8. Zero-based index where zero is the least significant byte.
	// If NumBits is, say, 33 the range of the index will be n E [0,4]. That is, 5 bytes.
	uint8 GetByte(int n) const;
	void SetByte(int n, uint8 b);
	uint8 GetNybble(int n) const;
	void SetNybble(int n, uint8 nyb);

	tBitField& operator=(const tBitField& s)																			{ if (this == &s) return *this; tStd::tMemcpy(ElemData, s.ElemData, sizeof(ElemData)); return *this; }

	// We ensure and assume any pad bits are clear. Since 0 &|^ 0 = 0, we don't need to bother clearing any pad bits.
	tBitField& operator&=(const tBitField& s)																			{ for (int i = 0; i < NumElements; i++) ElemData[i] &= s.ElemData[i]; return *this; }
	tBitField& operator|=(const tBitField& s)																			{ for (int i = 0; i < NumElements; i++) ElemData[i] |= s.ElemData[i]; return *this; }
	tBitField& operator^=(const tBitField& s)																			{ for (int i = 0; i < NumElements; i++) ElemData[i] ^= s.ElemData[i]; return *this; }

	// The pad bits are always zeroed when left shifting.
	tBitField& operator<<=(int);
	tBitField operator<<(int s) const																					{ tBitField<NumBits> set(*this); set <<= s; return set; }
	tBitField& operator>>=(int);
	tBitField operator>>(int s) const																					{ tBitField<NumBits> set(*this); set >>= s; return set; }
	tBitField operator~() const																							{ tBitField set(*this); set.InvertAll(); return set; }

	bool operator[](int n) const																						{ return GetBit(n); }
	bool operator==(const tBitField&) const;
	bool operator!=(const tBitField&) const;

private:
	template<typename T> void Extract(T&, uint8 fill = 0) const;

	// The tBitField guarantees clear bits in the internal representation if the number of bits is not a multiple of 32
	// (which is our internal storage type size). This function clears them (and only them).
	void ClearPadBits();

	const static int NumElements = (NumBits >> 5) + ((NumBits % 32) ? 1 : 0);

	// The bit-field is stored in an array of uint32s called elements. Any pad bits are set to 0 at all times. The
	// elements at smaller array indexes store less significant digits than the ones at larger indexes.
	uint32 ElemData[NumElements];
};


// These commutative binary operators belong _outside_ of the tBitField class for good OO reasons. i.e. They are global
// operators that act on bit-fields. They belong outside in the same way that operator+ does for regular integral types.
template<int NumBits> inline tBitField<NumBits> operator&(const tBitField<NumBits>& a, const tBitField<NumBits>& b)		{ tBitField<NumBits> set(a); set &= b; return set; }
template<int NumBits> inline tBitField<NumBits> operator|(const tBitField<NumBits>& a, const tBitField<NumBits>& b)		{ tBitField<NumBits> set(a); set |= b; return set; }
template<int NumBits> inline tBitField<NumBits> operator^(const tBitField<NumBits>& a, const tBitField<NumBits>& b)		{ tBitField<NumBits> set(a); set ^= b; return set; }


// The tbitNNN types represent convenient bit-field sizes. They can represent large sets of bits and even allow bit
// operations. They are a little slower that native 32 or 64 bit integers and do not support many math operations. For
// full arithmetic support in a 128-bit or larger integer consider using the tFixInt class. Note that since bit-fields
// do not support arithmetic, they should be considered unsigned at all times. For example, the right shift >> operator
// does not sign extend.
typedef tBitField<128> tbit128;
typedef tBitField<256> tbit256;
typedef tBitField<512> tbit512;


// Implementation below this line.


template<int N> inline void tBitField<N>::Set(const char* hexStr)
{
	if (!hexStr)
	{
		ClearAll();
		return;
	}

	tBitField< NumElements*32 > val;
	tAssert(NumElements == val.GetNumElements());
	tString hex(hexStr);
	hex.UpCase();
	hex.ExtractLeft("0X");
	int len = hex.Length();
	int nybIdx = 0;
	for (int i = len-1; i >= 0; i--)
	{
		char nybChar = hex[i];
		switch (nybChar)
		{
			case '0':	val.SetNybble(nybIdx++, 0x0); break;
			case '1':	val.SetNybble(nybIdx++, 0x1); break;
			case '2':	val.SetNybble(nybIdx++, 0x2); break;
			case '3':	val.SetNybble(nybIdx++, 0x3); break;
			case '4':	val.SetNybble(nybIdx++, 0x4); break;
			case '5':	val.SetNybble(nybIdx++, 0x5); break;
			case '6':	val.SetNybble(nybIdx++, 0x6); break;
			case '7':	val.SetNybble(nybIdx++, 0x7); break;
			case '8':	val.SetNybble(nybIdx++, 0x8); break;
			case '9':	val.SetNybble(nybIdx++, 0x9); break;
			case 'A':	val.SetNybble(nybIdx++, 0xA); break;
			case 'B':	val.SetNybble(nybIdx++, 0xB); break;
			case 'C':	val.SetNybble(nybIdx++, 0xC); break;
			case 'D':	val.SetNybble(nybIdx++, 0xD); break;
			case 'E':	val.SetNybble(nybIdx++, 0xE); break;
			case 'F':	val.SetNybble(nybIdx++, 0xF); break;
		}
		// We simply skip unrecognized characters.
	}
	
	for (int i = 0; i < NumElements; i++)
		ElemData[i] = val.GetElement(i);

	ClearPadBits();
}


template<int N> inline void tBitField<N>::SetBinary(const char* binaryStr)
{
	if (!binaryStr)
	{
		ClearAll();
		return;
	}

	tBitField< NumElements*32 > val;
	tAssert(NumElements == val.GetNumElements());
	tString bin(binaryStr);
	bin.UpCase();
	bin.ExtractLeft("0B");
	int len = bin.Length();
	int binIdx = 0;
	for (int i = len-1; i >= 0; i--)
	{
		char binChar = bin[i];
		if (binChar == '0')
			val.SetBit(binIdx++, false);
		else if (binChar == '1')
			val.SetBit(binIdx++, true);

		// We simply skip unrecognized characters.
	}
	
	for (int i = 0; i < NumElements; i++)
		ElemData[i] = val.GetElement(i);

	ClearPadBits();
}


template<int N> inline void tBitField<N>::Set(uint8 val)
{
	Clear();
	tAssert(NumElements >= 1);
	ElemData[0] = uint32(val);
	ClearPadBits();
}


template<int N> inline void tBitField<N>::Set(uint16 val)
{
	Clear();
	tAssert(NumElements >= 1);
	ElemData[0] = uint32(val);
	ClearPadBits();
}


template<int N> inline void tBitField<N>::Set(uint32 val)
{
	Clear();
	tAssert(NumElements >= 1);
	ElemData[0] = val;
	ClearPadBits();
}


template<int N> inline void tBitField<N>::Set(uint64 val)
{
	Clear();
	tAssert(NumElements >= 2);
	ElemData[0] = val & 0x00000000FFFFFFFFull;
	ElemData[1] = (val >> 32) & 0x00000000FFFFFFFFull;
	ClearPadBits();
}


template<int N> inline void tBitField<N>::Set(uint64 msb, uint64 lsb)
{
	Clear();
	tAssert(NumElements >= 4);
	ElemData[0] = lsb & 0x00000000FFFFFFFFull;
	ElemData[1] = (lsb >> 32) & 0x00000000FFFFFFFFull;
	ElemData[2] = msb & 0x00000000FFFFFFFFull;
	ElemData[3] = (msb >> 32) & 0x00000000FFFFFFFFull;
	ClearPadBits();
}


template<int N> inline void tBitField<N>::Set(uint64 msb, uint64 hb, uint64 lb, uint64 lsb)
{
	Clear();
	tAssert(NumElements >= 8);
	ElemData[0] = lsb & 0x00000000FFFFFFFFull;
	ElemData[1] = (lsb >> 32) & 0x00000000FFFFFFFFull;
	ElemData[2] = lb & 0x00000000FFFFFFFFull;
	ElemData[3] = (lb >> 32) & 0x00000000FFFFFFFFull;
	ElemData[4] = hb & 0x00000000FFFFFFFFull;
	ElemData[5] = (hb >> 32) & 0x00000000FFFFFFFFull;
	ElemData[6] = msb & 0x00000000FFFFFFFFull;
	ElemData[7] = (msb >> 32) & 0x00000000FFFFFFFFull;
	ClearPadBits();
}


template<int N> inline void tBitField<N>::Set(const uint8* src, int len)
{
	Clear();
	tAssert(NumElements*4 >= len);
	for (int i = len-1; i >= 0; i--)
	{
		int j = (len-1) - i;
		ElemData[j/4] |= src[j] << ((j%4)*8);
	}
	ClearPadBits();
}


template<int N> inline void tBitField<N>::Set(const uint16* src, int len)
{
	Clear();
	tAssert(NumElements*2 >= len);
	for (int i = len-1; i >= 0; i--)
	{
		int j = (len-1) - i;
		ElemData[j/2] |= src[j] << ((j%2)*16);
	}
	ClearPadBits();
}


template<int N> inline void tBitField<N>::Set(const uint32* src, int len)
{
	Clear();
	tAssert(NumElements >= len);
	for (int i = len-1; i >= 0; i--)
	{
		int j = (len-1) - i;
		ElemData[j] = src[j];
	}
	ClearPadBits();
}


template<int N> inline void tBitField<N>::Set(const uint64* src, int len)
{
	Clear();
	tAssert(NumElements >= len*2);
	for (int i = len-1; i >= 0; i--)
	{
		int j = (len-1) - i;
		ElemData[j*2]	= src[j] & 0x00000000FFFFFFFFull;
		ElemData[j*2+1]	= (src[j] >> 32) & 0x00000000FFFFFFFFull;
	}
	ClearPadBits();
}


template<int N> inline tString tBitField<N>::GetAsHexString() const
{
	tBitField< NumElements*32 > val;
	tAssert(NumElements == val.GetNumElements());

	for (int i = 0; i < NumElements; i++)
		val.SetElement(i, ElemData[i]);

	int numNybbles = 8*NumElements;
	tString result(numNybbles);
	for (int ny = 0; ny < numNybbles; ny++)
	{
		uint8 nybble = val.GetNybble(ny);
		int n = numNybbles-1-ny;
		switch (nybble)
		{
			case 0x00:		result[n] = '0';	break;
			case 0x01:		result[n] = '1';	break;
			case 0x02:		result[n] = '2';	break;
			case 0x03:		result[n] = '3';	break;
			case 0x04:		result[n] = '4';	break;
			case 0x05:		result[n] = '5';	break;
			case 0x06:		result[n] = '6';	break;
			case 0x07:		result[n] = '7';	break;
			case 0x08:		result[n] = '8';	break;
			case 0x09:		result[n] = '9';	break;
			case 0x0A:		result[n] = 'A';	break;
			case 0x0B:		result[n] = 'B';	break;
			case 0x0C:		result[n] = 'C';	break;
			case 0x0D:		result[n] = 'D';	break;
			case 0x0E:		result[n] = 'E';	break;
			case 0x0F:		result[n] = 'F';	break;
		}
	}
	result.RemoveLeading("0");
	return result;
}


template<int N> inline tString tBitField<N>::GetAsBinaryString() const
{
	tBitField< NumElements*32 > val;
	tAssert(NumElements == val.GetNumElements());
	for (int i = 0; i < NumElements; i++)
		val.SetElement(i, ElemData[i]);

	int numBits = 32*NumElements;
	tString result(numBits);
	for (int nb = 0; nb < numBits; nb++)
	{
		bool bit = val.GetBit(nb);
		int n = numBits-1-nb;
		if (bit)
			result[n] = '1';
		else
			result[n] = '0';
	}
	result.RemoveLeading("0");
	return result;
}


template<int N> inline bool tBitField<N>::GetBit(int n) const
{
	tAssert(n < N);
	int i = n >> 5;
	int d = n & 0x1F;
	uint32 mask = 1 << d;
	return (ElemData[i] & mask) ? true : false;
}


template<int N> inline void tBitField<N>::SetBit(int n, bool v)
{
	tAssert(n < N);
	int i = n >> 5;
	int d = n & 0x1F;
	uint32 mask = 1 << d;
	if (v)
		ElemData[i] |= mask;
	else
		ElemData[i] &= ~mask;
}


template<int N> inline void tBitField<N>::SetAll(bool v)
{
	for (int i = 0; i < NumElements; i++)
		ElemData[i] = v ? 0xFFFFFFFF : 0x00000000;

	if (v)
		ClearPadBits();
}


template<int N> inline void tBitField<N>::InvertAll()
{
	for (int i = 0; i < NumElements; i++)
		ElemData[i] = ~(ElemData[i]);

	ClearPadBits();
}


template<int N> inline bool tBitField<N>::AreAll(bool v) const
{
	// To test all clear we rely on any extra bits being cleared as well.
	if (!v)
	{
		for (int i = 0; i < NumElements; i++)
			if (ElemData[i] != 0x00000000)
				return false;
		return true;
	}

	for (int i = 0; i < NumElements-1; i++)
		if (ElemData[i] != 0xFFFFFFFF)
			return false;

	// The last slot in the array may not be full.
	int last = N & 0x1F;
	uint32 maxFull = (last ? (1<<last) : 0) - 1;

	return (ElemData[NumElements-1] == maxFull) ? true : false;
}


template<int N> inline int tBitField<N>::CountBits(bool v) const
{
	// First compute the total number set.
	int numSet = 0;
	for (int i = 0; i < NumElements; ++i)
	{
		uint32 v = ElemData[i];			// Count the number of bits set in v.
		uint32 c;						// c accumulates the total bits set in v.
		for (c = 0; v; c++)
			v &= v - 1;					// Clear the least significant bit set.
		numSet += c;
	}

	// Now numSet is correct. If that's what we were asked, we're done. If not, we just subtract.
	if (v)
		return numSet;

	return N - numSet;
}


template<int N> inline uint8 tBitField<N>::GetByte(int n) const
{
	int numBytes = (N / 8) + ((N % 8) ? 1 : 0);
	tAssert(n < numBytes);
	int idx = n / 4;
	int shift = (n % 4) << 3;
	uint32 elem = ElemData[idx];
	return (elem & (0xFF << shift)) >> shift;
}


template<int N> inline void tBitField<N>::SetByte(int n, uint8 b)
{
	int numBytes = (N / 8) + ((N % 8) ? 1 : 0);
	tAssert(n < numBytes);
	int idx = n / 4;
	int shift = (n % 4) << 3;
	uint32 elem = ElemData[idx];
	elem = ~(0xFF << shift) & elem;			// Clear the byte.
	ElemData[idx] = elem | (b << shift);	// Set the byte.
}


template<int N> inline uint8 tBitField<N>::GetNybble(int n) const
{
/*
	int numBytes = (N / 8) + ((N % 8) ? 1 : 0);
	tAssert(n < numBytes);
	int idx = n / 4;
	int shift = (n % 4) << 3;
	uint32 elem = ElemData[idx];
	return (elem & (0xFF << shift)) >> shift;

*/
	int numNybbles = (N / 4) + ((N % 4) ? 1 : 0);
	tAssert(n < numNybbles);
	int idx = n / 8;
	int shift = (n % 8) << 2;
	uint32 elem = ElemData[idx];
	return (elem & (0xF << shift)) >> shift;
}


template<int N> inline void tBitField<N>::SetNybble(int n, uint8 nyb)
{
	int numNybbles = (N / 4) + ((N % 4) ? 1 : 0);
	tAssert(n < numNybbles);
	int idx = n / 8;
	int shift = (n % 8) << 2;
	uint32 elem = ElemData[idx];
	elem = ~(0xF << shift) & elem;			// Clear the nybble.
	ElemData[idx] = elem | (nyb << shift);	// Set the nybble.
}


template<int N> inline tBitField<N>& tBitField<N>::operator<<=(int s)
{
	// First, if we are shifting by too much, we end up with all zeroes.
	tAssert(s >= 0);
	if (s >= N)
	{
		Clear();
		return *this;
	}

	// Now lets make an uber-set with zeroes to the right.
	uint32 uber[NumElements*2];
	for (int i = 0; i < NumElements; i++)
		uber[i] = 0x00000000;

	for (int i = 0; i < NumElements; i++)
		uber[i+NumElements] = ElemData[i];

	int bitIndex = NumElements*32;
	bitIndex -= s;

	// Finally copy the bits over from the new starting position. This might be a bit slow, but it works.
	Clear();
	for (int b = 0; b < N; b++, bitIndex++)
	{
		// Read.
		int i = bitIndex >> 5;
		int d = bitIndex & 0x1F;
		uint32 mask = 1 << d;
		bool val = (uber[i] & mask) ? true : false;

		// Write.
		SetBit(b, val);
	}

	return *this;
}


template<int N> inline tBitField<N>& tBitField<N>::operator>>=(int s)
{
	// First, if we are shifting by too much, we end up with all zeroes.
	tAssert(s >= 0);
	if (s >= N)
	{
		Clear();
		return *this;
	}

	// Now lets make an uber-set with zeroes to the left.
	uint32 uber[NumElements*2];
	for (int i = 0; i < NumElements; i++)
		uber[i+NumElements] = 0x00000000;

	for (int i = 0; i < NumElements; i++)
		uber[i] = ElemData[i];

	int bitIndex = 0;
	bitIndex += s;

	// Finally copy the bits over from the new starting position. This might be a bit slow, but it works.
	Clear();
	for (int b = 0; b < N; b++, bitIndex++)
	{
		// Read.
		int i = bitIndex >> 5;
		int d = bitIndex & 0x1F;
		uint32 mask = 1 << d;
		bool val = (uber[i] & mask) ? true : false;

		// Write.
		SetBit(b, val);
	}

	return *this;
}


template<int N> inline bool tBitField<N>::operator==(const tBitField& s) const
{
	// Remember, extra bits MUST be set to zero. This allows easy checking of only the array contents.
	for (int i = 0; i < NumElements; i++)
		if (ElemData[i] != s.ElemData[i])
			return false;

	return true;
}


template<int N> inline bool tBitField<N>::operator!=(const tBitField& s) const
{
	for (int i = 0; i < NumElements; i++)
		if (ElemData[i] != s.ElemData[i])
			return true;

	return false;
}


template<int N> inline tBitField<N>::operator bool() const
{
	for (int i = 0; i < NumElements; i++)
		if (ElemData[i])
			return true;

	return false;
}


template<int N> inline void tBitField<N>::ClearPadBits()
{
	int r = N%32;
	uint32& e = ElemData[NumElements-1];
	e &= r ? ~((0xFFFFFFFF >> r) << r) : 0xFFFFFFFF;
}


template<int N> template<typename T> inline void tBitField<N>::Extract(T& v, uint8 fill) const
{
	if (sizeof(v) <= sizeof(ElemData))
	{
		tStd::tMemcpy(&v, ElemData, sizeof(v));
	}
	else
	{
		tStd::tMemcpy(&v, ElemData, sizeof(ElemData));
		tStd::tMemset(reinterpret_cast<char*>(&v) + sizeof(v), fill, int(sizeof(v) - sizeof(ElemData)));
	}
}
