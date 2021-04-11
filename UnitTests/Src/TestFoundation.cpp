// TestFoundation.cpp
//
// Foundation module tests.
//
// Copyright (c) 2017, 2019, 2020, 2021 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <thread>
#include <chrono>
#include <Foundation/tVersion.cmake.h>
#include <Foundation/tArray.h>
#include <Foundation/tFixInt.h>
#include <Foundation/tBitField.h>
#include <Foundation/tList.h>
#include <Foundation/tMap.h>
#include <Foundation/tSmartPointers.h>
#include <Foundation/tPromise.h>
#include <Foundation/tRingBuffer.h>
#include <Foundation/tSort.h>
#include <Foundation/tPriorityQueue.h>
#include <Foundation/tPool.h>
#include "UnitTests.h"
using namespace tStd;
namespace tUnitTest
{


tTestUnit(Types)
{
	tRequire(sizeof(uint8) == 1);
	tRequire(sizeof(uint16) == 2);
	tRequire(sizeof(uint32) == 4);
	tRequire(sizeof(uint64) == 8);

	tRequire(sizeof(int8) == 1);
	tRequire(sizeof(int16) == 2);
	tRequire(sizeof(int32) == 4);
	tRequire(sizeof(int64) == 8);
}
	

tTestUnit(Array)
{
	tArray<int> arr(2, 3);
	arr.Append(1);
	arr.Append(2);

	// Grow by 3.
	arr.Append(3);
	arr.Append(4);
	arr.Append(5);
	
	// Grow by 3.
	arr.Append(6);

	tRequire(arr.GetNumAppendedElements() == 6);
	tRequire(arr.GetCapacity() == 8);

	for (int i = 0; i < arr.GetNumAppendedElements(); i++)
		tPrintf("Array index %d has value %d\n", i, arr[i]);
	tPrintf("Num appended items: %d  Capacity: %d\n", arr.GetNumAppendedElements(), arr.GetCapacity());

	tPrintf("Index 2 value change to 42.\n");
	arr[2] = 42;
	for (int i = 0; i < arr.GetNumAppendedElements(); i++)
		tPrintf("Array index %d has value %d\n", i, arr[i]);
	tPrintf("Num appended items: %d  Capacity: %d\n", arr.GetNumAppendedElements(), arr.GetCapacity());
	tRequire(arr.GetElements()[2] == 42);
}


// For list testing.
struct Item : public tLink<Item>
{
	Item(int val) : Value(val) { }
	int Value;
};


// For list testing.
bool LessThan(const Item& a, const Item& b)
{
	return a.Value < b.Value;
}


// For list testing.
struct NormItem
{
	NormItem()								: Value(0) { tPrintf("Constructing (Def) NormItem with value %d\n", Value); }
	NormItem(int val)						: Value(val) { tPrintf("Constructing (int) NormItem with value %d\n", Value); }
	NormItem(const NormItem& src)			: Value(src.Value) { tPrintf("Constructing (CC) NormItem with value %d\n", Value); }
	virtual ~NormItem()						{ tPrintf("Destructing NormItem with value %d\n", Value); }
	int Value;
};


bool LessThanNorm(const NormItem& a, const NormItem& b)
{
	return a.Value < b.Value;
}


struct MySuper : public tLink<MySuper>
{
	virtual ~MySuper()			{ tPrintf("Running ~MySuper\n"); }
};


struct MySub : public MySuper
{
	MySub(int id) : ID(id)		{ }
	virtual ~MySub()			{ tPrintf("Running ~MySub ID %d\n", ID); }
	int ID						= 0;
};


tTestUnit(List)
{
	tList<MySub> subs;
	subs.Append(new MySub(1));
	subs.Append(new MySub(2));
	subs.Append(new MySub(3));
	subs.Clear();

	tList<Item> itemList(tListMode::ListOwns);
	itemList.Append( new Item(7) );
	itemList.Append( new Item(3) );
	itemList.Append( new Item(4) );
	itemList.Append( new Item(9) );
	itemList.Append( new Item(1) );
	itemList.Append( new Item(5) );
	itemList.Append( new Item(4) );

	tPrintf("Before sorting: ");
	for (const Item* item = itemList.First(); item; item = item->Next())
		tPrintf("%d ", item->Value);
	tPrintf("\n");

	itemList.Sort( LessThan );
	// itemList.Bubble( LessThan , true);

	tPrintf("After sorting: ");
	for (const Item* item = itemList.First(); item; item = item->Next())
		tPrintf("%d ", item->Value);
	tPrintf("\n");
	tRequire(itemList.First()->Value < itemList.First()->Next()->Value);
	tRequire(itemList.First()->Next()->Value < itemList.First()->Next()->Next()->Value);

	// 1 3 4 4 5 7 9
	Item* inoutItem = new Item(5);
	itemList.Insert(inoutItem, LessThan);
	tPrintf("After insert sorted 5: ");
	for (const Item* item = itemList.First(); item; item = item->Next())
		tPrintf("%d ", item->Value);
	tPrintf("\n");
	itemList.Remove(inoutItem);

	// Test circular on intrusive lists.
	Item* itm = itemList.First();
	for (int circ = 0; circ < 100; circ++)
		itm = itemList.NextCirc(itm);
	tPrintf("NextCirc Item Val %d\n", itm->Value);
	tRequire(itm->Value == 4);

	for (int circ = 0; circ < 100; circ++)
		itm = itemList.PrevCirc(itm);
	tPrintf("PrevCirc Item Val %d\n", itm->Value);
	tRequire(itm->Value == 1);

	// Insert an item at the right place to keep sorted.
	itemList.Insert(new Item(6), LessThan);
	tPrintf("After sorted insert 6: ");
	for (const Item* item = itemList.First(); item; item = item->Next())
		tPrintf("%d ", item->Value);
	tPrintf("\n");

	itemList.Drop();
	itemList.Remove();

	// We need this if we didn't construct this list with a true flag.
	//itemList.Empty();

	tItList<NormItem> iterList(tListMode::ListOwns);
	iterList.Append( new NormItem(7) );
	iterList.Append( new NormItem(3) );
	iterList.Append( new NormItem(4) );
	iterList.Append( new NormItem(9) );
	iterList.Append( new NormItem(1) );
	iterList.Append( new NormItem(5) );
	iterList.Append( new NormItem(4) );

	// Test range-based iteration.
	for (NormItem& item : iterList)
	{
		tPrintf("Range-based %d\n", item.Value);
	}

	tPrintf("Iterating forward: ");
	for (tItList<NormItem>::Iter iter = iterList.First(); iter; iter++)
		tPrintf("%d ", (*iter).Value);
	tPrintf("\n");

	tPrintf("Iterating backward: ");
	for (tItList<NormItem>::Iter biter = iterList.Tail(); biter; --biter)
		tPrintf("%d ", (*biter).Value);
	tPrintf("\n");

	tItList<NormItem>::Iter lastIter = iterList.Last();
	NormItem ni = iterList[lastIter];
	tPrintf("Last NormItem: %d\n", ni.Value);

	for (int i = 0; i < 10; i++)
	{
		lastIter.NextCirc();
		tPrintf("NextCirc: %d\n", lastIter.GetObject()->Value);
	}

	iterList.Sort(LessThanNorm);
	tPrintf("AfterSorting:\n");
	for (tItList<NormItem>::Iter iter = iterList.First(); iter; iter++)
		tPrintf("%d ", (*iter).Value);
	tPrintf("\n");

	// We need this if we didn't construct this list with a true flag.
	// iterList.Empty();

	// Test static list.
	static tList<MySub> staticList(tListMode::Static);
	staticList.Append(new MySub(1));
	staticList.Append(new MySub(2));
	staticList.Append(new MySub(3));
	staticList.Empty();
}


// Stefan's extra list tests.
template <class T> struct NamedLink : public tLink<T>
{
	NamedLink<T>(const char* name)		{ Name.Set(name); }
	NamedLink<T>(int id)				{ tsPrintf(Name, "Name%d", id); }
	tString Name;
};


template <class T> struct NamedList : public tList<T>
{
	T* FindNodeByName(const tString& name);
};


template<class T> T* NamedList<T>::FindNodeByName(const tString& name)
{
	for (T* node = NamedList::Head(); node; node = node->Next())
		if (node->Name == name)
			return node;

	return nullptr;
}


struct NamedNode : public NamedLink<NamedNode>
{
	NamedNode(int id) : NamedLink(id+1), ID(id) { }
	int ID;
};


struct BigNode : public NamedLink<BigNode>
{
	BigNode(const char* name, const char* dependsOn, bool gen, bool always)		:
		NamedLink(name),
		DependsOn(dependsOn),
		Generate(gen),
		Always(always)															{ }

	tString DependsOn;
	bool Generate;
	bool Always;
};


bool BigSort(const BigNode& lhs, const BigNode& rhs)
{
	// Always always comes first.
	if (lhs.Always)
		return true;

	// Generate comes before non-generate.
	if(lhs.Generate && !rhs.Generate)
		return true;

	// If the rhs depends on the lhs, the lhs has to come before the node that depends on it.
	if (rhs.DependsOn == lhs.Name)
		return true;

	return false;
}


tTestUnit(ListExtra)
{
	NamedList<NamedNode> nodes;

	for (int i = 0; i < 4; i++)
		nodes.Append(new NamedNode(i));

	for (NamedNode* nn = nodes.First(); nn; nn = nn->Next())
		tPrintf("ListExtra: ID:%d  Name:%s\n", nn->ID, nn->Name.Chars());

	NamedNode* movedNode = nodes.Remove(nodes.Head());
	nodes.Insert(movedNode, nodes.Head()->Next());

	tPrintf("\nListExtra: Reordered\n");
	for (NamedNode* nn = nodes.First(); nn; nn = nn->Next())
		tPrintf("ListExtra: ID:%d  Name:%s\n", nn->ID, nn->Name.Chars());

	NamedNode* foundNode = nodes.FindNodeByName("Name4");
	tRequire(foundNode);
	tPrintf("ListExtra: Found ID%d:%s\n", foundNode->ID, tPod(foundNode->Name));

	tPrintf("Big Node Test\n");
	NamedList<BigNode> bigList;
	BigNode* bigNode = nullptr;

	// Always is only true if generated is true. The order is always, generated, not generated. Only generated nodes
	// will have dependencies. You can depend on exatly 1 node. There are no circular dependencies. You may be added
	// (as in this case) before the node you depend on. The desired outcome is thus:
	// Always at the front, master, dependent pairs intermixed with other generated or alwyas generated nodes,
	// followed by non-generated nodes.

	// Goes in at head since it's the 1st node. Const args: Name, Dep, Gen, Always.
	bigNode = new BigNode("A", nullptr, true, false);
	bigList.Insert(bigNode, BigSort);

	// Goes in at the head since Always is true
	bigNode = new BigNode("B", nullptr, true, true);
	bigList.Insert(bigNode, BigSort);

	// Goes in after all other generate nodes (so after "E" if "E" is already there)
	bigNode = new BigNode("C", "E", true, false);
	bigList.Insert(bigNode, BigSort);

	// Goes in after all the generate nodes
	bigNode = new BigNode("D", nullptr, true, false);
	bigList.Insert(bigNode, BigSort);

	// Should go in before C since C depends on it.
	bigNode = new BigNode("E", nullptr, true, false);
	bigList.Insert(bigNode, BigSort);

	tPrintf("Expected:\nB A E C D\nActual:\n");
	tString result;
	for (BigNode* mn = bigList.Head(); mn; mn = mn->Next())
	{
		tPrintf("%s ", mn->Name.Chars());
		result += mn->Name;
	}
	tPrintf("\n");
	tRequire(result == "BAECD");

	bigList.Sort(BigSort);
	tString result2;
	for (BigNode* mn = bigList.Head(); mn; mn = mn->Next())
		result2 += mn->Name;
	tRequire(result2 == "BAECD");
}


static void PrintMapStats(const tMap<tString, tString>& mp)
{
	tPrintf("NumItems HTsize HTcount percent coll: %02d %02d %02d %04.1f%% %02d\n", mp.GetNumItems(), mp.GetHashTableSize(), mp.GetHashTableEntryCount(), 100.0f*mp.GetHashTablePercent(), mp.GetHashTableCollisions());
}


tTestUnit(Map)
{
	tString testString("The real string");
	tPrintf("const char* Opertor() on string:%s\n", (const char*)testString);
	tPrintf("uint32 Opertor() on string:%08X\n", (uint32)testString);

	tMap<tString, tString> nameDescMap(8);
	tPrintf("initialLog2Size %d  HashTableSize %d\n", 8, nameDescMap.GetHashTableSize());

	nameDescMap.GetInsert("fred") = "Fred is smart and happy.";
	nameDescMap.GetInsert("joan") = "Joan is sly and sad.";
	nameDescMap.GetInsert("kim") = "Kim is tall and contemplative.";
	nameDescMap["john"] = "John cannot ego-surf.";
	tRequire(nameDescMap.GetNumItems() == 4);

	tPrintf("Iterate through key/value pairs using standard for loop.\n");
	for (auto pair = nameDescMap.First(); pair; ++pair)
		tPrintf("tMap Key Value: [%s] [%s]\n", pair.Key().Pod(), pair.Value().Pod());

	tPrintf("Iterate through key/value pairs using range-based for loop.\n");
	for (auto pair : nameDescMap)
		tPrintf("tMap Key Value: [%s] [%s]\n", pair.Key().Pod(), pair.Value().Pod());

	bool fredRemoved = nameDescMap.Remove("fred");
	tRequire(fredRemoved);
	tRequire(nameDescMap.GetNumItems() == 3);

	tString joanDesc = nameDescMap.GetInsert("joan");
	tRequire(joanDesc == "Joan is sly and sad.");

	tString johnDesc = nameDescMap.GetInsert("john");
	tRequire(johnDesc == "John cannot ego-surf.");

	tPrintf("Tests that require the tMap to rekey itself (grow)\n");
	// tMap<tString, tString> mymap(1, 2.0f);		// Tablesize 2, no rekey.
	// tMap<tString, tString> mymap(2, 2.0f);		// Tablesize 4, no rekey.
	// tMap<tString, tString> mymap(1, 0.25f);		// Tablesize 2, aggressive rekey.
	tMap<tString, tString> mymap(1, 0.9f);			// Tablesize 2, conservative rekey.
	PrintMapStats(mymap);
	tPrintf("\n");

	mymap["KAhy"] = "VA";
	PrintMapStats(mymap);

	mymap["KBrf"] = "VB";
	PrintMapStats(mymap);

	mymap["KCcd"] = "VC";
	PrintMapStats(mymap);

	mymap["KDjj"] = "VD";
	PrintMapStats(mymap);

	mymap["KE"] = "VE";
	PrintMapStats(mymap);

	mymap["KF"] = "VF";
	PrintMapStats(mymap);

	mymap["KG"] = "VG";
	PrintMapStats(mymap);
	for (auto pair : mymap)
		tPrintf("mymap KV: [%s] [%s]\n", pair.Key().Pod(), pair.Value().Pod());

	tMap<int, uint64> intMap;
	intMap[4] = 12;
	intMap[33] = 23;
	intMap[78] = 1718;
	intMap[9] = 19;
	tRequire(intMap[4] == 12);
	tRequire(intMap[33] == 23);
	tRequire(intMap[78] == 1718);
	tRequire(intMap[9] == 19);
	for (auto pair : intMap)
		tPrintf("intmap KV: [%d] [%d]\n", pair.Key(), pair.Value());
}


struct TestObject
{
	TestObject()		{ tPrintf("TestObject Constructor\n"); }
	~TestObject()		{ tPrintf("TestObject Destructor\n"); }
	int Val = 55;
};


tTestUnit(SmartPointers)
{
	tSharedPtr<float> pfloatA = new float(4.0f);
	tSharedPtr<float> pfloatB = pfloatA;

	// A and B point to same object. When they go out of scope, the float will be deleted.
	tPrintf("FloatA: %f\n", *pfloatA);
	tPrintf("FloatB: %f\n", *pfloatB);
	tRequire(pfloatA.IsValid());
	tRequire(pfloatB.IsValid());

	pfloatA = nullptr;
	tRequire(!pfloatA.IsValid());
	tRequire(pfloatB.IsValid());

	pfloatB = nullptr;
	tRequire(!pfloatA.IsValid());
	tRequire(!pfloatB.IsValid());

	tPrintf("Begin Scope\n");
	{
		tSharedPtr<TestObject> pfloatTA = new TestObject();
		tSharedPtr<TestObject> pfloatTB = pfloatTA;
		tRequire((pfloatTA.GetRefCount() == 2) && (pfloatTB.GetRefCount() == 2));
	}
	tPrintf("End Scope\n");
}


void GenerateFloats(tPromise<float>* floatPromise)
{
	// Lets generate 5 floats.
	for (int f = 0; f < 1; f++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		floatPromise->Fulfill(3.0f+float(f));
	}
}


tTestUnit(Promise)
{
	// Promiser.
	tPromise<float> promiseFloats;
	std::thread floatThread(GenerateFloats, &promiseFloats);

	while (promiseFloats.GetState() == tPromise<float>::State::Pending)
	{
		tPrintf("Waiting...\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	if (promiseFloats.GetState() == tPromise<float>::State::Fulfilled)
	{
		tPrintf("Promise resulted in: %f\n", promiseFloats.GetItem());
	}

	floatThread.join();
}


bool IntLess(const int& a, const int& b) { return (a < b); }


tTestUnit(Sort)
{
	int arr[] = { 5, 32, 7, 9, 88, 32, -3, 99, 55 };
	tPrintf("Before sorting:\n");
	for (int i = 0; i < sizeof(arr)/sizeof(*arr); i++)
		tPrintf("%d, ", arr[i]);
	tPrintf("\n");

	tSort::tShell(arr, sizeof(arr)/sizeof(*arr), IntLess);
	//tSort::tInsertion(arr, sizeof(arr)/sizeof(*arr), IntLess);
	//tSort::tQuick(arr, sizeof(arr)/sizeof(*arr), IntLess);

	tPrintf("After sorting:\n");
	for (int i = 0; i < sizeof(arr)/sizeof(*arr); i++)
		tPrintf("%d, ", arr[i]);
	tPrintf("\n");
	
	tRequire(arr[0] <= arr[1]);
	tRequire(arr[1] <= arr[2]);
	tRequire(arr[6] <= arr[7]);
	tRequire(arr[7] <= arr[8]);
}


tTestUnit(FixInt)
{
	// @todo Add a bunch of tRequire calls.
	tuint256 uvalA = 42;
	tuint256 uvalB(uvalA);
	tint256 valA(99);
	tuint256 uvalC(valA);
	tuint256 uvalD("FE", 16);
	tuint256 uvalE(uint16(88));
	uvalD.Set(uvalE);
	uvalC.Set(valA);
	uvalD.Set("808");
	int8 int8Val = int8(uvalE);
	float floatVal = float(uvalE);
	uvalE = uvalA;

	uvalD.MakeZero();
	uvalD.MakeMaxInt();
	uvalA += 2;
	tPrintf("%064|256X\n", uvalA);
	tPrintf("%064|256X\n", uvalB);
	tint256 valB = uvalB.AsSigned();
	const tint256 valC = uvalC.AsSigned();
	uvalA.ClearBit(0);
	uvalA.SetBit(0);
	uvalA.ToggleBit(0);
	bool bval = uvalA.GetBit(0);

	uvalA /= 10;
	tPrintf("%064|256X\n", uvalA);

	uvalC = tDivide(uvalA, uvalB);
	tPrintf("%064|256X\n", uvalA);

	uvalA &= tuint256(12);
	uvalA |= tuint256(12);
	uvalA ^= tuint256(12);
	uvalA >>= 2;
	uvalA <<= 4;
	uvalA += 8;
	uvalA += uvalB;
	uvalA -= 4;
	uvalA -= uvalB;
	uvalA /= tuint256(12);
	uvalA %= tuint256(20);
	uvalA = uvalA >> 2;
	uvalA = uvalA << 4;
	tPrintf("%064|256X\n", uvalA);

	uvalD *= 32;
	uvalD *= tuint256(3);
	if (uvalA < 25)
		tPrintf("Small\n");

	bval = uvalA == uvalB;
	bval = uvalA != uvalB;
	bval = uvalA < uvalB;
	bval = uvalA > uvalB;
	uvalB = uvalC & uvalD;
	uvalC++;
	++uvalC;
	bval = uvalC;
	bval = !uvalC;
	uvalA = ~uvalB;
	uvalB = -uvalB;
	uvalC = +uvalB;
	uvalD = tSqrt(uvalC);
	uvalE = tCurt(uvalD);
	uvalA = tFactorial(uvalE);

	int32 val = 0;
	val = tStrtoi32("0xFD");
	tPrintf("0xFD %d\n", val);

	val = tStrtoi32("#A001");
	tPrintf("#A001 %d\n", val);

	val = tStrtoi32("B01012");
	tPrintf("%b\n", val);

	val = tStrtoi32("0d-88");
	tPrintf("%b\n", val);

	uvalA.Set("0xabcd");
	tPrintf("%064|256X\n", uvalA);

	uvalA.RotateRight(3);

	tint256 a = 100;
	tint256 b = 11;
	tDivide(a, b);
	tDivide(a, 15);
}


tTestUnit(Bitfield)
{
	tString result;

	tbit128 a("0XAAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD");
	tPrintf("A: %032|128X\n", a);
	tsPrintf(result, "A: %032|128X", a);
	tRequire(result == "A: AAAAAAAABBBBBBBBCCCCCCCCDDDDDDDD");

	a.Set("FF", 16);
	tPrintf("A: %032|128X\n", a);
	tsPrintf(result, "A: %032|128X", a);
	tRequire(result == "A: 000000000000000000000000000000FF");

	tBitField<30> b;
	b.Set("0xCCCC12FF");
	tsPrintf(result, "%08|32X", b);
	tRequire(result == "0CCC12FF");

	b.Set("0xFCCC12FF");
	tsPrintf(result, "%08|32X", b);
	tRequire(result == "3CCC12FF");

	tBitField<33> bitset33;
	tStaticAssert(sizeof(bitset33) == 8);
	tPrintf("Setting 33 bitset to: decimal %s\n", "323456789");

	// Since there is no base prefix in the string, the Set call assumes base 10. Note that the
	// GetAsString calls default to base 16!
	bitset33.Set("323456789", 10);
	tPrintf("bitset 33 was set to: 0x%s\n", bitset33.GetAsString().Pod());
	tPrintf("bitset 33 was set to: d%s\n", bitset33.GetAsString(10).Pod());
	tRequire(bitset33.GetAsString() == "13478F15");
	tRequire(bitset33.GetAsString(10) == "323456789");

	tBitField<10> bitset10;
	bitset10.SetAll(true);
	tPrintf("bitset10 SetAll yields: d%s\n", bitset10.GetAsString(10).Pod());
	tRequire(bitset10.GetAsString(10) == "1023");

	tBitField<12> bitset12;
	bitset12.Set("abc");
	tPrintf("bitset12: %s\n", bitset12.GetAsString().Pod());
	tRequire(bitset12.GetAsString() == "ABC");

	bitset12 >>= 4;
	tPrintf("bitset12: %s\n", bitset12.GetAsString().Pod());
	tRequire(bitset12.GetAsString() == "AB");

	bitset12 <<= 4;
	tPrintf("bitset12: %s\n", bitset12.GetAsString().Pod());
	tRequire(bitset12.GetAsString() == "AB0");

	tBitField<12> bitsetAB0("AB0");
	tPrintf("bitsetAB0 == bitset12: %s\n", (bitsetAB0 == bitset12) ? "true" : "false");
	tRequire(bitsetAB0 == bitset12);
	tRequire(!(bitsetAB0 != bitset12));

	tBitField<17> bitset17;
	bitset17.SetBit(1, true);
	if (bitset17)
		tPrintf("bitset17: %s true\n", bitset17.GetAsString().Pod());
	else
		tPrintf("bitset17: %s false\n", bitset17.GetAsString().Pod());
	tRequire(bitset17);

	bitset17.InvertAll();
	tPrintf("bitset17: after invert: %s\n", bitset17.GetAsString().Pod());
	tRequire(bitset17.GetAsString() == "1FFFD");
}


tTestUnit(String)
{
	// Testing the string substitution code.
	tString src("abc1234abcd12345abcdef123456");
	tPrintf("Before: '%s'\n", src.ConstText());
	src.Replace("abc", "cartoon");
	tPrintf("Replacing abc with cartoon\n");
	tPrintf("After : '%s'\n\n", src.ConstText());
	tRequire(src == "cartoon1234cartoond12345cartoondef123456");

	src = "abc1234abcd12345abcdef123456";
	tPrintf("Before: '%s'\n", src.ConstText());
	src.Replace("abc", "Z");
	tPrintf("Replacing abc with Z\n");
	tPrintf("After : '%s'\n\n", src.ConstText());
	tRequire(src == "Z1234Zd12345Zdef123456");

	src = "abcabcabc";
	tPrintf("Before: '%s'\n", src.ConstText());
	src.Replace("abc", "");
	tPrintf("Replacing abc with \"\"\n");
	tPrintf("After : '%s'\n\n", src.ConstText());
	tRequire(src == "");

	src = "abcabcabc";
	tPrintf("Before: '%s'\n", src.ConstText());
	src.Replace("abc", 0);
	tPrintf("Replacing abc with null\n");
	tPrintf("After : '%s'\n\n", src.ConstText());
	tRequire(src == "");

	src.Clear();
	tPrintf("Before: '%s'\n", src.ConstText());
	src.Replace("abc", "CART");
	tPrintf("Replacing abc with CART\n");
	tPrintf("After : '%s'\n\n", src.ConstText());
	tRequire(src == "");

	tPrintf("Testing Explode:\n");
	tString src1 = "abc_def_ghi";
	tString src2 = "abcXXdefXXghi";
	tPrintf("src1: %s\n", src1.ConstText());
	tPrintf("src2: %s\n", src2.ConstText());

	tList<tStringItem> exp1(tListMode::ListOwns);
	tList<tStringItem> exp2(tListMode::ListOwns);
	int count1 = tExplode(exp1, src1, '_');
	int count2 = tExplode(exp2, src2, "XX");

	tPrintf("Count1: %d\n", count1);
	for (tStringItem* comp = exp1.First(); comp; comp = comp->Next())
		tPrintf("   Comp: '%s'\n", comp->ConstText());

	tPrintf("Count2: %d\n", count2);
	for (tStringItem* comp = exp2.First(); comp; comp = comp->Next())
		tPrintf("   Comp: '%s'\n", comp->ConstText());

	tList<tStringItem> expl(tListMode::ListOwns);
	tString exdup = "abc__def_ghi";
	tExplode(expl, exdup, '_');
	tPrintf("Exploded: ###%s### to:\n", exdup.ConstText());
	for (tStringItem* comp = expl.First(); comp; comp = comp->Next())
		tPrintf("   Comp:###%s###\n", comp->ConstText());

	tList<tStringItem> expl2(tListMode::ListOwns);
	tString exdup2 = "__a__b_";
	tExplode(expl2, exdup2, '_');
	tPrintf("Exploded: ###%s### to:\n", exdup2.ConstText());
	for (tStringItem* comp = expl2.First(); comp; comp = comp->Next())
		tPrintf("   Comp:###%s###\n", comp->ConstText());

	src = "abc1234abcd12345abcdef123456";
	tPrintf("Before: '%s'\n", src.ConstText());
	tString tgt = src.ExtractMid(3, 4);
	tPrintf("Extracting 1234 with ExtractMid(3, 4)\n");
	tPrintf("After (Extracted): '%s'\n\n", tgt.ConstText());
	tPrintf("After (Remain)   : '%s'\n\n", src.ConstText());
	tRequire(tgt == "1234" && src == "abcabcd12345abcdef123456");

	tString aa("aa");
	tString exaa = aa.ExtractLeft('a');
	tPrintf("\n\naa extract left word to a: Extracted:###%s###  Left:###%s###\n", exaa.ConstText(), aa.ConstText());

	tString sa1 = "A";
	tString sa2 = "A";
	tString sb1 = "B";
	const char* ca1 = "A";
	const char* ca2 = "A";
	const char* cb1 = "B";

	//Test string/string, string/char*, char*/string. char*char* cannot be overloaded in C++.
	tRequire(sa1 == sa2);
	tRequire(sa1 != sb1);
	tRequire(sa1 == ca1);
	tRequire(sa1 != cb1);
	tRequire(ca1 == sa1);
	tRequire(ca1 != sb1);

	// Test remove leading and trailing.
	tString leadtrail("cbbabaccMIDDLEbbccaab");
	tPrintf("LeadTrail [%s]\n", leadtrail.Chars());

	leadtrail.RemoveLeading("abc");
	tPrintf("LeadTrail [%s]\n", leadtrail.Chars());
	tRequire(leadtrail == "MIDDLEbbccaab");

	leadtrail.RemoveTrailing("abc");
	tPrintf("LeadTrail [%s]\n", leadtrail.Chars());
	tRequire(leadtrail == "MIDDLE");

	// Test remove prefix and suffix.
	tString presuf("prepreMIDDLEsufsuf");
	tPrintf("PreSuf [%s]\n", presuf.Chars());

	presuf.ExtractLeft("not");
	presuf.ExtractRight("not");
	tPrintf("PreSuf [%s]\n", presuf.Chars());
	tRequire(presuf == "prepreMIDDLEsufsuf");

	presuf.ExtractLeft("pre");
	tPrintf("PreSuf [%s]\n", presuf.Chars());
	tRequire(presuf == "preMIDDLEsufsuf");

	presuf.ExtractRight("suf");
	tPrintf("PreSuf [%s]\n", presuf.Chars());
	tRequire(presuf == "preMIDDLEsuf");

}


tTestUnit(RingBuffer)
{
	// We're going to use the middle 10 characters as our buffer and write a pattern into the full buffer
	// to check for over and under-runs.
	char buf[14];
	char rem[14];
	char rm;
	bool ok;
	tMemset(buf, '.', 14);
	tRingBuffer<char> ring(buf+2, 10);
	auto printBuf = [&]()
	{
		tPrintf("Buf: ");
		for (int c = 0; c < 14; c++)
			tPrintf("%c", buf[c]);
		tPrintf("\n     ");
		for (int c = 0; c < 14; c++)
		{
			char v = ' ';
			if ((c>=2) && (c<12))
			{
				if ((ring.GetHeadIndex() != -1) && (ring.GetHeadIndex()==c-2))
					v = 'H';
				if ((ring.GetTailIndex() != -1) && (ring.GetTailIndex()==c-2))
					{ if (v == 'H') v = 'B'; else v = 'T'; }
			}

			tPrintf("%c", v);
		}
		tPrintf("\n");
	};

	printBuf();
	tPrintf("\n");

	tPrintf("Append abcd\n");
	int numApp = ring.Append("abcdefghijkl", 4);	printBuf();
	tPrintf("Appended %d items\n\n", numApp);

	tPrintf("Remove 2\n");
	int numRem = ring.Remove(rem, 2);				printBuf();
	tPrintf("Removed %d items\n\n", numRem);

	tPrintf("Remove 1\n");
	ring.Remove(rm);								printBuf();
	tPrintf("Removed %c\n\n", rm);

	tPrintf("Append efghijkl\n");
	numApp = ring.Append("efghijkl", 8);			printBuf();
	tPrintf("Appended %d items\n\n", numApp);

	for (int r = 0; r < 11; r++)
	{
		tPrintf("Remove 1\n");
		ok = ring.Remove(rm);						printBuf();
		if (ok) tPrintf("Removed %c\n", rm);
	}
	tPrintf("\n");
}


tTestUnit(PriorityQueue)
{
	tPQ<int> Q(2, 2);
	int data = 42;

	Q.Insert( tPQ<int>::tItem(data, 7) );
	Q.Insert( tPQ<int>::tItem(data, 24) );
	Q.Insert( tPQ<int>::tItem(data, 2) );
	Q.Insert( tPQ<int>::tItem(data, 16) );
	Q.Insert( tPQ<int>::tItem(data, 24) );
	Q.Insert( tPQ<int>::tItem(data, 3) );
	Q.Insert( tPQ<int>::tItem(data, 1) );
	Q.Insert( tPQ<int>::tItem(data, 0) );
	Q.Insert( tPQ<int>::tItem(data, 43) );
	Q.Insert( tPQ<int>::tItem(data, 16) );

	tPrintf("GetMin %d\n", Q.GetMin().Key);
	tRequire(Q.GetNumItems() == 10);
	for (int i = 0; i < 10; i++)
		tPrintf("ExtractMin %d\n", Q.GetRemoveMin().Key);
}


tTestUnit(MemoryPool)
{
	tPrintf("Sizeof (uint8*): %d\n", sizeof(uint8*));

	int bytesPerItem = 2;
	int initNumItems = 4;
	int growNumItems = 3;
	bool threadSafe = true;

	tMem::tFastPool memPool(bytesPerItem, initNumItems, growNumItems, threadSafe);

	void* memA = memPool.Malloc();
	tPrintf("memA: %08X\n", memA);
	tRequire(memA);

	void* memB = memPool.Malloc();
	tPrintf("memB: %08X\n", memB);
	tRequire(memB);

	void* memC = memPool.Malloc();
	tPrintf("memC: %08X\n", memC);
	tRequire(memC);

	void* memD = memPool.Malloc();
	tPrintf("memD: %08X\n", memD);
	tRequire(memD);

	// Now a grow should happen.
	void* memE = memPool.Malloc();
	tPrintf("memE: %08X\n", memE);
	tRequire(memE);
	tRequire(memPool.GetNumExpansionBlocks() == 1);

	void* memF = memPool.Malloc();
	tPrintf("memF: %08X\n", memF);
	tRequire(memF);

	void* memG = memPool.Malloc();
	tPrintf("memG: %08X\n", memG);
	tRequire(memG);

	// And another grow.
	void* memH = memPool.Malloc();
	tPrintf("memH: %08X\n", memH);
	tRequire(memH);
	tRequire(memPool.GetNumExpansionBlocks() == 2);

	// Try to allocate something too big.
	void* tooBig = memPool.Malloc(9);
	tPrintf("tooBig: %08X\n", tooBig);
	tRequire(tooBig == nullptr);
	tRequire(memPool.GetNumAllocations() == 8);

	tPrintf("free B, free E\n");
	memPool.Free(memB);
	memPool.Free(memE);
	tRequire(memPool.GetNumAllocations() == 6);

	memE = memPool.Malloc();
	tPrintf("memE: %08X\n", memE);
	tRequire(memE);

	memB = memPool.Malloc();
	tPrintf("memB: %08X\n", memB);
	tRequire(memB);
	tRequire(memPool.GetNumAllocations() == 8);

	memPool.Free(memA);
	memPool.Free(memB);
	memPool.Free(memC);
	memPool.Free(memD);
	memPool.Free(memE);
	memPool.Free(memF);
	memPool.Free(memG);
	memPool.Free(memH);
	tRequire(memPool.GetNumAllocations() == 0);
}


}
