// tBitArray.h
//
// A tBitArray is a holder for an arbitrary number of bits and allows individual access to each bit, the ability to
// clear or set all bits, and some simple binary bitwise operators such as 'and', 'xor', and 'or'. It currently does
// not support dynamic growing or shrinking.
//
// Comparisons
// tBitArray  - Use when you want to store a large number of bits and you don't know how many at compile-time.
//              This type os primatily for storage and access to a large number of bits. Not many bitwise or
//              mathematical operators.
// tBitArray8 - Similar to a tBitArray but uses bytes as the elements. Slightly less efficient but the order of the
//              bits in memory exactly matches what is being represented in the array. Also less padding needed at end.
// tBitField  - Use when know how many bits at compile-time and you want bitwise logic opertors like and, or, xor,
//              shift, not, etc. Good for storing a fixed number of flags or channels etc.
// tFixInt    - Use when you want full mathematical operations like any built-in integral type. Size must be known at
//              compile time and must be a multiple of 32 bits. You get + - / * etc as well as all bitwise logic ops.
//
// Copyright (c) 2004-2006, 2015, 2017, 2019, 2021, 2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include "Foundation/tPlatform.h"
#include "Foundation/tStandard.h"


// This class uses 32-bit elements to store the bit array for efficiency. The bits are ordered from the LSB to the MSB so
// that the 0th bit (on the right) is the first one, 1st bit is the second, and so on. For example, with 35-bits:
// 1                                     35
// 10101111 11110000 10000010 11100011 011									The array we want to represent.
// 11000111 01000001 00001111 11110101 00000000 00000000 00000000 00000110	As 2 32-bit elements.
// 11110101 00001111 01000001 11000111 00000110 00000000 00000000 00000000  In memory on a little-endian machine.
//
class tBitArray
{
public:
	tBitArray()												/* Creates an invalid bit array. Call Set before using. */	{ }
	tBitArray(int numBits)									/* All bit values guaranteed to be 0 after this. */			{ Set(numBits); }
	tBitArray(uint32* data, int numBits, bool steal = false)/* Copies numBits from data. If steal true data is given. */{ Set(data, numBits, steal); }
	tBitArray(const tBitArray& src)																						{ Set(src); }
	~tBitArray()																										{ delete[] ElemData; }

	void Set(int numBits);									// All bit values guaranteed to be 0 after this.
	void Set(uint32* data, int numBits, bool steal = false);// Copies numBits from data. If steal is true data is given.
	void Set(const tBitArray& src);
	void Clear()											/* Invalidates. Use ClearAll if you want all bits clear. */	{ delete[] ElemData; ElemData = nullptr; NumBits = 0; }
	bool IsValid() const																								{ return ElemData ? true : false; }

	bool GetBit(int n) const;								// Gets the n-th bit. 0-based and n must be E [0, NumBits).
	uint8 GetBitInt(int n) const;							// Gets the n-th bit. 0-based and n must be E [0, NumBits).

	// n is the start bit (inclusive) and c is the count. You can get from 1 to 8 bits using this function c E [1, 8].
	// Return value undefined if c <= 0 or c > 8 or n >= GetNumBits. If it goes off the end no more bits are returned,
	// for example, if the bit array has 11101 and you call with (2,6) you'll get 101.
	uint8 GetBits(int n, int c) const;
	
	void SetBit(int n, bool v);
	void SetAll(bool v = true);
	void ClearAll();
	void InvertAll();
	bool AreAll(bool v) const;
	int GetNumBits() const																								{ return NumBits; }
	int CountBits(bool = true) const;						// Counts how many bits are set to supplied value.

	// These deal with the internal uint32 elements that store the bit array. The elements are always least-significant
	// at the beginning, regardless of machine endianness.
	int GetNumElements() const																							{ return ((NumBits + 31) / 32); }

	// Ensure array is valid and i is in range before calling these. That part is up to you.
	uint32 GetElement(int i) const																						{ tAssert(IsValid()); return ElemData[i]; }
	void SetElement(int i, uint32 val)																					{ tAssert(IsValid()); ElemData[i] = val; }

	void GetElements(uint32* dest) const					/* Least significant at the beginning. */					{ tAssert(dest); tStd::tMemcpy(dest, ElemData, GetNumElements()*4); }
	void SetElements(const uint32* src)						/* Least sig at the beginning. Clears unused bits. */		{ tAssert(src); tStd::tMemcpy(ElemData, src, GetNumElements()*4); ClearPadBits(); }

	uint32& Element(int i)																								{ return ElemData[i]; }
	uint32* Elements() const																							{ return ElemData; }

	// Returns index of first bit that is 0. Returns -1 if no bits are clear.
	int FindFirstClearBit() const;

	// Binary operators must operate on arrays with the same number of bits.
	tBitArray& operator=(const tBitArray& src)																			{ Set(src); return *this; }
	tBitArray& operator&=(const tBitArray&);
	tBitArray& operator|=(const tBitArray&);
	tBitArray& operator^=(const tBitArray&);

	const tBitArray operator~() const																					{ tBitArray r(*this); r.InvertAll(); return r; }
	bool operator[](int n) const																						{ return GetBit(n); }
	bool operator==(const tBitArray&) const;
	bool operator!=(const tBitArray&) const;

private:
	void ClearPadBits();

	int NumBits			= 0;								// Number of bits. Not number of fields.
	uint32* ElemData	= nullptr;							// If there are padding bits, they must be set to 0.
};


// This class uses 8-bit elements to store the bit array for efficiency. Unlike tBitArray the bits are ordered from MSB
// to LSB so that the 7th bit (on the left) is the first one, 6th bit is the second, and so on down to the 0th bit which
// is the eighth. For example, with 19-bits:
// 1                   19
// 10101111 11110000 101	   The array we want to represent.
// 10101111 11110000 10100000  As 3 8-bit elements.
// 10101111 11110000 10100000  In memory regardless of endianness.
//
class tBitArray8
{
public:
	tBitArray8()											/* Creates an invalid bit array. Call Set before using. */	{ }
	tBitArray8(int numBits)									/* All bit values guaranteed to be 0 after this. */			{ Set(numBits); }
	tBitArray8(uint8* data, int numBits, bool steal = false)/* Copies numBits from data. If steal true data is given. */{ Set(data, numBits, steal); }
	tBitArray8(const tBitArray8& src)																					{ Set(src); }
	~tBitArray8()																										{ delete[] ElemData; }

	void Set(int numBits);									// All bit values guaranteed to be 0 after this.
	void Set(uint8* data, int numBits, bool steal = false);	// Copies numBits from data. If steal is true data is given.
	void Set(const tBitArray8& src);
	void Clear()											/* Invalidates. Use ClearAll if you want all bits clear. */	{ delete[] ElemData; ElemData = nullptr; NumBits = 0; }
	bool IsValid() const																								{ return ElemData ? true : false; }

	bool GetBit(int n) const;								// Gets the n-th bit. 0-based and n must be E [0, NumBits).
	uint8 GetBitInt(int n) const;							// Gets the n-th bit. 0-based and n must be E [0, NumBits).

	// n is the start bit (inclusive) and c is the count. You can get from 1 to 8 bits using this function c E [1, 8].
	// Return value undefined if c <= 0 or c > 8 or n >= GetNumBits. If it goes off the end no more bits are returned,
	// for example, if the bit array has 11101 and you call with (2,6) you'll get 101.
	uint8 GetBits(int n, int c) const;

	void SetBit(int n, bool v);
	void SetAll(bool v = true);
	void ClearAll();
	void InvertAll();
	bool AreAll(bool v) const;
	int GetNumBits() const																								{ return NumBits; }
	int CountBits(bool = true) const;						// Counts how many bits are set to supplied value.

	// These deal with the internal uint8 elements that store the bit array.
	int GetNumElements() const																							{ return ((NumBits + 7) / 8); }

	// Ensure array is valid and i is in range before calling these. That part is up to you.
	uint8 GetElement(int i) const																						{ tAssert(IsValid()); return ElemData[i]; }
	void SetElement(int i, uint8 val)																					{ tAssert(IsValid()); ElemData[i] = val; }

	void GetElements(uint8* dest) const																					{ tAssert(dest); tStd::tMemcpy(dest, ElemData, GetNumElements()); }
	void SetElements(const uint8* src)						/* Clears unused bits. */									{ tAssert(src); tStd::tMemcpy(ElemData, src, GetNumElements()); ClearPadBits(); }

	uint8& Element(int i)																								{ return ElemData[i]; }
	uint8* Elements() const																								{ return ElemData; }

	// Returns index of first bit that is 0. Returns -1 if no bits are clear.
	int FindFirstClearBit() const;

	// Binary operators must operate on arrays with the same number of bits.
	tBitArray8& operator=(const tBitArray8& src)																		{ Set(src); return *this; }
	tBitArray8& operator&=(const tBitArray8&);
	tBitArray8& operator|=(const tBitArray8&);
	tBitArray8& operator^=(const tBitArray8&);

	const tBitArray8 operator~() const																					{ tBitArray8 r(*this); r.InvertAll(); return r; }
	bool operator[](int n) const																						{ return GetBit(n); }
	bool operator==(const tBitArray8&) const;
	bool operator!=(const tBitArray8&) const;

private:
	void ClearPadBits();

	int NumBits			= 0;								// Number of bits. Not number of fields.
	uint8* ElemData		= nullptr;							// If there are padding bits, they must be set to 0.
};


// Implementation below this line.

//
// tBitArray8 inlines.
//
inline uint8 tBitArray::GetBitInt(int index) const
{
	tAssert(index < NumBits);
	int fieldIndex = index >> 5;
	int offset = index & 0x1F;
	uint32 mask = 1 << offset;

	return (ElemData[fieldIndex] & mask) ? 1 : 0;
}


inline bool tBitArray::GetBit(int index) const
{
	return GetBitInt(index) ? true : false;
}


inline uint8 tBitArray::GetBits(int n, int c) const
{
	if ((c <= 0) || (c > 8) || (n >= NumBits))
		return 0;

	// Reduce count if it goes off end.
	if ((n + c) > NumBits)
		c = NumBits - n;

	// @todo This could be made more efficient by grabbing the one or two 32-bit elements and
	// doing bit ops on each to capture more than one bit at a time.
	uint8 result = 0;
	for (int i = 0; i < c; i++)
		result |= GetBitInt(n+i) << (c-i-1);

	return result;
}


inline void tBitArray::SetBit(int index, bool v)
{
	tAssert(index < NumBits);
	int fieldIndex = index >> 5;
	int offset = index & 0x1F;
	uint32 mask = 1 << offset;
	if (v)
		ElemData[fieldIndex] |= mask;
	else
		ElemData[fieldIndex] &= ~mask;
}


inline void tBitArray::SetAll(bool v)
{
	int n = GetNumElements();
	tStd::tMemset(ElemData, v ? 0xFF : 0, n*sizeof(uint32));
	if (v)
		ClearPadBits();
}


inline void tBitArray::ClearAll()
{
	tAssert(ElemData);
	int n = GetNumElements();
	tStd::tMemset(ElemData, 0, n*sizeof(uint32));
}


inline void tBitArray::InvertAll()
{
	int n = GetNumElements();
	for (int i = 0; i < n; i++)
		ElemData[i] = ~ElemData[i];

	ClearPadBits();
}


inline tBitArray& tBitArray::operator&=(const tBitArray& s)
{
	tAssert(NumBits == s.NumBits);
	int n = GetNumElements();
	for (int i = 0; i < n; i++)
		ElemData[i] &= s.ElemData[i];

	return *this;	// No need to ensure pad bits are cleared because 0 & 0 = 0.
}


inline tBitArray& tBitArray::operator|=(const tBitArray& s)
{
	tAssert(NumBits == s.NumBits);
	int n = GetNumElements();
	for (int i = 0; i < n; i++)
		ElemData[i] |= s.ElemData[i];

	return *this;	// No need to ensure pad bits are cleared because 0 | 0 = 0.
}


inline tBitArray& tBitArray::operator^=(const tBitArray& s)
{
	tAssert(NumBits == s.NumBits);
	int n = GetNumElements();
	for (int i = 0; i < n; i++)
		ElemData[i] ^= s.ElemData[i];

	return *this;	// No need to ensure pad bits are cleared because 0 ^ 0 = 0.
}


inline bool tBitArray::operator==(const tBitArray& s) const
{
	tAssert(GetNumBits() == s.GetNumBits());
	int n = GetNumElements();

	// Padding bits are guaranteed 0 so we can coompare elements.
	for (int i = 0; i < n; i++)
		if (ElemData[i] != s.ElemData[i])
			return false;

	return true;
}


inline void tBitArray::ClearPadBits()
{
	tAssert(ElemData);
	int n = GetNumElements();
	int last = NumBits & 0x1F;
	uint32 maxFull = (last ? (1 << last) : 0) - 1;
	ElemData[n-1] &= maxFull;
}


//
// tBitArray8 inlines.
//
inline uint8 tBitArray8::GetBitInt(int index) const
{
	tAssert(index < NumBits);
	int fieldIndex = index >> 3;
	int offset = 7-(index & 0x07);
	uint8 mask = 1 << offset;

	return (ElemData[fieldIndex] & mask) ? 1 : 0;
}


inline bool tBitArray8::GetBit(int index) const
{
	return GetBitInt(index) ? true : false;
}


inline uint8 tBitArray8::GetBits(int n, int c) const
{
	if ((c <= 0) || (c > 8) || (n >= NumBits))
		return 0;

	// Reduce count if it goes off end.
	if ((n + c) > NumBits)
		c = NumBits - n;

	// @todo This could be made more efficient by grabbing the one or two 8-bit elements and
	// doing bit ops on each to capture more than one bit at a time.
	uint8 result = 0;
	for (int i = 0; i < c; i++)
		result |= GetBitInt(n+i) << (c-i-1);

	return result;
}


inline void tBitArray8::SetBit(int index, bool v)
{
	tAssert(index < NumBits);
	int fieldIndex = index >> 3;
	int offset = 7-(index & 0x07);
	uint8 mask = 1 << offset;
	if (v)
		ElemData[fieldIndex] |= mask;
	else
		ElemData[fieldIndex] &= ~mask;
}


inline void tBitArray8::SetAll(bool v)
{
	int n = GetNumElements();
	tStd::tMemset(ElemData, v ? 0xFF : 0x00, n*sizeof(uint8));
	if (v)
		ClearPadBits();
}


inline void tBitArray8::ClearAll()
{
	tAssert(ElemData);
	int n = GetNumElements();
	tStd::tMemset(ElemData, 0, n*sizeof(uint8));
}


inline void tBitArray8::InvertAll()
{
	int n = GetNumElements();
	for (int i = 0; i < n; i++)
		ElemData[i] = ~ElemData[i];

	ClearPadBits();
}


inline tBitArray8& tBitArray8::operator&=(const tBitArray8& s)
{
	tAssert(NumBits == s.NumBits);
	int n = GetNumElements();
	for (int i = 0; i < n; i++)
		ElemData[i] &= s.ElemData[i];

	return *this;	// No need to ensure pad bits are cleared because 0 & 0 = 0.
}


inline tBitArray8& tBitArray8::operator|=(const tBitArray8& s)
{
	tAssert(NumBits == s.NumBits);
	int n = GetNumElements();
	for (int i = 0; i < n; i++)
		ElemData[i] |= s.ElemData[i];

	return *this;	// No need to ensure pad bits are cleared because 0 | 0 = 0.
}


inline tBitArray8& tBitArray8::operator^=(const tBitArray8& s)
{
	tAssert(NumBits == s.NumBits);
	int n = GetNumElements();
	for (int i = 0; i < n; i++)
		ElemData[i] ^= s.ElemData[i];

	return *this;	// No need to ensure pad bits are cleared because 0 ^ 0 = 0.
}


inline bool tBitArray8::operator==(const tBitArray8& s) const
{
	tAssert(GetNumBits() == s.GetNumBits());
	int n = GetNumElements();

	// Padding bits are guaranteed 0 so we can coompare elements.
	for (int i = 0; i < n; i++)
		if (ElemData[i] != s.ElemData[i])
			return false;

	return true;
}


inline void tBitArray8::ClearPadBits()
{
	tAssert(ElemData);
	int n = GetNumElements();
	int last = NumBits & 0x07;
	if (last)
	{
		uint8 maxFull = ((1 << last) - 1) << (8-last);
		ElemData[n-1] &= maxFull;
	}
}
