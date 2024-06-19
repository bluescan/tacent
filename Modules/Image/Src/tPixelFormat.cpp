// tPixelFormat.cpp
//
// Pixel formats in Tacent. Not all formats are fully supported. Certainly BC 4, 5, and 7 may not have extensive HW
// support at this time.
//
// Copyright (c) 2004-2006, 2017, 2019, 2021-2024 Tristan Grimmer.
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
#include <Foundation/tFundamentals.h>
#include "Image/tPixelFormat.h"


namespace tImage
{
	const char* PixelFormatNames[] =
	{
		// Packed formats.
		"R8",
		"R8G8",
		"R8G8B8",
		"R8G8B8A8",
		"B8G8R8",
		"B8G8R8A8",
		"G3B5R5G3",
		"G4B4A4R4",
		"B4A4R4G4",
		"G3B5A1R5G2",
		"G2B5A1R5G3",
		"L8A8",
		"A8",
		"L8",
		"R16",
		"R16G16",
		"R16G16B16",
		"R16G16B16A16",
		"R32",
		"R32G32",
		"R32G32B32",
		"R32G32B32A32",
		"R16f",
		"R16G16f",
		"R16G16B16f",
		"R16G16B16A16f",
		"R32f",
		"R32G32f",
		"R32G32B32f",
		"R32G32B32A32f",
		"R11G11B10uf",
		"B10G11R11uf",
		"R9G9B9E5uf",
		"E5B9G9R9uf",
		"R8G8B8M8",
		"R8G8B8D8",

		// Original BC (4x4 Block Compression) formats.
		"BC1DXT1",
		"BC1DXT1A",
		"BC2DXT2DXT3",
		"BC3DXT4DXT5",
		"BC4ATI1U",
		"BC4ATI1S",
		"BC5ATI2U",
		"BC5ATI2S",
		"BC6U",
		"BC6S",
		"BC7",
		"ETC1",
		"ETC2RGB",
		"ETC2RGBA",
		"ETC2RGBA1",
		"EACR11U",
		"EACR11S",
		"EACRG11U",
		"EACRG11S",

		// PVR (Power VR by Imagination) formats.
		"PVRBPP4",
		"PVRBPP2",
		"PVRHDRBPP8",
		"PVRHDRBPP6",
		"PVR2BPP4",
		"PVR2BPP2",
		"PVR2HDRBPP8",
		"PVR2HDRBPP6",

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
		"PAL1BIT",
		"PAL2BIT",
		"PAL3BIT",
		"PAL4BIT",
		"PAL5BIT",
		"PAL6BIT",
		"PAL7BIT",
		"PAL8BIT"
	};
	tStaticAssert(int(tPixelFormat::NumPixelFormats) == tNumElements(PixelFormatNames));
	const char** PixelFormatNames_Packed	= &PixelFormatNames[int(tPixelFormat::FirstPacked)];
	const char** PixelFormatNames_Block		= &PixelFormatNames[int(tPixelFormat::FirstBC)];
	const char** PixelFormatNames_PVR		= &PixelFormatNames[int(tPixelFormat::FirstPVR)];
	const char** PixelFormatNames_ASTC		= &PixelFormatNames[int(tPixelFormat::FirstASTC)];
	const char** PixelFormatNames_Vendor	= &PixelFormatNames[int(tPixelFormat::FirstVendor)];
	const char** PixelFormatNames_Palette	= &PixelFormatNames[int(tPixelFormat::FirstPalette)];


	const char* PixelFormatDescs[] =
	{
		// Packed formats.
		"R8 : 8 bit. Unsigned representing red. Some file-types that\n"
		"don't support A8 or L8 (eg. ktx2) will use this instead.",

		"R8G8 : 16 bit. Unsigned representing red and green.\n"
		"Vulkan has an analagous format.",

		"R8G8B8 : 24 bit. Full colour. No alpha. Matches GL_RGB source\n"
		"ordering. Not efficient. Most drivers will swizzle to BGR.",

		"R8G8B8A8 : 32  bit. Full alpha. Matches GL_RGBA source ordering.\n"
		"Not efficient. Most drivers will swizzle to ABGR.",

		"B8G8R8 : 24 bit. Full colour. No alpha. Matches GL_BGR source\n"
		"ordering. Efficient. Most drivers do not need to swizzle.",

		"B8G8R8A8 : 32 bit. Full alpha. Matches GL_BGRA source ordering.\n"
		"Most drivers do not need to swizzle.",

		"G3B5R5G3 : 16 bit. No alpha. Also known as B5G6R5. In memory\n"
		"it is GGGBBBBB RRRRRGGG which is more accurately G3B5R5G3.",

		"G4B4A4R4 : 16 bit. 12 colour bits. 4 bit alpha. Also\n"
		"known as B4G4R4A4. G4B4A4R4 describes the memory layout.",

		"B4A4R4G4 : 16 bit. 12 colour bits. 4 bit alpha. Also\n"
		"known as R4G4B4A4. B4A4R4G4 describes the memory layout.",

		"G3B5A1R5G2 : 16 bit. 15 colour bits. Binary alpha. Also\n"
		"known as B5G5R5A1. G3B5A1R5G2 describes the memory layout.",

		"G2B5A1R5G3 : 16 bit. 15 colour bits. Binary alpha. Also\n"
		"known as R5G5B5A1. G2B5A1R5G3 describes the memory layout.",

		"L8A8 : 16 bit. Alpha and Luminance.",
		"A8 : 8 bit. Alpha only.",
		"L8 : 8 bit. Luminance only.",
		"R16 : 16 bit. Red channel only.",
		"R16G16 : 32 bit. 16 bit R. 16 bit G.",
		"R16G16B16 : 48 bit. 16 bit each RGB.",
		"R16G16B16A16 : 64 bit. 16 bit each RGBA.",

		"R32 : 32 bit. Red channel only.",
		"R32G32 : 64 bit. 32 bit R. 32 bit G.",
		"R32G32B32 : 96 bit. 32 bit each RGB.",
		"R32G32B32A32 : 128 bit. 32 bit each RGBA.",

		"R16f : 16 bit. Half-float red/luminance channel only.\n"
		"HDR linear space.",

		"R16G16f : 32 bit. Two half-floats per pixel. RG.\n"
		"HDR linear space.",

		"R16G16B16f : 48 bit. Three half-floats per pixel. RGB.\n"
		"HDR linear space.",

		"R16G16B16A16f : 64 bit. Four half-floats per pixel. RGBA.\n"
		"HDR linear space.",

		"R32f : 32 bit. Float red/luminance channel only.\n"
		"HDR linear space.",

		"R32G32f : 64 bit. Two floats per pixel. RG.\n"
		"HDR linear space.",

		"R32G32B32f : 96 bit. Three floats per pixel. RGB.\n"
		"HDR linear space.",

		"R32G32B32A32f : 128 bit. Four floats per pixel. RGBA.\n"
		"HDR linear space.",

		"R11G11B10uf : 32 bit. Unsigned 11-bit floats for RG, and\n"
		"a 10-bit float for B. All use a 5-bit exponent.",

		"B10G11R11uf : 32 bit. Unsigned 10-bit floats for B, and 11-bit\n"
		"floats for GR. All use a 5-bit exponent.",

		"R9G9B9E5uf : 32 bit. Unsigned 14-bit floats for RGB. Always\n"
		"denorm and each share the same 5-bit exponent.",

		"E5B9G9R9uf : 32 bit. Unsigned 14-bit floats for RGB. Always\n"
		"denorm and each share the same 5-bit exponent.",

		"R8G8B8M8 : 32 bit. Poor-man's HDR. RGB plus shared 8-bit\n"
		"multiplier. Must supply MaxRange when decoding.",

		"R8G8B8D8 : 32 bit. Poor-man's HDR. RGB plus shared 8-bit\n"
		"divisor. Must supply MaxRange when decoding.",

		// Original BC (4x4 Block Compression) formats.
		"BC1DXT1 : BC 1, DXT1. No alpha.",
		"BC1DXT1A :BC 1, DXT1. Binary alpha.",

		"BC2DXT2DXT3 : BC 2. DXT2 (premult-alpha) and DXT3 share the same\n"
		"format. Large alpha gradients (alpha banding).",

		"BC3DXT4DXT5 : BC 3. DXT4 (premult-alpha) and DXT5 share the same\n"
		"format. Variable alpha (smooth).",

		"BC4ATI1U : BC 4. Unsigned. One colour channel only.\n"
		"May not be HW supported.",

		"BC4ATI1S",
		"BC5ATI2U",
		"BC5ATI2S",
		"BC6U",
		"BC6S",
		"BC7",
		"ETC1",
		"ETC2RGB",
		"ETC2RGBA",
		"ETC2RGBA1",
		"EACR11U",
		"EACR11S",
		"EACRG11U",
		"EACRG11S",

		// PVR (Power VR by Imagination) formats.
		"PVRBPP4",
		"PVRBPP2",
		"PVRHDRBPP8",
		"PVRHDRBPP6",
		"PVR2BPP4",
		"PVR2BPP2",
		"PVR2HDRBPP8",
		"PVR2HDRBPP6",

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
		"PAL1BIT",
		"PAL2BIT",
		"PAL3BIT",
		"PAL4BIT",
		"PAL5BIT",
		"PAL6BIT",
		"PAL7BIT",
		"PAL8BIT"
	};
	tStaticAssert(int(tPixelFormat::NumPixelFormats) == tNumElements(PixelFormatDescs));
	const char** PixelFormatDescs_Packed	= &PixelFormatDescs[int(tPixelFormat::FirstPacked)];
	const char** PixelFormatDescs_Block		= &PixelFormatDescs[int(tPixelFormat::FirstBC)];
	const char** PixelFormatDescs_PVR		= &PixelFormatDescs[int(tPixelFormat::FirstPVR)];
	const char** PixelFormatDescs_ASTC		= &PixelFormatDescs[int(tPixelFormat::FirstASTC)];
	const char** PixelFormatDescs_Vendor	= &PixelFormatDescs[int(tPixelFormat::FirstVendor)];
	const char** PixelFormatDescs_Palette	= &PixelFormatDescs[int(tPixelFormat::FirstPalette)];

	int PackedFormat_BitsPerPixel[] =
	{
		8,				// R8
		16,				// R8G8
		24,				// R8G8B8
		32,				// R8G8B8A8
		24,				// B8G8R8
		32,				// B8G8R8A8

		16,				// G3B5R5G3
		16,				// G4B4A4R4
		16,				// B4A4R4G4
		16,				// G3B5A1R5G2
		16,				// G2B5A1R5G3
		16,				// A8L8
		8,				// A8
		8,				// L8
		16,				// R16
		32,				// R16G16
		48,				// R16G16B16
		64,				// R16G16B16A16
		32,				// R32
		64,				// R32G32
		96,				// R32G32B32
		128,			// R32G32B32A32
		16,				// R16f
		32,				// R16G16f
		48,				// R16G16B16f
		64,				// R16G16B16A16f
		32,				// R32f
		64,				// R32G32f
		96,				// R32G32B32f
		128,			// R32G32B32A32f
		32,				// R11G11B10uf
		32,				// B10G11R11uf
		32,				// R9G9B9E5uf
		32,				// E5B9G9R9uf
		32,				// R8G8B8M8
		32				// R8G8B8D8
	};
	tStaticAssert(tNumElements(PackedFormat_BitsPerPixel) == int(tPixelFormat::NumPackedFormats));

	int BCFormat_BytesPerBlock[] =
	{
		8,				// BC1DXT1
		8,				// BC1DXT1A
		16,				// BC2DXT2DXT3
		16,				// BC3DXT4DXT5
		8,				// BC4ATI1U
		8,				// BC4ATI1S
		16,				// BC5ATI2U
		16,				// BC5ATI2S
		16,				// BC6U
		16,				// BC6S
		16,				// BC7
		8,				// ETC1
		8,				// ETC2RGB
		16,				// ETC2RGBA
		8,				// ETC2RGBA1
		8,				// EACR11U
		8,				// EACR11S
		16,				// EACRG11U
		16				// EACRG11S
	};
	tStaticAssert(tNumElements(BCFormat_BytesPerBlock) == int(tPixelFormat::NumBCFormats));

	int PVRFormat_BlockWidth[] =
	{
		4,				// PVRBPP4
		8,				// PVRBPP2
		0,				// PVRHDRBPP8. Unknown block size.
		0,				// PVRHDRBPP6. Unknown block size.
		4,				// PVRBPP4
		8,				// PVRBPP2
		0,				// PVRHDRBPP8. Unknown block size.
		0				// PVRHDRBPP6. Unknown block size.
	};
	tStaticAssert(tNumElements(PVRFormat_BlockWidth) == int(tPixelFormat::NumPVRFormats));

	int PVRFormat_BlockHeight[] =
	{
		4,				// PVRBPP4
		4,				// PVRBPP2
		0,				// PVRHDRBPP8. Unknown block size.
		0,				// PVRHDRBPP6. Unknown block size.
		4,				// PVRBPP4
		4,				// PVRBPP2
		0,				// PVRHDRBPP8. Unknown block size.
		0				// PVRHDRBPP6. Unknown block size.
	};
	tStaticAssert(tNumElements(PVRFormat_BlockHeight) == int(tPixelFormat::NumPVRFormats));

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

	int PVRFormat_BitsPerPixel[] =
	{
		4,				// PVRBPP4
		2,				// PVRBPP2
		8,				// PVRHDRBPP8
		6,				// PVRHDRBPP6
		4,				// PVR2BPP4
		2,				// PVR2BPP2
		8,				// PVR2HDRBPP8
		6,				// PVR2HDRBPP6
	};
	tStaticAssert(tNumElements(PVRFormat_BitsPerPixel) == int(tPixelFormat::NumPVRFormats));

	int VendorFormat_BitsPerPixel[] =
	{
		32,				// Radiance. 3 bytes for each RGB. 1 byte shared exponent.
		128				// OpenEXR. @todo There are multiple exr pixel formats. We don't yet determine which one.
	};
	tStaticAssert(tNumElements(VendorFormat_BitsPerPixel) == int(tPixelFormat::NumVendorFormats));

	int PaletteFormat_BitsPerPixel[] =
	{
		1,
		2,
		3,
		4,
		5,
		6,
		7,
		8
	};
	tStaticAssert(tNumElements(PaletteFormat_BitsPerPixel) == int(tPixelFormat::NumPaletteFormats));
}


int tImage::tGetBlockWidth(tPixelFormat format)
{
	if (tIsPackedFormat(format) || tIsVendorFormat(format) || tIsPaletteFormat(format))
		return 1;

	if (tIsBCFormat(format))
		return 4;

	// ASTC and PVR formats have different widths depending on format.
	if (tIsPVRFormat(format))
		return PVRFormat_BlockWidth[int(format) - int(tPixelFormat::FirstPVR)];

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

	// ASTC and PVR formats have different heights depending on format.
	if (tIsPVRFormat(format))
		return PVRFormat_BlockHeight[int(format) - int(tPixelFormat::FirstPVR)];

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

	if (tIsPVRFormat(format))
		return PVRFormat_BitsPerPixel[int(format) - int(tPixelFormat::FirstPVR)];

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

	if (tIsPVRFormat(format) && tIsLDRFormat(format))
		return 8;

	if (tIsASTCFormat(format))
		return 16;

	if (tIsPackedFormat(format))
		return tGetBitsPerPixel(format) / 8;

	return 0;
}


const char* tImage::tGetPixelFormatName(tPixelFormat fmt)
{
	if (!tIsValidFormat(fmt))
		return "Unknown";
	return PixelFormatNames[int(fmt)];
}


const char* tImage::tGetPixelFormatDesc(tPixelFormat fmt)
{
	if (!tIsValidFormat(fmt))
		return "Unknown";
	return PixelFormatDescs[int(fmt)];
}


tImage::tPixelFormat tImage::tGetPixelFormat(const char* name)
{
	if (!name || (name[0] == '\0'))
		return tPixelFormat::Invalid;

	for (int p = 0; p < int(tPixelFormat::NumPixelFormats); p++)
		if (tStd::tStrcmp(PixelFormatNames[p], name) == 0)
			return tPixelFormat(p);

	return tPixelFormat::Invalid;
}


const char* tImage::tAspectRatioNames[int(tAspectRatio::NumRatios)+1] =
{
	"Free",

	"3 : 1",
	"2 : 1",
	"16 : 9",
	"5 : 3",
	"16 : 10",
	"8 : 5",
	"3 : 2",
	"16 : 11",
	"7 : 5",
	"4 : 3",
	"22 : 17",
	"14 : 11",
	"5 : 4",

	"1 : 1",

	"4 : 5",
	"11 : 14",
	"17 : 22",
	"3 : 4",
	"5 : 7",
	"11 : 16",
	"2 : 3",
	"5 : 8",
	"10 : 16",
	"3 : 5",
	"9 : 16",
	"1 : 2",
	"1 : 3",

	"Print 2x3",		"Print 2x3L",
	"Print 3x5",		"Print 3x5L",
	"Print 4x4",
	"Print 4x6",		"Print 4x6L",
	"Print 5x7",		"Print 5x7L",
	"Print 5x15",		"Print 5x15L",
	"Print 8x8",
	"Print 8x10",		"Print 8x10L",
	"Print 8x24",		"Print 8x24L",
	"Print 8.5x11",		"Print 8.5x11L",
	"Print 9x16",		"Print 9x16L",
	"Print 11x14",		"Print 11x14L",
	"Print 11x16",		"Print 11x16L",
	"Print 12x12",
	"Print 12x18",		"Print 12x18L",
	"Print 12x36",		"Print 12x36L",
	"Print 16x20",		"Print 16x20L",
	"Print 18x24",		"Print 18x24L",
	"Print 20x30",		"Print 20x30L",
	"Print 24x36",		"Print 24x36L",

	"User"
};


tImage::tAspectRatio tImage::tGetReducedAspectRatio(tAspectRatio aspect)
{
	switch (aspect)
	{
		case tAspectRatio::Screen_16_10:	return tAspectRatio::Screen_8_5;
		case tAspectRatio::Screen_10_16:	return tAspectRatio::Screen_5_8;

		case tAspectRatio::Print_2x3:		return tAspectRatio::Screen_2_3;
		case tAspectRatio::Print_2x3_L:		return tAspectRatio::Screen_3_2;
		case tAspectRatio::Print_3x5:		return tAspectRatio::Screen_3_5;
		case tAspectRatio::Print_3x5_L:		return tAspectRatio::Screen_5_3;
		case tAspectRatio::Print_4x4:		return tAspectRatio::Screen_1_1;
		case tAspectRatio::Print_4x6:		return tAspectRatio::Screen_2_3;
		case tAspectRatio::Print_4x6_L:		return tAspectRatio::Screen_3_2;
		case tAspectRatio::Print_5x7:		return tAspectRatio::Screen_5_7;
		case tAspectRatio::Print_5x7_L:		return tAspectRatio::Screen_7_5;
		case tAspectRatio::Print_5x15:		return tAspectRatio::Screen_1_3;
		case tAspectRatio::Print_5x15_L:	return tAspectRatio::Screen_3_1;
		case tAspectRatio::Print_8x8:		return tAspectRatio::Screen_1_1;
		case tAspectRatio::Print_8x10:		return tAspectRatio::Screen_4_5;
		case tAspectRatio::Print_8x10_L:	return tAspectRatio::Screen_5_4;
		case tAspectRatio::Print_8x24:		return tAspectRatio::Screen_1_3;
		case tAspectRatio::Print_8x24_L:	return tAspectRatio::Screen_3_1;
		case tAspectRatio::Print_8p5x11:	return tAspectRatio::Screen_17_22;
		case tAspectRatio::Print_8p5x11_L:	return tAspectRatio::Screen_22_17;
		case tAspectRatio::Print_9x16:		return tAspectRatio::Screen_9_16;
		case tAspectRatio::Print_9x16_L:	return tAspectRatio::Screen_16_9;
		case tAspectRatio::Print_11x14:		return tAspectRatio::Screen_11_14;
		case tAspectRatio::Print_11x14_L:	return tAspectRatio::Screen_14_11;
		case tAspectRatio::Print_11x16:		return tAspectRatio::Screen_11_16;
		case tAspectRatio::Print_11x16_L:	return tAspectRatio::Screen_16_11;
		case tAspectRatio::Print_12x12:		return tAspectRatio::Screen_1_1;
		case tAspectRatio::Print_12x18:		return tAspectRatio::Screen_2_3;
		case tAspectRatio::Print_12x18_L:	return tAspectRatio::Screen_3_2;
		case tAspectRatio::Print_12x36:		return tAspectRatio::Screen_1_3;
		case tAspectRatio::Print_12x36_L:	return tAspectRatio::Screen_3_1;
		case tAspectRatio::Print_16x20:		return tAspectRatio::Screen_4_5;
		case tAspectRatio::Print_16x20_L:	return tAspectRatio::Screen_5_4;
		case tAspectRatio::Print_18x24:		return tAspectRatio::Screen_3_4;
		case tAspectRatio::Print_18x24_L:	return tAspectRatio::Screen_4_3;
		case tAspectRatio::Print_20x30:		return tAspectRatio::Screen_2_3;
		case tAspectRatio::Print_20x30_L:	return tAspectRatio::Screen_3_2;
		case tAspectRatio::Print_24x36:		return tAspectRatio::Screen_2_3;
		case tAspectRatio::Print_24x36_L:	return tAspectRatio::Screen_3_2;

		// Handles no reduction, Invalid, and User.
		default:							return aspect;
	}

	// We'll never get here.
	return tAspectRatio::Invalid;
}


namespace tImage
{
	struct tAspectFrac { int Num; int Den; };
	static tAspectFrac tAspectRatioTable[int(tAspectRatio::NumScreenRatios)] =
	{
		{  3, 1  },
		{  2, 1  },
		{ 16, 9  },
		{  5, 3  },
		{  8, 5  },		// Unused.
		{  8, 5  },
		{  3, 2  },
		{ 16, 11 },
		{  7, 5  },
		{  4, 3  },
		{ 22, 17 },
		{ 14, 11 },
		{  5, 4  },

		{  1, 1  },

		{  4, 5  },
		{ 11, 14 },
		{ 17, 22 },
		{  3, 4  },
		{  5, 7  },
		{ 11, 16 },
		{  2, 3  },
		{  5, 8  },
		{  5, 8  },		// Unused.
		{  3, 5  },
		{  9, 16 },
		{  1, 2  },
		{  1, 3  }
	};
}


bool tImage::tGetAspectRatioFrac(int& numerator, int& denominator, tAspectRatio aspect)
{
	switch (aspect)
	{
		case tAspectRatio::Invalid:
		case tAspectRatio::User:
			numerator = 0;
			denominator = 0;
			return false;
	}
	tReduceAspectRatio(aspect);

	tAspectFrac& frac = tAspectRatioTable[int(aspect)-1];
	numerator = frac.Num;
	denominator = frac.Den;
	return true;
}


tImage::tAspectRatio tImage::tGetAspectRatio(int numerator, int denominator)
{
	if ((numerator <= 0) || (denominator <= 0))
		return tAspectRatio::Invalid;

	// Next we need to reduce the fraction.
	int gcd = tMath::tGreatestCommonDivisor(numerator, denominator);
	numerator /= gcd;
	denominator /= gcd;

	// Now we look for it.
	int foundIndex = -1;
	for (int i = 0; i < int(tAspectRatio::NumScreenRatios); i++)
	{
		tAspectFrac& frac = tAspectRatioTable[i];
		if ((frac.Num == numerator) && (frac.Den == denominator))
		{
			foundIndex = i;
			break;
		}
	}

	if (foundIndex == -1)	
		return tAspectRatio::User;

	return tAspectRatio(foundIndex + 1);
}
