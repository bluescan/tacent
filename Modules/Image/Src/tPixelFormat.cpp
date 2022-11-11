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
	const char* PixelFormatNames[] =
	{
		"Unknown",

		// Packed formats.
		"R8",
		"R8G8",
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

		// Original BC (4x4 Block Compression) formats.
		"BC1DXT1",
		"BC1DXT1A",
		"BC2DXT2DXT3",
		"BC3DXT4DXT5",
		"BC4ATI1",
		"BC5ATI2",
		"BC6s",
		"BC6u",
		"BC7",

		// ASTC (Adaptive Scalable Texture Compression) formats.
		"ASTC4X4",
		"ASTC5X4",
		"ASTC5X5",
		"ASTC6X5",
		"ASTC6X6",
		"ASTC8X5",
		"ASTC8X6",
		"ASTC8X8",
		"ASTC10X5",
		"ASTC10X6",
		"ASTC10X8",
		"ASTC10X10",
		"ASTC12X10",
		"ASTC12X12",
		
		// Vendor-specific formats.
		"RADIANCE",
		"OPENEXR",

		// Palette formats.
		"PAL8BIT",
		"PAL4BIT",
		"PAL1BIT"
	};
	tStaticAssert(int(tPixelFormat::NumPixelFormats)+1 == tNumElements(PixelFormatNames));

	int PackedFormat_BitsPerPixel[] =
	{
		8,				// R8
		16,				// R8G8
		24,				// R8G8B8
		32,				// R8G8B8A8
		24,				// B8G8R8
		32,				// B8G8R8A8
		16,				// B5G6R5
		16,				// B4G4R4A4
		16,				// B5G5R5A1
		16,				// L8A8
		8,				// A8
		8,				// L8
		16,				// R16F
		32,				// R16G16F
		64,				// R16G16B16A16F
		32,				// R32F
		64,				// R32G32F
		128,			// R32G32B32A32F
	};
	tStaticAssert(tNumElements(PackedFormat_BitsPerPixel) == int(tPixelFormat::NumPackedFormats));

	int BCFormat_BytesPerBlock[] =
	{
		8,				// BC1DXT1
		8,				// BC1DXT1A
		16,				// BC2DXT2DXT3
		16,				// BC3DXT4DXT5
		8,				// BC4ATI1
		16,				// BC5ATI2
		16,				// BC6s
		16,				// BC6u
		16				// BC7
	};
	tStaticAssert(tNumElements(BCFormat_BytesPerBlock) == int(tPixelFormat::NumBCFormats));

	int ASTCFormat_BlockWidth[] =
	{
		4,				// ASTC4X4
		5,				// ASTC5X4
		5,				// ASTC5X5
		6,				// ASTC6X5
		6,				// ASTC6X6
		8,				// ASTC8X5
		8,				// ASTC8X6
		8,				// ASTC8X8
		10,				// ASTC10X5
		10,				// ASTC10X6
		10,				// ASTC10X8
		10,				// ASTC10X10
		12,				// ASTC12X10
		12				// ASTC12X12
	};
	tStaticAssert(tNumElements(ASTCFormat_BlockWidth) == int(tPixelFormat::NumASTCFormats));

	int ASTCFormat_BlockHeight[] =
	{
		4,				// ASTC4X4
		4,				// ASTC5X4
		5,				// ASTC5X5
		5,				// ASTC6X5
		6,				// ASTC6X6
		5,				// ASTC8X5
		6,				// ASTC8X6
		8,				// ASTC8X8
		5,				// ASTC10X5
		6,				// ASTC10X6
		8,				// ASTC10X8
		10,				// ASTC10X10
		10,				// ASTC12X10
		12				// ASTC12X12
	};
	tStaticAssert(tNumElements(ASTCFormat_BlockHeight) == int(tPixelFormat::NumASTCFormats));

	int VendorFormat_BitsPerPixel[] =
	{
		32,				// Radiance. 3 bytes for each RGB. 1 byte shared exponent.
		128				// OpenEXR. @todo There are multiple exr pixel formats. We don't yet determine which one.
	};
	tStaticAssert(tNumElements(VendorFormat_BitsPerPixel) == int(tPixelFormat::NumVendorFormats));

	int PaletteFormat_BitsPerPixel[] =
	{
		8,
		4,
		1
	};
	tStaticAssert(tNumElements(PaletteFormat_BitsPerPixel) == int(tPixelFormat::NumPaletteFormats));
}


int tImage::tGetBlockWidth(tPixelFormat format)
{
	if (tIsPackedFormat(format) || tIsVendorFormat(format) || tIsPaletteFormat(format))
		return 1;

	if (tIsBCFormat(format))
		return 4;

	// ASTC formats have different widths depending on format.
	if (tIsASTCFormat(format))
		return ASTCFormat_BlockWidth[int(format) - int(tPixelFormat::FirstASTC)];

	return 0;
}


int tImage::tGetBlockHeight(tPixelFormat format)
{
	if (tIsPackedFormat(format) || tIsVendorFormat(format) || tIsPaletteFormat(format))
		return 1;

	if (tIsBCFormat(format))
		return 4;

	// ASTC formats have different heights depending on format.
	if (tIsASTCFormat(format))
		return ASTCFormat_BlockHeight[int(format) - int(tPixelFormat::FirstASTC)];

	return 0;
}


int tImage::tGetBitsPerPixel(tPixelFormat format)
{
	if (tIsPackedFormat(format))
		return PackedFormat_BitsPerPixel[int(format) - int(tPixelFormat::FirstPacked)];

	if (tIsBCFormat(format))
		return (8*tGetBytesPerBlock(format)) >> 4;

	if (tIsVendorFormat(format))
		return VendorFormat_BitsPerPixel[int(format) - int(tPixelFormat::FirstVendor)];

	if (tIsPaletteFormat(format))
		return PaletteFormat_BitsPerPixel[int(format) - int(tPixelFormat::FirstPalette)];

	return 0;
}


float tImage::tGetBitsPerPixelFloat(tPixelFormat format)
{
	int bitsPerPixel = tGetBitsPerPixel(format);
	if (bitsPerPixel)
		return float(bitsPerPixel);

	int pixelsPerBlock = tGetBlockWidth(format) * tGetBlockHeight(format);
	int bitsPerBlock = tGetBytesPerBlock(format) * 8;
	return float(bitsPerBlock) / float(pixelsPerBlock);
}


int tImage::tGetBytesPerBlock(tPixelFormat format)
{
	if (tIsBCFormat(format))
		return BCFormat_BytesPerBlock[int(format) - int(tPixelFormat::FirstBC)];

	if (tIsASTCFormat(format))
		return 16;

	if (tIsPackedFormat(format))
		return tGetBitsPerPixel(format) / 8;

	return 0;
}


const char* tImage::tGetPixelFormatName(tPixelFormat pixelFormat)
{
	int index = int(pixelFormat) + 1;
	return PixelFormatNames[index];
}


tImage::tPixelFormat tImage::tGetPixelFormat(const char* name)
{
	if (!name || (name[0] == '\0'))
		return tPixelFormat::Invalid;

	for (int p = 0; p < int(tPixelFormat::NumPixelFormats); p++)
		if (tStd::tStricmp(PixelFormatNames[p+1], name) == 0)
			return tPixelFormat(p);

	return tPixelFormat::Invalid;
}
