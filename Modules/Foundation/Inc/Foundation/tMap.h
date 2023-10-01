// tMap.h
//
// A map class (dictionary) that keeps track of keys and associated values, both of which may be any type. The
// requirements are: The key type must be copyable, comparable, and convertable to a uint32. The value type must be
// copyable. tMap is implemented as a hash-table with lists to resolve collisions and has expected O(1) running
// time for insertions and value retrievals. The hash table automatically grows when a threshold percentage of the hash
// table is used (defaulting to 60%). Keys are unique -- the last value assigned to a key is the one stored in the tMap.
//
// You may iterate through a tMap to retrieve all keys and values. Range-based for loops are supported. Note that this
// is slightly less efficient than iterating through a tList though, as empty nodes in the hash table are visited.
//
// Copyright (c) 2020, 2023 Tristan Grimmer.
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
#include "Foundation/tList.h"


template<typename K, typename V> class tMap
{
public:
	// Set therekey percent > 1.0f to prevent all rekeying / resizing.
	tMap(int initialLog2Size = 8, float rekeyPercent = 0.6f);
	~tMap();

	// These are fast with expected O(1) running time.
	V& GetInsert(const K&);
	V& operator[](const K&);
	bool Remove(const K&);
	int GetNumItems() const																								{ return NumItems; }

	// These are mostly for debugging or checking performance of the hash table.
	int GetHashTableSize() const																						{ return HashTableSize; }
	int GetHashTableEntryCount() const																					{ return HashTableEntryCount; }
	int GetHashTableCollisions() const																					{ return NumItems - HashTableEntryCount; }
	float GetHashTablePercent() const																					{ return float(HashTableEntryCount)/float(HashTableSize); }

private:
	struct Pair
	{
		Pair(const Pair& pair)																							: Key(pair.Key), Value(pair.Value) { }

		// Note that values-types like int are default-constructed as 0 by Value().
		Pair(const K& key)																								: Key(key), Value() { }
		K Key; V Value;
	};
	struct HashTableItem
	{
		tItList<Pair> Pairs;
	};
	void Rekey(int newSize);
	int NumItems;
	int HashTableSize;
	int HashTableEntryCount;
	HashTableItem* HashTable;
	float RekeyPercent;

public:
	// If needed you can use this iterator to query every key and/or value in the tMap. Note that the keys and values
	// are stored unordered. Further, since not all hash table entries will have valid data, it is slightly less
	// efficient to iterate through a tMap vs interating through, for example, a tItList, since empties must be skipped.
	class Iter
	{
	public:
		Iter()																											: Map(nullptr), TableIndex(-1), PairIter() { }
		Iter(const Iter& src)																							: Map(src.Map), TableIndex(src.TableIndex), PairIter(src.PairIter) { }

		bool IsValid() const																							{ return PairIter.IsValid(); }
		void Clear()																									{ Map = nullptr; TableIndex = -1; PairIter.Clear(); }
		void Next();
		V& Value() const																								{ return PairIter->Value; }
		K& Key() const																									{ return PairIter->Key; }

		Iter& operator*()																								{ return *this; }
		Iter* operator->() const																						{ return this; }
		operator bool() const																							{ return PairIter.IsValid(); }
		Iter& operator=(const Iter& i)																					{ if (this != &i) { Map = i.Map; TableIndex = i.TableIndex; PairIter = i.PairIter; } return *this; }

		// Use ++iter instead of iter++ when possible. Since the hash table is unordered, there's no point in offering
		// both forward and backwards iteration. Therefore there's only First, Next, operator++ etc.
		const Iter operator++(int)																						{ Iter curr(*this); Next(); return curr; }
		const Iter operator++()																							{ Next(); return *this; }
		const Iter operator+(int offset) const																			{ Iter i = *this; while (offset--) i.Next(); return i; }
		Iter& operator+=(int offset)																					{ tAssert(offset >= 0); while (offset--) Next(); return *this; }
		bool operator==(const Iter& i) const																			{ return (Map == i.Map) && (TableIndex == i.TableIndex) && (PairIter == i.PairIter); }
		bool operator!=(const Iter& i) const																			{ return (Map != i.Map) || (TableIndex != i.TableIndex) || (PairIter != i.PairIter); }

	private:
		friend class tMap;
		Iter(const tMap<K,V>* map, int tableIndex, typename tItList<Pair>::Iter iter)									: Map(map), TableIndex(tableIndex), PairIter(iter) { }
		const tMap<K,V>* Map;
		int TableIndex;
		typename tItList<Pair>::Iter PairIter;
	};

public:
	Iter First() const;
	Iter begin() const										/* For range-based iteration supported by C++11. */			{ return First(); }
	Iter end() const										/* For range-based iteration supported by C++11. */			{ return Iter(this, -1, typename tItList<Pair>::Iter()); }
};


// Implementation below this line.


template<typename K, typename V> inline tMap<K,V>::tMap(int initialLog2Size, float rekeyPercent)
{
	NumItems = 0;

	tAssert(initialLog2Size >= 0);
	HashTableSize = 1 << initialLog2Size;

	tAssert(rekeyPercent >= 0.0f);
	RekeyPercent = rekeyPercent;

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
	if (GetHashTablePercent() >= RekeyPercent)
		Rekey(2*HashTableSize);

	uint32 hash = uint32(key);
	int hashBits = 	tMath::tLog2(HashTableSize);
	hash = hash & (0xFFFFFFFF >> (32-hashBits));
	tAssert(hash < HashTableSize);

	HashTableItem& item = HashTable[hash];
	for (Pair& pair : item.Pairs)
	{
		if (pair.Key == key)
			return pair.Value;
	}

	if (item.Pairs.IsEmpty())
		HashTableEntryCount++;
	NumItems++;
	return item.Pairs.Append(new Pair(key))->Value;
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
		for (Pair& pair : HashTable[i].Pairs)
		{
			uint32 hashNew = uint32(pair.Key);
			int hashBitsNew = tMath::tLog2(newSize);
			hashNew = hashNew & (0xFFFFFFFF >> (32-hashBitsNew));

			tAssert(hashNew < newSize);
			newTable[hashNew].Pairs.Append(new Pair(pair));
		}
		HashTable[i].Pairs.Clear();
	}

	HashTableSize = newSize;
	delete[] HashTable;
	HashTable = newTable;
}


template<typename K, typename V> inline void tMap<K,V>::Iter::Next()
{
	// If we can just advance the PairIter, we're done.
	PairIter++;
	if (PairIter.IsValid())
		return;

	// Try next hash table entries until we either find a non-empty one or reach the end of the table.
	while (++TableIndex < Map->HashTableSize)
	{
		HashTableItem& item = Map->HashTable[TableIndex];
		if (!item.Pairs.IsEmpty())
		{
			PairIter = item.Pairs.First();
			return;
		}
	};

	// It is vital to have 'this' be the same as end() here, as ranged-based for loops must return false when
	// comparing the last Next() with end() using != operator. This is why must set the index to -1. To  make
	// sure it matches end().
	TableIndex = -1;
	PairIter = typename tItList<Pair>::Iter();	
}


template<typename K, typename V> inline typename tMap<K,V>::Iter tMap<K,V>::First() const
{
	for (int tableIndex = 0; tableIndex < HashTableSize; tableIndex++)
	{
		HashTableItem& item = HashTable[tableIndex];
		if (!item.Pairs.IsEmpty())
			return Iter(this, tableIndex, item.Pairs.First());
	}
	return Iter(this, -1, typename tItList<Pair>::Iter());
}
