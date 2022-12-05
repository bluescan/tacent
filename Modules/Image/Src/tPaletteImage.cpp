// tPaletteImage.cpp
//
// A simple palettized image. Comprised of Width x Height pixel data storing indexes into a palette. The palette is
// simply an array of tPixels (RGBA). Index resolution is determined by the pixel format (1, 2, 4, 8, or 16 bits). The
// number of palette entries (colours) is 2 ^ the index-resolution.
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

#include <Foundation/tAssert.h>
#include <Foundation/tStandard.h>
#include "Image/tPaletteImage.h"
namespace tImage
{


bool tPaletteImage::Set(const tPaletteImage& src)
{
	if (&src == this)
		return true;

	Clear();
	if (!src.IsValid())
		return false;

	PixelFormat = src.PixelFormat;
	Width = src.Width;
	Height = src.Height;

	int dataSize = src.GetDataSize();
	tAssert((dataSize > 0) && src.PixelData);
	PixelData = new uint8[dataSize];
	tStd::tMemcpy(PixelData, src.PixelData, dataSize);

	int palSize = src.GetPaletteSize();
	tAssert((palSize > 0) && src.Palette);
	Palette = new tColour3i[palSize];
	tStd::tMemcpy(Palette, src.Palette, palSize*sizeof(tColour3i));

	return true;
}


bool tPaletteImage::Set(tPixelFormat fmt, int width, int height)
{
	Clear();
	if (!tIsPaletteFormat(fmt) || (width <= 0) || (height <= 0))
		return false;

	PixelFormat = fmt;
	Width = width;
	Height = height;

	int dataSize = GetDataSize();
	tAssert(dataSize > 0);
	PixelData = new uint8[dataSize];
	tStd::tMemset(PixelData, 0, dataSize);

	int palSize = GetPaletteSize();
	tAssert(palSize > 0);
	Palette = new tColour3i[palSize];
	tStd::tMemset(Palette, 0, palSize*sizeof(tColour3i));

	return true;
}


bool tPaletteImage::Set(tPixelFormat fmt, int width, int height, const uint8* pixelData, const tColour3i* palette)
{
	Clear();
	if (!tIsPaletteFormat(fmt) || (width <= 0) || (height <= 0) || !pixelData || !palette)
		return false;

	PixelFormat = fmt;
	Width = width;
	Height = height;

	int dataSize = GetDataSize();
	tAssert(dataSize > 0);
	PixelData = new uint8[dataSize];
	tStd::tMemcpy(PixelData, pixelData, dataSize);

	int palSize = GetPaletteSize();
	tAssert(palSize > 0);
	Palette = new tColour3i[palSize];
	tStd::tMemcpy(Palette, palette, palSize*sizeof(tColour3i));

	return true;
}


bool tPaletteImage::Set(tPixelFormat fmt, int width, int height, const tPixel* pixels, tQuantizeMethod quantMethod)
{
	Clear();
	if (!tIsPaletteFormat(fmt) || (width <= 0) || (height <= 0) || !pixels)
		return false;

	if ((fmt != tPixelFormat::PAL8BIT) && (quantMethod != tQuantizeMethod::Fixed))
		return false;

// WIP
	tPixel3* rgbPixels = new tPixel3[width*height];
//	for (int i = 0; i < width*height; i++)
//		rgbPixels[i].Se
	bool success = Set(fmt, width, height, rgbPixels, quantMethod);

	delete[] rgbPixels;
	return true;
}


bool tPaletteImage::Set(tPixelFormat fmt, int width, int height, const tPixel3* pixels, tQuantizeMethod quantMethod)
{
	Clear();
	if (!tIsPaletteFormat(fmt) || (width <= 0) || (height <= 0) || !pixels)
		return false;

	if ((fmt != tPixelFormat::PAL8BIT) && (quantMethod != tQuantizeMethod::Fixed))
		return false;

	// WIP
	// Step 1. Create the palette.

	// Step 2. Use a perceptual colour distance
	return true;
}


int tPaletteImage::GetDataSize() const
{
	return 0;
}


int tPaletteImage::GetPaletteSize() const
{
	return tMath::tPow2(tGetBitsPerPixel(PixelFormat));
}


}
