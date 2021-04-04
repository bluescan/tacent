// tSmartPointers.h
//
// Wanted to try my hand at some smart pointers starting with a shared pointer implementation, but eventually including
// a weak-pointer implementation.
//
// Copyright (c) 2021 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <mutex>
#include "Foundation/tList.h"


template<typename T> class tSharedPtr
{
public:
	tSharedPtr()																										{ SatData = new SatelliteData(); }
	tSharedPtr(const tSharedPtr& src);						// Copy cons.
	tSharedPtr(tSharedPtr&& src);							// Move cons.
	tSharedPtr(T* src)																									: ObjPointer(src) { SatData = new SatelliteData(1); }
	~tSharedPtr()																										{ DerefDelete(); }

	tSharedPtr& operator=(const tSharedPtr& src);			// Copy.
	tSharedPtr& operator=(tSharedPtr&& src);				// Move.

	T* GetObject() const																								{ return ObjPointer; }
	T* operator->() const																								{ return ObjPointer; }
	T& operator*() const																								{ return *ObjPointer; }

	int GetRefCount() const;									// Debugging.
	bool IsValid() const																								{ return ObjPointer ? true : false; }

private:
	void Invalidate()																									{ ObjPointer = nullptr; SatData = nullptr; }
	void DerefDelete();

	struct SatelliteData
	{
		SatelliteData(int count)																						: RefCount(count) { }
		int RefCount = 0;

		// Mods to the satellite information are considered as critical sections.
		std::mutex Mutex;

		// @todo Keep track of weak references here. Will need a tList of them. May need ti invalidate them.
	};

	T* ObjPointer = nullptr;
	SatelliteData* SatData = nullptr;
};


// Implementation below this line.


template<typename T> inline tSharedPtr<T>::tSharedPtr(const tSharedPtr<T>& src)
{
	// We share both the object pointer and satellite data (inc ref count) of the source.
	ObjPointer = src.ObjPointer;
	SatData = src.SatData;
	if (IsValid())
	{
		SatData->Mutex.lock();
		(SatData->RefCount)++;
		SatData->Mutex.unlock();
	}
}


template<typename T> inline tSharedPtr<T>::tSharedPtr(tSharedPtr&& src)
{
	// We share both the object pointer and satellite data (inc ref count) of the source.
	ObjPointer = src.ObjPointer;
	SatData = src.SatData;
	src.Invalidate();
}


template<typename T> inline tSharedPtr<T>& tSharedPtr<T>::operator=(const tSharedPtr& src)
{
	if (this == &src)
		return *this;

	// This could be valid. Dec ref count / maybe delete.
	DerefDelete();
	
	// We share both the object pointer and satellite data (inc ref count) of the source.
	ObjPointer = src.ObjPointer;
	SatData = src.SatData;
	if (src.IsValid())
	{
		SatData->Mutex.lock();
		(SatData->RefCount)++;
		SatData->Mutex.unlock();
	}
}


template<typename T> inline	tSharedPtr<T>& tSharedPtr<T>::operator=(tSharedPtr&& src)
{
	DerefDelete();
	
	// We share both the object pointer and satellite data (inc ref count) of the source.
	ObjPointer = src.ObjPointer;
	SatData = src.SatData;
	src.Invalidate();
}


template<typename T> inline	int tSharedPtr<T>::GetRefCount() const	
{
	int refCount = 0;
	if (!SatData)
		return refCount;
	SatData->Mutex.lock();
	refCount = SatData->RefCount;
	SatData->Mutex.unlock();
	return refCount;
}


template<typename T> inline void tSharedPtr<T>::DerefDelete()
{
	bool deleteData = false;
	tAssert(SatData);
	SatData->Mutex.lock();
	(SatData->RefCount)--;
	if (SatData->RefCount == 0)
		deleteData = true;
	SatData->Mutex.unlock();

	if (deleteData)
	{
		delete ObjPointer;
		ObjPointer = nullptr;
		delete SatData;
		SatData = nullptr;
	}
}
