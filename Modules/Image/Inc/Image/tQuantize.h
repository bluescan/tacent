// tQuantize.h
//
// Generic interface for quantizing colours (creating colour palettes).
//
// Copyright (c) 2022, 2024 Tristan Grimmer.
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
namespace tImage {
	
	
namespace tQuantize
{
	enum class Method
	{
		Fixed,					// Supports from 2 to 256 colours. Low quality because used predefined palettes.
		Spatial,				// AKA scolorq. Supports from 2 to 256 colours. Good for 32 colours or fewer. Very slow for 64 colours or higher.
		Neu,					// AKA NeuQuant. Supports from 2 to 256 colours. Best for 64 to 256.
		Wu,						// AKA XiaolinWu. Supports from 2 to 256 colours. Best for 64 to 256.
		NumMethods
	};
	const char* GetMethodName(Method);

	// This performs an exact palettization of an image if the number of unique colours in an image is less-than-or-equal
	// to the supplied numColours (palette size). If there are too many unique colours, this function does nothing to
	// either destPalette or destIndices and returns false. destPalette should have space for numColours colours,
	// destIndices should have space for width*height indices.
	bool QuantizeImageExact
	(
		int numColours, int width, int height, const tPixel3b* pixels, tColour3b* destPalette, uint8* destIndices
	);

	// Given a palette, array of indices, and the width and height of an image, this funcion converts back into a raw
	// pixel array. You must ensure there is enough room for width*height pixels in destPixels and that all indices
	// stay in the range of the palette you provide. srcIndices shoudd also have width*height entries.
	// Returns true on success.
	bool ConvertToPixels
	(
		tPixel3b* destPixels, int width, int height,
		const tColour3b* srcPalette, const uint8* srcIndices
	);

	// Same as above but writes to RGBA pixels. If preserveDestAlpha is true, it will not write to the alphs component
	// of the destPixels. Whatever was there before stays. If true, it writes 255 (fully opaque).
	bool ConvertToPixels
	(
		tPixel4b* destPixels, int width, int height,
		const tColour3b* srcPalette, const uint8* srcIndices, bool preserveDestAlpha = false
	);
}


namespace tQuantizeFixed
{
	// This is the function for quantizing an image based on a fixed palette of colours without any 'smarts'.
	// The static palettes are not noteworthy in any particular regard -- they favour green, then red, then blue (the
	// human eye is less sensitive to blue). The colours are roughy spread out evenly in the RGB cube. In all cases pure
	// black and pure white are included. In particular the 2-colour (1-bit) palette has only black and white. For the
	// 256-colour palette this ends up being the "8-8-4 levels RGB" palette. See:
	// https://en.wikipedia.org/wiki/List_of_software_palettes for more information.
	//
	// The palettes for a non-power-of-2 number of colours are based on the next higher power-of-2 with some entries
	// removed in a flip-flop skip pattern. The flip-flop controls which end the colour is removed from, the skip
	// ensures adjacent colour entries are not removed. 
	//
	// Generating palettes without inspecting the image pixels will never produce good results, so if you need quality
	// use one of the other adaptive quantizers. I could have used something like one of the CIE colour spaces or HSV
	// and vary the angle etc, but since palette generation involves perception _and_ is subjective, _and_ can't be done
	// well for arbitrary images, it's probably better to use noticably average fixed palettes -- even if only to
	// encourage use of a different method like NeuQuant, Scolorq, or Wu. Note, to figure out what palette-index a
	// particular pixel should map to, the red-mean colour difference function is used -- a common perceptual metric.
	//
	// destPalette should have space for numColours colours,
	// destIndices should have space for width*height indices.
	// The second variant is same as first but accepts RGBA pixels ignoring alpha.
	bool QuantizeImage
	(
		int numColours, int width, int height, const tPixel3b* pixels, tColour3b* destPalette, uint8* destIndices,
		bool checkExact = true
	);
	bool QuantizeImage
	(
		int numColours, int width, int height, const tPixel4b* pixels, tColour3b* destPalette, uint8* destIndices,
		bool checkExact = true
	);
}


namespace tQuantizeSpatial
{
	// If ditherLevel is 0.0 uses ComputeBaseDither, otherwise ditherLevel must be > 0.0. filterSize must be 1, 3, or 5.
	//
	// If checkExact is true it will inspect all supplied pixels in case there are <= numColours of them. If that is
	// true then the image is exactly representable given the palette size and the quantize is not needed. The operation
	// to gather unique pixel colours is a little slow, so you are given the ability to turn this off.
	//
	// destPalette should have space for numColours colours,
	// destIndices should have space for width*height indices.
	// The second variant is same as first but accepts RGBA pixels ignoring alpha.
	bool QuantizeImage
	(
		int numColours, int width, int height, const tPixel3b* pixels, tColour3b* destPalette, uint8* destIndices,
		bool checkExact = true, double ditherLevel = 0.0, int filterSize = 3
	);
	bool QuantizeImage
	(
		int numColours, int width, int height, const tPixel4b* pixels, tColour3b* destPalette, uint8* destIndices,
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
	//
	// destPalette should have space for numColours colours,
	// destIndices should have space for width*height indices.
	// The second variant is same as first but accepts RGBA pixels ignoring alpha.
	bool QuantizeImage
	(
		int numColours, int width, int height, const tPixel3b* pixels, tColour3b* destPalette, uint8* destIndices,
		bool checkExact = true, int sampleFactor = 1
	);
	bool QuantizeImage
	(
		int numColours, int width, int height, const tPixel4b* pixels, tColour3b* destPalette, uint8* destIndices,
		bool checkExact = true, int sampleFactor = 1
	);
}


namespace tQuantizeWu
{
	// If checkExact is true it will inspect all supplied pixels in case there are <= numColours of them. If that is
	// true then the image is exactly representable given the palette size and the quantize is not needed. The operation
	// to gather unique pixel colours is a little slow, so you are given the ability to turn this off.
	//
	// destPalette should have space for numColours colours,
	// destIndices should have space for width*height indices.
	// The second variant is same as first but accepts RGBA pixels ignoring alpha.
	bool QuantizeImage
	(
		int numColours, int width, int height, const tPixel3b* pixels, tColour3b* destPalette, uint8* destIndices,
		bool checkExact = true
	);
	bool QuantizeImage
	(
		int numColours, int width, int height, const tPixel4b* pixels, tColour3b* destPalette, uint8* destIndices,
		bool checkExact = true
	);
}


}
