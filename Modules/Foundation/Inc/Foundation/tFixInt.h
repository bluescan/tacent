// tFixInt.h
//
// A tFixInt is a fixed sized integer type that may be larger than the native types. It is not a 'big' integer class
// in that the size of the int is fixed at compile time. This loss of dynamic precision is well worth the efficiency
// gains and simple memory layout that a fixed size affords. tFixInt is ideal for storing integer values where a uint32
// or uint64 is not big enough. The tFixIntU is the unsigned version of a tFixInt. tFixIntU is a superclass of a
// tFixInt. This header also typedefs some commonly used sizes. Specifically it allows one to use the types tint128,
// tint256, tint512, tuint128, tuint256, and tuint512 simply by including this header.
//
// Comparisons
// tBitArray - Use when you want to store a large number of bits and you don't know how many at compile-time.
//             This type os primatily for storage and access to a large number of bits. Not many bitwise or
//             mathematical operators.
// tBitField - Use when know how many bits at compile-time and you want bitwise logic opertors like and, or, xor,
//             shift, not, etc. Good for storing a fixed number of flags or channels etc.
// tFixInt   - Use when you want full mathematical operations like any built-in integral type. Size must be known at
//             compile time and must be a multiple of 32 bits. You get + - / * etc as well as all bitwise logic ops.
//             You can construct a tFixInt from a tBitField of the same size.
//
// Copyright (c) 2004-2006, 2015, 2017, 2020, 2021, 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// This class is based on the BigInt number class by M Phillips - 2005. http://homepages.ihug.co.nz/~aurora76/Malc/
// The original header follows in the next paragraph. It should be considered  part of the copyright notice and
// included with all copies of this work.
//
// Original header text:
// "Thanks also to Zero Soma Valintine, Edward King, and David Brackman for several bug fixes. This code is provided
// as-is with no warranties or guarantees of any kind. Permission to use and modify this code however you like, to suit
// your needs, and redistribute the modified source, hereby granted. But please retain my name, email address, and
// website link at the top of the resulting source file. And please send an email to M Phillips (mbp2@i4free.co.nz or
// mbp2nz@ihug.co.nz) a) if you use this file in a released product, or b) if you find any bugs, or c) if you have any
// suggestions."
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include "Foundation/tString.h"
#include "Foundation/tFundamentals.h"
#include "Foundation/tBitField.h"
template<int> class tFixInt;


// NumBits is the number of bits in the tFixIntU. Good values might be something like 128, or even 256. NumBits must be
// a multiple of 32. Fixed int classes are supported by tPrintf so just call something like:
// tsPrintf(str, "%032|128X", val128) to convert it to a string.
template<int NumBits> class tFixIntU
{
public:
	tFixIntU()																											{ tStaticAssertMsg(NumBits % 32 == 0, "tFixIntU must be a multiple of 32 bits in size."); }

	// Disabled CopyCons so class remains a POD-type. Allows it to be passed to tPrintf for non MSVC compilers.
	// tFixIntU(const tFixIntU& src)																					{ Set(src); }
	// tFixIntU(const tFixInt<NumBits>& src)																			{ Set(src); }

	// A base argument of < 2 means determine the base from a prefix supplied inside the string itself, like "0x".
	// See tStd::tStrtoi for a full description of the behaviour.
	tFixIntU(const char* s, int base = -1)																				{ Set(s, base); }
	tFixIntU(int8 v)																									{ Set(v); }
	tFixIntU(int16 v)																									{ Set(v); }
	tFixIntU(int v)																										{ Set(v); }
	tFixIntU(int64 v)																									{ Set(v); }
	tFixIntU(uint8 v)																									{ Set(v); }
	tFixIntU(uint16 v)																									{ Set(v); }
	tFixIntU(uint v)																									{ Set(v); }
	tFixIntU(uint64 v)																									{ Set(v); }
	tFixIntU(float v)																									{ Set(v); }
	tFixIntU(double v)																									{ Set(v); }
	tFixIntU(const tBitField<NumBits>& v)																				{ Set(v); }

	void Set(const tFixIntU& src)																						{ *this = src; }
	void Set(const tFixInt<NumBits>& src)																				{ *this = src.AsUnsigned(); }
	void Set(const char* s, int base = -1)																				{ *this = tStd::tStrtoiT< tFixInt<NumBits> >(s, base); }
	void Set(int8 v)																									{ Init(v, (v >= 0) ? 0 : 0xFF); }
	void Set(int16 v)																									{ Init(v, (v >= 0) ? 0 : 0xFF); }
	void Set(int v)																										{ Init(v, (v >= 0) ? 0 : 0xFF); }
	void Set(int64 v)																									{ Init(v, (v >= 0) ? 0 : 0xFF); }
	void Set(uint8 v)																									{ Init(v); }
	void Set(uint16 v)																									{ Init(v); }
	void Set(uint v)																									{ Init(v); }
	void Set(uint64 v)																									{ Init(v); }
	void Set(float);
	void Set(double);
	void Set(const tBitField<NumBits>& v)																				{ for (int e = 0; e < NumElements; e++) ElemData[e] = v.GetElement(e); }

	operator int8() const																								{ int8 r; Extract(r); return r; }
	operator int16() const																								{ int16 r; Extract(r); return r; }
	operator int32() const																								{ int32 r; Extract(r); return r; }
	operator int64() const																								{ int64 r; Extract(r); return r; }
	operator uint8() const																								{ uint8 r; Extract(r); return r; }
	operator uint16() const																								{ uint16 r; Extract(r); return r; }
	operator uint32() const																								{ uint32 r; Extract(r); return r; }
	operator uint64() const																								{ uint64 r; Extract(r); return r; }
	operator float() const;
	operator double() const;

	static inline void Swap(tFixIntU& a, tFixIntU& b)																	{ for (int i = 0; i < NumElements; i++) tStd::tSwap(a.ElemData[i], b.ElemData[i]); }
	tFixIntU& operator=(const tFixInt<NumBits>& v)																		{ return *this = v.AsUnsigned(); }
	tFixIntU& operator=(const tBitField<NumBits>& v)																	{ Set(v); return *this; }

	template<int N, bool LhsGreater> struct AssignHelper																{ template <typename T> void operator()(tFixIntU& lhs, const T& rhs) const; };
	template<int N> struct AssignHelper<N, false>																		{ template <typename T> void operator()(tFixIntU& lhs, const T& rhs) const; };
	template<int N> tFixIntU& operator=(const tFixIntU<N>& rhs)															{ AssignHelper<N, (NumBits>N)>()(*this, rhs); return *this; }

	void MakeZero()																										{ for (int i = 0; i < NumElements; i++) ElemData[i] = 0; }
	void MakeMax()																										{ for (int i = 0; i < NumElements; i++) ElemData[i] = 0xFFFFFFFF; }

	const tFixInt<NumBits>& AsSigned() const																			{ return reinterpret_cast<const tFixInt<NumBits>&>(*this); }
	tFixInt<NumBits>& AsSigned()																						{ return reinterpret_cast<tFixInt<NumBits>&>(*this); }
	tString GetAsString(int base) const;

	void ClearBit(int index);
	void SetBit(int index);
	void ToggleBit(int index);
	bool GetBit(int index) const;

	// Gets the n'th byte as a uint8. Zero-based index where zero is the least significant byte.
	// If NumBits is, say, 33 the range of the index will be n E [0,4]. That is, 5 bytes.
	uint8 GetByte(int n) const;

	// Assumes bytes are given from most-significant to least. You need to supply NumBits / 8 of them.
	void SetFromBytes(const uint8* bytes);

	tFixIntU& operator&=(const tFixIntU& v)																				{ for (int i = 0; i < NumElements; i++) ElemData[i] &= v.ElemData[i]; return *this; }
	tFixIntU& operator|=(const tFixIntU& v)																				{ for (int i = 0; i < NumElements; i++) ElemData[i] |= v.ElemData[i]; return *this; }
	tFixIntU& operator^=(const tFixIntU& v)																				{ for (int i = 0; i < NumElements; i++) ElemData[i] ^= v.ElemData[i]; return *this; }
	tFixIntU& operator>>=(int shift);
	tFixIntU& operator<<=(int shift);
	tFixIntU& operator+=(const tFixIntU&);
	tFixIntU& operator-=(const tFixIntU&);
	tFixIntU& operator/=(const tFixIntU& v)																				{ *this = tDivide(*this, v); return *this; }
	tFixIntU& operator/=(int v)																							{ *this = tDivide(*this, v); return *this; }
	tFixIntU& operator%=(const tFixIntU& v)																				{ tDivide(*this, v, this); return *this; }
	tFixIntU& operator%=(int v)																							{ int r; tDivide(*this, v, &r); *this = r; return *this; }
	tFixIntU operator>>(int shift) const																				{ tFixIntU result(*this); result >>= shift; return result; }
	tFixIntU operator<<(int shift) const																				{ tFixIntU result(*this); result <<= shift; return result; }
	tFixIntU& operator*=(const uint32 v);
	tFixIntU& operator*=(const tFixIntU& v);

	// Interestingly, friend functions declared (and even defined) inside class declarations are not considered in the
	// scope of the enclosing class -- they are in the outer scope. Makes perfect sense, just didn't know before.
	friend bool operator==(const tFixIntU& a, const tFixIntU& b)														{ for (int i = 0; i < NumElements; i++) if (a.ElemData[i] != b.ElemData[i]) return false; return true; }
	friend bool operator!=(const tFixIntU& a, const tFixIntU& b)														{ return !(a == b); }
	friend bool operator>(const tFixIntU& a, const tFixIntU& b)															{ return (b < a); }
	friend bool operator<=(const tFixIntU& a, const tFixIntU& b)														{ return !(b < a); }
	friend bool operator>=(const tFixIntU& a, const tFixIntU& b)														{ return !(a < b); }
	friend tFixIntU operator&(const tFixIntU& a, const tFixIntU& b)														{ tFixIntU result; for (int i = 0; i < NumElements; i++) result.ElemData[i] = a.ElemData[i] & b.ElemData[i]; return result; }
	friend tFixIntU operator|(const tFixIntU& a, const tFixIntU& b)														{ tFixIntU result; for (int i = 0; i < NumElements; i++) result.ElemData[i] = a.ElemData[i] | b.ElemData[i]; return result; }
	friend tFixIntU operator^(const tFixIntU& a, const tFixIntU& b)														{ tFixIntU result; for (int i = 0; i < NumElements; i++) result.ElemData[i] = a.ElemData[i] ^ b.ElemData[i]; return result; }
	friend tFixIntU operator+(const tFixIntU& a, const tFixIntU& b)														{ tFixIntU result; result = a; result += b; return result; }
	friend tFixIntU operator-(const tFixIntU& a, const tFixIntU& b)														{ tFixIntU result; result = a; result -= b; return result; }
	friend tFixIntU operator*(const tFixIntU& a, const tFixIntU& b)														{ tFixIntU result; result = a; result *= b; return result; }
	friend tFixIntU operator/(const tFixIntU& a, const tFixIntU& b)														{ return tDivide(a, b); }
	friend tFixIntU operator%(const tFixIntU& a, const tFixIntU& b)														{ tFixIntU result; tDivide(a, b, &result); return result; }

	tFixIntU& operator++();
	tFixIntU operator++(int)								/* The dummy int means postfix. */							{ tFixIntU result; result = *this; ++*this; return result; }
	tFixIntU& operator--();
	tFixIntU operator--(int)																							{ tFixIntU result; result = *this; --*this; return result; }

	// We may want to consider the pitfalls of operator bool() here. See http://www.artima.com/cppsource/safebool.html
	operator bool() const																								{ for (int i = 0; i < NumElements; i++) if (ElemData[i]) return true; return false; }	// Non-zero returns true.
	bool operator!() const																								{ for (int i = 0; i < NumElements; i++) if (ElemData[i]) return false; return true; }	// Only zero returns true.
	tFixIntU operator~() const																							{ tFixIntU result; for (int i = 0; i < NumElements; i++) result.ElemData[i] = ~ElemData[i]; return result; }
	tFixIntU operator+() const																							{ return *this; }	// Unary positive.
	tFixIntU operator-() const																							{ tFixIntU result; for (int i = 0; i < NumElements; i++) result.ElemData[i] = ~ElemData[i]; ++result; return result; }	// Negates a number.

	friend bool operator<(const tFixIntU& a, int b)																		{ return a < tFixInt<NumBits>(b).AsUnsigned(); }
	friend bool operator>(const tFixIntU& a, int b)																		{ return tFixInt<NumBits>(b).AsUnsigned() < a; }
	friend bool operator<=(const tFixIntU& a, int b)																	{ return !(tFixInt<NumBits>(b).AsUnsigned() < a); }
	friend bool operator>=(const tFixIntU& a, int b)																	{ return !(a < tFixInt<NumBits>(b).AsUnsigned()); }
	friend bool operator==(const tFixIntU& a, int b)																	{ return a == tFixInt<NumBits>(b).AsUnsigned(); }
	friend bool operator!=(const tFixIntU& a, int b)																	{ return a != tFixInt<NumBits>(b).AsUnsigned(); }
	friend bool operator<(int a, const tFixIntU& b)																		{ return tFixInt<NumBits>(a).AsUnsigned() < b; }
	friend bool operator>(int a, const tFixIntU& b)																		{ return b < tFixInt<NumBits>(a).AsUnsigned(); }
	friend bool operator<=(int a, const tFixIntU& b)																	{ return !(b < tFixInt<NumBits>(a).AsUnsigned()); }
	friend bool operator>=(int a, const tFixIntU& b)																	{ return !(tFixInt<NumBits>(a).AsUnsigned() < b); }
	friend bool operator==(int a, const tFixIntU& b)																	{ return tFixInt<NumBits>(a).AsUnsigned() == b; }
	friend bool operator!=(int a, const tFixIntU& b)																	{ return tFixInt<NumBits>(a).AsUnsigned() != b; }

	friend tFixIntU operator&(const tFixIntU& a, int b)																	{ return a & tFixIntU(b); }
	friend tFixIntU operator|(const tFixIntU& a, int b)																	{ return a | tFixIntU(b); }
	friend tFixIntU operator^(const tFixIntU& a, int b)																	{ return a ^ tFixIntU(b); }
	friend tFixIntU operator&(int a, const tFixIntU& b)																	{ return tFixIntU(a) & b; }
	friend tFixIntU operator|(int a, const tFixIntU& b)																	{ return tFixIntU(a) | b; }
	friend tFixIntU operator^(int a, const tFixIntU& b)																	{ return tFixIntU(a) ^ b; }
	friend tFixIntU operator+(const tFixIntU& a, int b)																	{ return a + tFixInt<NumBits>(b).AsUnsigned(); }
	friend tFixIntU operator-(const tFixIntU& a, int b)																	{ return a - tFixInt<NumBits>(b).AsUnsigned(); }
	friend tFixIntU operator*(const tFixIntU& a, int b)																	{ return a * tFixInt<NumBits>(b).AsUnsigned(); }
	friend tFixIntU operator/(const tFixIntU& a, int b)																	{ return tDivide(a, b); }
	friend int operator%(const tFixIntU& a, int b)																		{ int result; tDivide(a, b, &result); return result; }
	friend tFixIntU operator+(int a, const tFixIntU& b)																	{ return tFixInt<NumBits>(a).AsUnsigned() + b; }
	friend tFixIntU operator-(int a, const tFixIntU& b)																	{ return tFixInt<NumBits>(a).AsUnsigned() - b; }
	friend tFixIntU operator*(int a, const tFixIntU& b)																	{ return tFixInt<NumBits>(a).AsUnsigned() * b; }
	friend tFixIntU operator/(int a, const tFixIntU& b)																	{ return tFixInt<NumBits>(a).AsUnsigned() / b; }
	friend tFixIntU operator%(int a, const tFixIntU& b)																	{ return tFixInt<NumBits>(a).AsUnsigned() % b; }

	void RotateRight(int shift);
	int FindHighestBitSet() const;
	int FindLowestBitSet() const;

	// These deal with the internal uint32 elements that store the integer. The elements are always least-significant
	// at the beginning, regardless of machine endianness.
	int GetNumElements() const						/* Returns how many uint32s are used for the integer. */			{ return NumElements; }

	uint32 GetElement(int i) const																						{ return ElemData[i]; }
	void SetElement(int i, uint32 val)																					{ ElemData[i] = val; }

	void GetElements(uint32* dst) const				/* Least significant at the beginning. */							{ tAssert(dst); tStd::tMemcpy(dst, ElemData, NumElements*4); }
	void SetElementsZ(const uint32* src)			/* Least sig at the beginning. */									{ tAssert(src); tStd::tMemcpy(ElemData, src, NumElements*4); }

	uint32& Element(int i)																								{ return ElemData[i]; }
	uint32* Elements() const																							{ return (uint32*)ElemData; }

	// As expected, there will be one of these per unique template instantiation.
	static const int NumElements = NumBits / 32;

	// LS stands for Least Significant. MS stands for Most Significant.
	static const int LSIndex = 0;
	static const int MSIndex = NumElements-1;
	static int BaseIndex(int x)																							{ return x; }

	uint32 ElemData[NumElements];																						// ElemData[0] is the LEAST significant uint32.

protected:
	template<typename T> void Init(T, uint8 fill = 0);
	template<typename T> void Extract(T&, uint8 fill = 0) const;
};


// Not all operators should be members, especially binary ones, if you want to be properly object oriented.
template<int N> bool operator<(const tFixIntU<N>& a, const tFixIntU<N>& b);
template<int N> tFixIntU<N> tSqrt(const tFixIntU<N>&);	// Square root.
template<int N> tFixIntU<N> tCurt(const tFixIntU<N>&);	// Cube root.
template<int N> tFixIntU<N> tFactorial(const tFixIntU<N>&);
template<int N> bool tIsPower2(const tFixIntU<N>&);
template<int N> tFixIntU<N> tNextPower2(const tFixIntU<N>&);
template<int N> uint32 tCeilLog2(const tFixIntU<N>&);
template<int N> tFixIntU<N> tPow(tFixIntU<N> a, int b);
template<int N> tFixIntU<N> tModPow(tFixIntU<N> base, tFixIntU<N> exp, const tFixIntU<N>& mod);


// tDivide is here because it offers divide with remainder unlike the binary operator/(). There's also a version here
// that is faster if you only need to divide by a smaller (positive) integer.
template<int N> tFixIntU<N> tDivide(tFixIntU<N> a, tFixIntU<N> b, tFixIntU<N>* remainder = nullptr);
template<int N> tFixIntU<N> tDivide(tFixIntU<N> a, int b, int* remainder = nullptr);


// Now we overload the unsigned functions to provide the necessary differences for signed numbers.
template<int NumBits> class tFixInt : public tFixIntU<NumBits>
{
public:
	using tFixIntU<NumBits>::Init;
	using tFixIntU<NumBits>::Extract;
	using tFixIntU<NumBits>::ElemData;
	using tFixIntU<NumBits>::MSIndex;

	tFixInt()																											{ tStaticAssertMsg(NumBits % 32 == 0, "tFixInt must be a multiple of 32 bits in size."); }
	tFixInt(const char* s, int base = -1)																				{ Set(s, base); }
	tFixInt(int8 v)																										{ Set(v); }
	tFixInt(int16 v)																									{ Set(v); }
	tFixInt(int v)																										{ Set(v); }
	tFixInt(int64 v)																									{ Set(v); }
	tFixInt(uint8 v)																									{ Set(v); }
	tFixInt(uint16 v)																									{ Set(v); }
	tFixInt(uint v)																										{ Set(v); }
	tFixInt(uint64 v)																									{ Set(v); }
	tFixInt(float v)																									{ Set(v); }
	tFixInt(double v)																									{ Set(v); }

	void Set(const char* s, int base = -1)																				{ *this = tStd::tStrtoiT< tFixInt<NumBits> >(s, base); }
	void Set(int8 v)																									{ Init(v, (v >= 0) ? 0 : 0xFF); }
	void Set(int16 v)																									{ Init(v, (v >= 0) ? 0 : 0xFF); }
	void Set(int v)																										{ Init(v, (v >= 0) ? 0 : 0xFF); }
	void Set(int64 v)																									{ Init(v, (v >= 0) ? 0 : 0xFF); }
	void Set(uint8 v)																									{ Init(v); }
	void Set(uint16 v)																									{ Init(v); }
	void Set(uint v)																									{ Init(v); }
	void Set(uint64 v)																									{ Init(v); }
	void Set(float);
	void Set(double);

	operator int8() const																								{ int8 v; Extract(v, (*this >= 0) ? 0 : 0xFF); return v; }
	operator int16() const																								{ int16 v; Extract(v, (*this >= 0) ? 0 : 0xFF); return v; }
	operator int() const																								{ int v; Extract(v, (*this >= 0) ? 0 : 0xFF); return v; }
	operator int64() const																								{ int64 v; Extract(v, (*this >= 0) ? 0 : 0xFF); return v; }
	operator uint8() const																								{ uint8 v; Extract(v); return v; }
	operator uint16() const																								{ uint16 v; Extract(v); return v; }
	operator uint() const																								{ uint v; Extract(v); return v; }
	operator uint64() const																								{ uint64 v; Extract(v); return v; }
	operator float() const																								{ if (ElemData[MSIndex] >> (32-1) != 0u) return -(float)(-*this).AsUnsigned(); else return (float)AsUnsigned(); }
	operator double() const																								{ if (ElemData[MSIndex] >> (32-1) != 0u) return -(double)(-*this).AsUnsigned(); else return (double)AsUnsigned(); }

	template<int N> tFixInt(const tFixInt<N>& v)																		{ *this = v;}
	template<int N> tFixInt(const tFixIntU<N>& v)																		{ *this = v.AsSigned();}
	template<int N> tFixInt& operator=(const tFixIntU<N>& v)															{ return *this = v.AsSigned(); }
	template<int N, bool LhsGreater> struct AssignHelper																{ template <typename T2> void operator()(tFixInt& lhs, const T2& rhs) const; };
	template<int N> struct AssignHelper<N, false>																		{ template <typename T2> void operator()(tFixInt& lhs, const T2& rhs) const; };
	template<int N> tFixInt& operator=(const tFixInt<N>& rhs)															{ AssignHelper< N, (NumBits>N) >()(*this, rhs); return *this; }
	void MakeMin()																										{ *this = tFixIntU<NumBits>(1) << (NumBits-1); }
	void MakeMax()																										{ MakeMin(*this); --(*this); }

	const tFixIntU<NumBits>& AsUnsigned() const																			{ return reinterpret_cast< const tFixIntU<NumBits>& >(*this); }
	tFixIntU<NumBits>& AsUnsigned()																						{ return reinterpret_cast< tFixIntU<NumBits>& >(*this); }

	tFixInt& operator&=(const tFixInt& v)																				{ return ( AsUnsigned() &= v.AsUnsigned() ).AsSigned(); }
	tFixInt& operator|=(const tFixInt& v)																				{ return ( AsUnsigned() |= v.AsUnsigned() ).AsSigned(); }
	tFixInt& operator^=(const tFixInt& v)																				{ return ( AsUnsigned() ^= v.AsUnsigned() ).AsSigned(); }
	tFixInt& operator>>=(int shift);
	tFixInt& operator<<=(int shift)																						{ return ( AsUnsigned() <<= shift ).AsSigned(); }
	tFixInt& operator*=(const tFixInt& v);
	tFixInt& operator/=(const tFixInt& v)																				{ return *this = Divide(*this, v, 0); }
	tFixInt& operator%=(const tFixInt& v)																				{ Divide(*this, v, this); return *this; }

	friend bool operator==(const tFixInt& a, const tFixInt& b)															{ return a.AsUnsigned() == b.AsUnsigned(); }
	friend bool operator!=(const tFixInt& a, const tFixInt& b)															{ return a.AsUnsigned() != b.AsUnsigned(); }
	friend bool operator>(const tFixInt& a, const tFixInt& b)															{ return (b < a); }
	friend bool operator<=(const tFixInt& a, const tFixInt& b)															{ return !(b < a); }
	friend bool operator>=(const tFixInt& a, const tFixInt& b)															{ return !(a < b); }

	const tFixInt operator>>(int shift) const																			{ tFixInt result; result = *this; return (result >>= shift); }
	const tFixInt operator<<(int shift) const																			{ return (AsUnsigned() << shift).AsSigned(); }
	friend const tFixInt operator&(const tFixInt& a, const tFixInt& b)													{ return ( a.AsUnsigned() & b.AsUnsigned() ).AsSigned(); }
	friend const tFixInt operator|(const tFixInt& a, const tFixInt& b)													{ return ( a.AsUnsigned() | b.AsUnsigned() ).AsSigned(); }
	friend const tFixInt operator^(const tFixInt& a, const tFixInt& b)													{ return ( a.AsUnsigned() ^ b.AsUnsigned() ).AsSigned(); }
	friend const tFixInt operator+(const tFixInt& a, const tFixInt& b)													{ return ( a.AsUnsigned() + b.AsUnsigned() ).AsSigned(); }
	friend const tFixInt operator-(const tFixInt& a, const tFixInt& b)													{ return ( a.AsUnsigned() - b.AsUnsigned() ).AsSigned(); }
	friend const tFixInt operator*(const tFixInt& a, const tFixInt& b)													{ tFixInt result; result = a; return (result *= b); }
	friend const tFixInt operator/(const tFixInt& a, const tFixInt& b)													{ return tDivide(a, b); }
	friend const tFixInt operator%(const tFixInt& a, const tFixInt& b)													{ tFixInt result; tDivide(a, b, &result); return result; }

	tFixInt& operator++()																								{ return (++AsUnsigned()).AsSigned(); }
	const tFixInt operator++(int)																						{ return (AsUnsigned()++).AsSigned(); }		// @todo Check.
	tFixInt& operator--()																								{ return (--AsUnsigned()).AsSigned(); }
	const tFixInt operator--(int)																						{ return (AsUnsigned()--).AsSigned(); }		// @todo Check.
	const tFixInt operator~() const																						{ return ( ~AsUnsigned() ).AsSigned(); }
	tFixInt& operator+() const																							{ return (tFixInt&)*this; }					// Unary positive.
	const tFixInt operator-() const																						{ return ( -AsUnsigned() ).AsSigned(); }	// Unary negate.

	friend bool operator<(const tFixInt& a, int b)																		{ return a < tFixInt(b); }
	friend bool operator>(const tFixInt& a, int b)																		{ return tFixInt(b) < a; }
	friend bool operator<=(const tFixInt& a, int b)																		{ return !(tFixInt(b) < a); }
	friend bool operator>=(const tFixInt& a, int b)																		{ return !(a < tFixInt(b)); }
	friend bool operator==(const tFixInt& a, int b)																		{ return a == tFixInt(b); }
	friend bool operator!=(const tFixInt& a, int b)																		{ return a != tFixInt(b); }
	friend bool operator<(int a, const tFixInt& b)																		{ return tFixInt(a) < b; }
	friend bool operator>(int a, const tFixInt& b)																		{ return b < tFixInt(a); }
	friend bool operator<=(int a, const tFixInt& b)																		{ return !(b < tFixInt(a)); }
	friend bool operator>=(int a, const tFixInt& b)																		{ return !(tFixInt(a) < b); }
	friend bool operator==(int a, const tFixInt& b)																		{ return tFixInt(a) == b; }
	friend bool operator!=(int a, const tFixInt& b)																		{ return tFixInt(a) != b; }

	friend const tFixInt operator&(const tFixInt& a, int b)																{ return a & tFixInt(b); }
	friend const tFixInt operator|(const tFixInt& a, int b)																{ return a | tFixInt(b); }
	friend const tFixInt operator^(const tFixInt& a, int b)																{ return a ^ tFixInt(b); }
	friend const tFixInt operator&(int a, const tFixInt& b)																{ return tFixInt(a) & b; }
	friend const tFixInt operator|(int a, const tFixInt& b)																{ return tFixInt(a) | b; }
	friend const tFixInt operator^(int a, const tFixInt& b)																{ return tFixInt(a) ^ b; }

	friend const tFixInt operator+(const tFixInt& a, int b)																{ return a + tFixInt(b); }
	friend const tFixInt operator-(const tFixInt& a, int b)																{ return a - tFixInt(b); }
	friend const tFixInt operator*(const tFixInt& a, int b)																{ return a * tFixInt(b); }
	friend const tFixInt operator/(const tFixInt& a, int b)																{ return tDivide(a, b); }
	friend const int operator%(const tFixInt& a, int b)																	{ int r; tDivide(a, b, &r); return r; }
	friend const tFixInt operator+(int a, const tFixInt& b)																{ return tFixInt(a) + b; }
	friend const tFixInt operator-(int a, const tFixInt& b)																{ return tFixInt(a) - b; }
	friend const tFixInt operator*(int a, const tFixInt& b)																{ return tFixInt(a) * b; }
	friend const tFixInt operator/(int a, const tFixInt& b)																{ return tFixInt(a) / b; }
	friend const tFixInt operator%(int a, const tFixInt& b)																{ return tFixInt(a) % b; }
};


// Not all operators should be members, especially binary ones, if you want to be properly object oriented. tDivide is
// here because it offers divide with remainder unlike the binary operator/(). There's also a version here that is
// faster if you only need to divide by a smaller (positive) integer.
template<int N> bool operator<(const tFixInt<N>& a, const tFixInt<N>& b);
template<int N> tFixInt<N> tSqrt(const tFixInt<N>&);		// Square root.
template<int N> tFixInt<N> tCurt(const tFixInt<N>&);		// cube root.
template<int N> tFixInt<N> tAbs(const tFixInt<N>& v);
template<int N> tFixInt<N> tFactorial(const tFixInt<N>&);
template<int N> tFixInt<N> tPow(const tFixInt<N>& a, int b);
template<int N> bool tIsPrime(const tFixInt<N>&);
template<int N> tFixInt<N> tDivide(const tFixInt<N>& a, const tFixInt<N>& b, tFixInt<N>* remainder = nullptr);
template<int N> tFixInt<N> tDivide(const tFixInt<N>& a, int b, int* remainder = nullptr);


// The Tacent tint types are based on the Tacent fix integer (tFixInt). They can represent large integers and allow all
// arithmetic and bit operations. They are a bit slower than native 32 or 64 bit integers; however, they are still fixed
// size at runtime and therefore faster than other BigInteger implementations that can grow arbitrarily large. The tint
// types support both signed and unsigned versions.
typedef tFixInt<128> tint128;
typedef tFixInt<256> tint256;
typedef tFixInt<512> tint512;
typedef tFixIntU<128> tuint128;
typedef tFixIntU<256> tuint256;
typedef tFixIntU<512> tuint512;


// Implementation below this line.


template<int N> inline void tFixIntU<N>::SetFromBytes(const uint8* bytes)
{
	MakeZero();
	for (int i = MSIndex; i >= LSIndex; i--)
	{
		uint32& idat = ElemData[i];
		idat = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);
		bytes += 4;
	}
}


template<int N> inline void tFixIntU<N>::Set(float v)
{
	*this = 0;
	if (v != v || (v == v && v-v != 0.0f))	// nan or +/-inf
		return;

	bool neg = false;
	if (v < 0.f)
	{
		v = -v;
		neg = true;
	}

	if (v < 1.0f)
		return;

	int e;
	float mant = tStd::tFrexp(v, &e);
	while (mant > 0.0f && e-- > 0)
	{
		mant *= 2.0f;
		if (mant >= 1.0f)
		{
			mant -= 1.0f;
			if (e < N)
				SetBit(e);
		}
	}
	if (neg)
		*this = -*this;
}


template<int N> inline void tFixIntU<N>::Set(double v)
{
	*this = 0;
	if (v != v || (v == v && v-v != 0.0))	// nan or +/-inf
		return;

	bool neg = false;
	if (v < 0.0)
	{
		v = -v;
		neg = true;
	}

	if (v < 1.0)
		return;

	int e;
	double mant = tStd::tFrexp(v, &e);
	while (mant > 0.0 && e-- > 0)
	{
		mant *= 2.0;
		if (mant >= 1.0)
		{
			mant -= 1.0;
			if (e < N)
				SetBit(e);
		}
	}

	if (neg)
		*this = -*this;
}


template<int N> inline tFixIntU<N>::operator float() const
{
	static const float multiplier = (float(0xFFFFFFFF) + 1.0f);
	float result = 0;

	for (int i = NumElements-1; i >= 0; i--)
		result = result * multiplier + ElemData[i];

	return result;
}


template<int N> inline tFixIntU<N>::operator double() const
{
	static const double multiplier = (double(0xFFFFFFFF) + 1.0);
	double result = 0;

	for (int i = NumElements-1; i >= 0; i--)
		result = result * multiplier + ElemData[i];

	return result;
}


template<int N> inline tString tFixIntU<N>::GetAsString(int base) const
{
	// Worst case for string length required is base 2, where N characters are needed.
	tString str(N);
	tStd::tItostrT< tFixIntU<N> >(*this, str.Text(), N+1, base);
	return str;
}


template<int N> inline void tFixIntU<N>::ClearBit(int b)
{
	ElemData[b/32] &= ~(uint32(1)<<(b%32));
}


template<int N> inline void tFixIntU<N>::SetBit(int b)
{
	ElemData[b/32] |= uint32(1)<<(b%32);
}


template<int N> inline void tFixIntU<N>::ToggleBit(int b)
{
	ElemData[b/32] ^= uint32(1)<<(b%32);
}


template<int N> inline bool tFixIntU<N>::GetBit(int b) const
{
	return (ElemData[b/32] & (uint32(1)<<(b%32))) != 0;
}


template<int N> inline uint8 tFixIntU<N>::GetByte(int n) const
{
	int numBytes = (N / 8) + ((N % 8) ? 1 : 0);
	tAssert(n < numBytes);
	int idx = n / 4;
	int shift = (n % 4) << 3;
	uint32 elem = ElemData[idx];
	return (elem & (0xFF << shift)) >> shift;
}


template<int N> template<typename T> inline void tFixIntU<N>::Init(T v, uint8 fill)
{
	if (sizeof(v) <= sizeof(ElemData))
	{
		tStd::tMemcpy(ElemData, &v, sizeof(v));
		tStd::tMemset(reinterpret_cast<char*>(ElemData) + sizeof(v), fill, sizeof(ElemData) - sizeof(v));
	}
	else
	{
		tStd::tMemcpy(ElemData, &v, sizeof(ElemData));
	}
}


template<int N> template<typename T> inline void tFixIntU<N>::Extract(T& v, uint8 fill) const
{
	if (sizeof(v) <= sizeof(ElemData))
	{
		tStd::tMemcpy(&v, ElemData, sizeof(v));
	}
	else
	{
		tStd::tMemcpy(&v, ElemData, sizeof(ElemData));
		tStd::tMemset(reinterpret_cast<char*>(&v) + sizeof(v), fill, int(sizeof(v) - sizeof(ElemData)));
	}
}


template<int N> template<int B, bool LhsGreater> template<typename T> inline void tFixIntU<N>::AssignHelper<B, LhsGreater>::operator()(tFixIntU& lhs, const T& rhs) const
{
	const uint32* const rhsData = (uint32*)&rhs;
	int i = 0;
	// If rhs is smaller, copy what we can over and fill in the rest with 0.
	// If lhs is smaller, we may lose info (like casting an int to a short).
	int lhsNum = lhs.NumElements;
	int rhsNum = rhs.NumElements;
	for (; i < tMath::tMin(lhsNum, rhsNum); i++)
		lhs.ElemData[BaseIndex(i)] = rhsData[BaseIndex(i)];
	for (; i < NumElements; i++)
		lhs.ElemData[BaseIndex(i)] = 0u;
}


template<int N> template<int B> template<typename T> inline void tFixIntU<N>::AssignHelper<B, false>::operator()(tFixIntU& lhs, const T& rhs) const
{
	const uint32* rhsData = (uint32*)&rhs;
	for (int i = 0; i < NumElements; i++)
		lhs.ElemData[BaseIndex(i)] = rhsData[BaseIndex(i)];
}


template<int N> inline tFixIntU<N> tDivide(tFixIntU<N> a, tFixIntU<N> b, tFixIntU<N>* remainder)
{
	int shiftcnt = 0;
	if (!b)
	{
		shiftcnt = 1 / shiftcnt;
		tFixIntU<N> r;
		r.MakeMax();
		return r;
	}

	shiftcnt = a.FindHighestBitSet() - b.FindHighestBitSet();
	if (shiftcnt > 0)
		b <<= shiftcnt;

	if (b > a)
	{
		b >>= 1;
		--shiftcnt;
	}

	tFixIntU<N> c(0u);
	while (shiftcnt >= 0)
	{
		if (b <= a)\
		{
			a -= b;
			c.SetBit(shiftcnt);
		}
		b >>= 1;
		--shiftcnt;
	}

	if (remainder)
		*remainder = a;
	return c;
}


template<int N> inline tFixIntU<N> tDivide(tFixIntU<N> a, int b, int* remainder)
{
	// Special version of division that's fast but only works with small divisors. 'a' must be positive.
	tAssert(b);
	uint32 temp = 0;
	bool seenNonZero = false;
	tFixInt<N> result = 0;

	for (int i = a.GetNumElements()-1; i >= 0; i--)
	{
		if (seenNonZero || a.GetElement(i) != 0)
		{
			uint32 sh1 = a.GetElement(i) >> 16;
			temp = (temp << 16) + sh1;
			sh1 = temp / b;
			temp %= b;

			uint32 sh2 = (a.GetElement(i) << 16) >> 16;
			temp = (temp << 16) + sh2;
			sh2 = temp / b;
			temp %= b;

			result.SetElement(i, (sh1 << 16) | sh2);
			seenNonZero = true;
		}
	}
	if (remainder)
		*remainder = int(temp);

	return result;
}


template<int N> inline tFixIntU<N>& tFixIntU<N>::operator*=(const tFixIntU& m)
{
	// This method of multiplication is exactly how you do long multiplication by hand except
	// that it's base 2 instead of base 10.
	tFixIntU v(m);
	tFixIntU t;
	t = *this;
	*this = tFixIntU(0u);
	do
	{
		if ((v.ElemData[LSIndex] & 1U) != 0u)
			*this += t;
		v >>= 1;
		t <<= 1;
	} while (!!v);

	return *this;
}


template<int N> inline tFixIntU<N>& tFixIntU<N>::operator*=(const uint32 m)
{
	uint32 v = m;
	tFixIntU t;
	t = *this;
	*this = (tFixIntU)0u;
	int shift = 0;
	while (v)
	{
		// Skip consecutive zeroes all at once.
		while ((v & 1U) == 0)
		{
			v >>= 1;
			++shift;
		}
		t <<= shift;
		v >>= 1;
		shift = 1;
		*this += t;
	}
	return *this;
}


template<int N> inline bool operator<(const tFixIntU<N>& a, const tFixIntU<N>& b)
{
	for (int i = a.GetNumElements()-1; i >= 0; i--)
	{
		if (a.GetElement(i) < b.GetElement(i))
			return true;
		if (a.GetElement(i) > b.GetElement(i))
			return false;
	}

	return false;
}


template<int N> inline tFixIntU<N>& tFixIntU<N>::operator>>=(int shift)
{
	int source = shift / 32;
	int remaindershift = shift & (32-1);
	int othershift = 32 - remaindershift;

	for (int i = 0; i < NumElements; i++)
	{
		if (source < NumElements)
		{
			ElemData[i] = ElemData[BaseIndex(source)] >> remaindershift;
			if (++source < NumElements && othershift < 32)
				ElemData[i] |= ElemData[BaseIndex(source)] << othershift;
		}
		else
		{
			ElemData[i] = 0u;
		}
	}
	return *this;
}


template<int N> inline tFixIntU<N>& tFixIntU<N>::operator<<=(int shift)
{
	int source = NumElements-1 - shift / 32;
	int remaindershift = shift & (32-1);
	int othershift = 32 - remaindershift;

	for (int i = NumElements-1; i >= 0; i--)
	{
		if (source >= 0)
		{
			ElemData[i] = ElemData[BaseIndex(source)] << remaindershift;
			if (--source >= 0 && othershift < 32)
				ElemData[i] |= ElemData[BaseIndex(source)] >> othershift;
		}
		else
		{
			ElemData[i] = 0u;
		}
	}
	return *this;
}


template<int N> inline tFixIntU<N>& tFixIntU<N>::operator+=(const tFixIntU& v)
{
	uint32 carry = 0u;
	for (int i = 0; i < NumElements; i++)
	{
		ElemData[i] += v.ElemData[i] + carry;
		if (!carry)
			carry = (ElemData[i] < v.ElemData[i]);
		else
			carry = (ElemData[i] <= v.ElemData[i]);
	}
	return *this;
}


template<int N> inline tFixIntU<N>& tFixIntU<N>::operator-=(const tFixIntU& v)
{
	uint32 borrow = 0u, prevdigit;

	for (int i = 0; i < NumElements; i++)
	{
		prevdigit = ElemData[i];
		ElemData[i] -= v.ElemData[i] + borrow;
		if (!borrow)
			borrow = (prevdigit < v.ElemData[i]);
		else
			borrow = (prevdigit <= v.ElemData[i]);
	}
	return *this;
}


template<int N> inline tFixIntU<N>& tFixIntU<N>::operator++()
{
	++ElemData[LSIndex];
	for (int i = 0; i < NumElements-1; i++)
	{
		if (!ElemData[i])
			++ElemData[i+1];
		else
			break;
	}
	return *this;
}


template<int N> inline tFixIntU<N>& tFixIntU<N>::operator--()
{
	--ElemData[LSIndex];
	for (int i = 0; i < NumElements-1; i++)
	{
		if (ElemData[i] == 0xFFFFFFFF)
			--ElemData[i+1];
		else
			break;
	}
	return *this;
}


template<int N> inline tFixIntU<N> tSqrt(const tFixIntU<N>& v)
{
	static const tFixIntU<N> mask = ~tFixIntU<N>(1);
	if (!v)
		return v;

	tFixIntU<N> x = v >> (v.FindHighestBitSet()>>1);
	tFixIntU<N> dx;
	do
	{
		// We are really performing the fuction: dx = (y/x - x) / 2 but since these are unsigned numbers we MUST
		// do the subtraction last in order for the x += dx evaluation to work properly.
		dx = (v>>1)/x - (x>>1);
		x += dx;
	} while (!!(dx & mask));
			
	// truncate answer
	if (x*x > v)
		--x;

	return x;
}


template<int N> inline tFixIntU<N> tCurt(const tFixIntU<N>& v)
{
	if (!v)
		return v;

	tFixIntU<N> x = v >> ((2*v.FindHighestBitSet())/3);
	tFixIntU<N> dx;
	do
	{
		dx = (v/(x*x) - x)/3;
		x += dx;
	} while (!!dx);

	if (x*x*x > v)
		--x;

	return x;
}


template<int N> inline tFixIntU<N> tFactorial(const tFixIntU<N>& v)
{
	uint32 f = v;

	// If the number to take the factorial of is bigger than can fit into an unsigned long then the there's no way in
	// hell the result can be stored in conventional PC memory anyway.
	tFixIntU<N> result = 1;
	while (f > 0u)
		result *= f--;

	return result;
}


template<int N> inline void tFixIntU<N>::RotateRight(int shift)
{
	tFixIntU result;
	int source = (shift / 32) % NumElements;
	int remaindershift = shift & (32-1);

	if (remaindershift != 0)
	{
		int othershift = 32 - remaindershift;

		for (int i = 0; i < NumElements; i++)
		{
			int source1 = source;
			if (++source == NumElements)
				source = 0;
			result.ElemData[i] = (ElemData[BaseIndex(source1)] >> remaindershift) | (ElemData[BaseIndex(source)] << othershift);
		}
	}
	else
	{
		for (int i = 0; i < NumElements; i++)
		{
			result.ElemData[i] = ElemData[BaseIndex(source)];
			if (++source == NumElements)
				source = 0;
		}
	}
	*this = result;
}


template<int N> inline int tFixIntU<N>::FindHighestBitSet() const
{
	int result = N-1;
	for (int i = tFixIntU<N>::NumElements-1; i >= 0; i--)
	{
		if (ElemData[i] != 0)
		{
			uint32 temp = ElemData[i];
			uint32 tempMax = 0xFFFFFFFF;
			int shift = 16;
			do
			{
				tempMax <<= shift;
				if ((temp & tempMax) == 0)
				{
					result -= shift;
					temp = temp << shift;
				}
			} while ((shift>>=1) > 0);
			break;
		}
		result -= 32;
	}
	return result;
}


template<int N> inline int tFixIntU<N>::FindLowestBitSet() const
{
	int result = 0;
	for (int i = 0; i < NumElements; i++)
	{
		if (ElemData[i] != 0)
		{
			uint32 temp = ElemData[i];
			uint32 tempMax = 0xFFFFFFFF;
			int shift = 16;
			do
			{
				tempMax >>= shift;
				if ((temp & tempMax) == 0)
				{
					result += shift;
					temp = temp >> shift;
				}
			} while ((shift>>=1) > 0);
			break;
		}
		result += 32;
	}
	return result;
}


template<int N> inline bool tIsPower2(const tFixIntU<N>& v)
{
	bool found = false;
	for (int i = 0; i < tFixIntU<N>::NumElements; i++)
	{
		if (v.ElemData[i] != 0)
		{
			if (found)
				return false;
			if ((v.ElemData[i] & (v.ElemData[i]-1)) != 0)
				return false;

			found = true;
		}
	}
	return true;
}


template<int N> inline tFixIntU<N> tNextPow2(const tFixIntU<N>& v)
{
	tFixIntU<N> result;
	result = v - (tFixIntU<N>)1U;
	int shift = 1;
	do
	{
		result |= result >> shift;
		shift <<= 1;
	} while (shift < N);
	++result;
	return result;
}


template<int N> inline uint32 tCeilLog2(const tFixIntU<N>& v)
{
	tFixIntU<N> temp, temp2;
	uint32 result = 0u;
	int shift = N >> 1;

	temp = v;
	do
	{
		temp2 = temp >> shift;
		if (!!temp2)
		{
			temp = temp2;
			result |= shift;
		}
	} while ((shift >>= 1) > 0);
	return result;
}


template<int N> inline tFixIntU<N> tPow(tFixIntU<N> a, int b)
{
	tFixIntU<N> result;
	if (b < 0)
		return (tFixIntU<N>)0;

	result = (tFixIntU<N>)1U;

	if (a == result)
		return result;
	while (b)
	{
		if (b & 1)
			result *= a;
		b >>= 1;
		a *= a;
	}
	return result;
}


template<int N> inline tFixIntU<N> tModPow(tFixIntU<N> base, tFixIntU<N> exp, const tFixIntU<N>& mod)
{
	tFixIntU<N> result;
	result = 1;
	while (exp > 0)
	{
		if (!!(exp & 1))
			result = (result * base) % mod;
		exp >>= 1;
		base = (base * base) % mod;
	}
	return result;
}


template<int N> inline void tFixInt<N>::Set(float f)
{
	*this = 0;
	if (f != f || (f == f && f-f != 0.f))		// NAN or +/-inf.
	{
		tFixIntU<N>::SetBit(N-1);
		return;
	}

	bool neg = false;
	if (f < 0.f)
	{
		f = -f;
		neg = true;
	}

	if (f < 1.0f)
		return;

	int e;
	float mant = tStd::tFrexp(f, &e);
	while (mant > 0.0f && e-- > 0)
	{
		mant *= 2.0f;
		if (mant >= 1.0f)
		{
			mant -= 1.0f;
			if (e < N)
				tFixIntU<N>::SetBit(e);
		}
	}

	if (neg)
		*this = -*this;
}


template<int N> inline void tFixInt<N>::Set(double d)
{
	*this = 0;
	if (d != d || (d == d && d-d != 0.0))		// nan or +/-inf
	{
		tFixIntU<N>::SetBit(N-1);
		return;
	}

	bool neg = false;
	if (d < 0.0)
	{
		d = -d;
		neg = true;
	}

	if (d < 1.0)
		return;

	int e;
	double mant = tStd::tFrexp(d, &e);
	while (mant > 0.0 && e-- > 0)
	{
		mant *= 2.0;
		if (mant >= 1.0)
		{
			mant -= 1.0;
			if (e < N)
				tFixIntU<N>::SetBit(e);
		}
	}

	if (neg)
		*this = -*this;
}


template<int B> template<int N, bool LhsGreater> template<typename T2> inline void tFixInt<B>::AssignHelper<N, LhsGreater>::operator()(tFixInt& lhs, const T2& rhs) const
{
	const uint32* const accessorHack = (uint32*)&rhs;
	int i = 0;
	
	int lhsNum = lhs.NumElements;
	int rhsNum = rhs.NumElements;
	for (; i < tMath::tMin(lhsNum, rhsNum); i++)
		lhs.ElemData[tFixInt<B>::BaseIndex(i)] = accessorHack[tFixInt<B>::BaseIndex(i)];

	if (rhs < tFixInt<N>(0u))		// Sign extend.
	{
		for (; i < tFixIntU<B>::NumElements; i++)
			lhs.ElemData[tFixIntU<B>::BaseIndex(i)] = 0xFFFFFFFF;
	}
	else
	{
		for (; i < tFixIntU<B>::NumElements; i++)
			lhs.ElemData[tFixIntU<B>::BaseIndex(i)] = 0u;
	}
}


template<int B> template<int N> template<typename T2> inline void tFixInt<B>::AssignHelper<N, false>::operator()(tFixInt& lhs, const T2& rhs) const
{
	const uint32 *accessorHack = (uint32*)&rhs;
	for (int i = 0; i < tFixIntU<B>::NumElements; i++)
		lhs.ElemData[tFixIntU<B>::BaseIndex(i)] = accessorHack[tFixIntU<B>::BaseIndex(i)];
}


template<int N> inline tFixInt<N>& tFixInt<N>::operator>>=(int shift)
{
	if (ElemData[MSIndex] >> (32-1) != 0)
	{
		int source = shift / 32;
		int remaindershift = shift & (32-1);
		int othershift = 32 - remaindershift;

		for (int i = 0; i < tFixIntU<N>::NumElements; i++)
		{
			if (source < tFixIntU<N>::NumElements)
			{
				ElemData[i] = this->ElemData[tFixIntU<N>::BaseIndex(source++)] >> remaindershift;
				if (othershift < 32)
					ElemData[i] |= ((source < tFixIntU<N>::NumElements) ? ElemData[tFixIntU<N>::BaseIndex(source)] : 0xFFFFFFFF) << othershift;
			}
			else
			{
				ElemData[i] = 0xFFFFFFFF;
			}
		}
	}
	else
	{
		AsUnsigned() >>= shift;
	}
	return *this;
}


template<int N> inline tFixInt<N>& tFixInt<N>::operator*=(const tFixInt& v)
{
	if (ElemData[MSIndex] >> (32-1) != 0u)
	{
		*this = -*this;
		if (v.ElemData[MSIndex] >> (32-1) != 0u)
		{
			return (AsUnsigned() *= (-v).AsUnsigned()).AsSigned();
		}
		else
		{
			AsUnsigned() *= v.AsUnsigned();
			return *this = -*this;
		}
	}
	else
	{
		if (v.ElemData[MSIndex] >> (32-1) != 0u)
		{
			AsUnsigned() *= (-v).AsUnsigned();
			return *this = -*this;
		}
		else
		{
			return (AsUnsigned() *= v.AsUnsigned()).AsSigned();
		}
	}
}


template<int N> inline bool operator<(const tFixInt<N>& a, const tFixInt<N>& b)
{
	uint32 lhsHighChunk = a.ElemData[tFixIntU<N>::MSIndex] ^ (uint32(1) << (32-1));
	uint32 rhsHighChunk = b.ElemData[tFixIntU<N>::MSIndex] ^ (uint32(1) << (32-1));

	if (lhsHighChunk < rhsHighChunk)
		return true;
	if (lhsHighChunk > rhsHighChunk)
		return false;

	for (int i = tFixIntU<N>::NumElements-2; i >= 0; i--)
	{
		if (a.ElemData[i] < b.ElemData[i])
			return true;
		if (a.ElemData[i] > b.ElemData[i])
			return false;
	}
	return false;
}


template<int N> inline tFixInt<N> tSqrt(const tFixInt<N>& v)
{
	if (v.ElemData[tFixIntU<N>::MSIndex] >> (32-1) != 0u)
		return tFixInt<N>(0u);							// If negative just return zero.

	return tSqrt(v.AsUnsigned()).AsSigned();
}


template<int N> inline tFixInt<N> tCurt(const tFixInt<N>& v)
{
	tFixInt<N> x, dx;

	if (!v)
	{
		return v;
	}
	else
	{
		x = v >> ((2*v.FindHighestBitSet())/3);

		do
		{
			dx = (v/(x*x) - x)/3;
			x += dx;
		} while (!!dx);

		if (v < 0)									// Truncate answer.
		{
			if (x*x*x < v)
				++x;
		}
		else
		{
			if (x*x*x > v)
				--x;
		}
		return x;
	}
}


template<int N> inline tFixInt<N> tAbs(const tFixInt<N>& v)
{
	return (v.ElemData[tFixIntU<N>::MSIndex] >> (32-1) != 0u) ? -v : v;
}


template<int N> inline tFixInt<N> tFactorial(const tFixInt<N>& v)
{
	if (v.ElemData[tFixIntU<N>::MSIndex] >> (32-1) != 0u)
		return tFixInt<N>(0u);

	return tFactorial(v.AsUnsigned()).AsSigned();
}


template<int N> inline tFixInt<N> tPow(const tFixInt<N>& a, int b)
{
	tFixInt<N> temp;
	temp = tPow(tAbs(a).AsUnsigned(), b).AsSigned();
	return ((a.ElemData[tFixIntU<N>::MSIndex] >> (32-1) != 0u) && ((b & 1) != 0)) ? -temp : temp;
}


template<int N> inline bool tIsPrime(const tFixInt<N>& v)
{
	if (v.ElemData[tFixIntU<N>::MSIndex] >> (32-1) != 0u)
		return false;
	return v.AsUnsigned().tIsPrime();
}


template<int N> inline tFixInt<N> tDivide(const tFixInt<N>& a, const tFixInt<N>& b, tFixInt<N>* remainder)
{
	tAssert(b != 0);
	if (a.ElemData[tFixInt<N>::MSIndex] >> (32-1) != 0u)
	{
		tFixInt<N> result;
		if (b.ElemData[tFixInt<N>::MSIndex] >> (32-1) != 0u)
			result = tDivide((-a).AsUnsigned(), (-b).AsUnsigned(), (tFixIntU<N>*)remainder).AsSigned();
		else
			result = -tDivide((-a).AsUnsigned(), b.AsUnsigned(), (tFixIntU<N>*)remainder).AsSigned();
		
		if (remainder)
			*remainder = -*remainder;
		
		return result;
	}
	else
	{
		if (b.ElemData[tFixInt<N>::MSIndex] >> (32-1) != 0u)
			return -tDivide(a.AsUnsigned(), (-b).AsUnsigned(), (tFixIntU<N>*)remainder).AsSigned();
		else
			return tDivide(a.AsUnsigned(), b.AsUnsigned(), (tFixIntU<N>*)remainder).AsSigned();
	}
}


template<int N> inline tFixInt<N> tDivide(const tFixInt<N>& a, int b, int* remainder)
{
	tAssert(b != 0);
	if (a.ElemData[tFixInt<N>::MSIndex] >> (32-1) != 0u)
	{
		tFixInt<N> result;
		if (b < 0)
			result = tDivide((-a).AsUnsigned(), -b, remainder).AsSigned();
		else
			result = -tDivide((-a).AsUnsigned(), b, remainder).AsSigned();
		
		if (remainder)
			*remainder = -*remainder;
		
		return result;
	}
	else
	{
		if (b < 0)
			return -tDivide(a.AsUnsigned(), -b, remainder).AsSigned();
		else
			return tDivide(a.AsUnsigned(), b, remainder).AsSigned();
	}
}
