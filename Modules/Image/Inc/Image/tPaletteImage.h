// tPaletteImage.h
//
// A simple palettized image. Comprised of Width x Height pixel data storing indexes into a palette. The palette is
// simply an array of tPixels (RGB). Index resolution is determined by the pixel format (1, 2, 4, 8, or 16 bits). The
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

#pragma once
#include <Math/tColour.h>
#include <Image/tPixelFormat.h>
#include <Image/tQuantize.h>
namespace tImage
{


// A simple palettized image class supporting 1, 2, 4, 8, and 16 bits per pixel-index. Origin is at the bottom-left and
// rows are ordered left to right moving up the image. This palette only contains RGB values (no alpha). Formats like
// gif handle (binary) alpha separately, and colour quantizers work on RGB values, so having a tPaletteImage with alpha
// makes little sense.
class tPaletteImage
{
public:
	// Creates an invalid image.
	tPaletteImage()																										{ }

	tPaletteImage(const tPaletteImage& src)																				{ Set(src); }

	// The pixel format must be one of the PALNNIDX formats. The palette size is determined by the pixel format. This
	// constructor creates a palette with all black/zero-alpha colours and every pixel indexing the first palette entry.
	// Note that internally there may be padding of the pixel-data for some palette pixel formats. For example, if the
	// pixel format is PAL1IDX (1 bit per pixel) and you have a 9x10 image, you will need 2 bytes for the first row. 7
	// bits of the second byte will be unused. There will be 10 such rows resulting in 70 bits of
	// padding (8 bytes + 6 bits).
	tPaletteImage(tPixelFormat fmt, int width, int height)																{ Set(fmt, width, height); }

	// This is similar to above except you can construct a full image with palette and pixel-data. Caller is responsibe
	// for making sure a) the palette is the correct length, and b) the pixel-data indexes are the correct size and
	// there are width*height of them. The data is copied out of the supplied arrays. You can use
	// tGetBitsPerPixel(tPixelFormat) to give you the size in bits of each pixel-index. Each row must be padded to a
	// byte-boundary. Call tPow2 with that value to give you the palette length needed.
	tPaletteImage(tPixelFormat fmt, int width, int height, const uint8* pixelData, const tColour3i* palette)			{ Set(fmt, width, height, pixelData, palette); }

	// This is the workhorse constructor because it needs to quantize the present colours to create the palette.
	// Quantizing, or rather doing a good job of quantizing, is quite complex. The Neu algorithm uses a neural net to
	// accomplish this and gives good results. Alpha is ignored in the pixel array.
	// Note: Neu and Wu quantizing methods support PAL8BIT only.
	tPaletteImage(tPixelFormat fmt, int width, int height, const tPixel* pixels, tQuantizeMethod quantMethod)			{ Set(fmt, width, height, pixels, quantMethod); }

	// Same as above but processed pixel data in RGB instead of ignoring the alpha.
	tPaletteImage(tPixelFormat fmt, int width, int height, const tPixel3* pixels, tQuantizeMethod quantMethod)			{ Set(fmt, width, height, pixels, quantMethod); }

	virtual ~tPaletteImage()																							{ Clear(); }

	// Frees internal layer data and makes the layer invalid.
	void Clear()																										{ PixelFormat = tPixelFormat::Invalid; Width = Height = 0; delete[] PixelData; PixelData = nullptr; delete[] Palette; Palette = nullptr; }

	// See the corresponding constructor comments for the set calls.
	bool Set(const tPaletteImage&);
	bool Set(tPixelFormat, int width, int height);
	bool Set(tPixelFormat, int width, int height, const uint8* pixelData, const tColour3i* palette);
	bool Set(tPixelFormat, int width, int height, const tPixel* pixels, tQuantizeMethod);
	bool Set(tPixelFormat, int width, int height, const tPixel3* pixels, tQuantizeMethod);

	// Populates the supplied pixel array. It is up to you to make sure there is enough room for width*height tPixels in
	// the supplied array. Returns success. You'll get false if either this image is invalid or null is passed in.
	bool Get(tPixel* pixels);

	// Same as above but populates an RGB array.
	bool Get(tPixel3* pixels);

	bool IsValid() const																								{ return (PixelData && Palette) ? true : false; }

	// Returns the size of the data in bytes by reading the Width, Height, and PixelFormat.
	int GetDataSize() const;

	// Returns the size of the palette in tColour4i's.
	int GetPaletteSize() const;

	tPixelFormat PixelFormat			= tPixelFormat::Invalid;
	int Width							= 0;
	int Height							= 0;
	uint8* PixelData					= nullptr;
	tColour3i* Palette					= nullptr;
};


}
