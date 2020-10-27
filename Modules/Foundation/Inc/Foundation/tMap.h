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
#include "Foundation/tFundamentals.h"


template<typename K, typename V> class tMap
{
public:
	tMap(int initialLog2Size = 8, float rehashPercent = 0.6f);
	~tMap();

	V& GetInsert(const K&);
	V& operator[](const K&);
	bool Remove(const K&);

	int GetNumItems() const																								{ return NumItems; }
	int GetHashTableSize() const																						{ return HashTableSize; }
	float GetPercentFull() const																						{ return float(HashTableEntryCount)/float(HashTableSize); }

private:
	template<typename KK, typename VV> struct Pair
	{
		Pair(const Pair& pair) : Key(pair.Key), Value(pair.Value) { }
		Pair(const K& key) : Key(key), Value() { }
		KK Key; VV Value;
	};
	struct HashTableItem
	{
		tItList<Pair<K,V>> Pairs;
	};
	int NumItems;
	int HashTableSize;
	int HashTableEntryCount;
	HashTableItem* HashTable;
	float RehashPercent;

	void Rekey(int newSize);
};


// Implementation below this line.


template<typename K, typename V> inline tMap<K,V>::tMap(int initialLog2Size, float rehashPercent)
{
	NumItems = 0;

	tAssert(initialLog2Size >= 0);
	HashTableSize = 1 << initialLog2Size;

	tAssert((rehashPercent >= 0.0f) && (rehashPercent <= 1.0f));
	RehashPercent = rehashPercent;

	HashTableEntryCount = 0;
	HashTable = new HashTableItem[HashTableSize];
}


template<typename K, typename V> inline tMap<K,V>::~tMap()
{
	delete[] HashTable;
}


template<typename K, typename V> inline V& tMap<K,V>::GetInsert(const K& key)
{
	// Do we need to grow the hash table?
	if (GetPercentFull() >= RehashPercent)
		Rekey(2*HashTableSize);

	uint32 hash = uint32(key);
	int hashBits = 	tMath::tLog2(HashTableSize);
	hash = hash & (0xFFFFFFFF >> (32-hashBits));
	tAssert(hash < HashTableSize);

	HashTableItem& item = HashTable[hash];
	for (Pair<K,V>& pair : item.Pairs)
	{
		if (pair.Key == key)
			return pair.Value;
	}

	if (item.Pairs.IsEmpty())
		HashTableEntryCount++;
	NumItems++;
	return item.Pairs.Append(new Pair<K,V>(key))->Value;
}


template<typename K, typename V> inline V& tMap<K,V>::operator[](const K& key)
{
	return GetInsert(key);
}


template<typename K, typename V> inline bool tMap<K,V>::Remove(const K& key)
{
	uint32 hash = uint32(key);
	int hashBits = 	tMath::tLog2(HashTableSize);
	hash = hash & (0xFFFFFFFF >> (32-hashBits));
	tAssert(hash < HashTableSize);

	HashTableItem& item = HashTable[hash];

	for (auto iter = item.Pairs.First(); iter; iter++)
	//for (Pair& pair : item.Pairs)
	{
		if (iter->Key == key)
		{
			delete item.Pairs.Remove(iter);
			if (item.Pairs.IsEmpty())
				HashTableEntryCount--;
			NumItems--;
			return true;
		}
	}

	return false;
}


template<typename K, typename V> inline void tMap<K,V>::Rekey(int newSize)
{
	tAssert((newSize > HashTableSize) && tMath::tIsPower2(newSize));
	HashTableItem* newTable = new HashTableItem[newSize];

	// Loop throught existing keys and rekey them into the new table
	for (int i = 0; i < HashTableSize; i++)
	{
		for (Pair<K,V>& pair : HashTable[i].Pairs)
		{
			uint32 hashNew = uint32(pair.Key);
			int hashBitsNew = tMath::tLog2(newSize);
			hashNew = hashNew & (0xFFFFFFFF >> (32-hashBitsNew));

			tAssert(hashNew < newSize);
			newTable[hashNew].Pairs.Append(new Pair<K,V>(pair));
		}
		HashTable[i].Pairs.Clear();
	}

	HashTableSize = newSize;
	delete[] HashTable;
	HashTable = newTable;
}
