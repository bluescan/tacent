// tArray.h
//
// A simple array implementation that can grow its memory as needed. Adding elements or to an array or adding two
// arrays together are the sorts if things that may cause an internal grow of the memory.
//
// Copyright (c) 2004-2005, 2017, 2020 Tristan Grimmer.
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
#include "Foundation/tFundamentals.h"


template<typename T> class tArray
{
public:
	tArray()												/* Initially empty array with growCount of 256.*/			{ }
	tArray(int capacity, int growCount = 256)				/* Capacity and growcount must be >= 0. */					: GrowCount(growCount) { Clear(capacity); }
	tArray(const tArray& src)																							{ *this = src; }

	virtual ~tArray()																									{ delete[] Elements; }

	// Frees the current content. Allows you to optionally set the new initial capacity. If growCount == -1, the
	// growCount remains unchanged.
	void Clear(int capacity = 0, int growCount = -1);
	int GetNumAppendedElements() const																					{ return NumAppendedElements; }
	T* GetElements() const																								{ return Elements; }

	// Returns the number of elements that may be stored in the array before a costly grow operation.
	int GetCapacity() const																								{ return Capacity; }

	// Grows the max size (capacity) of the array by the specified number of items.
	bool GrowCapacity(int numElementsGrow);

	// The append calls will grow the array if necessary. If growCount is 0 and there's no more room, false is returned.
	bool Append(const T&);

	// For this append call if the GrowCount is 0 and there is not enough current room, false is returned and the array
	// is left unmodified. If growing is necessary, it will succeed even if the space needed exceeds a single grow.
	// It does this in one shot, but grows by a multiple of the GrowCount.
	bool Append(const T*, int numElements);

	operator T*()																										{ return Elements; }
	operator T*() const																									{ return Elements; }
	operator const T*()																									{ return Elements; }
	operator const T*() const																							{ return Elements; }
	T& operator[](int index)																							{ tAssert((index < Capacity) && (index >= 0)); return Elements[index]; }
	const T operator[](int index) const																					{ tAssert((index < Capacity) && (index >= 0)); return Elements[index]; }
	bool operator==(const tArray&) const;					// Empty arrays are considered equal.
	bool operator!=(const tArray& rhs) const																			{ return !(*this == rhs); }
	const tArray& operator+(const tArray& src)																			{ Append(src.GetElements(), src.GetNumElements()); }
	const tArray& operator=(const tArray&);

private:
	T* Elements = nullptr;
	int NumAppendedElements = 0;

	int Capacity = 0;
	int GrowCount = 256;
};


// Implementation below this line.


template<typename T> inline void tArray<T>::Clear(int capacity, int growCount)
{
	tAssert((capacity >= 0) && (growCount >= -1))
	delete[] Elements;
	Elements = nullptr;
	Capacity = capacity;
	NumAppendedElements = 0;

	if (Capacity > 0)
		Elements = new T[Capacity];

	if (growCount > -1)
		GrowCount = growCount;
}


template<typename T> inline bool tArray<T>::GrowCapacity(int numElementsGrow)
{
	if (numElementsGrow <= 0)
		return false;

	int newCap = Capacity + numElementsGrow;
	T* newItems = new T[newCap];
	for (int e = 0; e < Capacity; e++)
		newItems[e] = Elements[e];

	delete[] Elements;
	Elements = newItems;
	Capacity = newCap;
	return true;
}


template<typename T> inline bool tArray<T>::Append(const T& item)
{
	if (NumAppendedElements >= Capacity)
		GrowCapacity(GrowCount);

	if (NumAppendedElements >= Capacity)
		return false;

	tAssert(NumAppendedElements < Capacity);
	Elements[NumAppendedElements++] = item;
	return true;
}


template<typename T> inline bool tArray<T>::Append(const T* elements, int numElementToAppend)
{
	tAssert(elements && (numElementToAppend > 0));
	int numAvail = Capacity - NumAppendedElements;
	if ((GrowCount <= 0) && (numElementToAppend > numAvail))
		return false;

	// First we append all elements that do not require the array to grow. We could do this in no more than two memcpys
	// if we want a performance boost.
	int count = (numElementToAppend < numAvail) ? numElementToAppend : numAvail;
	int index = 0;
	for (; index < count; index++)
		Elements[NumAppendedElements + index] = elements[index];

	NumAppendedElements += count;
	numElementToAppend -= count;
	if (numElementToAppend <= 0)
		return true;

	// There are some left so we need to grow the array. We grow once in multiples of GrowCount.
	// At this point, GrowCount should be guaranteed to be > 0 since we early exited.
	tAssert(GrowCount > 0);
	tMath::tDivt quotRem = tMath::tDiv(numElementToAppend, GrowCount);
	int numGrows = quotRem.Quotient;
	if (quotRem.Remainder > 0)
		numGrows++;

	GrowCapacity(GrowCount * numGrows);

	// And now we just have a tight loop to copy them in.
	for (int i = 0; i < numElementToAppend; i++)
		Elements[NumAppendedElements + i] = elements[index++];

	NumAppendedElements += numElementToAppend;
	return true;
}


template<typename T> inline const tArray<T>& tArray<T>::operator=(const tArray<T>& src)
{
	if (&src == this)
		return *this;

	Clear(0, src.GrowCount);
	NumElements = src.NumElements;
	Capacity = src.Capacity;
	if (Capacity > 0)
		Elements = new T[Capacity];

	for (int i = 0; i < NumElements; i++)
		Elements[i] = src.Elements[i];

	return *this;
}


template<typename T> inline bool tArray<T>::operator==(const tArray& rhs) const
{
	if (NumElements != rhs.NumElements)
		return false;

	for (int i = 0; i < NumElements; i++)
		if (Elements[i] != rhs.Elements[i])
			return false;

	return true;
}
