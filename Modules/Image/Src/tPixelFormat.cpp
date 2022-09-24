// tPixelFormat.cpp
//
// Pixel formats in Tacent. Not all formats are fully supported. Certainly BC 4, 5, and 7 may not have extensive HW
// support at this time.
//
// Copyright (c) 2004-2006, 2017, 2019, 2021, 2022 Tristan Grimmer.
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
#include "Image/tPixelFormat.h"


namespace tImage
{
	int NormalFormat_BytesPerPixel[] =
	{
		3,				// R8G8B8
		4,				// R8G8B8A8
		3,				// B8G8R8
		4,				// B8G8R8A8
		2,				// G3B5A1R5G2
		2,				// G4B4A4R4
		2,				// G3B5R5G3
		2,				// L8A8
		1,				// A8
		1,				// L8
		4,				// R32F
		8,				// G32R32F
		16,				// A32B32G32R32F
	};
	tStaticAssert(tNumElements(NormalFormat_BytesPerPixel) == int(tPixelFormat::NumNormalFormats));

	int BlockFormat_BytesPer4x4PixelBlock[] =
	{
		8,				// BC1_DXT1
		8,				// BC1_DXT1BA
		16,				// BC2_DXT3
		16,				// BC3_DXT5
		8,				// BC4_ATI1
		16,				// BC5_ATI2
		16,				// BC6H_S16
		16				// BC7_UNORM
	};
	tStaticAssert(tNumElements(BlockFormat_BytesPer4x4PixelBlock) == int(tPixelFormat::NumBlockFormats));

	int VendorFormat_BytesPerPixel[] =
	{
		4,				// Radiance. 3 bytes for each RGB. 1 byte shared exponent.
		0				// OpenEXR. @todo There are multiple exr pixel formats. We don't yet determine which one.
	};
	tStaticAssert(tNumElements(VendorFormat_BytesPerPixel) == int(tPixelFormat::NumVendorFormats));
}


bool tImage::tIsNormalFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstNormal) && (format <= tPixelFormat::LastNormal))
		return true;

	return false;
}


bool tImage::tIsBlockCompressedFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstBlock) && (format <= tPixelFormat::LastBlock))
		return true;

	return false;
}


bool tImage::tIsVendorFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstVendor) && (format <= tPixelFormat::LastVendor))
		return true;

	return false;
}


bool tImage::tIsPaletteFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstPAL) && (format <= tPixelFormat::LastPAL))
		return true;

	return false;
}


bool tImage::tFormatSupportsAlpha(tPixelFormat format)
{
	switch (format)
	{
		case tPixelFormat::R8G8B8A8:
		case tPixelFormat::B8G8R8A8:
		case tPixelFormat::G3B5A1R5G2:
		case tPixelFormat::G4B4A4R4:
		case tPixelFormat::A8L8:
		case tPixelFormat::R32F:
		case tPixelFormat::A32B32G32R32F:
		case tPixelFormat::BC1_DXT1BA:
		case tPixelFormat::BC2_DXT2_DXT3:
		case tPixelFormat::BC3_DXT4_DXT5:
		case tPixelFormat::BC7:
		case tPixelFormat::OPENEXR_HDR:

		// For palettized the palette may have an entry that can be considered alpha. However for only 1-bit
		// palettes we consider it dithered (ColourA/ColourB) and not to have an alpha.
		case tPixelFormat::PAL_8BIT:
		case tPixelFormat::PAL_4BIT:
			return true;
	}

	return false;
}


int tImage::tGetBitsPerPixel(tPixelFormat format)
{
	if (tIsNormalFormat(format))
		return 8*NormalFormat_BytesPerPixel[int(format) - int(tPixelFormat::FirstNormal)];

	if (tIsBlockCompressedFormat(format))
		return (8*tGetBytesPer4x4PixelBlock(format)) >> 4;

	if (tIsVendorFormat(format))
		return 8*VendorFormat_BytesPerPixel[int(format) - int(tPixelFormat::FirstVendor)];

	if (tIsPaletteFormat(format))
	{
		switch (format)
		{
			case tPixelFormat::PAL_8BIT:		return 8;
			case tPixelFormat::PAL_4BIT:		return 4;
			case tPixelFormat::PAL_1BIT:		return 1;
		}
	}

	return -1;
}


int tImage::tGetBytesPer4x4PixelBlock(tPixelFormat format)
{
	if (format == tPixelFormat::Invalid)
		return -1;

	tAssert(tIsBlockCompressedFormat(format));
	int index = int(format) - int(tPixelFormat::FirstBlock);
	return BlockFormat_BytesPer4x4PixelBlock[index];
}


const char* tImage::tGetPixelFormatName(tPixelFormat pixelFormat)
{
	const char* names[] =
	{
		"Unknown",
		"R8G8B8",
		"R8G8B8A8",
		"B8G8R8",
		"B8G8R8A8",
		"G3B5A1R5G2",
		"G4B4A4R4",
		"G3B5R5G3",
		"L8A8",
		"A8",
		"L8",
		"R32F",
		"G32R32F",
		"A32B32G32R32F",
		"BC1_DXT1",
		"BC1_DXT1BA",
		"BC2_DXT3",
		"BC3_DXT5",
		"BC4_ATI1",
		"BC5_ATI2",
		"BC6H",
		"BC7",
		"RADIANCE",
		"EXR",
		"PAL8BIT",
		"PAL4BIT",
		"PAL1BIT"
	};

	tStaticAssert(int(tPixelFormat::NumPixelFormats)+1 == sizeof(names)/sizeof(*names));
	int index = int(pixelFormat) + 1;
	return names[index];
}
