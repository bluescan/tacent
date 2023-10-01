// tFundamentals.h
//
// Core math functions needed by the rest or of the module as well as for external use. Functions include trigonometric
// functions, intervals, angle manipulation, power functions, and other analytic functions.
//
// Copyright (c) 2004, 2017, 2019, 2020, 2022, 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <math.h>
#include <functional>
#include "Foundation/tPlatform.h"
#include "Foundation/tConstants.h"
#include "Foundation/tList.h"
#include "Foundation/tString.h"
namespace tMath
{


enum class tAngleMode
{
	Radians,	// Circle divided into 2Pi angle units.
	Degrees,	// Circle divided into 360 angle units.
	Norm256,	// Circle divided into 256 angle units.
	NormOne,	// Circle divided into one angle unit.
	Default		= Radians,
};

// For functions below there may be variants starting with 'ti'. The 'i' means 'in-place' (ref var). Supports chaining.
inline int tAbs(int val)																								{ return (val < 0) ? -val : val; }
inline float tAbs(float val)																							{ return (val < 0.0f) ? -val : val; }
inline double tAbs(double val)																							{ return (val < 0.0) ? -val : val; }
inline int& tiAbs(int& v)																								{ v = (v < 0) ? -v : v; return v; }
inline float& tiAbs(float& v)																							{ v = (v < 0.0f) ? -v : v; return v; }
inline double& tiAbs(double& v)																							{ v = (v < 0.0) ? -v : v; return v; }

// A mathematical modulo. Does not just return remainder like the % operator. i.e. Handles negatives 'properly'.
// The & and fmod functions are also here but called more appropriately tRem (for remainder). 
inline int tMod(int n, int d)																							{ int m = n % d; if (m < 0) m = (d < 0) ? m - d : m + d; return m; }
inline float tMod(float n, float d)																						{ float m = fmod(n, d); if (m < 0.0f) m = (d < 0.0f) ? m - d : m + d; return m; }
inline int tRem(int n, int d)																							{ return n % d; }
inline float tRem(float n, float d)																						{ return fmodf(n, d); }

template<typename T> inline T tMin(const T& a, const T& b)																{ return a < b ? a : b; }
template<typename T> inline T tMax(const T& a, const T& b)																{ return a > b ? a : b; }
template<typename T> inline T tMin(const T& a, const T& b, const T& c)													{ T ab = a < b ? a : b; return ab < c ? ab : c; }
template<typename T> inline T tMax(const T& a, const T& b, const T& c)													{ T ab = a > b ? a : b; return ab > c ? ab : c; }
template<typename T> inline T tMin(const T& a, const T& b, const T& c, const T& d)										{ T ab = a < b ? a : b; T cd = c < d ? c : d; return ab < cd ? ab : cd; }
template<typename T> inline T tMax(const T& a, const T& b, const T& c, const T& d)										{ T ab = a > b ? a : b; T cd = c > d ? c : d; return ab > cd ? ab : cd; }

template<typename T> inline T tClamp(T val, T min, T max)																{ return (val < min) ? min : ((val > max) ? max : val); }
template<typename T> inline T tClampMin(T val, T min)																	{ return (val < min) ? min : val; }
template<typename T> inline T tClampMax(T val, T max)																	{ return (val > max) ? max : val; }
template<typename T> inline T tSaturate(T val)																			{ return (val < T(0)) ? T(0) : ((val > T(1)) ? T(1) : val); }
template<typename T> inline T& tiClamp(T& val, T min, T max)															{ val = (val < min) ? min : ((val > max) ? max : val); return val; }
template<typename T> inline T& tiClampMin(T& val, T min)																{ val = (val < min) ? min : val; return val; }
template<typename T> inline T& tiClampMax(T& val, T max)																{ val = (val > max) ? max : val; return val; }
template<typename T> inline T& tiSaturate(T& val)																		{ val = (val < T(0)) ? T(0) : ((val > T(1)) ? T(1) : val); return val; }

// Interval Notation. When you see [a,b] or (a,b) the square bracket means include the endpoint and the rounded brackets
// mean exclude. See http://www.coolmath.com/algebra/07-solving-inequalities/03-interval-notation-01. When a function
// takes a bias argument, a Low bias will cause the return value to include the lower extent of the interval and exclude
// the high extent. A High bias will exclude the low end and include the high end. As a notational convenience When a
// function takes a bias argument, we'll write the interval as [(a,b)] and the enum below shows how to interpret it:
enum class tBias
{
	Full,		Outer = Full,		// [a,b]
	Low,		Left = Low,			// [a,b)
	High,		Right = High,		// (a,b]
	Center,		Inner = Center		// (a,b)
};

// These are handy functions that return appropriate comparison operators when computing biased intervals.
// For tBiasLess we care about the RHS. If the RHS is inclusive we get <=. If the RHS is exclusive we get <.
// For tBiasGrtr we care about the LHS. If the LHS is inclusive we get >=. If the LHS is exclusive we get >.
std::function<bool(float,float)> tBiasLess(tBias);
std::function<bool(float,float)> tBiasGrtr(tBias);

template<typename T> inline bool tInIntervalII(const T val, const T min, const T max)	/* Returns val E [min, max]	*/	{ if ((val >= min) && (val <= max)) return true; return false; }	// Implements val E [min, max]
template<typename T> inline bool tInIntervalIE(const T val, const T min, const T max)	/* Returns val E [min, max)	*/	{ if ((val >= min) && (val < max)) return true; return false; }		// Implements val E [min, max)
template<typename T> inline bool tInIntervalEI(const T val, const T min, const T max)	/* Returns val E (min, max]	*/	{ if ((val > min) && (val <= max)) return true; return false; }		// Implements val E (min, max]
template<typename T> inline bool tInIntervalEE(const T val, const T min, const T max)	/* Returns val E (min, max)	*/	{ if ((val > min) && (val < max)) return true; return false; }		// Implements val E (min, max)
template<typename T> inline bool tInInterval(const T val, const T min, const T max)		/* Returns val E [min, max]	*/	{ return tInIntervalII(val, min, max); }
template<typename T> inline bool tInInterval(const T val, const T min, const T max, tBias bias)							{ switch (bias) { case tBias::Left:	return tInIntervalIE(val, min, max); case tBias::Right:	return tInIntervalEI(val, min, max); case tBias::Inner:	return tInIntervalEE(val, min, max); } return tInIntervalII(val, min, max); }
template<typename T> inline bool tInRange(const T val, const T min, const T max)		/* Returns val E [min, max]	*/	{ return tInInterval(val, min, max); }
template<typename T> inline bool tInRange(const T val, const T min, const T max, tBias bias)							{ return tInInterval(val, min, max, bias); }

template<typename T> inline T tSign(T val)																				{ return val < T(0) ? T(-1) : val > T(0) ? T(1) : T(0); }
template<typename T> inline T tBinarySign(T val)																		{ return val < T(0) ? T(-1) : T(1); }	// Same as Sign but does not return 0 ever.  Two return values only.
template<typename T> inline bool tIsZero(T a)																			{ return a == T(0); }
template<typename T> inline bool tApproxEqual(T a, T b, float e = Epsilon)												{ return (tAbs(a-b) < e); }
template<typename T> inline bool tEquals(T a, T b)																		{ return a == b; }
template<typename T> inline bool tNotEqual(T a, T b)																	{ return a != b; }

struct tDivt																											{ int Quotient; int Remainder; };
tDivt tDiv(int numerator, int denominator);
struct tDiv32t																											{ int32 Quotient; int32 Remainder; };
tDiv32t tDiv32(int32 numerator, int32 denominator);
struct tDivU32t																											{ uint32 Quotient; uint32 Remainder; };
tDivU32t tDivU32(uint32 numerator, uint32 denominator);
struct tDiv64t																											{ int64 Quotient; int64 Remainder; };
tDiv64t tDiv64(int64 numerator, int64 denominator);
struct tDivU64t																											{ uint64 Quotient; uint64 Remainder; };
tDivU64t tDivU64(uint64 numerator, uint64 denominator);

// Finds the Greatest Common Divisor of a and b. The biggest natural number that divides into both. Note that any number
// other than 0 will divide 0, so tGCD(0,12) == 12. Also note tGCD(0,0) returns the biggest number we can: MaxInt.
// Negatives are well defined. Result is always positive. tGCD(-12,8), tGCD(12,-8), and tGCD(-12,-8) == 4.
// Note that gcd(a,b,c) = gcd(gcd(a,b),c)
int tGCD(int a, int b);
inline int tGreatestCommonDivisor(int a, int b)																			{ return tGCD(a, b); }

// Finds the Least Common Multiple of a and b. Always returns a positive result (or 0). If either a or b (or both) are
// 0, the smallest multiple is also 0. Note that lcm(a,b,c) = lcm(lcm(a,b),c)
int tLCM(int a, int b);
inline int tLeastCommonMultiple(int a, int b)																			{ return tLCM(a, b); }

// Use this instead of casting to int. The only difference is it rounds instead of truncating and is way faster -- The
// FPU stays in rounding mode so pipeline not flushed.
inline int tFloatToInt(float val)																						{ return int(val + 0.5f); }

// tCeiling and tFloor both need to change the FPU from round mode to truncate. Could have performance hit.
inline float tCeiling(float v)																							{ return ceilf(v); }
inline float& tiCeiling(float& v)																						{ v = ceilf(v); return v; }
inline float tFloor(float v)																							{ return floorf(v); }
inline float& tiFloor(float& v)																							{ v = floorf(v); return v; }

// The 'nearest' round variants let you round to the nearest [value]. For example, tRound(5.17f, 0.2f) = 5.2f
inline float tRound(float v)																							{ return floorf(v + 0.5f); }
inline float& tiRound(float& v)																							{ v = floorf(v + 0.5f); return v; }
float tRound(float v, float nearest);
inline float& tiRound(float& v, float nearest)																			{ v = tRound(v, nearest); return v; }
inline uint8 tReverseBits(uint8 v)																						{ v = (v & 0xF0)>>4 | (v & 0x0F)<<4; v = (v & 0xCC)>>2 | (v & 0x33)<<2; v = (v & 0xAA)>>1 | (v & 0x55)<<1; return v; }
inline uint8& tiReverseBits(uint8& v)																					{ v = tReverseBits(v); return v; }
inline uint16 tReverseBits(uint16 v);
inline uint16& tiReverseBits(uint16& v)																					{ v = tReverseBits(v); return v; }
inline uint32 tReverseBits(uint32 v);
inline uint32& tiReverseBits(uint32& v)																					{ v = tReverseBits(v); return v; }

// Find index of first unset (0) bit starting from the LSB (right). For uint8 will return a value in [-1, 7]. For
// uint16 will return a value in [-1, 15]. For uint32 will return a value in [-1, 31]. The -1 is returned if no bits
// were clear. These functions use fancy bit manipulations to get the result -- they do not loop through inspecting
// individual bits.
int tFindFirstClearBit(uint8 v);
int tFindFirstClearBit(uint16 v);
int tFindFirstClearBit(uint32 v);

// Similar to above but for finding first set (1) bit.
inline int tFindFirstSetBit(uint8 v)																					{ return tFindFirstClearBit(uint8(~v)); }
inline int tFindFirstSetBit(uint16 v)																					{ return tFindFirstClearBit(uint16(~v)); }
inline int tFindFirstSetBit(uint32 v)																					{ return tFindFirstClearBit(uint32(~v)); }

// The following Abs function deserves a little explanation. Some linear algebra texts use the term absolute value and
// norm interchangeably. Others suggest that the absolute value of a matrix is the matrix with each component
// being the absolute value of the original. This is what the following template function returns -- not the L2 norm
// (scalar length).
template <typename T> inline T tAbs(T v)																				{ T result; for (int c = 0; c < T::GetNumComponents(); c++) result[c] = tAbs(v[c]); return result; }
inline float tFrac(float val)																							{ return tAbs(val - float(int(val))); }
inline float tSquare(float v)																							{ return v*v; }
inline float tCube(float v)																								{ return v*v*v; }
inline float tSqrt(float x)																								{ return sqrtf(x); }
float tSqrtFast(float x);
inline float tRecipSqrt(float x)																						{ return 1.0f/sqrtf(x); }
float tRecipSqrtFast(float x);

inline float tDegToRad(float deg)																						{ return deg * Pi / 180.0f; }
inline float tRadToDeg(float rad)																						{ return rad * 180.0f / Pi; }
inline float& tiDegToRad(float& ang)																					{ ang = ang * Pi / 180.0f; return ang; }
inline float& tiRadToDeg(float& ang)																					{ ang = ang * 180.0f / Pi; return ang; }

inline float tSin(float x)																								{ return sinf(x); }
inline float tSinFast(float x);								// For x E [0, Pi/2].
inline float tCos(float x)																								{ return cosf(x); }
inline float tCosFast(float x)								/* For x E [0, Pi/2]. */									{ float s = tSinFast(x); return tSqrtFast(1.0f - s*s); }
inline void tCosSin(float& cos, float& sin, float x);
inline void tCosSinFast(float& cos, float& sin, float x);	// For x E [0, Pi/2].
inline float tTan(float x)																								{ return tanf(x); }
inline float tArcSin(float x)																							{ return asinf(x); }
inline float tArcCos(float x)																							{ return acosf(x); }
inline float tArcTan(float y, float x)						/* Order is y, x. Returns angle of a slope (rise/run). */	{ return atan2f(y, x); }
inline float tArcTan(float m)																							{ return atanf(m); }
inline float tExp(float a)																								{ return expf(a); }
inline double tExp(double a)																							{ return exp(a); }
inline float tLog(float x)									/* Natural logarithm. */									{ return logf(x); }
inline float tSa(float x)									/* Unnormalized (sampling) sinc. */							{ if (x == 0.0f) return 1.0f; return tSin(x) / x; }
inline float tSinc(float x)									/* Normalized sinc. */										{ if (x == 0.0f) return 1.0f; float pix = Pi*x; return tSin(pix) / pix; }
inline float tPow(float a, float b)																						{ return powf(a, b); }
inline double tPow(double a, double b)																					{ return pow(a, b); }
inline int tPow2(int n)										/* 2 ^ n. */												{ return 1 << n; }

// Returns integral base 2 logarithm. If v is <= 0 returns MinInt32. If v is a power of 2 you will get an exact
// result. If v is not a power of two it will return the logarithm of the next lowest power of 2. For example,
// Log2(2) = 1, Log2(3) = 1, and Log2(4) = 2.
inline int tLog2(int v);

// This overload is for unsigned ints. It uses a slightly different (but also fast) algorithm to compute the base-2 log.
// If 0 is passed in, returns -1 as the answer.
inline int tLog2(uint32 v);

// For the 'ti' versions of the functions, the 'i' means 'in-place' (ref var).
inline bool tIsPower2(int v)																							{ if (v < 1) return false; return (v & (v-1)) ? false : true; }
inline uint& tiNextLowerPower2(uint& v)																					{ uint pow2 = 1; while (pow2 < v) pow2 <<= 1; pow2 >>= 1; v = pow2 ? pow2 : 1; return v; }
inline uint tNextLowerPower2(uint v)																					{ uint pow2 = 1; while (pow2 < v) pow2 <<= 1; pow2 >>= 1; return pow2 ? pow2 : 1; }
inline uint& tiNextHigherPower2(uint& v)																				{ uint pow2 = 1; while (pow2 <= v) pow2 <<= 1; v = pow2; return v; }
inline uint tNextHigherPower2(uint v)																					{ uint pow2 = 1; while (pow2 <= v) pow2 <<= 1; return pow2; }
inline uint& tiClosestPower2(uint& v)																					{ if (tIsPower2(v)) return v; int h = tNextHigherPower2(v); int l = tNextLowerPower2(v); v = ((h - v) < (v - l)) ? h : l; return v; }
inline uint tClosestPower2(uint v)																						{ if (tIsPower2(v)) return v; int h = tNextHigherPower2(v); int l = tNextLowerPower2(v); return ((h - v) < (v - l)) ? h : l; }
float& tiNormalizeAngle(float& angle, tBias = tBias::Low);				// Results in angle E [(-Pi,Pi)].
inline float tNormalizedAngle(float angle, tBias bias = tBias::Low)														{ tiNormalizeAngle(angle, bias); return angle; }
float& tiNormalizeAngle2Pi(float& angle, tBias = tBias::Low);			// Results in angle E [(0,2Pi)].
inline float tNormalizedAngle2Pi(float angle, tBias bias = tBias::Low)													{ tiNormalizeAngle2Pi(angle, bias); return angle; }

// Gets the range (y) value of a normal distribution with mean = 0, and given variance. Pass in the domain (x) value.
inline float tNormalDist(float variance, float x)																		{ return tPow(2*Pi*variance, -0.5f) * tExp(-tPow(x, 2.0f) / (2.0f*variance)); }

// These functions compute f(x) with x E [0, 1]. When not flipped f(0) = 0 and f(1) = 1. Furthermore, 0 <= f(x) <= 1.
// Curve shape may be controlled by one or more constant (c) arguments. These functions always exist in the unit square,
// even when flipped. Essentially flipping in x flips around the x=1/2 line, and y about the y=1/2 line.
enum tUnitFlip
{
	tUnitFlip_None																										= 0x00000000,
	tUnitFlip_X																											= 0x00000001,
	tUnitFlip_Y																											= 0x00000002,
	tUnitFlip_XY																										= tUnitFlip_X | tUnitFlip_Y
};

// Plot: http://www.wolframalpha.com/input/?i=Plot%5B%28Sin%28x*pi-pi%2F2%29%2B1%29%2F2%2C+%7Bx%2C0%2C1%7D%5D
float tUnitSin(float x, uint32 = tUnitFlip_None);

// Plot: http://www.wolframalpha.com/input/?i=Plot%5BSin%28x*pi%2F2%29%2C+%7Bx%2C0%2C1%7D%5D
float tUnitSinHalf(float x, uint32 = tUnitFlip_None);

// Plot: http://www.wolframalpha.com/input/?i=Plot%5Bpow%28x%2C+c%29%2C+%7Bx%2C0%2C1%7D%2C+%7Bc%2C0.1%2C3%7D%5D
// c E (0, inf). c < 1 pulls towards top left. c > 1 pulls towards bottom right.
float tUnitPow(float x, float c = 1.0f, uint32 = tUnitFlip_None);

// Plot: http://www.wolframalpha.com/input/?i=Plot+Piecewise%5B%7B%7Bpow%282x%2C+3%29%2F2%2C+x%3C0.5%7D%2C+%7B1+-+%28pow%281-2%28x-1%2F2%29%2C+3%29%29%2F2%2C+x%3E0.5%7D%7D%5D%2C+%7Bx%2C0%2C1%7D
float tUnitPowPlateau(float x, float c = 1.0f, uint32 = tUnitFlip_None);

// Plot: http://www.wolframalpha.com/input/?i=Plot+sqrt%281+-+%281-x%29%5E2%29%2C+%7Bx%2C0%2C1%7D
float tUnitArc(float x, uint32 = tUnitFlip_None);																		


}


// Implementation below this line.


inline std::function<bool(float,float)> tMath::tBiasLess(tBias bias)
{
	switch (bias)
	{
		case tBias::Right:
		case tBias::Outer:
			return [](float a, float b) { return a <= b; };

		case tBias::Left:
		case tBias::Inner:
		default:
			return [](float a, float b) { return a < b; };
	}
}


inline std::function<bool(float,float)> tMath::tBiasGrtr(tBias bias)
{
	switch (bias)
	{
		case tBias::Left:
		case tBias::Outer:
			return [](float a, float b) { return a >= b; };

		case tBias::Right:
		case tBias::Inner:
		default:
			return [](float a, float b) { return a > b; };
	}
}


inline tMath::tDivt tMath::tDiv(int numerator, int denominator)
{
	div_t d = div(numerator, denominator);
	tDivt r;
	r.Quotient = d.quot;
	r.Remainder = d.rem;
	return r;
}


inline tMath::tDiv32t tMath::tDiv32(int32 numerator, int32 denominator)
{
	div_t d = div(numerator, denominator);
	tDiv32t r;
	r.Quotient = d.quot;
	r.Remainder = d.rem;
	return r;
}


inline tMath::tDivU32t tMath::tDivU32(uint32 numerator, uint32 denominator)
{
	tDivU32t r;
	r.Quotient = numerator/denominator;
	r.Remainder = numerator - r.Quotient*denominator;
	return r;
}


inline tMath::tDiv64t tMath::tDiv64(int64 numerator, int64 denominator)
{
	lldiv_t d = div(numerator, denominator);
	tDiv64t r;
	r.Quotient = d.quot;
	r.Remainder = d.rem;
	return r;
}


inline tMath::tDivU64t tMath::tDivU64(uint64 numerator, uint64 denominator)
{
	tDivU64t r;
	r.Quotient = numerator/denominator;
	r.Remainder = numerator - r.Quotient*denominator;
	return r;
}


// This helper shouldn't be in the header -- but it's small and not worth losing inline by putting it in the cpp.
namespace tMath
{
	inline int tGCDRec(int a, int b)
	{
		// This is the Euclidean algorithm:
		// b<a : gdc(a,b) = gdc(a-b,b).
		// Eg. gdc(16,4) == gdc(12,4) == gdc(8,4) == gdc(4,4) (done).
		// Instead of looping through the subracts one by one, we can just use the remainder / mod operator.
		if (b == 0)
			return a;
		
		return tGCDRec(b, a % b);
	}
}


inline int tMath::tGCD(int a, int b)
{
	// Both zeroes is undefined. Any number divides zero, but since the other number is also zero, the
	// most reasonable it to return the biggest number we can.
	if ((a == 0) && (b == 0))
		return MaxInt;

	if (a < 0) a = -a;
	if (b < 0) b = -b;
	return tGCDRec(a, b);
}


inline int tMath::tLCM(int a, int b)
{
	// lcm(a,b) = |a| * (|b|/gcd(a,b))
	// Since tGCD returns MaxInt for tGCD(0,0), we don't need to worry about divide by zero.
	if (a < 0) a = -a;
	if (b < 0) b = -b;
	return a * ( b / tGCD(a,b) );
}


inline float tMath::tRound(float v, float nearest)
{
	if (tApproxEqual(nearest, 0.0f))
		return v;

	tiClamp(nearest, 0.000001f, 1000000.0f);
	float numNearests = v/nearest;
	float rnded = tRound(numNearests);
	return rnded * nearest;
}


inline uint16 tMath::tReverseBits(uint16 v)
{
	uint8 upper = tReverseBits( uint8((v & 0xFF00)>>8) );
	uint8 lower = tReverseBits( uint8(v & 0x00FF) );
	v = uint16(lower)<<8 | uint16(upper);
	return v;
}


inline uint32 tMath::tReverseBits(uint32 v)
{
	uint16 upper = tReverseBits( uint16((v & 0xFFFF0000)>>16) );
	uint16 lower = tReverseBits( uint16(v & 0x0000FFFF) );
	v = uint32(lower)<<16 | uint32(upper);
	return v;
}


inline int tMath::tFindFirstClearBit(uint8 v)
{
	// Find the first zero bit. The operation we do is log2((a xor (a+1)) +1)
	// This is guaranteed to be a power of two.
	uint8 freeBit = (v ^ (v+1)) + 1;

	// If 0 it means nothing was found.
	if (!freeBit)
		return -1;

	// Now get the log in base 2 of freeBit and wrap if position is 0.
	int c = tMath::tLog2(freeBit) ;
	if (c == 0)
		c = 8;

	// This is the first cleared index in the bit array (from the LSB).
	return c-1;
}


inline int tMath::tFindFirstClearBit(uint16 v)
{
	// See comments for the uint8 version. This one works the same.
	uint16 freeBit = (v ^ (v+1)) + 1;
	if (!freeBit)
		return -1;

	int c = tMath::tLog2(freeBit) ;
	if (c == 0)
		c = 16;

	return c-1;
}


inline int tMath::tFindFirstClearBit(uint32 v)
{
	// See comments for the uint8 version. This one works the same.
	uint32 freeBit = (v ^ (v+1)) + 1;
	if (!freeBit)
		return -1;

	int c = tMath::tLog2(freeBit) ;
	if (c == 0)
		c = 32;

	return c-1;
}


inline int tMath::tLog2(int x)
{
	if (x <= 0)
		return 0x80000000;

	// Trick to use floating point rep.
	float f = float(x);
	return ((( *(uint32*)((void*)&f) ) & 0x7f800000) >> 23) - 127;
}


inline int tMath::tLog2(uint32 x)
{
	if (x == 0)
		return -1;

	// See "http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog".
	const uint32 b[] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
	int c = (x & b[0]) != 0;

	c |= ((x & b[4]) != 0) << 4;
	c |= ((x & b[3]) != 0) << 3;
	c |= ((x & b[2]) != 0) << 2;
	c |= ((x & b[1]) != 0) << 1;
	return c;
}


inline float tMath::tSqrtFast(float x)
{
	#ifdef PLATFORM_WINDOWS
	__m128 in = _mm_set_ss(x);
	__m128 out = _mm_sqrt_ss(in);
	return *(float*)(&out);
	#else
	return tSqrt(x);
	#endif
}


inline float tMath::tRecipSqrtFast(float x)
{
	#ifdef PLATFORM_WINDOWS
	__m128 in = _mm_set_ss(x);
	__m128 out = _mm_rsqrt_ss(in);
	return *(float*)(&out);
	#else
	return (1.0f / tSqrt(x));
	#endif
}


inline float tMath::tSinFast(float x)
{
	float x2 = x*x;
	float r = 7.61e-03f;
	r *= x2;
	r -= 1.6605e-01f;
	r *= x2;
	r += 1.0f;
	r *= x;

	return r;
}


inline void tMath::tCosSin(float& c, float& s, float x)
{
	// We can make a faster version by using pythagoras: cos^2 + sin^2 = 1. However, I don't feel like dealing with the negative roots right
	// now, so it's implemented naively.
	c = tCos(x);
	s = tSin(x);
}


inline void tMath::tCosSinFast(float& c, float& s, float x)
{
	// The fast versions are domain limited so we can use pythagoras without worrying about the negative roots.
	s = tSinFast(x);
	c = tSqrtFast(1.0f - s*s);
}


inline float& tMath::tiNormalizeAngle(float& a, tBias bias)
{
	std::function<bool(float,float)> less = tBiasLess(bias);
	std::function<bool(float,float)> grtr = tBiasGrtr(bias);
	while (less(a, -Pi)) a += TwoPi;
	while (grtr(a,  Pi)) a -= TwoPi;
	return a;
}


inline float& tMath::tiNormalizeAngle2Pi(float& a, tBias bias)
{
	std::function<bool(float,float)> less = tBiasLess(bias);
	std::function<bool(float,float)> grtr = tBiasGrtr(bias);
	while (less(a, 0.0f))  a += TwoPi;
	while (grtr(a, TwoPi)) a -= TwoPi;
	return a;
}


inline float tMath::tUnitSin(float x, uint32 flip)
{
	tiClamp(x, 0.0f, 1.0f);
	x = (flip & tUnitFlip_X) ? 1.0f-x : x;
	float y = (tSin(x*Pi - PiOver2) + 1.0f)/2.0f;
	y = (flip & tUnitFlip_Y) ? 1.0f-y : y;
	return y;
}


inline float tMath::tUnitSinHalf(float x, uint32 flip)
{
	tiClamp(x, 0.0f, 1.0f);
	x = (flip & tUnitFlip_X) ? 1.0f-x : x;
	float y = tSin(x*PiOver2);
	y = (flip & tUnitFlip_Y) ? 1.0f-y : y;
	return y;
}


inline float tMath::tUnitPow(float x, float c, uint32 flip)
{
	tiClamp(x, 0.0f, 1.0f);
	x = (flip & tUnitFlip_X) ? 1.0f-x : x;
	float y = tPow(x, c);
	y = (flip & tUnitFlip_Y) ? 1.0f-y : y;
	return y;
}


inline float tMath::tUnitPowPlateau(float x, float c, uint32 flip)
{
	tiClamp(x, 0.0f, 1.0f);
	x = (flip & tUnitFlip_X) ? 1.0f-x : x;

	float y = 0.0f;
	if (x < 0.5f)
		y = 0.5f*tUnitPow(2.0f*x, c);
	else
		y = 0.5f + 0.5f*tUnitPow(2.0f*(x-0.5f), c, tUnitFlip_XY);

	y = (flip & tUnitFlip_Y) ? 1.0f-y : y;
	return y;
}


inline float tMath::tUnitArc(float x, uint32 flip)
{
	tiClamp(x, 0.0f, 1.0f);
	x = (flip & tUnitFlip_X) ? 1.0f-x : x;
	float y = tSqrt(1.0f - (1.0f-x)*(1.0f-x));
	y = (flip & tUnitFlip_Y) ? 1.0f-y : y;
	return y;
}
