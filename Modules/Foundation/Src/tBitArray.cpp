// tBitArray.cpp
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

#include "Foundation/tBitArray.h"
#include "Foundation/tStandard.h"
#include "Foundation/tFundamentals.h"


void tBitArray::Set(int numBits)
{
	Clear();
	tAssert(numBits > 0);

	NumBits = numBits;
	int n = GetNumElements();
	ElemData = new uint32[n];
	tStd::tMemset(ElemData, 0, n*sizeof(uint32));
}


void tBitArray::Set(uint32* data, int numBits, bool steal)
{
	Clear();
	tAssert(data && numBits);

	NumBits = numBits;
	int n = GetNumElements();

	if (steal)
	{
		ElemData = data;
	}
	else
	{
		ElemData = new uint32[n];
		tStd::tMemcpy(ElemData, data, n*sizeof(uint32));
	}
	ClearPadBits();
}


void tBitArray::Set(const tBitArray& src)
{
	if (&src == this)
		return;

	Clear();
	if (!src.IsValid())
		return;

	NumBits = src.NumBits;
	int n = src.GetNumElements();
	ElemData = new uint32[n];
	tStd::tMemcpy(ElemData, src.ElemData, n*sizeof(uint32));
}


bool tBitArray::AreAll(bool v) const
{
	tAssert(ElemData);
	int n = GetNumElements();
	uint32 fullField = v ? 0xFFFFFFFF : 0x00000000;
	for (int i = 0; i < n-1; i++)
		if (ElemData[i] != fullField)
			return false;

	// Deal with the bits in the last field.
	int last = NumBits & 0x1F;
	uint32 maxFull = (last ? (1 << last) : 0) - 1;
	fullField = v ? maxFull : 0x00000000;
	return (ElemData[n-1] & maxFull) == fullField;
}


int tBitArray::CountBits(bool val) const
{
	// Uses technique described here "http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan".
	// This is one reason the pad bits must always be cleared.
	tAssert(ElemData);
	int numFields = GetNumElements();

	int count = 0;
	for (int n = 0; n < numFields; n++)
	{
		uint32 v = ElemData[n];			// Count the number of bits set in v.
		uint32 c;						// c accumulates the total bits set in v.
		for (c = 0; v; c++)
			v &= v - 1;					// Clear the least significant bit set.
		count += c;
	}
	
	return val ? count : (NumBits - count);
}


int tBitArray::FindFirstClearBit() const
{
	int n = GetNumElements();
	int index = -1;
	for (int i = 0; i < n; i++)
	{
		uint32 elem = ElemData[i];
		if (elem != 0xFFFFFFFF)
		{
			index = 32*i + tMath::tFindFirstClearBit(elem);
			break;
		}
	}

	// If the zero was found in the padding bits, it doesn't count.
	if (index >= NumBits)
		return -1;

	return index;
}


//
// tBitArray8.
//
void tBitArray8::Set(int numBits)
{
	Clear();
	tAssert(numBits > 0);

	NumBits = numBits;
	int n = GetNumElements();
	ElemData = new uint8[n];
	tStd::tMemset(ElemData, 0, n);
}


void tBitArray8::Set(uint8* data, int numBits, bool steal)
{
	Clear();
	tAssert(data && numBits);
	NumBits = numBits;
	int n = GetNumElements();
	if (steal)
	{
		ElemData = data;
	}
	else
	{
		ElemData = new uint8[n];
		tStd::tMemcpy(ElemData, data, n);
	}
	ClearPadBits();
}


void tBitArray8::Set(const tBitArray8& src)
{
	if (&src == this)
		return;

	Clear();
	if (!src.IsValid())
		return;

	NumBits = src.NumBits;
	int n = src.GetNumElements();
	ElemData = new uint8[n];
	tStd::tMemcpy(ElemData, src.ElemData, n);
}


bool tBitArray8::AreAll(bool v) const
{
	tAssert(ElemData);
	int n = GetNumElements();
	uint8 fullField = v ? 0xFF : 0x00;
	for (int i = 0; i < n-1; i++)
		if (ElemData[i] != fullField)
			return false;

	// Deal with the bits in the last field.
	int last = NumBits & 0x07;
	if (!last)
		return true;

	uint8 maxFull = ((1 << last) - 1) << (8-last);
	fullField = v ? maxFull : 0x00;
	return (ElemData[n-1] & maxFull) == fullField;
}


int tBitArray8::CountBits(bool val) const
{
	// Uses technique described here "http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan".
	// This is one reason the pad bits must always be cleared.
	tAssert(ElemData);
	int numFields = GetNumElements();

	int count = 0;
	for (int n = 0; n < numFields; n++)
	{
		uint8 v = ElemData[n];			// Count the number of bits set in v.
		uint8 c;						// c accumulates the total bits set in v.
		for (c = 0; v; c++)
			v &= v - 1;					// Clear the least significant bit set.
		count += c;
	}
	
	return val ? count : (NumBits - count);
}


int tBitArray8::FindFirstClearBit() const
{
	int n = GetNumElements();
	int index = -1;
	for (int i = 0; i < n; i++)
	{
		uint8 elem = ElemData[i];
		if (elem != 0xFF)
		{
			tMath::tiReverseBits(elem);
			index = 8*i + tMath::tFindFirstClearBit(elem);
			break;
		}
	}

	// If the zero was found in the padding bits, it doesn't count.
	if (index >= NumBits)
		return -1;

	return index;
}
