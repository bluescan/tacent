// tInterval.h
//
// Intervals and IntervalSets. A tInterval is a set of numbers (over a discrete or continuous domain) that has a start
// and an end. Mathematical 'interval notation' is used to represent such a set. In this notation square brackets []
// mean the endpoint is included, and normal brackets / US-parens () means the endpoint is excluded.
// For example, [0,5) over integers represents the set { 0, 1, 2, 3, 4 }
//
// The tIntervalSet class represents collections of possibly disjoint intervals. For example, the interval set:
// [0,3)U(10,14) over integers represents the set { 0, 1, 2, 11, 12, 13 }. The tIntervalSet class has the ability to
// add new intervals to a set, detect overlaps and/or joins, and represent the new set in the simplest possible form.
//
// Copyright (c) 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Foundation/tFundamentals.h>
namespace tMath
{


// Specifies how intervals are represented when stored as strings.
enum class tIntervalRep
{
	Normal,		// { 4, 5, 6 } represented as string "[4,7)" or some variation. [] means inclusive.
	Range		// { 4, 5, 6 } represented as string "!3-7!" or some variation. ! means exclusive. Does not handle negative ranges.
};


// The tInterval type represents a single interval over integral types. The string form follows the mathematical notation
// (a,b), (a,b], [a,b), or [a,b]. In this representation the square brackets mean inclusive and normal brackets
// (US parens) are exclusive.
// The interval 5     -> [5,5] -> { 5 }
// The interval [0,5) -> { 0 1 2 3 4 }
// The interval (5,5) -> empty
// The interval [5,5) -> empty
// The interval (5,5] -> empty
// The interval [5,5] -> { 5 }
// The interval (4,5] -> { 5 }
// The interval (4,5) -> empty
// @todo In the future we will rename tInterval to tIntervalDiscrete and tunn it into a template that supports any
// integral type: int, uint, int64, etc.
struct tInterval
{
	// Creates an empty interval. We use (0,0) with inner bias as the default empty set.
	tInterval()																											{ Set(0, 0, tBias::Inner); }
	tInterval(const tInterval& src)																						{ Set(src); }
	tInterval(int a, int b, tBias bias = tBias::Full)																	{ Set(a, b, bias); }

	// String representation is auto-detected.
	tInterval(const tString& s)																							{ Set(s); }

	void Clear()																										{ Set(0, 0, tBias::Inner); }

	// Returns true if the interval is not the empty set (contains nothing). Note that (4,5) is invalid for ints but
	// valid for floats/doubles.
	bool IsValid() const;

	// Returns true if the interval is the empty set.
	bool IsEmpty() const																								{ return !IsValid(); }
	void Set(const tInterval& i)																						{ A = i.A; B = i.B; Bias = i.Bias; }
	void Set(int a, int b, tBias bias = tBias::Full)																	{ A = a; B = b; Bias = bias; }

	// Returns true on success. If string not well-formed interval will be empty and false returned. The input
	// representation is auto-detected. Presence of any []() will cause representation to be parsed as 'normal'.
	bool Set(const tString& s);
	void Get(tString& s, tIntervalRep = tIntervalRep::Normal) const;
	tString Get(tIntervalRep rep = tIntervalRep::Normal) const															{ tString s; Get(s, rep); return s; }

	// Integral intervals can always be converted to inclusive endpoints only. If the interval was empty to begin with,
	// it will be empty after. Returns a reference to self.
	// @note This function will not be present for continuous domain (float, double) implementation.
	tInterval& MakeInclusive();

	bool InclusiveLeft() const																							{ return InclusiveLeft(Bias); }
	bool InclusiveRight() const																							{ return InclusiveRight(Bias); }
	bool ExclusiveLeft() const																							{ return ExclusiveLeft(Bias); }
	bool ExclusiveRight() const																							{ return ExclusiveRight(Bias); }

	bool Contains(int v) const																							{ return tInInterval(v, A, B, Bias); }
	bool Contains(const tInterval& v) const;
	bool Overlaps(const tInterval& v, bool checkForJoins = false) const;

	// Extend only increases the interval if there was an overlap. Returns true on success.
	bool Extend(const tInterval& v, bool allowJoins = false);

	// Encapsulate increases the interval even if they don't overlap. Returns true on success.
	bool Encapsulate(const tInterval& v);

	// Returns the number of integers in this interval, or 0 for empty intervals.
	// @note Will not be possible for continuous domain (float,double) intervals.
	int Count() const;

	tInterval& operator=(const tInterval& i)																			{ Set(i);  return *this; }

	// A, B, and Bias must match exactly. For integral intervals, no conversion to inclusive and subsequent compare.
	bool operator==(const tInterval& i) const																			{ return (A == i.A) && (B == i.B) && (Bias == i.Bias); }

	tBias Bias	= tBias::Inner;
	int A		= 0;
	int B		= 0;

	inline static bool InclusiveLeft(tBias bias)	{ return (bias == tBias::Left)  || (bias == tBias::Full); }
	inline static bool InclusiveRight(tBias bias)	{ return (bias == tBias::Right) || (bias == tBias::Full); }
	inline static bool ExclusiveLeft(tBias bias)	{ return (bias == tBias::Right) || (bias == tBias::Inner); }
	inline static bool ExclusiveRight(tBias bias)	{ return (bias == tBias::Left)  || (bias == tBias::Inner); }
};


// The tIntervalSetRep only defines the syntax of the union of individual intervals rather than the syntax of the
// intervals themselves.
enum class tIntervalSetRep
{
	Bar,		// Uses | to join intervals. { 1, 2, 3, 4, 5, 6 } could be represented as "[1,3]|[4,6]" or "1-3|4-6"
	Union,		// Uses U to join intervals. { 1, 2, 3, 4, 5, 6 } could be represented as "[1,3]U[4,6]" or "1-3U4-6"
	Cross		// Uses + to join intervals. { 1, 2, 3, 4, 5, 6 } could be represented as "[1,3]+[4,7]" or "1-3+4-6"
};


// This class represents a collection of multiple intervals. The intervals may not overlap, but if they do this class
// knows how to simplify the collection to the fewest number of intervals possible.
// @todo In future this will be renamed to tIntervalSetDiscrete and templatized to accept any integral type.
struct tIntervalSet
{
	tIntervalSet()																										{ Clear(); }
	tIntervalSet(const tIntervalSet& src)																				{ Set(src); }
	tIntervalSet(const tString& s)																						{ Set(s); }

	void Clear()																										{ Intervals.Empty(); }

	// Returns true if this object has any intervals. They are guaranteed to be valid non-empty ones if it does.
	bool IsValid() const																								{ return !Intervals.IsEmpty(); }
	bool IsEmpty() const																								{ return Intervals.IsEmpty(); }

	void Set(const tIntervalSet& src)																					{ Clear(); for (tItList<tInterval>::Iter i = src.Intervals.First(); i; ++i) Intervals.Append(new tInterval(*i)); }

	// String should be in form "[4,6)U[5,8]" or "[4,6)|[5,8]". Think of the | as an 'or' (or U for union). It means a
	// value is in the set of intervals if it is in the first interval, _or_ the second, _or) the third etc. This set
	// call is 'smart' and will deal with overlaps between intervals. In the example above:
	// [4,6)|[5,8] -> [4,8]. You may also pass in a string using the 'Range' notation. See tIntervalRep enum.
	void Set(const tString& src);

	void Get(tString& s, tIntervalRep = tIntervalRep::Normal, tIntervalSetRep = tIntervalSetRep::Bar) const;
	tString Get(tIntervalRep intRep = tIntervalRep::Normal, tIntervalSetRep setRep = tIntervalSetRep::Bar) const		{ tString s; Get(s, intRep, setRep); return s; }

	// Returns true if added successfully. This function can detect overlaps and joins when adding the new interval. It
	// is the workhorse of this class allowing the interval-set to be built up consistently and in the simplest form.
	bool Add(const tInterval& interval);

	// Non-empty integral intervals can always be converted to inclusive endpoints only. This function converts all the
	// intervals in the set to inclusive form. It still represents the same set afterwards. If the interval set was
	// empty to begin with, it will be empty after. Returns reference to self.
	// @note This function will not be present for continuous domain (float, double) implementation.
	tIntervalSet& MakeInclusive();

	// If any interval in the set contains v, true is returned.
	bool Contains(int v) const;
	bool Contains(const tInterval& v) const;

	// Returns the number of integers in this interval set, or 0 for an empty interval set.
	// @note Will not be possible for continuous domain (float,double) intervals.
	int Count() const;

	tItList<tInterval> Intervals;
};


// @todo Implement tIntervalContinuous and tIntervalSetContinuous that are both templates that will support
// 'continuous' types like float and double. Although float and double are only approximations of continuous real
// numbers, they are much closer than the descrete integral types. See comments in the tInterval implementatons for
// an indication of where things will be different. In particular there will be no MakeInclusive ability and joining
// will not be necessary -- only properly overlapping intervals may be extended. This is because with a discrete number
// there is a well-defined next and previous value. With continuous numbers there is no 'next' number, as no matter
// what you choose, there are an infinite set of numbers in between. Basically intervals in the real number domain are
// quite different than intervals in the integer domain.
//
// The tIntervalContinuous classes (over float or double) will support the following example intervals:
// The interval 5.0       -> [5.0,5.0] -> { 5.0 }
// The interval [0.0,5.0) -> x ε R | 0.0 <= x <= 5.0
// The interval (5.0,5.0) -> empty
// The interval [5.0,5.0) -> empty
// The interval (5,0,5.0] -> empty
// The interval [5.0,5.0] -> { 5.0 }
// The interval (4.0,5.0] -> x ε R | 4.0 < x <= 5.0
// The interval (4.0,5.0) -> x ε R | 4.0 < x <  5.0


}


// Implementation below this line.


inline bool tMath::tInterval::IsValid() const
{
	// For integers any exclusive endpoints can be converted to inclusive.
	tInterval incForm(*this);
	incForm.MakeInclusive();
	return (incForm.B >= incForm.A);
	// For float we check if a == b. If so, we must have outer bias. If not same, b must be > a for it to be valid.
	// return ((a == b) && (Bias == tBias::Outer)) || (b > a);
}


inline bool tMath::tInterval::Set(const tString& s)
{
	Clear();

	// The string should be in the form "[(a,b)]". For convenience, if the string is simply of form "a" it will be
	// converted to [a,a].
	tString str(s);

	// This handles a single integer by itself and represents it in normal syntax.
	if (str.IsNumeric())
		str = tString("[") + str + "," + str + "]";

	tIntervalRep rep = tIntervalRep::Range;
	if (str.FindAny("[]()") != -1)
		rep = tIntervalRep::Normal;

	// @note We will need '.' here for continuous domain implementation.
	if (rep == tIntervalRep::Normal)
		str.RemoveAnyNot("[(,0123456789)]-");
	else
		str.RemoveAnyNot(",0123456789!-");

	bool inclusiveA = true;
	bool inclusiveB = true;
	char separator = ',';
	if (rep == tIntervalRep::Normal)
	{
		if ((str[0] != '[') && (str[0] != '('))
			return false;
		inclusiveA = (str[0] == '[');

		if ((str[str.Length()-1] != ']') && (str[str.Length()-1] != ')'))
			return false;
		inclusiveB = (str[str.Length()-1] == ']');
	}
	else
	{
		if (str[0] == '!')
			inclusiveA = false;
		if (str[str.Length()-1] == '!')
			inclusiveB = false;
		separator = '-';
	}

	str.RemoveAny("[()]!");
	if (str.CountChar(separator) != 1)
		return false;

	tString a = str.ExtractLeft(separator);		// b is left in str.
	A = a.AsInt(10);
	B = str.AsInt(10);

	if (inclusiveA && inclusiveB)
		Bias = tBias::Outer;
	else if (!inclusiveA && !inclusiveB)
		Bias = tBias::Inner;
	else if (inclusiveA)
		Bias = tBias::Left;
	else
		Bias = tBias::Right;

	return true;
}


inline void tMath::tInterval::Get(tString& s, tIntervalRep rep) const
{
	s.Clear();
	char buf[64];
	if (rep == tIntervalRep::Range)
		sprintf
		(
			buf, "%s%d-%d%s",
			InclusiveLeft() ? "" : "!",
			A, B,
			InclusiveRight() ? "" : "!"
		);
	else
		sprintf
		(
			buf, "%c%d,%d%c",
			InclusiveLeft() ? '[' : '(',
			A, B,
			InclusiveRight() ? ']' : ')'
		);

	s.Set(buf);
}


inline tMath::tInterval& tMath::tInterval::MakeInclusive()
{
	if (ExclusiveLeft())
	{
		A += 1;
		Bias = (Bias == tBias::Right) ? tBias::Outer : tBias::Left;
	}

	if (ExclusiveRight())
	{
		B -= 1;
		Bias = tBias::Outer;
	}

	return *this;
}


inline bool tMath::tInterval::Contains(const tInterval& v) const
{
	if (IsEmpty() || v.IsEmpty())
		return false;

	#ifdef INTERVAL_FLOAT
	// We need to be careful here when the endpoints are equal. It will depend on endpoint exclusivness.
	bool leftcon = (A == v.A) ?
		((InclusiveLeft() == v.InclusiveLeft()) ? true : InclusiveLeft()) :
		(v.A > A);

	bool rghtcon = (B == v.B) ?
		((InclusiveRight() == v.InclusiveRight()) ? true : InclusiveRight()) :
		(v.B < B);
	#else
	tInterval incThis(*this);	incThis.MakeInclusive();
	tInterval incTest(v);		incTest.MakeInclusive();
	bool leftcon = (incTest.A >= incThis.A);
	bool rghtcon = (incTest.B <= incThis.B);
	#endif

	return (leftcon && rghtcon);
}


inline bool tMath::tInterval::Overlaps(const tInterval& v, bool checkForJoins) const
{
	if (IsEmpty() || v.IsEmpty())
		return false;

	#ifdef INTERVAL_FLOAT
	#else
	tInterval incA(*this);	incA.MakeInclusive();
	tInterval incB(v);		incB.MakeInclusive();

	bool overlap =
		tInIntervalII(incA.A, incB.A, incB.B) ||
		tInIntervalII(incA.B, incB.A, incB.B) ||
		tInIntervalII(incB.A, incA.A, incA.B) ||
		tInIntervalII(incB.B, incA.A, incA.B);

	if (!overlap && checkForJoins)
	{
		if ((incA.A == incB.B+1) || (incA.B == incB.A-1))
			overlap = true;
	}
	return overlap;
	#endif
}


inline bool tMath::tInterval::Extend(const tInterval& v, bool allowJoins)
{
	if (IsEmpty() || v.IsEmpty() || !Overlaps(v, allowJoins))
		return false;
	tInterval incA(*this);	incA.MakeInclusive();
	tInterval incB(v);		incB.MakeInclusive();
	A = tMin(incA.A, incB.B);
	B = tMax(incA.B, incB.B);
	Bias = tBias::Outer;
	return true;
}


inline bool tMath::tInterval::Encapsulate(const tInterval& v)
{
	if (IsEmpty() || v.IsEmpty())
		return false;
	tInterval incA(*this);	incA.MakeInclusive();
	tInterval incB(v);		incB.MakeInclusive();
	A = tMin(incA.A, incB.A);
	B = tMax(incA.B, incB.B);
	Bias = tBias::Outer;
	return true;
}


inline int tMath::tInterval::Count() const
{
	if (IsEmpty())
		return 0;
	tInterval inc(*this);
	inc.MakeInclusive();
	return inc.B-inc.A+1;
}


// tIntervalSet below.


inline void tMath::tIntervalSet::Set(const tString& src)
{
	Clear();
	if (src.IsEmpty())
		return;
	tString s(src);

	// @note We will need '.' here for continuous domain implementation.
	s.RemoveAnyNot("[(,0123456789)]|U+-!");

	s.Replace('U', '|');
	s.Replace('+', '|');
	tList<tStringItem> intervals;
	tStd::tExplode(intervals, s, '|');
	for (tStringItem* interval = intervals.First(); interval; interval = interval->Next())
		Add( tInterval(*interval) );
}


inline void tMath::tIntervalSet::Get(tString& s, tIntervalRep intRep, tIntervalSetRep setRep) const
{
	s.Clear();

	const char* sep = "|";
	if (setRep == tIntervalSetRep::Union)
		sep = "U";
	else if (setRep == tIntervalSetRep::Cross)
		sep = "+";
	for (tItList<tInterval>::Iter i = Intervals.First(); i; ++i)
	{
		s += i->Get(intRep);
		if (i != Intervals.Last())
			s += sep;
	}
}


inline bool tMath::tIntervalSet::Add(const tInterval& interval)
{
	bool modified = false;

	// This first step either exits early if the interval we're adding is completely contained in an existing one,
	// or it removes existing intervals that are completely inside what we're adding.
	tItList<tInterval>::Iter i = Intervals.First();
	while (i.IsValid())
	{
		tItList<tInterval>::Iter next = i;
		++next;

		if (i->Contains(interval))
			return false;
		else if (interval.Contains(*i))
			Intervals.Remove(i);

		i = next;
	}

	// Next we find the first and last overlapping intervals.
	tItList<tInterval>::Iter frst;
	for (auto it = Intervals.First(); it; ++it)
	{
		// Over integers there is another possibility. The intervals may join even if there is no strict overlap.
		if (it->Overlaps(interval, true))
		{
			frst = it;
			break;
		}
	}

	tItList<tInterval>::Iter last;
	for (auto it = Intervals.Last(); it; --it)
	{
		if (it->Overlaps(interval, true))
		{
			last = it;
			break;
		}
	}

	// Now we are in 1 of 4 situations:
	// 1. We are not overlapping the first or last. (even if first == last or they don't exist).
	// 2. We are overlapping the first and last. (works even if first == last).
	// 3. We are overlapping the first and not the last. (works even if first == last).
	// 4. We are overlapping the last and not the first. (works even if first == last).

	// Case 1. Add the interval and we are done.
	if (!frst && !last)
	{
		// We could just append the interval and it would work. However, to keep things organized we can aslo
		// decide to insert the new interval at the ordered position ascending from left to right. Since they
		// are all disjoint (no overlaps or joins) we only need to check the starting position, A.
		tItList<tInterval>::Iter where;
		tInterval incInt = interval; incInt.MakeInclusive();
		for (auto i = Intervals.First(); i; ++i)
		{
			tInterval incI = *i; incI.MakeInclusive();
			if (incInt.A < incI.A)
			{
				where = i;
				break;
			}
		}
		if (where)
			Intervals.Insert(new tInterval(interval), where);
		else
			Intervals.Append(new tInterval(interval));

		return true;
	}

	// Case 2.
	else if (frst && last)
	{
		if (frst != last)
			Intervals.Remove(last);
		frst->Encapsulate(interval);
	}

	// Case 3.
	else if (frst && !last)
	{
		frst->Extend(interval, true);
	}

	// Case 3.
	else if (last && !frst)
	{
		last->Extend(interval, true);
	}

	return true;
}


inline tMath::tIntervalSet& tMath::tIntervalSet::MakeInclusive()
{
	for (auto it = Intervals.First(); it; ++it)
		it->MakeInclusive();

	return *this;
}


inline bool tMath::tIntervalSet::Contains(int v) const
{
	for (auto it = Intervals.First(); it; ++it)
		if (it->Contains(v))
			return true;

	return false;
}


inline bool tMath::tIntervalSet::Contains(const tInterval& v) const
{
	for (auto it = Intervals.First(); it; ++it)
		if (it->Contains(v))
			return true;

	return false;
}


inline int tMath::tIntervalSet::Count() const
{
	int total = 0;
	for (auto it = Intervals.First(); it; ++it)
		total += it->Count();

	return total;
}
