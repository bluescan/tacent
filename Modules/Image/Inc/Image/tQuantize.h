// tQuantize.h
//
// Generic interface for quantizing colours (creating colour palettes).
//
// Copyright (c) 2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Image/tPixelFormat.h>
namespace tImage
{


enum class tQuantizeMethod
{
	Fixed,					// Supports from 2 to 256 colours. Low quality because used predefined palettes.
	Spatial,				// AKA scolorq. Supports from 2 to 256 colours. Good for 32 colours or fewer. Very slow for 64 colours or higher.
	Neu,					// AKA NeuQuant. Supports from 64 to 256 colours. Best for 128 to 256.
	Wu						// AKA XiaolinWu. Supports from 2 to 256 colours. Best for 128 to 256.
};
const char* tGetQuantizeMethodName(tQuantizeMethod);


namespace tQuantizeFixed
{
	// This is the interface for quantizing an image based on a fixed palette of colours without any 'smarts'.
	bool QuantizeImage
	(
		int numColours, int width, int height, const tPixel3* pixels,
		tColour3i* destPalette, uint8* destIndices,
		bool checkExact = true
	);

	// This performs an exact palettization of an image if the number of unique colours in an image is less-than-or-equal
	// to the supplied palette size (numColours). If there are too many unique colours, this function does nothing to
	// either destPalette or destIndices and returns false.
	bool QuantizeImageExact
	(
		int numColours, int width, int height, const tPixel3* pixels,
		tColour3i* destPalette, uint8* destIndices
	);
}


namespace tQuantizeSpatial
{
	// If ditherLevel is 0.0 uses ComputeBaseDither, otherwise ditherLevel must be > 0.0. filterSize must be 1, 3, or 5.
	//
	// If checkExact is true it will inspect all supplied pixels in case there are <= numColours of them. If that is
	// true then the image is exactly representable given the palette size and the quantize is not needed. The operation
	// to gather unique pixel colours is a little slow, so you are given the ability to turn this off.
	bool QuantizeImage
	(
		int numColours, int width, int height, const tPixel3* pixels,
		tColour3i* destPalette, uint8* destIndices,
		bool checkExact = true, double ditherLevel = 0.0, int filterSize = 3
	);

	double ComputeBaseDither(int width, int height, int numColours);
}


namespace tQuantizeNeu
{
	// With a sampling factor of 1 the entire image is used in the learning phase. With a factor of 10, a
	// pseudo-random subset of 1/10 of the pixels are used in the learning phase. sampleFactor must be in [1, 30].
	// Bigger values are faster but lower quality.
	//
	// If checkExact is true it will inspect all supplied pixels in case there are <= numColours of them. If that is
	// true then the image is exactly representable given the palette size and the quantize is not needed. The operation
	// to gather unique pixel colours is a little slow, so you are given the ability to turn this off.
	bool QuantizeImage
	(
		int numColours, int width, int height, const tPixel3* pixels,
		tColour3i* destPalette, uint8* destIndices,
		bool checkExact = true, int sampleFactor = 1
	);
}


}


// Implementation below this line.


inline const char* tImage::tGetQuantizeMethodName(tQuantizeMethod method)
{
	switch (method)
	{
		case tQuantizeMethod::Fixed:	return "fixed";
		case tQuantizeMethod::Spatial:	return "scolorq";
		case tQuantizeMethod::Neu:		return "neuquant";
		case tQuantizeMethod::Wu:		return "wu";
	}
	return "unknown";
}
