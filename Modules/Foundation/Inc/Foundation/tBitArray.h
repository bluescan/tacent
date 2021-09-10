// tBitArray.h
//
// A tBitArray is a holder for an arbitrary number of bits and allows individual access to each bit, the ability to
// clear or set all bits, and some simple binary bitwise operators such as 'and', 'xor', and 'or'. It currently does
// not support dynamic growing or shrinking.
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
// Copyright (c) 2004-2006, 2015, 2017, 2019, 2021 Tristan Grimmer.
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


class tBitArray
{
public:
	tBitArray()												/* Creates an invalid bit array. Call Set before using. */	: NumBits(0), ElemData(nullptr) { }
	tBitArray(int numBits)									/* All bit values guaranteed to be 0 after this. */			: NumBits(0), ElemData(nullptr) { Set(numBits); }
	tBitArray(const uint32* data, int numBits)				/* Copies numBits from data. */								: NumBits(0), ElemData(nullptr) { Set(data, numBits); }
	tBitArray(const tBitArray& src)																						: NumBits(0), ElemData(nullptr) { Set(src); }
	~tBitArray()																										{ delete[] ElemData; }

	void Set(int numBits);									// All bit values guaranteed to be 0 after this.
	void Set(const uint32* data, int numBits);				// Copies numBits from data.
	void Set(const tBitArray& src);
	void Clear()											/* Invalidates. Use ClearAll if you want all bits clear. */	{ delete[] ElemData; ElemData = nullptr; NumBits = 0; }
	bool IsValid() const																								{ return ElemData ? true : false; }

	bool GetBit(int n) const;								// n is the bit index with 0 being the least significant.
	void SetBit(int n, bool v);
	void SetAll(bool v = true);
	void ClearAll();
	void InvertAll();
	bool AreAll(bool v) const;
	int GetNumBits() const																								{ return NumBits; }
	int CountBits(bool = true) const;						// Counts how many bits are set to supplied value.

	// These deal with the internal uint32 elements that store the bit array. The elements are always least-significant
	// at the beginning, regardless of machine endianness.
	int GetNumElements() const																							{ return (NumBits>>5) + ((NumBits & 0x1F) ? 1 : 0); }

	// Ensure array is valid and i is in range before calling these. That part is up to you.
	uint32 GetElement(int i) const																						{ tAssert(IsValid()); return ElemData[i]; }
	void SetElement(int i, uint32 val)																					{ tAssert(IsValid()); ElemData[i] = val; }

	void GetElements(uint32* dest) const					/* Least significant at the beginning. */					{ tAssert(dest); tStd::tMemcpy(dest, ElemData, GetNumElements()*4); }
	void SetElements(const uint32* src)						/* Least sig at the beginning. Clears unused bits. */		{ tAssert(src); tStd::tMemcpy(ElemData, src, GetNumElements()*4); ClearPadBits(); }

	uint32& Element(int i)																								{ return ElemData[i]; }
	uint32* Elements() const																							{ return ElemData; }

	// Returns index of a bit that is clear. Which one is arbitrary. Returns -1 if none are available.
	int GetClearedBitPos() const;

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
	int GetClearedBit(int index) const;
	void ClearPadBits();

	int NumBits;											// Number of bits. Not number of fields.
	uint32* ElemData;										// If there are padding bits, they must be set to 0.
};


// Implementation below this line.


inline bool tBitArray::GetBit(int index) const
{
	tAssert(index < NumBits);
	int fieldIndex = index >> 5;
	int offset = index & 0x1F;
	uint32 mask = 1 << offset;

	return (ElemData[fieldIndex] & mask) ? true : false;
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


inline bool tBitArray::operator==(const tBitArray& s) const
{
	tAssert(GetNumBits() == s.GetNumBits());
	int n = GetNumElements();
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
