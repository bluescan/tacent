// tPaletteImage.h
//
// A simple palettized image. Comprised of Width x Height pixel data storing indexes into a palette. The palette is
// simply an array of tPixels (RGB). Index resolution is determined by the pixel format (1 to 8 bits). The number of
// palette entries (colours) is 2 ^ the index-resolution.
//
// Copyright (c) 2022-2024 Tristan Grimmer.
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
#include <Image/tPixelFormat.h>
#include <Image/tQuantize.h>
namespace tImage
{


// A simple palettized image class supporting 1 to 8 bits per pixel-index. Origin is at the bottom-left and rows are
// ordered left to right moving up the image. This palette only contains RGB values (no alpha). Formats like gif handle
// (binary) alpha separately, and colour quantizers work on RGB values, so no alpha for tPaletteImage.
class tPaletteImage
{
public:
	// Creates an invalid image.
	tPaletteImage()																										{ }

	tPaletteImage(const tPaletteImage& src)																				{ Set(src); }

	// The pixel format must be one of the PALNNIDX formats. The palette size is determined by the pixel format. This
	// constructor creates a palette with all black colours and every pixel indexing the first palette entry. Note that
	// internally there may be padding of the pixel-data for some palette pixel formats. It pads to 8 bits. For example,
	// if the pixel format is PAL1BIT (1 bit per pixel) and you have a 9x10 image, you will need 90 bits. That requires
	// 12 8-bit chunks costing 96 bits total. The last 6 bits get padded with 0.
	tPaletteImage(tPixelFormat fmt, int width, int height)																{ Set(fmt, width, height); }

	// This is similar to above except you can construct a full image with palette and pixel-data. Caller is responsibe
	// for making sure a) the palette is the correct length, and b) the pixel-data indexes are the correct size and
	// there are width*height of them. The supplied pixelData array should include any necessary padding. eg. a 10x10
	// PAL1BIT image should be 13 bytes of data with 4 bits padded at the end. The data is copied out of the supplied
	// arrays. You can use tGetBitsPerPixel(tPixelFormat) to give you the size in bits of each pixel-index. Call tPow2
	// with that value to give you the palette length needed.
	tPaletteImage(tPixelFormat fmt, int width, int height, const uint8* pixelData, const tColour3b* palette)			{ Set(fmt, width, height, pixelData, palette); }

	// This is the workhorse constructor because it needs to quantize the present colours to create the palette.
	// Quantizing, or rather doing a good job of quantizing, is quite complex. The NeuQuant algorithm uses a neural net
	// to accomplish this and gives good results. Alpha is ignored in the pixel array.
	tPaletteImage(tPixelFormat fmt, int width, int height, const tPixel4* pixels, tQuantize::Method quantMethod)		{ Set(fmt, width, height, pixels, quantMethod); }

	// Same as above but processes pixel data in RGB directly.
	tPaletteImage(tPixelFormat fmt, int width, int height, const tPixel3* pixels, tQuantize::Method quantMethod)		{ Set(fmt, width, height, pixels, quantMethod); }

	virtual ~tPaletteImage()																							{ Clear(); }

	// Frees internal layer data and makes the layer invalid.
	void Clear()																										{ PixelFormat = tPixelFormat::Invalid; Width = Height = 0; delete[] PixelData; PixelData = nullptr; delete[] Palette; Palette = nullptr; }

	// See the corresponding constructor comments for the set calls.
	bool Set(const tPaletteImage&);
	bool Set(tPixelFormat, int width, int height);
	bool Set(tPixelFormat, int width, int height, const uint8* pixelData, const tColour3b* palette);
	bool Set(tPixelFormat, int width, int height, const tPixel4* pixels, tQuantize::Method);
	bool Set(tPixelFormat, int width, int height, const tPixel3* pixels, tQuantize::Method);

	// Populates the supplied pixel array. It is up to you to make sure there is enough room for width*height tPixels in
	// the supplied array. Returns success. You'll get false if either this image is invalid or null is passed in.
	bool Get(tPixel4* pixels);

	// Same as above but populates an RGB array.
	bool Get(tPixel3* pixels);

	bool IsValid() const																								{ return (PixelData && Palette && Width && Height && tIsPaletteFormat(PixelFormat)); }

	// Returns the size of the data in bytes by reading the Width, Height, and PixelFormat. Includes padding. The
	// returned value is always a multiple of 4 butes.
	int GetDataSize() const;

	// Returns the size of the palette in tColour3b's.
	int GetPaletteSize() const;

	tPixelFormat PixelFormat			= tPixelFormat::Invalid;
	int Width							= 0;
	int Height							= 0;
	uint8* PixelData					= nullptr;
	tColour3b* Palette					= nullptr;
};


}
