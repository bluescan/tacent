// tConstants.h
//
// Physical and mathematical constants.
//
// Copyright (c) 2004-2006, 2015, 2017, 2019, 2020, 2023, 2025 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include "Foundation/tPlatform.h"
namespace tMath
{


const float fPi									= 3.1415'926'535'8979'3'23'84'62'64'338'32'79'502f;		// 180 degrees.
const float fTwoPi								= fPi * 2.0f;				// 360 degrees.
const float fPiOver2							= fPi / 2.0f;				// 90  degrees.
const float fPiOver3							= fPi / 3.0f;				// 60  degrees.
const float fPiOver4							= fPi / 4.0f;				// 45  degrees;
const float fPiOver6							= fPi / 6.0f;				// 30  degrees;
const float fDefaultGamma						= 2.2f;
const float fMin								= 1.175494351e-38f;
const float fMax								= 3.402823466e+38f;

// This is a practical epsilon that can be used in many circumstances. One one millionth.
const float fEpsilon							= 0.000001f;

// This is the smallest positive value that can be represented by a single precision floating point.
const float fEpsilonRep							= fMin;

// This is the smallest effective positive float such that 1.0f+EpsilonEff != 1.0f. It is considerably larger than fEpsilonRep.
const float fEpsilonEff							= 1.192092896e-07f;

const float fInfinity							= fMax;

// The infinities below are not the special IEEE bit patterns that represent infinity. They are the very largest and
// smallest values that may be represented without use of the special bit patterns.
const float fPosInfinity						= fMax;
const float fNegInfinity						= -fMax;

// Same as above but for doubles.
const double dPi								= 3.1415'926'535'8979'3'23'84'62'64'338'32'79'502;		// 180 degrees.
const double dTwoPi								= dPi * 2.0;				// 360 degrees.
const double dPiOver2							= dPi / 2.0;				// 90  degrees.
const double dPiOver3							= dPi / 3.0;				// 60  degrees.
const double dPiOver4							= dPi / 4.0;				// 45  degrees;
const double dPiOver6							= dPi / 6.0;				// 30  degrees;
const double dDefaultGamma						= 2.2;
const double dMin								= 2.2250738585072014e-308;
const double dMax								= 1.7976931348623158e+308;
const double dEpsilon							= 0.000001;
const double dEpsilonRep						= dMin;
const double dEpsilonEff						= 2.2204460492503131e-016;
const double dInfinity							= dMax;
const double dPosInfinity						= dMax;
const double dNegInfinity						= -dMax;

// C++11 should promote 2147483648 to long long int, but VS currently turns it into and unsigned int. That's why we're
// using the hex string literal representation for now.
const int32 i32Min								= 0x80000000;	// Decimal value -2147483648.
const int32 i32Max								= 2147483647;	// Hex value 0x7FFFFFFF.
const uint32 ui32Min							= 0;
const uint32 ui32Max							= 0xFFFFFFFF;
const uint32 ui16Min							= 0;
const uint32 ui16Max							= 0xFFFF;
const int iMin									= 1 << (sizeof(int)*8 - 1);
const int iMax									= uint(iMin) - 1;

}
