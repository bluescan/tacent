// tMap.h
//
// tMap is implemented as a hash table with incremented resolves.
// 256 expected items.
// 512 table sise.
// x = 8 bit hash E [0, 256)
// n = number of items.
// Exspected unique U = x(1 - (1 - 1/x)^n)
// At n=256, U = 256(1 - (1 - 1/256)^256) = 162   (256/162)=1,58items with space per item)
// Collisions = 256-162 = 94. Free94/256Total is the probability of next one being free = 36%
// x = 512 -> U = 201  -> C = 55. Free(512-201)/512 = 60%
// 512/201 =2.54
//
// Copyright (c) 2020 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include "Foundation/tAssert.h"
#include "Foundation/tPlatform.h"
#include "Math/tFundamentals.h"


template<typename K, typename V> class tMap
{
public:
	tMap(int initialLog2Size = 8);
	~tMap();
	void Insert(const K&, const V&);
	V& Get(const K&);
	V& Remove(const K&);

	V& operator[](const K&);

//private:
	struct HashTableItem
	{
		uint32 InUse = 0;
		V Value;
	};
	int HashTableSize;
	HashTableItem* HashTable;
};


// Implementation below this line.


template<typename K, typename V> inline tMap<K,V>::tMap(int initialLog2Size)
{
	tAssert(initialLog2Size >= 0);
	HashTableSize = 1 << initialLog2Size;
	HashTable = new HashTableItem[HashTableSize];
}


template<typename K, typename V> inline tMap<K,V>::~tMap()
{
	delete[] HashTable;
}


template<typename K, typename V> inline void tMap<K,V>::Insert(const K& key, const V& value)
{
	// Relies on overloaded cast operator of the key type.
	uint32 hash = uint32(key);
	int hashBits = 	tStd::tLog2(HashTableSize);
	hash = hash & (0xFFFFFFFF >> (32-hashBits));
	tAssert(hash < HashTableSize);

	HashTableItem& item = HashTable[hash];
	if (item.InUse)
	{
		// @wip deal with collision.
	}
	else
	{
		item.InUse = 1;
		item.Value = value;
	}
}


template<typename K, typename V> inline V& tMap<K,V>::Get(const K& key)
{
	uint32 hash = uint32(key);
	int hashBits = 	tStd::tLog2(HashTableSize);
	hash = hash & (0xFFFFFFFF >> (32-hashBits));
	tAssert(hash < HashTableSize);

	HashTableItem& item = HashTable[hash];
	return item.Value;
}


template<typename K, typename V> inline V& tMap<K,V>::Remove(const K& key)
{
	// Relies on overloaded cast operator of the key type.
	uint32 hash = uint32(key);

	// @wip
}


