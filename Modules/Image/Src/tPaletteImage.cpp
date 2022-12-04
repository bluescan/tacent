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
	return true;
}


bool tPaletteImage::Set(tPixelFormat fmt, int width, int height)
{
	return true;
}


bool tPaletteImage::Set(tPixelFormat fmt, int width, int height, const uint8* pixelData, const tColour4i* palette)
{
	return true;
}


bool tPaletteImage::Set(tPixelFormat fmt, int width, int height, const tPixel* pixels, tQuantizeAlgo quantAlgo)
{
	return true;
}


int tPaletteImage::GetDataSize() const
{
	return true;
}


int tPaletteImage::GetPaletteSize() const
{
	return tMath::tPow2(tGetBitsPerPixel(PixelFormat));
}


}
