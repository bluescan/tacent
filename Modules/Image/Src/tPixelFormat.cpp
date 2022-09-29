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
		2,				// B5G6R5
		2,				// B4G4R4A4
		2,				// B5G5R5A1
		2,				// L8A8
		1,				// A8
		1,				// L8
		2,				// R16F
		4,				// R16G16F
		8,				// R16G16B16A16F
		4,				// R32F
		8,				// R32G32F
		16,				// R32G32B32A32F
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
		16,				// BC6H_U16
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
		"B5G6R5",
		"B4G4R4A4",
		"B5G5R5A1",
		"L8A8",
		"A8",
		"L8",
		"R16f",
		"R16G16f",
		"R16G16B16A16f",
		"R32f",
		"R32G32f",
		"R32G32B32A32f",
		"BC1_DXT1",
		"BC1_DXT1BA",
		"BC2_DXT3",
		"BC3_DXT5",
		"BC4_ATI1",
		"BC5_ATI2",
		"BC6s",
		"BC6u",
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
