// tHalf.h
//
// This is a half-precision (16-bit) floating point class. It is 'light' on purpose. The point of this class is just to
// convert between float <-> half. Arithmetic should be performed on regular floats which can leverage the FPU circuitry
// on any modern CPU.
//
// The conversion functions are based on code from Fabian "ryg" Giesen in the public domain:
// I hereby place this code in the public domain, as per the terms of the
// CC0 license: https://creativecommons.org/publicdomain/zero/1.0/
// float -> half : https://gist.github.com/rygorous/2156668
// half -> float : https://gist.github.com/rygorous/2144712
//
// Modifications are ISC:
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
#include "Foundation/tPlatform.h"


#pragma pack(push, 2)
struct tHalf
{
	// C++ defaults are used for op=, copy cons, etc. That is, memory copy works fine.
	// The float constructor allows a tHalf to be created from a float.
	tHalf(float f)																										{ FP32 f32; f32.f = f; Half = FloatToHalf(f32); }

	// The raw constructor allows a tHalf to be created from raw bits.
	tHalf(uint16 raw)																									{ Half.u = raw; }

	// The uint8 array constructor reads two bytes to create the tHalf. Useful if loading from a binary stream.
	tHalf(uint8 raw[2])																									{ Half.u = (raw[0] << 8) | raw[1]; }

	// These allow the tHalf type to be automatically converted to a float.
	operator float()																									{ return Float(); }
	operator float() const																								{ return Float(); }

	float Float() const																									{ FP32 f32 = HalfToFloat(Half); return f32.f; }
	uint16 Raw() const																									{ return Half.u; }

private:
	union FP32
	{
		uint32 u; float f;
		struct { uint32 Mantissa:23; uint32 Exponent:8; uint32 Sign:1; };
	};

	union FP16
	{
		uint16 u;
		struct { uint16 Mantissa:10; uint16 Exponent:5; uint16 Sign:1; };
	};

	// FloatToHalf is based on float_to_half_fast3_RTNE from 'rygorous'. RTNE means round-to-nearest-even (on-equal).
	static FP16 FloatToHalf(FP32);

	// HalfToFloat is based on half_to_float_fast4 from 'rygorous'.
	static FP32 HalfToFloat(FP16);

	FP16 Half;
};
#pragma pack(pop)


// tHalf must remain pod. No virtual function table etc.
tStaticAssert(sizeof(tHalf) == 2);


// Implementation below this line.


inline tHalf::FP16 tHalf::FloatToHalf(FP32 f)
{
	FP32 f32infty		= { 255 << 23 };
	FP32 f16max			= { (127 + 16) << 23 };
	FP32 denormMagic	= { ((127 - 15) + (23 - 10) + 1) << 23 };
	uint32 signMask		= 0x80000000u;
	FP16 o				= { 0 };
	uint32 sign			= f.u & signMask;
	f.u					^= sign;

	// All the integer compares in this function can be safely compiled into signed compares since all operands are
	// below 0x80000000.
	if (f.u >= f16max.u)
	{
		// Result is Inf or NaN (all exponent bits set). NaN->qNaN and Inf->Inf.
		o.u = (f.u > f32infty.u) ? 0x7e00 : 0x7c00;
	}
	else
	{
		// (De)normalized number or zero.
		if (f.u < (113 << 23))
		{
			// Resulting FP16 is subnormal or zero. Use a magic value to align our 10 mantissa bits at the bottom of
			// the float. As long as FP addition is round-to-nearest-even this just works.
			f.f += denormMagic.f;

			// One integer subtract of the bias later, we have our final float!
			o.u = f.u - denormMagic.u;
		}
		else
		{
			// Is resulting mantissa is odd?
			uint32 mantOdd = (f.u >> 13) & 1;

			// Update exponent, rounding bias part 1.
			f.u += ((15 - 127) << 23) + 0xfff;

			// Rounding bias part 2.
			f.u += mantOdd;

			// Take the bits!
			o.u = f.u >> 13;
		}
	}

	o.u |= sign >> 16;
	return o;
}


inline tHalf::FP32 tHalf::HalfToFloat(FP16 h)
{
	const FP32 magic				= { 113 << 23 };
	const uint32 shiftedExp			= 0x7c00 << 13;				// Exponent mask after shift.

	FP32 o;
	o.u								= (h.u & 0x7fff) << 13;		// Exponent/mantissa bits.
	uint32 exp						= shiftedExp & o.u;			// Just the exponent.
	o.u += (127 - 15) << 23;		// Exponent adjust.

	// Handle exponent special cases. First check for Inf/NaN.
	if (exp == shiftedExp)
	{
		// Extra exp adjust.
		o.u += (128 - 16) << 23;
	}
	else if (exp == 0)
	{
		// Zero/Denormal.
		o.u += 1 << 23;			 	// Extra exp adjust.
		o.f -= magic.f;			 	// Renormalize.
	}

	o.u |= (h.u & 0x8000) << 16;	// Sign bit.
	return o;
}
