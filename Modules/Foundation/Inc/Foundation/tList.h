// tList.h
//
// Linked list implementations. There are two implementations in this module. tList is intrusive, tItList (i for
// iterator) is non-intrusive. Use the intrusive one for performance and fewer fragmentation issues if possible.
//
// tList advantages: Faster and less memory fragmentation (one new per object).
// tList disadvantages: An object can only be on one list at a time. You must derive from tLink.
//
// tItList advantages: The same item only in one list at a time. No change in memory image for the objects. Cleaner
// iterator syntax similar to the STL containers. Supports the new C++11 range-based for loop syntax.
// tItList disadvantages: More memory allocs. Not quite as fast.
//
// Copyright (c) 2004-2006, 2015, 2017, 2020, 2022-2024 Tristan Grimmer.
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
#include "Foundation/tAssert.h"
#include "Foundation/tPlatform.h"


enum class tListSortAlgorithm
{
	Merge,													// Guaranteed O(n ln(n)) even in worst case.
	Bubble													// As bad as O(n^2) on unsorted data. Only O(n) on sorted.
};


// You need to derive your object from this if you want to put them on a tList.
template<typename T> class tLink
{
public:
	tLink()																												: NextItem(nullptr), PrevItem(nullptr) { }
	tLink(const tLink&)																									: NextItem(nullptr), PrevItem(nullptr) { }
	T* Next() const																										{ return NextItem; }
	T* Prev() const																										{ return PrevItem; }

private:
	template<typename U> friend class tList;
	T* NextItem;
	T* PrevItem;
};


enum class tListMode
{
	Static,				StaticZero = Static,		// Static must be the first enum item and have value 0. Does not own items.
	External,			UserOwns = External,
	Internal,			ListOwns = Internal
};


// A tList is an intrusive list implementation. Intrusive results in less memory fragmentation but objects can only be
// on one list at a time. A tList may be used in the cdata (global) memory section where zero-initialization is
// guaranteed. In this mode it does NOT delete its items on destruction to avoid both the init and de-init fiasco.
// tLists may also be used on the stack or heap, in which case it is optional to construct with the ListOwns flag. It
// is vital that for zero-init you call the constructor with mode as StaticZero IF you intend to have it populated by
// other statically initialized objects before main() is entered. This is because we don't know what items will have
// already been put on the list before the constructor is called (again, the C++ init fiasco). The constructor in
// StaticZero mode does NOT clear the state variables so it doesn't matter when.
//
// In static mode, the list does not consider itself to own the things on the list. It is safe to add 'this'
// pointers to globally constructed objects for instance. If however you do want the list to delete the items, tList
// will allow you to call Empty on a static-zero list. You can also call Reset (no deletes). Clear will be the same
// as Reset for static-zero lists. For the implementation, the main difference between Static and External is that
// with Static the constructor does not clear the head, tail, and count members because the constructor may be called
// after items are added.
template<typename T> class tList
{
public:
	// The default constructor has the list owning the items. List mode is Internal.
	tList()																												: Mode(tListMode::ListOwns), HeadItem(nullptr), TailItem(nullptr), ItemCount(0) { }

	// If mode is external the objects will not be deleted when the list is destroyed. You manage item lifetime.
	tList(tListMode mode)																								: Mode(mode) { if (mode != tListMode::StaticZero) { HeadItem = nullptr; TailItem = nullptr; ItemCount = 0; } }
		
	virtual ~tList()																									{ if (Owns()) Empty(); }
	
	T* Insert(T* item);										// Insert item at head.	Returns item.
	T* Insert(T* item, T* here);							// Insert item before here. Returns item.
	T* Append(T* item);										// Append item at tail.	Returns item.
	T* Append(T* item, T* here);							// Append item after here. Returns item.
	T* Remove(T* item);										// Removes and returns item.
	T* Remove();											// Removes and returns head item.
	T* Drop();												// Removes and returns tail item.

	void Clear()											/* Clears the list. Deletes items if list owns them. */		{ if (Owns()) Empty(); else Reset(); }
	void Reset()											/* Resets the list. Never deletes the objects. */			{ HeadItem = nullptr; TailItem = nullptr; ItemCount = 0; }
	void Empty()											/* Empties the list. Always deletes the objects. */			{ while (!IsEmpty()) delete Remove(); }

	T* Head() const																										{ return HeadItem; }
	T* Tail() const																										{ return TailItem; }
	T* First() const																									{ return HeadItem; }
	T* Last() const																										{ return TailItem; }

	T* NextCirc(const T* here) const						/* Circular. Gets item after here. */ 						{ return here->NextItem ? here->NextItem : HeadItem; }
	T* PrevCirc(const T* here) const						/* Circular. Gets item before here. */						{ return here->PrevItem ? here->PrevItem : TailItem; }

	int GetNumItems() const																								{ return ItemCount; }
	int NumItems() const																								{ return ItemCount; }
	int Count() const																									{ return ItemCount; }
	bool Owns() const																									{ return (Mode == tListMode::Internal); }
	bool IsEmpty() const																								{ return !HeadItem; }
	bool Contains(const T& item) const						/* To use this there must be an operator== for type T. */	{ for (const T* n = First(); n; n = n->Next()) if (*n == item) return true; return false; }

	// Sorts the list using the algorithm specified. The supplied compare function should never return true on equal.
	// To sort ascending return the truth of a < b. Return a > b to sort in descending order. Returns the number of
	// compares performed.
	//
	// A simple compare function would implement "bool CompareFunc(const T& a, const T& b);"
	// If you want to send user information into your compare function, there is no need to create a static or global
	// object that is error-prone (and definitely not thread-safe). There is also no need for the Sort function to be
	// polluted with an extra user-data argument. Instead use C++ FunctionObjects. A function object is an object that
	// implements operator(). As an example:
	//
	//	struct CompareFunctionObject
	//	{
	//		bool ascending = true; // Plus whatever other sort-specification data you want.
	//		bool operator() (const YourType& a, const YourType& b) const
	//		{
	//			return ascending ? a < b : a > b;	// You implement the compare.
	//		}
	//	}
	//
	// To use this simply do somthing like:
	//
	//	CompareFunctionObject comp;
	//	comp.ascending = false;
	//	MyList.Sort(comp);
	//
	template<typename CompareFunc> int Sort(CompareFunc, tListSortAlgorithm alg = tListSortAlgorithm::Merge);

	// Inserts item in a sorted list. It will remain sorted.
	template<typename CompareFunc> T* Insert(T* item, CompareFunc);

	// This does an O(n) single pass of a bubble sort iteration. Allows the cost of sorting to be distributed over time
	// for objects that do not change their order very often. Do the expensive merge sort when the list is initially
	// populated, then use this to keep it approximately correct.
	//
	// The direction is important when doing this type of partial sort. A forward bubble will result in the last object
	// in the list being correct after one pass, last two for two passes etc. If getting the front of the list correct
	// sooner is more important you'll want to bubble backwards. This is true regardless of whether the compare
	// function implements an ascending or descending sort.
	//
	// maxCompares sets how far into the list to perform possible swaps. If maxCompares == -1 it means go all the way.
	// That is, GetNumItems() - 1. Returns number of swaps performed.
	template<typename CompareFunc> int Bubble(CompareFunc compare, bool backwards = false, int maxCompares = -1)		{ if (backwards) return BubbleBackward(compare, maxCompares); else return BubbleForward(compare, maxCompares); }

private:
	// These return the number of swaps performed. If 0 is returned, the items considered were already in the correct
	// order. The maxCompares allows you to limit how many compares are performed. -1 means numItems-1 compares will be
	// made (a full sweep). The variable gets clamped to [0, numItems) otherwise.
	template<typename CompareFunc> int BubbleForward(CompareFunc, int maxCompares = -1);
	template<typename CompareFunc> int BubbleBackward(CompareFunc, int maxCompares = -1);

	// These return the number of compares performed.
	template<typename CompareFunc> int SortMerge(CompareFunc);
	template<typename CompareFunc> int SortBubble(CompareFunc);

	// Since tList supports static zero-initialization, all defaults for all member vars should be 0.
	tListMode Mode;
	T* HeadItem;
	T* TailItem;
	int ItemCount;
};


// Same as a tList but the default constructor puts the list in External mode.
template<typename T> class teList : public tList<T>
{
public:
	// The default constructor has the list not owning the items. List mode is External.
	teList()																											: tList<T>(tListMode::External) { }
	teList(tListMode mode)																								: tList<T>(mode) { }
	virtual ~teList()																									{ }
};


// Same as a tList but the default constructor puts the list in Static mode.
template<typename T> class tzList : public tList<T>
{
public:
	// The default constructor sets the list mode as Static.
	tzList()																											: tList<T>(tListMode::Static) { }
	tzList(tListMode mode)																								: tList<T>(mode) { }
	virtual ~tzList()																									{ }
};


// Same as a tList but thread-safe. tListMode can be Static, External, or Intrernal. The 'thread-safeness' of a tsList
// extends only to keeping the list consisten (adding, removing, sorting, etc) -- it does _not_ extend to managing and
// synchronizing the lifetime of the items you put on the list. That is your job. For this reason, be careful with
// Internal list-mode, as the list destructor deletes the items just like a regular tList.
template<typename T> class tsList : public tList<T>
{
public:
	// The default constructor uses list-mode External.
	tsList()																											: tList<T>(tListMode::External) { }

	// Mode must be External or Static for thread-safe tsLists.
	tsList(tListMode mode)																								: tList<T>(mode) { }
	virtual ~tsList()																									{ }

	T* Insert(T* item)																									{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Insert(item); }
	T* Insert(T* item, T* here)																							{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Insert(item, here); }
	T* Append(T* item)																									{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Append(item); }
	T* Append(T* item, T* here)																							{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Append(item, here); }
	T* Remove(T* item)																									{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Remove(item); }
	T* Remove()																											{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Remove(); }
	T* Drop()																											{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Drop(); }
	void Clear()																										{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Clear(); }
	void Reset()																										{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Reset(); }
	void Empty()																										{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Empty(); }
	T* Head() const																										{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Head(); }
	T* Tail() const																										{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Tail(); }
	T* First() const																									{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::First(); }
	T* Last() const																										{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Last(); }
	T* NextCirc(const T* here) const																					{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::NextCirc(here); }
	T* PrevCirc(const T* here) const																					{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::PrevCirc(here); }
	int GetNumItems() const																								{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::GetNumItems(); }
	int NumItems() const																								{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::NumItems(); }
	int Count() const																									{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Count(); }
	bool Owns() const																									{ return tList<T>::Owns(); }
	bool IsEmpty() const																								{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::IsEmpty(); }
	bool Contains(const T& item) const																					{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Contains(); }
	template<typename CompareFunc> int Sort(CompareFunc comp, tListSortAlgorithm alg = tListSortAlgorithm::Merge)		{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Sort(comp, alg); }
	template<typename CompareFunc> T* Insert(T* item, CompareFunc comp)													{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Insert(item, comp); }
	template<typename CompareFunc> int Bubble(CompareFunc comp, bool backwards = false, int maxCompares = -1)			{ const std::lock_guard<std::mutex> lock(Mutex); return tList<T>::Bubble(comp, backwards, maxCompares); }

private:
	mutable std::mutex Mutex;
};


// The tItList implements a doubly linked non-intrusive iterator-based list. This list class is implemented by using a
// tList of structs that point to the objects in the list.
template<typename T> class tItList
{
public:
	tItList()																											: Mode(tListMode::ListOwns), Nodes(tListMode::ListOwns) { }
	tItList(tListMode mode)																								: Mode(mode), Nodes(tListMode::ListOwns) { }
	virtual ~tItList()																									{ if (Owns()) Empty(); else Clear(); }

private:
	// This internal datatype is declared early on to make inlining a bit easier.
	struct IterNode : public tLink<IterNode>
	{
		IterNode(const T* obj)																							: tLink<IterNode>(), Object(obj) { }
		const T* Get() const																							{ return Object; }
		const T* Object;
	};

public:
	// The tItList iterator class for external use.
	class Iter
	{
	public:
		Iter()																											: Node(nullptr), List(nullptr) { }
		Iter(const Iter& src)																							: Node(src.Node), List(src.List) { }

		// Iterators may be dereferenced to get to the object.
		T& operator*() const																							{ return *((T*)Node->Object); }
		T* operator->() const																							{ return (T*)Node->Object; }
		operator bool() const																							{ return Node ? true : false; }

		// Iterators may be treated as pointers to the object.
		operator T*()																									{ return GetObject(); }
		operator const T*() const																						{ return GetObject(); }
		Iter& operator=(const Iter& i)																					{ if (this != &i) { Node = i.Node; List = i.List; } return *this; }

		// Use ++iter instead of iter++ when possible.
		const Iter operator++(int)																						{ Iter curr(*this); Next(); return curr; }
		const Iter operator--(int)																						{ Iter curr(*this); Prev(); return curr; }
		const Iter operator++()																							{ Next(); return *this; }
		const Iter operator--()																							{ Prev(); return *this; }
		const Iter operator+(int offset) const																			{ tAssert(offset >= 0); Iter i = *this; while (offset--) i.Next(); return i; }
		const Iter operator-(int offset) const																			{ tAssert(offset >= 0); Iter i = *this; while (offset--) i.Prev(); return i; }
		Iter& operator+=(int offset)																					{ tAssert(offset >= 0); while (offset--) Next(); return *this; }
		Iter& operator-=(int offset)																					{ tAssert(offset >= 0); while (offset--) Prev(); return *this; }
		bool operator==(const Iter& i) const																			{ return (Node == i.Node) && (List == i.List); }
		bool operator!=(const Iter& i) const																			{ return (Node != i.Node) || (List != i.List); }

		bool IsValid() const																							{ return Node ? true : false; }
		void Clear()																									{ Node = nullptr; List = nullptr; }
		void Next()																										{ if (Node) Node = Node->Next(); }
		void Prev()																										{ if (Node) Node = Node->Prev(); }

		// Accessors that allow the list to be treated as circular.
		void NextCirc()																									{ if (Node) Node = Node->Next(); if (!Node) Node = List->Nodes.Head(); }
		void PrevCirc()																									{ if (Node) Node = Node->Prev(); if (!Node) Node = List->Nodes.Tail(); }
		T* GetObject() const																							{ return (T*)(Node ? Node->Object : nullptr); }

	private:
		friend class tItList<T>;
		Iter(IterNode* listNode, const tItList<T>* list)																: Node(listNode), List(list) { }
		IterNode* Node;
		const tItList<T>* List;
	};

	// Insert before head and append after tail. If 'here' is supplied, inserts before here or appends after here.
	T* Insert(T* obj)																									{ tAssert(obj); Nodes.Insert(new IterNode(obj)); return obj; }
	T* Insert(T* obj, const Iter& here)																					{ tAssert(obj); tAssert(this == here.List); Nodes.Insert(new IterNode(obj), here.Node); return obj; }
	T* Append(T* obj)																									{ tAssert(obj); Nodes.Append(new IterNode(obj)); return obj; }
	T* Append(T* obj, const Iter& here)																					{ tAssert(obj); tAssert(this == here.List); Nodes.Append(new IterNode(obj), here.Node); return obj; }

	const T* Insert(const T* obj)																						{ tAssert(obj); Nodes.Insert(new IterNode(obj)); return obj; }
	const T* Insert(const T* obj, const Iter& here)																		{ tAssert(obj); tAssert(this == here.List); Nodes.Insert(new IterNode(obj), here.Node); return obj; }
	const T* Append(const T* obj)																						{ tAssert(obj); Nodes.Append(new IterNode(obj)); return obj; }
	const T* Append(const T* obj, const Iter& here)																		{ tAssert(obj); tAssert(this == here.List); Nodes.Append(new IterNode(obj), here.Node); return obj; }

	T* Remove()												/* Removes and returns head. */								{ Iter head = Head(); return Remove(head); }
	T* Remove(Iter&);										// Removed object referred to by Iter. Invalidates Iter.
	T* Drop()												/* Drops and returns tail. */								{ Iter tail = Tail(); return Drop(tail); }
	T* Drop(Iter& iter)										/* Same a Remove. */										{ return Remove(iter); }

	void Clear()											/* Clears the list. Deletes items if ownership flag set. */	{ if (Owns()) Empty(); else Reset(); }
	void Reset()											/* Resets the list. Never deletes the objects. */			{ while (!IsEmpty()) Remove(); }
	void Empty()											/* Empties the list. Always deletes the objects. */			{ while (!IsEmpty()) delete Remove(); }

	Iter Head() const																									{ return Iter(Nodes.Head(), this); }
	Iter Tail() const																									{ return Iter(Nodes.Tail(), this); }
	Iter First() const																									{ return Iter(Nodes.Head(), this); }
	Iter Last() const																									{ return Iter(Nodes.Tail(), this); }

	// Searches list forward for a particular item. Returns its iterator or an invalid one if it wasn't found.
	Iter Find(const T* item) const																						{ for (IterNode* n = Nodes.First(); n; n = n->Next()) if (n->Object == item) return Iter(n, this); return Iter(); }

	int GetNumItems() const																								{ return Nodes.GetNumItems(); }
	int NumItems() const																								{ return Nodes.NumItems(); }
	int Count() const																									{ return Nodes.Count(); }
	bool IsEmpty()	const																								{ return Nodes.IsEmpty(); }
	bool Owns() const																									{ return (Mode == tListMode::ListOwns); }

	// For range-based iteration supported in C++11. It worth noting that ++Iter must return a value that matches
	// whatever end() returns here. That is != must return false when comparing the final ++Iter next call with
	// what is returned by end(). That is why we return an Iter with a null Node but a _valid_ List pointer.
	Iter begin() const																									{ return Head(); }
	Iter end() const																									{ return Iter(nullptr, this); }

	const T& operator[](const Iter& iter) const																			{ tAssert(iter.IsValid() && (iter.List == this)); return *iter.Node->Object; }
	T& operator[](const Iter& iter)																						{ tAssert(iter.IsValid() && (iter.List == this)); return *((T*)iter.Node->Object); }

	// Sorts the list using the algorithm specified. The supplied compare function should never return true on equal.
	// To sort ascending return the truth of a < b. Return a > b to sort in descending order. Returns the number of
	// compares performed. The compare function should implement bool CompareFunc(const T& a, const T& b)
	template<typename CompareFunc> int Sort(CompareFunc compare, tListSortAlgorithm algo = tListSortAlgorithm::Merge)	{ auto cmp = [&compare](const IterNode& a, const IterNode& b) { return compare(*a.Get(), *b.Get()); }; return Nodes.Sort(cmp, algo); }

	// Inserts item in a sorted list. It will remain sorted.
	template<typename CompareFunc> T* Insert(const T* item, CompareFunc compare)										{ auto cmp = [&compare](IterNode& a, IterNode& b) { return compare(*a.Get(), *b.Get()); }; return Nodes.Insert(item, cmp); }

	// This does an O(n) single pass of a bubble sort iteration. Allows the cost of sorting to be distributed over time
	// for objects that do not change their order very often. Do the expensive merge sort when the list is initially
	// populated, then use this to keep it approximately correct.
	//
	// The direction is important when doing this type of partial sort. A forward bubble will result in the last object
	// in the list being correct after one pass, last two for two passes etc. If getting the front of the list correct
	// sooner is more important you'll want to bubble backwards. This is true regardless of whether the compare
	// function implements an ascending or descending sort.
	//
	// maxCompares sets how far into the list to perform possible swaps. If maxCompares == -1 it means go all the way.
	// That is, GetNumItems() - 1. Returns number of swaps performed.
	//
	// Note that any iterators that you are maintaining should remain valid.
	template<typename CompareFunc> int Bubble(CompareFunc compare, bool backwards = false, int maxCompares = -1)		{ return Nodes.Bubble(compare, backwards, maxCompares); }

private:
	// tItList is implemented using a tList of Nodes that point to the objects.
	tListMode Mode;
	tList<IterNode> Nodes;
};


// Implementation below this line.


template<typename T> inline T* tList<T>::Insert(T* item)
{
	if (HeadItem)
		HeadItem->PrevItem = item;

	item->NextItem = HeadItem;
	item->PrevItem = nullptr;
	HeadItem = item;
	if (!TailItem)
		TailItem = item;

	ItemCount++;
	return item;
}


template<typename T> inline T* tList<T>::Append(T* item)
{
	if (TailItem)
		TailItem->NextItem = item;

	item->PrevItem = TailItem;
	item->NextItem = nullptr;
	TailItem = item;
	if (!HeadItem)
		HeadItem = (T*)item;

	ItemCount++;
	return item;
}


template<typename T> template<typename CompareFunc> inline T* tList<T>::Insert(T* item, CompareFunc compare)
{
	// Find the first item (starting from the smallest/head) that our item is less than.
	for (T* contender = Head(); contender; contender = contender->Next())
		if (compare(*item, *contender))
			return Insert(item, contender);
	
	return Append(item);	
}


template<typename T> inline T* tList<T>::Insert(T* item, T* where)
{
	tAssert(item);
	if (!where)
		return Insert(item);

	item->NextItem = where;
	item->PrevItem = where->PrevItem;
	where->PrevItem = item;

	if (item->PrevItem)
		item->PrevItem->NextItem = item;
	else
		HeadItem = item;

	ItemCount++;
	return item;
}


template<typename T> inline T* tList<T>::Append(T* item, T* where)
{
	tAssert(item);
	if (!where)
		return Append(item);

	item->PrevItem = where;
	item->NextItem = where->NextItem;
	where->NextItem = item;

	if (item->NextItem)
		item->NextItem->PrevItem = item;
	else
		TailItem = item;

	ItemCount++;
	return (T*)item;
}


template<typename T> inline T* tList<T>::Remove()
{
	// It's OK to try to remove from an empty list.
	if (!HeadItem)
		return nullptr;

	T* removed = HeadItem;
	HeadItem = (T*)HeadItem->NextItem;
	if (!HeadItem)
		TailItem = nullptr;
	else
		HeadItem->PrevItem = nullptr;

	ItemCount--;
	return removed;
}


template<typename T> inline T* tList<T>::Drop()
{
	// It's OK to try to lose something from an empty list.
	if (!TailItem)
		return nullptr;

	T* dropped = TailItem;
	TailItem = (T*)TailItem->PrevItem;
	if (!TailItem)
		HeadItem = nullptr;
	else
		TailItem->NextItem = nullptr;

	ItemCount--;
	return dropped;
}


template<typename T> inline T* tList<T>::Remove(T* item)
{
	if (item->PrevItem)
		item->PrevItem->NextItem = item->NextItem;
	else
		HeadItem = item->NextItem;

	if (item->NextItem)
		item->NextItem->PrevItem = item->PrevItem;
	else
		TailItem = item->PrevItem;

	ItemCount--;
	return item;
}


template<typename T> template<typename CompareFunc> inline int tList<T>::Sort(CompareFunc compare, tListSortAlgorithm algorithm)
{
	switch (algorithm)
	{
		case tListSortAlgorithm::Bubble:
			return SortBubble(compare);
			break;

		case tListSortAlgorithm::Merge:
		default:
			return SortMerge(compare);
			break;
	}

	return 0;
}


template<typename T> template<typename CompareFunc> inline int tList<T>::SortMerge(CompareFunc compare)
{
	if (!HeadItem)
		return 0;

	// Treat every node as a separate list, completely sorted, starting with 1 element each.
	int numNodesPerList = 1;
	int numCompares = 0;

	while (1)
	{
		T* p = HeadItem;
		HeadItem = nullptr;
		TailItem = nullptr;

		// Num merges in this loop.
		int numMerges = 0;

		while (p)
		{
			numMerges++;
			T* q = p;
			int numPNodes = 0;
			for (int i = 0; i < numNodesPerList; i++)
			{
				numPNodes++;
				q = q->Next();
				if (!q)
					break;
			}

			int numQNodes = numNodesPerList;

			// Merge the two lists.
			while (numPNodes > 0 || (numQNodes > 0 && q))
			{
				// Decide whether next tBaseLink of merge comes from p or q.
				T* e;

				if (numPNodes == 0)
				{
					// p is empty; e must come from q.
					e = q;
					q = q->Next();

					numQNodes--;
				}
				else if (numQNodes == 0 || !q)
				{
					// q is empty; e must come from p.
					e = p;
					p = p->Next();

					numPNodes--;
				}
				else if (++numCompares && !compare(*q, *p))
				{
					// p is lower so e must come from p.
					e = p;
					p = p->Next();
					
					numPNodes--;
				}
				else
				{
					// First gBaseNode of q is bigger or equal; e must come from q.
					e = q;
					q = q->Next();
					
					numQNodes--;
				}

				// add the next gBaseNode to the merged list.
				if (TailItem)
					TailItem->NextItem = e;
				else
					HeadItem = e;

				e->PrevItem = TailItem;
				TailItem = e;
			}

			// P and Q have moved numNodesPerList places along.
			p = q;
		}
		TailItem->NextItem = nullptr;

		// If we have done only one merge, we're all sorted.
		if (numMerges <= 1)
			return numCompares;

		// Otherwise repeat, merging lists twice the size.
		numNodesPerList *= 2;
	}

	return numCompares;
}


template<typename T> template<typename CompareFunc> inline int tList<T>::SortBubble(CompareFunc compare)
{
	// Performs a full bubble sort.
	int numCompares = 0;
	for (int maxCompares = ItemCount - 1; maxCompares >= 1; maxCompares--)
	{
		int numSwaps = BubbleForward(compare, maxCompares);
		numCompares += maxCompares;

		// Early exit detection. If any bubble pass resulted in no swaps, we're done!
		if (!numSwaps)
			return numCompares;
	}

	return numCompares;
}


template<typename T> template<typename CompareFunc> inline int tList<T>::BubbleForward(CompareFunc compare, int maxCompares)
{
	// @todo Can -1 ever happen?
	if (maxCompares == -1)
		maxCompares = ItemCount-1;
	else if (maxCompares > ItemCount-1)
		maxCompares = ItemCount-1;

	// If there are no items, only a single item, or no compares are requested, we're done.
	if ((ItemCount < 2) || (maxCompares == 0))
		return 0;

	T* a = HeadItem;
	int numSwaps = 0;
	int numCompares = 0;

	while ((a != TailItem) && (numCompares < maxCompares))
	{
		T* b = a->NextItem;

		// We're sorting from lowest to biggest, so if b < a we need to swap them.
		if (compare(*b, *a))
		{
			// Swap.
			if (a->PrevItem)
				a->PrevItem->NextItem = b;

			if (b->NextItem)
				b->NextItem->PrevItem = a;

			a->NextItem = b->NextItem;
			b->PrevItem = a->PrevItem;

			a->PrevItem = b;
			b->NextItem = a;

			// Fix head and tail if they were involved in the swap.
			if (HeadItem == a)
				HeadItem = b;

			if (TailItem == b)
				TailItem = a;

			// Since we swapped, a is now correctly ready for the next loop.
			numSwaps++;
		}
		else
		{
			a = a->NextItem;
		}
		numCompares++;
	}

	return numSwaps;
}


template<typename T> template<typename CompareFunc> inline int tList<T>::BubbleBackward(CompareFunc compare, int maxCompares)
{
	if (maxCompares == -1)
		maxCompares = ItemCount-1;
	else if (maxCompares > ItemCount-1)
		maxCompares = ItemCount-1;

	// If there are no items, or only a single one, we're done.
	if ((ItemCount < 2) || (maxCompares == 0))
		return 0;

	T* a = TailItem;
	int numSwaps = 0;
	int numCompares = 0;

	while ((a != HeadItem) && (numCompares < maxCompares))
	{
		T* b = a->PrevItem;

		// We're sorting from lowest to biggest, so if a < b we need to swap them.
		if (compare(*a, *b))
		{
			// Swap.
			if (a->NextItem)
				a->NextItem->PrevItem = b;

			if (b->PrevItem)
				b->PrevItem->NextItem = a;

			a->PrevItem = b->PrevItem;
			b->NextItem = a->NextItem;

			a->NextItem = b;
			b->PrevItem = a;

			// Fix head and tail if they were involved in the swap.
			if (HeadItem == b)
				HeadItem = a;

			if (TailItem == a)
				TailItem = b;

			// Since we swapped, a is now correctly ready for the next loop.
			numSwaps++;
		}
		else
		{
			a = a->PrevItem;
		}
		numCompares++;
	}

	return numSwaps;
}


template<typename T> inline T* tItList<T>::Remove(Iter& iter)
{
	// It is perfectly valid to try to remove an object referenced by an invalid iterator.
	if (!iter.IsValid() || (this != iter.List))
		return nullptr;

	IterNode* node = Nodes.Remove(iter.Node);
	T* obj = (T*)node->Object;

	delete node;
	iter.Node = 0;

	return obj;
}
