// tResample.h
//
// Resample an image using various filers like nearest-neighbour, box, bilinear, and various bicubics.
//
// Copyright (c) 2020 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Math/tColour.h>
namespace tImage
{


enum class tResampleFilter
{
	Nearest,
	Box,
	Bilinear,

	// The bicubic filter coefficients (b,c) and names are described here:
	// https://entropymine.com/imageworsener/bicubic/
	// The order in which the cubic filters are list below matches my opinion of overall quality.
	Bicubic_Standard,		// Cardinal.				B=0		C=3/4
	Bicubic_CatmullRom,		// Cardinal.				B=0		C=1/2
	Bicubic_Mitchell,		// Balanced.				B=1/3	C=1/3
	Bicubic_Cardinal,		// Pure Cardinal.			B=0		C=1
	Bicubic_BSpline,		// Pure BSpline. Blurry.	B=1		C=0

	// Lanczos is useful for cases where increased contrast is needed, esp at edges. Overall is a bit 'ringy'.
	// See https://en.wikipedia.org/wiki/Lanczos_resampling for a description of the Lanczos kernel.
	Lanczos_Narrow,			// Sinc-based.				A = 2
	Lanczos_Normal,			// Sinc-based.				A = 3
	Lanczos_Wide,			// Sinc-based.				A = 4

	Invalid,
	NumFilters				= Invalid,

	// Aliaes.
	Bicubic					= Bicubic_Standard,
	Lanczos					= Lanczos_Normal
};


extern const char* tResampleFilterNames[int(tResampleFilter::NumFilters)];


enum class tResampleEdgeMode
{
	Clamp,
	Wrap
};


// Resample the image using the supplied filter. All channels are treated equally. With some resamplers the alpha
// channels gets multiplied into the colours, we do not. This simplicity has some repercussions -- specifically the
// texture author should extend the colours into the areas where the alpha is 0 to make sure rescaling near these
// borders does not introduce colour artifacts when upscaling.
//
// The edge mode is either clamp or wrap. In wrap node if a pixel to the right (or up) is needed for the resample and
// we are at the edge of the image, it is taken from the other side. Some libraries also support a 'reflect' mode but
// since I'm not sure when this is useful, it is being excluded.
bool Resample
(
	tPixel* src, int srcW, int srcH,
	tPixel* dst, int dstW, int dstH,
	tResampleFilter = tResampleFilter::Bilinear,
	tResampleEdgeMode = tResampleEdgeMode::Clamp
);


}
