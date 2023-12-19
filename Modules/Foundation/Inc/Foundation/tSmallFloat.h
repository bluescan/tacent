// tSmallFloat.h
//
// Small float (less than 32-bit) representations. Includes a 16-bit half-sized float (tHalf) and a couple of packed
// formats using 10, 11, and 14-bit floats. Arithmetic is not directly supported by these classes, they are used for
// simply converting raw bit batterns to and from 32-BIT IEEE-754 floats (which will have hardware support anyways).
//
// As a reminder, IEEE-754 floats have 1 sign bit, 8 exponent bits, and 23 mantissa bits, They are usually encoded with
// an implicit +1 in the mantissa, but support 'denormals' for small values (sub-normal) where there is no implied +1.
// All denormals are sub-normal in IEEE-754.
//
// For the 16-bit half-precision floats the conversion functions are based on code from Fabian "ryg" Giesen in the
// public domain: "I hereby place this code in the public domain, as per the terms of the CC0
// license: https://creativecommons.org/publicdomain/zero/1.0/"
// float -> half : https://gist.github.com/rygorous/2156668
// half -> float : https://gist.github.com/rygorous/2144712
// All modifications from the original should be considerd ISC as seen below.
//
// For the 32-bit packed M9M9M9E5 type the implementation is based on
// https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_shared_exponent.txt
// There is no licence information provided, but the comment reads: "This source code provides ANSI C routines. It
// assumes the C 'float' data type is stored with the IEEE 754 32-bit floating-point format." Authors appear to be one
// or more of the following: Contact: Mark J. Kilgard, NVIDIA Corporation (mjk 'at' nvidia.com). Contributors: Pat Brown
// NVIDIA, Jon Leech, and Bruce Merry ARM. The extensions appear to be from an area of the site for implementers with
// this note: "No fee or agreement is needed to implement a Khronos API or use it in a product but you may not claim
// compliance and may not use its trademark and logo."
//
// In the following code you may see left shifts of negative signed integers. This is commonly thought to be undefined
// in C++, but it shouldn't be... and as of c++20 the standard does not even draw a distinction between negative and
// positive signed int left-shifts, which is good. I am considering it well-defined to left-shift a negative:
// Since C++20:
// The value of a << b is the unique value congruent to a * 2b modulo 2N where N is the number of bits in the return
// type (that is, bitwise left shift is performed and the bits that get shifted out of the destination type are
// discarded). The value of a >> b is a/2b, rounded towards negative infinity (in other words, right shift on signed a
// is arithmetic right shift).
//
// Copyright (c) 2022, 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include "Foundation/tFundamentals.h"


#pragma pack(push, 2)


union tFP32U
{
	tFP32U() : Raw(0) { }
	explicit tFP32U(float f) : Flt(f) { }
	explicit tFP32U(uint32 r) : Raw(r) { }
	uint32 Raw;
	float Flt;
};
tStaticAssert(sizeof(tFP32U) == 4);


// Half precision floats are 16 bit. They support the sign bit and denormals. They have 1 sign bit, 5 exponent bits,
// and 10 mantissa bits. FloatToHalfRaw is based on float_to_half_fast3 or float_to_half_fast3_RTNE depending on
// if you have TACENT_HALF_FLOAT_RTNE defined. The former rounds to +inf, the latter rounds-to-nearest-even (on-equal).
// IEEE-754 Bits: SEEEEEEEEMMMMMMMMMMMMMMMMMMMMMMM
// Half Bits    : SEEEEEMMMMMMMMMM
inline uint16 FloatToHalfRaw(float);
inline float  HalfRawToFloat(uint16);


// tHalf is a convenience type that will call FloatToHalfRaw and HalfRawToFloat for you. tHalf must remain pod. No
// virtual function table etc. C++ defaults are used for op=, copy cons, etc. That is, memory copy works fine.
struct tHalf
{
	tHalf(float flt)				/* The float constructor allows a tHalf to be created from a float. */				{ Set(flt); }
	tHalf(uint16 raw)				/* The raw constructor allows a tHalf to be created from raw bits. */				{ Set(raw); }
	tHalf(uint8 raw[2])				/* The raw array should be supplied in LE order. */									{ Set(raw); }
	void Set(float flt)																									{ Raw = FloatToHalfRaw(flt); }
	void Set(uint16 raw)																								{ Raw = raw; }
	void Set(uint8 raw[2])																								{ Raw = (raw[0] << 8) | raw[1]; }

	// These allow the tHalf type to be automatically converted to a float.
	operator float()																									{ return Float(); }
	operator float() const																								{ return Float(); }

	float Float() const																									{ return HalfRawToFloat(Raw); }
	uint16 Raw;
};
tStaticAssert(sizeof(tHalf) == 2);


// This is a packed format that stores 3 unsigned floats in 32 bits. The first two floats (MSBs) are 11 bits each and
// the third (LSBs) is 10 bits. They all have no sign bit and the exponent bitdepth is 5 for all three. Denorm numbers
// are supported. This class is primarily for decoding and encoding the packed format into 3 regular 32-bit floats.
// tPackedF11F11F10 must remain pod. No virtual function table etc. C++ defaults are used for op=, copy cons, etc. That
// is, memory copy works fine.
struct tPackedF11F11F10
{
	tPackedF11F11F10(float flt)					/* All 3 values get set to flt. */										{ Set(flt); }
	tPackedF11F11F10(float flt[3])				/* Sets the 3 floats from an array. */									{ Set(flt); }
	tPackedF11F11F10(float x, float y, float z)	/* Sets all 3 floats. */												{ Set(x, y, z); }
	tPackedF11F11F10(uint32 raw)				/* The raw constructor creates the pack from raw bits. */				{ Set(raw); }
	tPackedF11F11F10(uint8 raw[4])				/* The raw array should be supplied in LE order. */						{ Set(raw); }

	inline void Set(float flt)																							{ Set(flt, flt, flt); }
	inline void Set(float flt[3])																						{ Set(flt[0], flt[1], flt[2]); }
	inline void Set(float x, float y, float z);
	inline void Set(uint32 raw)																							{ Raw = raw; }
    inline void Set(uint8 raw[4])																						{ Raw = (raw[0]<<24) | (raw[1]<<16) | (raw[2]<<8) | raw[3]; }

	inline void Get(float flt[3]) const																					{ Get(flt[0], flt[1], flt[2]); }
	inline void Get(float& x, float& y, float& z) const;

	uint32 Raw;	// 6M5E 6M5E 5M5E
};
tStaticAssert(sizeof(tPackedF11F11F10) == 4);


// This is a packed is exactly the same as the above except the the first (MSBs) float is the 10-bit one and the two
// 11-bit floats go in the LSBs. All other comments apply.
struct tPackedF10F11F11
{
	tPackedF10F11F11(float flt)					/* All 3 values get set to flt. */										{ Set(flt); }
	tPackedF10F11F11(float flt[3])				/* Sets the 3 floats from an array. */									{ Set(flt); }
	tPackedF10F11F11(float x, float y, float z)	/* Sets all 3 floats. */												{ Set(x, y, z); }
	tPackedF10F11F11(uint32 raw)				/* The raw constructor creates the pack from raw bits. */				{ Set(raw); }
	tPackedF10F11F11(uint8 raw[4])				/* The raw array should be supplied in LE order. */						{ Set(raw); }

	inline void Set(float flt)																							{ Set(flt, flt, flt); }
	inline void Set(float flt[3])																						{ Set(flt[0], flt[1], flt[2]); }
	inline void Set(float x, float y, float z);
	inline void Set(uint32 raw)																							{ Raw = raw; }
    inline void Set(uint8 raw[4])																						{ Raw = (raw[0]<<24) | (raw[1]<<16) | (raw[2]<<8) | raw[3]; }

	inline void Get(float flt[3]) const																					{ Get(flt[0], flt[1], flt[2]); }
	inline void Get(float& x, float& y, float& z) const;

	uint32 Raw;	// 5M5E 6M5E 6M5E
};
tStaticAssert(sizeof(tPackedF10F11F11) == 4);


// This is a packed format that stores three 14-bit unsigned floats that each share a 5-bit exponent in 32 bits. The
// mantissa for each float is 9 bits. This format is special in that a) there is no sign bit, and b) the values are
// never encoded normalized -- the mantissas are always denorm so they can share the same exponent. This class is
// primarily for decoding the packed format into 3 regular 32-bit floats. tPackedM9M9M9E5 must remain pod. No virtual
// function table etc.
struct tPackedM9M9M9E5
{
	tPackedM9M9M9E5(float flt)					/* All 3 values get set to flt. */										{ Set(flt); }
	tPackedM9M9M9E5(float flt[3])				/* Sets the 3 floats from an array. */									{ Set(flt); }
	tPackedM9M9M9E5(float x, float y, float z)	/* Sets all 3 floats. */												{ Set(x, y, z); }
	tPackedM9M9M9E5(uint32 raw)					/* The raw constructor creates the pack from raw bits. */				{ Set(raw); }
	tPackedM9M9M9E5(uint8 raw[4])				/* The raw array should be supplied in LE order. */						{ Set(raw); }

	inline void Set(float flt)																							{ Set(flt, flt, flt); }
	inline void Set(float flt[3])																						{ Set(flt[0], flt[1], flt[2]); }
	inline void Set(float x, float y, float z);
	inline void Set(uint32 raw)																							{ Raw = raw; }
    inline void Set(uint8 raw[4])																						{ Raw = (raw[0]<<24) | (raw[1]<<16) | (raw[2]<<8) | raw[3]; }

	inline void Get(float flt[3]) const																					{ Get(flt[0], flt[1], flt[2]); }
	inline void Get(float& x, float& y, float& z) const;

	uint32 Raw;		// 9M 9M 9M 5E
};
tStaticAssert(sizeof(tPackedM9M9M9E5) == 4);


// This is a packed is exactly the same as the above except the the first 5 MS bits represent the exponent rather than
// it going at the end. All other comments apply.
struct tPackedE5M9M9M9
{
	tPackedE5M9M9M9(float flt)					/* All 3 values get set to flt. */										{ Set(flt); }
	tPackedE5M9M9M9(float flt[3])				/* Sets the 3 floats from an array. */									{ Set(flt); }
	tPackedE5M9M9M9(float x, float y, float z)	/* Sets all 3 floats. */												{ Set(x, y, z); }
	tPackedE5M9M9M9(uint32 raw)					/* The raw constructor creates the pack from raw bits. */				{ Set(raw); }
	tPackedE5M9M9M9(uint8 raw[4])				/* The raw array should be supplied in LE order. */						{ Set(raw); }

	inline void Set(float flt)																							{ Set(flt, flt, flt); }
	inline void Set(float flt[3])																						{ Set(flt[0], flt[1], flt[2]); }
	inline void Set(float x, float y, float z);
	inline void Set(uint32 raw)																							{ Raw = raw; }
    inline void Set(uint8 raw[4])																						{ Raw = (raw[0]<<24) | (raw[1]<<16) | (raw[2]<<8) | raw[3]; }

	inline void Get(float flt[3]) const																					{ Get(flt[0], flt[1], flt[2]); }
	inline void Get(float& x, float& y, float& z) const;

	uint32 Raw;		// 5E 9M 9M 9M
};
tStaticAssert(sizeof(tPackedE5M9M9M9) == 4);


#pragma pack(pop)


// Implementation below this line.


#define TACENT_HALF_FLOAT_RTNE


inline uint16 FloatToHalfRaw(float f)
{
#ifdef TACENT_HALF_FLOAT_RTNE
	tFP32U f32(f);
	uint32 f32inf		= 0xFF << 23;				// Pos infinity. All 8 exponent bits set.
	uint32 f16max		= (0x007F + 0x0010) << 23;
	tFP32U denormMagic	(((0x007Fu - 0x000Fu) + (23u - 10u) + 1u) << 23);
	uint32 sgnmsk		= 0x80000000;
	uint16 out			= 0x0000;
	uint32 sign			= f32.Raw & sgnmsk;
	f32.Raw				^= sign;

	// All the integer compares in this function can be safely compiled into signed compares since all operands are
	// below 0x80000000.
	if (f32.Raw >= f16max)
	{
		out = (f32.Raw > f32inf) ? 0x7e00 : 0x7c00;	// Result is Inf or NaN (all exponent bits set). NaN->qNaN and Inf->Inf.
	}
	else
	{
		if (f32.Raw < (0x00000071 << 23))			// (De)normalized number or zero.
		{
			// Resulting FP16 is subnormal or zero. Use a magic value to align our 10 mantissa bits at the bottom of
			// the float. As long as FP addition is round-to-nearest-even this just works.
			f32.Flt += denormMagic.Flt;
			out = f32.Raw - denormMagic.Raw;		// One integer subtract of the bias later, we have our final float!
		}
		else
		{
			uint32 mantOdd = (f32.Raw >> (23u - 10u)) & 0x00000001;	// Is resulting mantissa is odd?
			f32.Raw += ((0x0F - 0x7F) << 23) + 0x0FFF;				// Update exponent, rounding bias part 1.
			f32.Raw += mantOdd;										// Rounding bias part 2.
			out = f32.Raw >> 13;									// Take the bits!
		}
	}

	out |= sign >> 16;
	return out;
#else
	// Based off of float_to_half_fast3. Rounds to +inf.
	tFP32U f32(f);
	uint32 f32inf		= 0xFFu << 23;		// Pos infinity. All 8 exponent bits set.
	uint32 f16inf		= 0x1Fu << 23;		// Pos infinity. All 5 exponent bits set.
	tFP32U f32magic		( 0x0Fu << 23 );
	uint32 f32sgnmsk	= 0x80000000u;
	uint32 rndmask		= ~0x00000FFFu;
	uint16 out			= 0x0000u;

	uint32 sign = f32.Raw & f32sgnmsk;
	f32.Raw ^= sign;

	if (f32.Raw >= f32inf)							// Inf or NaN (all exponent bits set)
	{
		out = (f32.Raw > f32inf) ? 0x7e00u : 0x7c00u;// NaN->qNaN and Inf->Inf
	}
	else											// (De)normalized number or zero.
	{
		f32.Raw &= rndmask;
		f32.Flt *= f32magic.Flt;
		f32.Raw -= rndmask;

		if (f32.Raw > f16inf)						// Clamp to signed infinity if overflew.
			f32.Raw = f16inf;

		out = f32.Raw >> 13;						// Take the bits!
	}

    out |= sign >> 16;
	return out;
#endif
}


inline float HalfRawToFloat(uint16 raw)
{
	// Based off of half_to_float_fast4.
	const tFP32U magic				( 0x71u << 23 );
	const uint32 shiftedExp			= 0x7c00 << 13;				// Exponent mask after shift.
	tFP32U out						( (raw & 0x7FFFu) << 13 );	// Exponent/mantissa bits.
	uint32 exp						= shiftedExp & out.Raw;		// Just the exponent.
	out.Raw							+= (0x7F - 15) << 23;		// Exponent adjust.

	if (exp == shiftedExp)					// Handle exponent special cases. First check for Inf/NaN.
	{
		out.Raw += (0x80 - 0x10) << 23;		// Extra exp adjust.
	}
	else if (exp == 0)						// Zero/Denormal.
	{
		out.Raw += 1 << 23;				 	// Extra exp adjust.
		out.Flt -= magic.Flt;				// Renormalize.
	}

	out.Raw |= (raw & 0x8000) << 16;		// Sign bit.
	return out.Flt;
}


inline void tPackedF11F11F10::Set(float x, float y, float z)
{
	// SEEEEEMMMMMMMMMM
	uint16 xhalf = FloatToHalfRaw(x);
	uint16 yhalf = FloatToHalfRaw(y);
	uint16 zhalf = FloatToHalfRaw(z);

	// The masks and shifts below remove the sign bit. No sign for F11F11F10. Negatives become positive.
	// The mantissa for each float gets truncated.
	// EEEE EMMM MMMM MMM0 0000 0000 0000 0000 & 1111 1111 1110 0000 0000 0000 0000 0000
	// 0000 0000 00SE EEEE MMMM MMMM MM00 0000 & 0000 0000 0001 1111 1111 1100 0000 0000
	// 0000 0000 0000 0000 0000 0SEE EEEM MMMM & 0000 0000 0000 0000 0000 0011 1111 1111
	uint32 rx = (xhalf << 17) & 0xFFE00000;
	uint32 ry = (yhalf << 6 ) & 0x001FFC00;
	uint32 rz = (zhalf >> 5 ) & 0x000003FF;
	Raw = rx | ry | rz;
}


inline void tPackedF11F11F10::Get(float& x, float& y, float& z) const
{
	// Raw: EEEEEMMMMMMEEEEEMMMMMMEEEEEMMMMM
	//      XXXXXXXXXXXYYYYYYYYYYYZZZZZZZZZZ
	uint16 xhalf = (Raw >> 17) & 0x7FF0;		// 0EEEEEMMMMMMEEEE & 0111111111110000
	uint16 yhalf = (Raw >> 6 ) & 0x7FF0;		// MEEEEEMMMMMMEEEE & 0111111111110000
	uint16 zhalf = (Raw << 5 ) & 0x7FE0;		// MEEEEEMMMMM00000 & 0111111111100000
	x = HalfRawToFloat(xhalf);
	y = HalfRawToFloat(yhalf);
	z = HalfRawToFloat(zhalf);
}


inline void tPackedF10F11F11::Set(float x, float y, float z)
{
	// SEEEEEMMMMMMMMMM
	uint16 xhalf = FloatToHalfRaw(x);
	uint16 yhalf = FloatToHalfRaw(y);
	uint16 zhalf = FloatToHalfRaw(z);

	// The masks and shifts below remove the sign bit. No sign for F10F11F11. Negatives become positive.
	// The mantissa for each float gets truncated.
	// EEEE EMMM MMMM MMM0 0000 0000 0000 0000 & 1111 1111 1100 0000 0000 0000 0000 0000
	// 0000 0000 0SEE EEEM MMMM MMMM M000 0000 & 0000 0000 0011 1111 1111 1000 0000 0000
	// 0000 0000 0000 0000 0000 SEEE EEMM MMMM & 0000 0000 0000 0000 0000 0111 1111 1111
	uint32 rx = (xhalf << 17) & 0xFFC00000;
	uint32 ry = (yhalf << 7 ) & 0x003FF800;
	uint32 rz = (zhalf >> 4 ) & 0x000007FF;
	Raw = rx | ry | rz;
}


inline void tPackedF10F11F11::Get(float& x, float& y, float& z) const
{
	// Raw: EEEEEMMMMMEEEEEMMMMMMEEEEEMMMMMM
	//      XXXXXXXXXXYYYYYYYYYYYZZZZZZZZZZZ
	uint16 xhalf = (Raw >> 17) & 0x7FE0;		// 0EEEEEMMMMMEEEEE & 0111111111100000
	uint16 yhalf = (Raw >> 7 ) & 0x7FF0;		// MEEEEEMMMMMMEEEE & 0111111111110000
	uint16 zhalf = (Raw << 4 ) & 0x7FF0;		// MEEEEEMMMMMM0000 & 0111111111110000
	x = HalfRawToFloat(xhalf);
	y = HalfRawToFloat(yhalf);
	z = HalfRawToFloat(zhalf);
}


// Shared-exponent common constants and functions.
namespace M999E5
{
	const int EXP_BIAS				= 15;
	const int MAX_VALID_BIASED_EXP	= 31;
	const int MAX_EXP				= MAX_VALID_BIASED_EXP - EXP_BIAS;
	const int MANTISSA_VALUES		= 1 << 9;
	const int MAX_MANTISSA			= MANTISSA_VALUES-1;
	const float MAX					= float(MAX_MANTISSA)/float(MANTISSA_VALUES) * float(1<<MAX_EXP);
	const float EPSILON				= (1.0f/MANTISSA_VALUES) / (1<<EXP_BIAS);

	// Original note: "Ok, FloorLog2 is not correct for the denorm and zero values, but we are going to do a max of
	// this value with the minimum rgb9e5 exponent that will hide these problem cases."
	inline int FloorLog2(float f)
	{
		// SEEEEEEE EMMMMMMM MMMMMMMM MMMMMMMM
		tFP32U f32(f);
		int exp = (f32.Raw >> 23) & 0x000000FF;
		return (exp - 127);
	}
}


inline void tPackedM9M9M9E5::Set(float x, float y, float z)
{
	float xc = tMath::tClamp(x, 0.0f, M999E5::MAX);
	float yc = tMath::tClamp(y, 0.0f, M999E5::MAX);
	float zc = tMath::tClamp(z, 0.0f, M999E5::MAX);
	float maxxyz = tMath::tMax(xc, yc, zc);
	int expShared = tMath::tMax(-M999E5::EXP_BIAS-1, M999E5::FloorLog2(maxxyz)) + 1 + M999E5::EXP_BIAS;
	tAssert((expShared <= M999E5::MAX_VALID_BIASED_EXP) && (expShared >= 0));

	// This pow function could be replaced by a table.
	double denom = tMath::tPow(2.0, expShared - M999E5::EXP_BIAS - 9);
	int maxm = int(tMath::tFloor(maxxyz / denom + 0.5));
	tAssert(maxm <= M999E5::MAX_MANTISSA+1);
	if (maxm == M999E5::MAX_MANTISSA+1)
	{
		denom *= 2.0;
		expShared += 1;
	    tAssert(expShared <= M999E5::MAX_VALID_BIASED_EXP);
	}

	int xm = int(tMath::tFloor(xc/denom + 0.5));
	int ym = int(tMath::tFloor(yc/denom + 0.5));
	int zm = int(tMath::tFloor(zc/denom + 0.5));
	tAssert((xm <= M999E5::MAX_MANTISSA) && (xm >= 0));
	tAssert((ym <= M999E5::MAX_MANTISSA) && (ym >= 0));
	tAssert((zm <= M999E5::MAX_MANTISSA) && (zm >= 0));

	// XXXXXXXX XYYYYYYY YYZZZZZZ ZZZEEEEE
	Raw = (xm << 23) | (ym << 14) | (zm << 5) | (expShared & 0x0000001F);
}


inline void tPackedM9M9M9E5::Get(float& x, float& y, float& z) const
{
	// XXXXXXXX XYYYYYYY YYZZZZZZ ZZZEEEEE
	int exponent = (Raw & 0x0000001F) - M999E5::EXP_BIAS - 9;
	float scale = float(tMath::tPow(2.0, exponent));

	x = float((Raw >> 23) & 0x000001FF) * scale;
	y = float((Raw >> 14) & 0x000001FF) * scale;
	z = float((Raw >> 5)  & 0x000001FF) * scale;
}


inline void tPackedE5M9M9M9::Set(float x, float y, float z)
{
	float xc = tMath::tClamp(x, 0.0f, M999E5::MAX);
	float yc = tMath::tClamp(y, 0.0f, M999E5::MAX);
	float zc = tMath::tClamp(z, 0.0f, M999E5::MAX);
	float maxxyz = tMath::tMax(xc, yc, zc);
	int expShared = tMath::tMax(-M999E5::EXP_BIAS-1, M999E5::FloorLog2(maxxyz)) + 1 + M999E5::EXP_BIAS;
	tAssert((expShared <= M999E5::MAX_VALID_BIASED_EXP) && (expShared >= 0));

	// This pow function could be replaced by a table.
	double denom = tMath::tPow(2.0, expShared - M999E5::EXP_BIAS - 9);
	int maxm = int(tMath::tFloor(maxxyz / denom + 0.5));
	tAssert(maxm <= M999E5::MAX_MANTISSA+1);
	if (maxm == M999E5::MAX_MANTISSA+1)
	{
		denom *= 2.0;
		expShared += 1;
	    tAssert(expShared <= M999E5::MAX_VALID_BIASED_EXP);
	}

	int xm = int(tMath::tFloor(xc/denom + 0.5));
	int ym = int(tMath::tFloor(yc/denom + 0.5));
	int zm = int(tMath::tFloor(zc/denom + 0.5));
	tAssert((xm <= M999E5::MAX_MANTISSA) && (xm >= 0));
	tAssert((ym <= M999E5::MAX_MANTISSA) && (ym >= 0));
	tAssert((zm <= M999E5::MAX_MANTISSA) && (zm >= 0));

	// EEEEEXXX XXXXXXYY YYYYYYYZ ZZZZZZZZ
	Raw = ((expShared & 0x0000001F) << 27) | (xm << 18) | (ym << 9) | zm;
}


inline void tPackedE5M9M9M9::Get(float& x, float& y, float& z) const
{
	// EEEEEXXX XXXXXXYY YYYYYYYZ ZZZZZZZZ
	int exponent = ((Raw & 0xF8000000)>>27) - M999E5::EXP_BIAS - 9;
	float scale = float(tMath::tPow(2.0, exponent));

	x = float((Raw >> 18) & 0x000001FF) * scale;
	y = float((Raw >> 9)  & 0x000001FF) * scale;
	z = float((Raw >> 0)  & 0x000001FF) * scale;
}
