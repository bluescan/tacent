// TestFoundation.cpp
//
// Foundation module tests.
//
// Copyright (c) 2017, 2019-2022 Tristan Grimmer.
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
#include <future>
#include <Foundation/tVersion.cmake.h>
#include <Foundation/tStandard.h>
#include <Foundation/bString.h>
#include <Foundation/tArray.h>
#include <Foundation/tBitArray.h>
#include <Foundation/tBitField.h>
#include <Foundation/tFixInt.h>
#include <Foundation/tList.h>
#include <Foundation/tMap.h>
#include <Foundation/tRingBuffer.h>
#include <Foundation/tSort.h>
#include <Foundation/tPriorityQueue.h>
#include <Foundation/tPool.h>
#include <System/tFile.h>
#include "UnitTests.h"
using namespace tStd;
using namespace tSystem;


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


tsList<Item> ThreadSafeList(tListMode::Static);


void ListAddThreadEvens()
{
	for (int even = 0; even < 10; even += 2)
	{
		ThreadSafeList.Append(new Item(even));
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}


void ListAddThreadOdds()
{
	for (int odd = 1; odd < 10; odd += 2)
	{
		ThreadSafeList.Append(new Item(odd));
		std::this_thread::sleep_for(std::chrono::milliseconds(7));
	}
}


tTestUnit(List)
{
	tPrintf("Thread-safe tsList\n");
	std::thread thread1Add(ListAddThreadEvens);
	std::thread thread2Add(ListAddThreadOdds);
	thread1Add.join();
	thread2Add.join();

	ThreadSafeList.Sort( LessThan );

	int itemNum = 0;
	for (Item* item = ThreadSafeList.First(); item; item = item->Next())
		tPrintf("Thread-Safe List Item %d Value %d\n", itemNum++, item->Value);
	ThreadSafeList.Empty();

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
	NamedLink(const char* n)			{ Name.Set(n); }
	NamedLink(int id)					{ tsPrintf(Name, "Name%d", id); }
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
		tPrintf("ListExtra: ID:%d  Name:%s\n", nn->ID, nn->Name.Chr());

	NamedNode* movedNode = nodes.Remove(nodes.Head());
	nodes.Insert(movedNode, nodes.Head()->Next());

	tPrintf("\nListExtra: Reordered\n");
	for (NamedNode* nn = nodes.First(); nn; nn = nn->Next())
		tPrintf("ListExtra: ID:%d  Name:%s\n", nn->ID, nn->Name.Chr());

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
		tPrintf("%s ", mn->Name.Chr());
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


// For testing chained promises, we promise a float > 0 and a next promise. If the
// chain is to end, the float value will be 0.
struct PromiseObject
{
	float TheFloat = 0.0f;
	// @wip std::promise<PromiseObject> NextPromise;
	PromiseObject()								{ tPrintf("PromiseObject Constructor\n"); }
	~PromiseObject()							{ tPrintf("PromiseObject Destructor\n"); }
};


std::promise<PromiseObject> GiveMeFloats()
{
	// @wip
	// Trying tosetup a test case where:
	// a) You don't know a-priori how many objects will be produced.
	// b) It takes a long time between production of each one. You only know when there
	// are nomore to give.
	// Thought is to promise a PromiseObject and supply a NextPromise every time. If the
	// PromiseObject has a flag (or for example, a neg float), the consumer will know it's the
	// last in the sequence and to ignore the NextPromise.
	static float val = 1.0f;
	
	// Do first one.
	PromiseObject prom;
	prom.TheFloat = val;	val += 1.0f;

	return std::promise<PromiseObject>();
}


tTestUnit(Promise)
{
	#if 0
	auto promise = std::promise<std::string>();
	auto producer = std::thread([&]
	{
		std::this_thread::sleep_for(std::chrono::seconds(5));
		promise.set_value("The String");
	});
	
	auto future = promise.get_future();
	auto consumer = std::thread([&]
	{
		std::cout << future.get().c_str();
	});
	
	producer.join();
	consumer.join();
	#endif
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


tTestUnit(BitArray)
{

}


tTestUnit(BitField)
{
	tString result;

	tbit128 a("0XAAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD");
	tPrintf("A: %032|128X\n", a);
	tsPrintf(result, "A: %032|128X", a);
	tRequire(result == "A: AAAAAAAABBBBBBBBCCCCCCCCDDDDDDDD");

	a.Set("FF");
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
	bitset33.Set("0x10000000A");
	tPrintf("bitset 33 was set to: 0x%s\n", bitset33.GetAsHexString().Pod());
	tRequire(bitset33.GetAsHexString() == "10000000A");

	tBitField<10> bitset10;
	bitset10.SetAll(true);
	tPrintf("bitset10 SetAll yields: 0x%s\n", bitset10.GetAsHexString().Pod());
	tRequire(bitset10.GetAsHexString() == "3FF");

	tBitField<12> bitset12;
	bitset12.Set("abc");
	tPrintf("bitset12: %s\n", bitset12.GetAsHexString().Pod());
	tRequire(bitset12.GetAsHexString() == "ABC");

	bitset12 >>= 4;
	tPrintf("bitset12: %s\n", bitset12.GetAsHexString().Pod());
	tRequire(bitset12.GetAsHexString() == "AB");

	bitset12 <<= 4;
	tPrintf("bitset12: %s\n", bitset12.GetAsHexString().Pod());
	tRequire(bitset12.GetAsHexString() == "AB0");

	tBitField<12> bitsetAB0("AB0");
	tPrintf("bitsetAB0 == bitset12: %s\n", (bitsetAB0 == bitset12) ? "true" : "false");
	tRequire(bitsetAB0 == bitset12);
	tRequire(!(bitsetAB0 != bitset12));

	tBitField<17> bitset17;
	bitset17.SetBit(1, true);
	if (bitset17)
		tPrintf("bitset17: %s true\n", bitset17.GetAsHexString().Pod());
	else
		tPrintf("bitset17: %s false\n", bitset17.GetAsHexString().Pod());
	tRequire(bitset17);

	bitset17.InvertAll();
	tPrintf("bitset17: after invert: %s\n", bitset17.GetAsHexString().Pod());
	tRequire(bitset17.GetAsHexString() == "1FFFD");

	// Test extracting bytes from a bitfield. Start by creating a random bitfield.
	tbit256 bitField;
	bitField.SetBinary
	(
		"00000010_00100100_10011111_11010100_00100100_10000101_01100011_01001000_"
		"00101001_01111011_00111010_01011111_00100110_11010000_11111111_11001100_"
		"00011100_11100010_00111000_11010000_00110011_11011011_01001100_00101110_"
		"10010011_00111000_01000100_10000111_10001011_00010000_10101011_00100101"
	);
	tPrintf("VAL\n%0256|256b\n", bitField);
	tPrintf("STR\n______%s\n", bitField.GetAsBinaryString().Chr());

	tPrintf("BYT\n");
	int cr = 0;
	for (int b = 31; b >= 0; b--)
	{
		uint8 byte = bitField.GetByte(b);
		tPrintf("%08b", byte);
	}

	tBitField<33> bits33;
	bits33.Set("1ABCDEF23");
	tPrintf("\nbits33 was set to:\n%s\n", bits33.GetAsHexString().Pod());
	for (int b = 4; b >= 0; b--)
	{
		uint8 byte = bits33.GetByte(b);
		tPrintf("%02x", byte);
	}

	// Test conversion into built-in types and promotion for use in if statements.
	tbit512 b111 = 0x00000003; //fullBitsLocal & tbit512(0x000007FF);
	tbit512 anded = b111 & tbit512(0x000007FF);
	tPrintf("\n\nANDED\n%0512|512b\n", anded);
	tRequire(anded == b111);
	tRequire(anded);
	anded.ClearAll();
	tRequire(!anded);

	// Test conversion to tFixInt of same size.
	tuint512 val512 = tbit512(0xAA0007FF);
	tPrintf("\nASINT\n%0512|512X\n", val512);
	tRequire(val512);

	val512.MakeZero();
	tRequire(!val512);

	// Should not compile.
	// tuint256 val256 = tbit512(0x000007FF);
	// tPrintf("\nASINT\n%0256|256b\n", val256);

	// Test to make sure operator= being called on non-constructor assignment.
	tuint512 val2;
	val2 = tbit512(0xAA0007FF);
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
	uvalD.MakeMax();
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

	// Should be a staic assert if uncommented.
	// tFixInt<33> Test33FixInt;
}


tTestUnit(String)
{
	// Testing the string substitution code.
	tString src("abc1234abcd12345abcdef123456");
	tPrintf("Before: '%s'\n", src.Chr());
	src.Replace("abc", "cartoon");
	tPrintf("Replacing abc with cartoon\n");
	tPrintf("After : '%s'\n\n", src.Chr());
	tRequire(src == "cartoon1234cartoond12345cartoondef123456");

	src = "abc1234abcd12345abcdef123456";
	tPrintf("Before: '%s'\n", src.Chr());
	src.Replace("abc", "Z");
	tPrintf("Replacing abc with Z\n");
	tPrintf("After : '%s'\n\n", src.Chr());
	tRequire(src == "Z1234Zd12345Zdef123456");

	src = "abcabcabc";
	tPrintf("Before: '%s'\n", src.Chr());
	src.Replace("abc", "");
	tPrintf("Replacing abc with \"\"\n");
	tPrintf("After : '%s'\n\n", src.Chr());
	tRequire(src == "");

	src = "abcabcabc";
	tPrintf("Before: '%s'\n", src.Chr());
	src.Replace("abc", 0);
	tPrintf("Replacing abc with null\n");
	tPrintf("After : '%s'\n\n", src.Chr());
	tRequire(src == "");

	src.Clear();
	tPrintf("Before: '%s'\n", src.Chr());
	src.Replace("abc", "CART");
	tPrintf("Replacing abc with CART\n");
	tPrintf("After : '%s'\n\n", src.Chr());
	tRequire(src == "");

	tPrintf("Testing Explode:\n");
	tString src1 = "abc_def_ghi";
	tString src2 = "abcXXdefXXghi";
	tPrintf("src1: %s\n", src1.Chr());
	tPrintf("src2: %s\n", src2.Chr());

	tList<tStringItem> exp1(tListMode::ListOwns);
	tList<tStringItem> exp2(tListMode::ListOwns);
	int count1 = tExplode(exp1, src1, '_');
	int count2 = tExplode(exp2, src2, "XX");

	tPrintf("Count1: %d\n", count1);
	for (tStringItem* comp = exp1.First(); comp; comp = comp->Next())
		tPrintf("   Comp: '%s'\n", comp->Chr());

	tPrintf("Count2: %d\n", count2);
	for (tStringItem* comp = exp2.First(); comp; comp = comp->Next())
		tPrintf("   Comp: '%s'\n", comp->Chr());

	tList<tStringItem> expl(tListMode::ListOwns);
	tString exdup = "abc__def_ghi";
	tExplode(expl, exdup, '_');
	tPrintf("Exploded: ###%s### to:\n", exdup.Chr());
	for (tStringItem* comp = expl.First(); comp; comp = comp->Next())
		tPrintf("   Comp:###%s###\n", comp->Chr());

	tList<tStringItem> expl2(tListMode::ListOwns);
	tString exdup2 = "__a__b_";
	tExplode(expl2, exdup2, '_');
	tPrintf("Exploded: ###%s### to:\n", exdup2.Chr());
	for (tStringItem* comp = expl2.First(); comp; comp = comp->Next())
		tPrintf("   Comp:###%s###\n", comp->Chr());

	src = "abc1234abcd12345abcdef123456";
	tPrintf("Before: '%s'\n", src.Chr());
	tString tgt = src.ExtractMid(3, 4);
	tPrintf("Extracting 1234 with ExtractMid(3, 4)\n");
	tPrintf("After (Extracted): '%s'\n\n", tgt.Chr());
	tPrintf("After (Remain)   : '%s'\n\n", src.Chr());
	tRequire(tgt == "1234" && src == "abcabcd12345abcdef123456");

	tString aa("aa");
	tString exaa = aa.ExtractLeft('a');
	tPrintf("\n\naa extract left word to a: Extracted:###%s###  Left:###%s###\n", exaa.Chr(), aa.Chr());

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
	tPrintf("LeadTrail [%s]\n", leadtrail.Chr());

	leadtrail.RemoveLeading("abc");
	tPrintf("LeadTrail [%s]\n", leadtrail.Chr());
	tRequire(leadtrail == "MIDDLEbbccaab");

	leadtrail.RemoveTrailing("abc");
	tPrintf("LeadTrail [%s]\n", leadtrail.Chr());
	tRequire(leadtrail == "MIDDLE");

	// Test remove prefix and suffix.
	tString presuf("prepreMIDDLEsufsuf");
	tPrintf("PreSuf [%s]\n", presuf.Chr());

	presuf.ExtractLeft("not");
	presuf.ExtractRight("not");
	tPrintf("PreSuf [%s]\n", presuf.Chr());
	tRequire(presuf == "prepreMIDDLEsufsuf");

	presuf.ExtractLeft("pre");
	tPrintf("PreSuf [%s]\n", presuf.Chr());
	tRequire(presuf == "preMIDDLEsufsuf");

	presuf.ExtractRight("suf");
	tPrintf("PreSuf [%s]\n", presuf.Chr());
	tRequire(presuf == "preMIDDLEsuf");
}


tTestUnit(StringNew)
{
	// Test construction.
	//bString str;

	// Testing the string substitution code.
	bString strAsc("abc1234abcd12345abcdef123456");
	tPrintf("strAsc:[%s] Len:%d Cap:%d\n", strAsc.Chr(), strAsc.Length(), strAsc.Capacity());

	bString strUtf(u8"abc1234abcd12345abcdef123456");
	tPrintf("strUtf:[%s] Len:%d Cap:%d\n", strUtf.Chr(), strUtf.Length(), strUtf.Capacity());

//	src.Replace("abc", "cartoon");
//	tPrintf("Replacing abc with cartoon\n");
//	tPrintf("After : '%s'\n\n", src.Chr());
//	tRequire(src == "cartoon1234cartoond12345cartoondef123456");

	// Test Left, Mid, Right.
	bString left, mid, right;
	bString lmr("leftMIDright");
	tPrintf("LMR [%s]\n", lmr.Chr());

	tPrintf("\nMarker left/right\n");
	left = lmr.Left('M');
	tPrintf("LEFT(mrk) [%s]\n", left.Chr());
	tRequire(left == "left");
	tRequire(left.Length() == 4);

	left = lmr.Left('l');
	tPrintf("LEFT(mrk) [%s]\n", left.Chr());
	tRequire(left == "");
	tRequire(left.Length() == 0);

	right = lmr.Right('D');
	tPrintf("RIGHT(mrk) [%s]\n", right.Chr());
	tRequire(right == "right");
	tRequire(right.Length() == 5);

	right = lmr.Right('t');
	tPrintf("RIGHT(mrk) [%s]\n", right.Chr());
	tRequire(right == "");
	tRequire(right.Length() == 0);

	tPrintf("\nCount left/mid/right.\n");
	left = lmr.Left(4);
	tPrintf("LEFT(cnt) [%s]\n", left.Chr());
	tRequire(left == "left");
	tRequire(left.Length() == 4);

	left = lmr.Left(0);
	tPrintf("LEFT(cnt) [%s]\n", left.Chr());
	tRequire(left == "");
	tRequire(left.Length() == 0);

	mid = lmr.Mid(4, 3);
	tPrintf("MID(cnt) [%s]\n", mid.Chr());
	tRequire(mid == "MID");
	tRequire(mid.Length() == 3);

	right = lmr.Right(5);
	tPrintf("RIGHT(cnt) [%s]\n", right.Chr());
	tRequire(right == "right");
	tRequire(right.Length() == 5);

	right = lmr.Right(0);
	tPrintf("RIGHT(cnt) [%s]\n", right.Chr());
	tRequire(right == "");
	tRequire(right.Length() == 0);

	// Testing ExtractLeft and ExtractRight.
	tPrintf("\nMarker extract left/right\n");
	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	left = lmr.ExtractLeft('_');
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("LEFT (after): %s\n", left.Chr());
	tRequire(lmr.Length() == 7);
	tRequire(left == "abc");
	tRequire(left.Length() == 3);

	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	left = lmr.ExtractLeft('a');
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("LEFT (after): %s\n", left.Chr());
	tRequire(lmr.Length() == 10);
	tRequire(left == "");
	tRequire(left.Length() == 0);

	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	right = lmr.ExtractRight('_');
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("RIGHT(after): %s\n", right.Chr());
	tRequire(lmr.Length() == 7);
	tRequire(right == "ghi");
	tRequire(right.Length() == 3);

	tPrintf("\nCount extract left/right\n");
	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	left = lmr.ExtractLeft(3);
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("LEFT (after): %s\n", left.Chr());
	tRequire(lmr.Length() == 8);
	tRequire(left == "abc");
	tRequire(left.Length() == 3);

	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	left = lmr.ExtractLeft(0);
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("LEFT (after): %s\n", left.Chr());
	tRequire(lmr.Length() == 11);
	tRequire(left == "");
	tRequire(left.Length() == 0);

	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	mid = lmr.ExtractMid(4, 3);
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("MID  (after): %s\n", mid.Chr());
	tRequire(lmr.Length() == 8);
	tRequire(mid == "def");
	tRequire(mid.Length() == 3);

	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	mid = lmr.ExtractMid(9, 3);
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("MID  (after): %s\n", mid.Chr());
	tRequire(lmr.Length() == 9);
	tRequire(mid == "hi");
	tRequire(mid.Length() == 2);

	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	mid = lmr.ExtractMid(0, 4);
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("MID  (after): %s\n", mid.Chr());
	tRequire(lmr.Length() == 7);
	tRequire(mid == "abc_");
	tRequire(mid.Length() == 4);

	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	right = lmr.ExtractRight(3);
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("RIGHT(after): %s\n", right.Chr());
	tRequire(lmr.Length() == 8);
	tRequire(right == "ghi");
	tRequire(right.Length() == 3);

	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	right = lmr.ExtractRight(0);
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("RIGHT(after): %s\n", right.Chr());
	tRequire(lmr.Length() == 11);
	tRequire(right == "");
	tRequire(right.Length() == 0);

	tPrintf("\nPrefix/Suffix extract left/right\n");
	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	left = lmr.ExtractLeft("abc");
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("LEFT (after): %s\n", left.Chr());
	tRequire(lmr.Length() == 8);
	tRequire(left == "abc");
	tRequire(left.Length() == 3);

	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	left = lmr.ExtractLeft("");
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("LEFT (after): %s\n", left.Chr());
	tRequire(lmr.Length() == 11);
	tRequire(left == "");
	tRequire(left.Length() == 0);

	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	right = lmr.ExtractRight("ghi");
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("RIGHT(after): %s\n", right.Chr());
	tRequire(lmr.Length() == 8);
	tRequire(right == "ghi");
	tRequire(right.Length() == 3);

	lmr = "abc_def_ghi";
	tPrintf("LMR  (before): %s\n", lmr.Chr());
	right = lmr.ExtractRight("");
	tPrintf("LMR  (after): %s\n", lmr.Chr());
	tPrintf("RIGHT(after): %s\n", right.Chr());
	tRequire(lmr.Length() == 11);
	tRequire(right == "");
	tRequire(right.Length() == 0);
}


tTestUnit(UTF)
{
	// Test conversions between various UTF text encodings. Tacent supports:
	// UTF-8  : The native encoding for tString and most char* functions.
	// UTF-16 : For marshalling data to and from OS calls, especially on Windows.
	// UTF-32 : For representing individual characters as a single data-type. This helps reduce complexity for some functions.
	tPrintf("Testing conversions between UTF encodings.\n");

	const char8_t* utf8src =
		u8"wΔ𝒞\n"
		"I refuse to prove that I exist for proof denies faith and without faith I am nothing.\n"
		"Ah, but the Babel fish proves you exist, therefore you don't.\n"
		"And here are some Unicode characters: wΔ𝒞 (the third should look similar to a C)\n"
		"w is ASCII, Δ is in the Basic Multilingual Plane, and 𝒞 is in an Astral plane.";

	// Convert UTF-8 to UTF-16 and write to file.
	int length16 = tStd::tUTF16s(nullptr, utf8src);
	tPrintf("%d char16 codeunits are needed for the UTF-16 encoding of:\n%s\n", length16, utf8src);
	char16_t* utf16str = new char16_t[length16+1];
	tStd::tUTF16s(utf16str, utf8src);

	const char* wfilename16 = "TestData/UTF/WrittenUTF16.txt";
	tPrintf("Writing UTF-16 string to %s\n", wfilename16);
	tCreateFile(wfilename16, utf16str, length16, true);
	const char* rfilename16 = "TestData/UTF/UTF16.txt";
	tRequire(tSystem::tFilesIdentical(wfilename16, rfilename16));

	// Convert UTF-16 back to UTF-8 and write to file.
	int length8 = tStd::tUTF8s(nullptr, utf16str);
	tPrintf("%d char8 codeunits are needed for the UTF-8 encoding.\n", length8);
	char8_t* utf8str = new char8_t[length8+1];
	tStd::tUTF8s(utf8str, utf16str);

	const char* wfilename8 = "TestData/UTF/WrittenUTF8.txt";
	tPrintf("Writing UTF-8 string to %s\n", wfilename8);
	tCreateFile(wfilename8, utf8str, length8, false);
	const char* rfilename8 = "TestData/UTF/UTF8.txt";
	tRequire(tSystem::tFilesIdentical(wfilename8, rfilename8));

	// Convert UTF-8 to UTF-32 and write to file.
	int length32 = tStd::tUTF32s(nullptr, utf8src);
	tPrintf("%d char32 codeunits are needed for the UTF-32 encoding.\n", length32);
	char32_t* utf32str = new char32_t[length32+1];
	tStd::tUTF32s(utf32str, utf8src);

	const char* wfilename32 = "TestData/UTF/WrittenUTF32.txt";
	tPrintf("Writing UTF-32 string to %s\n", wfilename32);
	tCreateFile(wfilename32, utf32str, length32, true);
	const char* rfilename32 = "TestData/UTF/UTF32.txt";
	tRequire(tSystem::tFilesIdentical(wfilename32, rfilename32));

	delete[] utf8str;
	delete[] utf16str;
	delete[] utf32str;

	// Test the tString UTF conversion functions.
	tString testUTF16AndBack(u8"wΔ𝒞 went from UTF-8 to UTF-16 and back to UTF-8");
	tString orig16 = testUTF16AndBack;
	int len16 = testUTF16AndBack.GetUTF16(nullptr);
	utf16str = new char16_t[len16];
	testUTF16AndBack.GetUTF16(utf16str);
	testUTF16AndBack.SetUTF16(utf16str);
	tRequire(testUTF16AndBack == orig16);
	tPrintf("%s\n", testUTF16AndBack.Chr());
	const char* wfilename8A = "TestData/UTF/WrittenUTF8_UTF16_UTF8.txt";
	tCreateFile(wfilename8A, testUTF16AndBack);

	tString testUTF32AndBack(u8"wΔ𝒞 went from UTF-8 to UTF-32 and back to UTF-8");
	tString orig32 = testUTF32AndBack;
	int len32 = testUTF32AndBack.GetUTF32(nullptr);
	utf32str = new char32_t[len32];
	testUTF32AndBack.GetUTF32(utf32str);
	testUTF16AndBack.SetUTF32(utf32str);
	tRequire(testUTF32AndBack == orig32);
	tPrintf("%s\n", testUTF32AndBack.Chr());
	const char* wfilename8B = "TestData/UTF/WrittenUTF8_UTF32_UTF8.txt";
	tCreateFile(wfilename8B, testUTF32AndBack);

	delete[] utf16str;
	delete[] utf32str;
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


tTestUnit(Hash)
{
	const char* testString = "This is the text that is being used for testing hash functions.";
	tPrintf("%s\n\n", testString);

	tPrintf("Fast 32  bit hash: %08x\n", tHash::tHashStringFast32(testString));
	tPrintf("Good 32  bit hash: %08x\n", tHash::tHashString32(testString));
	tPrintf("Good 64  bit hash: %016|64x\n", tHash::tHashString64(testString));

	// For reference and testing:
	// MD5("The quick brown fox jumps over the lazy dog") = 9e107d9d372bb6826bd81d3542a419d6
	// MD5("The quick brown fox jumps over the lazy dog.") = e4d909c290d0fb1ca068ffaddf22cbd0
	const char* md5String = "The quick brown fox jumps over the lazy dog";
	tuint128 md5HashComputed = tHash::tHashStringMD5(md5String);
	tuint128 md5HashCorrect("0x9e107d9d372bb6826bd81d3542a419d6");
	tPrintf("MD5 String   : %s\n", md5String);
	tPrintf("MD5 Correct  : %032|128x\n", md5HashCorrect);
	tPrintf("MD5 Computed : %032|128x\n\n", md5HashComputed);
	tRequire(md5HashComputed == md5HashCorrect);

	tuint256 hash256 = tHash::tHashString256(testString);
	tPrintf("Good 256 bit hash: %064|256X\n\n", hash256);

	tPrintf("Fast 32  bit hash: %08x\n", tHash::tHashStringFast32(testString));
	tPrintf("Good 32  bit hash: %08x\n", tHash::tHashString32(testString));
	tPrintf("Good 64  bit hash: %016|64x\n\n", tHash::tHashString64(testString));

	uint32 hash32single = tHash::tHashStringFast32("This is a string that will be separated into two hash computations.");
	tPrintf("Fast 32 bit single hash  : %08x\n", hash32single);
	uint32 part32 = tHash::tHashStringFast32("This is a string that will be sepa");
	part32 = tHash::tHashStringFast32("rated into two hash computations.", part32);
	tPrintf("Fast 32 bit two part hash: %08x\n\n", part32);
	tRequire(hash32single == part32);

	// From the header: The HashData32/64/128/256 and variants do _not_ guarantee the same hash value if they are chained together.
	tPrintf("Single 64 bit hash  : %016|64x\n", tHash::tHashString64("This is a string that will be separated into two hash computations."));
	uint64 part64 = tHash::tHashString64("This is a string that will be sepa");
	part64 = tHash::tHashString64("rated into two hash computations.", part64);
	tPrintf("Two part 64 bit hash: %016|64x\n\n", part64);

	// This makes sure nobody changes how the hash functions work, which would be bad. It does this by hardcoding the
	// result into the test. @todo We should do this for all hash function variants.
	tuint256 hashString256 = tHash::tHashString256(testString);
	const char* realHashString256 = "6431af73 c538aa59 318121fd 25696a9f e3c05e59 8cb3c9c2 74bfbde6 3b1be458";
	tuint256 hashStringCorrect256(realHashString256, 16);
	tPrintf
	(
		"Good 256 bit hash: %0_64|256x\n"
		"Real 256 bit hash: %s\n\n",
		hashString256, realHashString256
	);
	tRequire(hashString256 == hashStringCorrect256);

	// SHA256 testing. The default IV _must_ be used. The test vectors are from known correct SHA256 results.
	tPrintf("Testing SHA-256 Implementation:\n");
	const char*	shaMesg;
	tuint256	shaComp;
	tuint256	shaCorr;

	shaMesg = "";
	shaComp = tHash::tHashStringSHA256(shaMesg);
	shaCorr.Set("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", 16);
	tPrintf("Message : [%s]\n" "Computed: %0_64|256X\n" "Correct : %0_64|256X\n", shaMesg, shaComp, shaCorr);
	tRequire(shaComp == shaCorr);

	shaMesg = "abc";
	shaComp = tHash::tHashStringSHA256(shaMesg);
	shaCorr.Set("BA7816BF 8F01CFEA 414140DE 5DAE2223 B00361A3 96177A9C B410FF61 F20015AD", 16);
	tPrintf("Message : [%s]\n" "Computed: %0_64|256X\n" "Correct : %0_64|256X\n", shaMesg, shaComp, shaCorr);
	tRequire(shaComp == shaCorr);

	shaMesg = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
	shaComp = tHash::tHashStringSHA256(shaMesg);
	shaCorr.Set("a8ae6e6ee929abea3afcfc5258c8ccd6f85273e0d4626d26c7279f3250f77c8e", 16);
	tPrintf("Message : [%s]\n" "Computed: %0_64|256X\n" "Correct : %0_64|256X\n", shaMesg, shaComp, shaCorr);
	tRequire(shaComp == shaCorr);

	shaMesg = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde";
	shaComp = tHash::tHashStringSHA256(shaMesg);
	shaCorr.Set("057ee79ece0b9a849552ab8d3c335fe9a5f1c46ef5f1d9b190c295728628299c", 16);
	tPrintf("Message : [%s]\n" "Computed: %0_64|256X\n" "Correct : %0_64|256X\n", shaMesg, shaComp, shaCorr);
	tRequire(shaComp == shaCorr);

	shaMesg = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0";
	shaComp = tHash::tHashStringSHA256(shaMesg);
	shaCorr.Set("2a6ad82f3620d3ebe9d678c812ae12312699d673240d5be8fac0910a70000d93", 16);
	tPrintf("Message : [%s]\n" "Computed: %0_64|256X\n" "Correct : %0_64|256X\n", shaMesg, shaComp, shaCorr);
	tRequire(shaComp == shaCorr);

	shaMesg = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
	shaComp = tHash::tHashStringSHA256(shaMesg);
	shaCorr.Set("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", 16);
	tPrintf("Message : [%s]\n" "Computed: %0_64|256X\n" "Correct : %0_64|256X\n", shaMesg, shaComp, shaCorr);
	tRequire(shaComp == shaCorr);

	shaMesg = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
	shaComp = tHash::tHashStringSHA256(shaMesg);
	shaCorr.Set("cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1", 16);
	tPrintf("Message : [%s]\n" "Computed: %0_64|256X\n" "Correct : %0_64|256X\n", shaMesg, shaComp, shaCorr);
	tRequire(shaComp == shaCorr);

	uint8 binMsg[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10 };
	shaComp = tHash::tHashDataSHA256(binMsg, 16);
	shaCorr.Set("411d3f1d2390ff3f482ac8df4e730780bb081a192f283d2f373138fd101dc8fe", 16);
	tPrintf("Message : Binary:%s\n" "Computed: %0_64|256X\n" "Correct : %0_64|256X\n", "0x0123456789ABCDEFFEDCBA9876543210", shaComp, shaCorr);
	tRequire(shaComp == shaCorr);

	// Test a million-character-long message.
	uint8* millionA = new uint8[1000000];
	tStd::tMemset(millionA, 'a', 1000000);
	shaComp = tHash::tHashDataSHA256(millionA, 1000000);
	shaCorr.Set("CDC76E5C 9914FB92 81A1C7E2 84D73E67 F1809A48 A497200E 046D39CC C7112CD0", 16);
	tPrintf("Message : One million 'a's\n" "Computed: %0_64|256X\n" "Correct : %0_64|256X\n", shaComp, shaCorr);
	tRequire(shaComp == shaCorr);
}


}
