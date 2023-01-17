// tPaletteImage.cpp
//
// A simple palettized image. Comprised of Width x Height pixel data storing indexes into a palette. The palette is
// simply an array of tPixels (RGB). Index resolution is determined by the pixel format (1 to 8 bits). The number of
// palette entries (colours) is 2 ^ the index-resolution.
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
#include <Foundation/tBitArray.h>
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


bool tPaletteImage::Set(tPixelFormat fmt, int width, int height, const tPixel* pixels, tQuantize::Method quantMethod)
{
	Clear();
	if (!tIsPaletteFormat(fmt) || (width <= 0) || (height <= 0) || !pixels)
		return false;

	tPixel3* rgbPixels = new tPixel3[width*height];
	for (int i = 0; i < width*height; i++)
	{
		rgbPixels[i].R = pixels[i].R;
		rgbPixels[i].G = pixels[i].G;
		rgbPixels[i].B = pixels[i].B;
	}
	bool success = Set(fmt, width, height, rgbPixels, quantMethod);

	delete[] rgbPixels;
	return success;
}


bool tPaletteImage::Set(tPixelFormat fmt, int width, int height, const tPixel3* pixels, tQuantize::Method quantMethod)
{
	Clear();
	if (!tIsPaletteFormat(fmt) || (width <= 0) || (height <= 0) || !pixels)
		return false;

	PixelFormat = fmt;
	Width = width;
	Height = height;
	int numColours			= GetPaletteSize();
	Palette					= new tColour3i[numColours];
	int dataSize			= GetDataSize();
	PixelData				= new uint8[dataSize];

	uint8* indices = new uint8[width*height];

	// Step 1. Call quantize. Populates the palette and the indices.
	switch (quantMethod)
	{
		case tQuantize::Method::Fixed:
			tQuantizeFixed::QuantizeImage(numColours, width, height, pixels, Palette, indices);
			break;

		case tQuantize::Method::Spatial:
			tQuantizeSpatial::QuantizeImage(numColours, width, height, pixels, Palette, indices);
			break;

		case tQuantize::Method::Neu:
			tQuantizeNeu::QuantizeImage(numColours, width, height, pixels, Palette, indices);
			break;

		case tQuantize::Method::Wu:
			tQuantizeWu::QuantizeImage(numColours, width, height, pixels, Palette, indices);
			break;

		default:
			delete[] indices;
			Clear();
			return false;
	}

	// Step 2. Populate PixelData from indices.
	int bpp = tGetBitsPerPixel(fmt);
	int numBits = Width*Height*bpp;
	int bitIndex = 0;
	tBitArray8 bitArray(PixelData, numBits, true);
	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			bitArray.SetBits(bitIndex, bpp, indices[x + y*Width]);
			bitIndex += bpp;
		}
	}
	
	delete[] indices;
	return true;
}


bool tPaletteImage::Get(tPixel* pixels)
{
	if (!IsValid() || !pixels)
		return false;

	int bpp = tGetBitsPerPixel(PixelFormat);
	int numBits = Width*Height*bpp;
	int bitIndex = 0;
	tBitArray8 bitArray(PixelData, numBits, true);
	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			uint8 palIdx = bitArray.GetBits(bitIndex, bpp);
			tColour3i& colour = Palette[palIdx];
			pixels[x + y*Width].Set(colour.R, colour.G, colour.B);
			bitIndex += bpp;
		}
	}

	return true;
}


bool tPaletteImage::Get(tPixel3* pixels)
{
	if (!IsValid() || !pixels)
		return false;

	int bpp = tGetBitsPerPixel(PixelFormat);
	int numBits = Width*Height*bpp;
	int bitIndex = 0;
	tBitArray8 bitArray(PixelData, numBits, true);
	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			uint8 palIdx = bitArray.GetBits(bitIndex, bpp);
			tColour3i& colour = Palette[palIdx];
			pixels[x + y*Width].Set(colour.R, colour.G, colour.B);
			bitIndex += bpp;
		}
	}

	return true;
}


int tPaletteImage::GetDataSize() const
{
	int numBits = Width*Height*tGetBitsPerPixel(PixelFormat);
	int numBytes = (numBits + 7) / 8;
	return numBytes;
}


int tPaletteImage::GetPaletteSize() const
{
	return tMath::tPow2(tGetBitsPerPixel(PixelFormat));
}


}
