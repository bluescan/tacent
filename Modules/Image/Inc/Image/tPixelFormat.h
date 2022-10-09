// tPixelFormat.h
//
// Pixel formats in Tacent. Not all formats are fully supported. Certainly BC 4, 5, and 7 may not have extensive HW
// support at this time.
//
// Copyright (c) 2004-2006, 2017, 2019, 2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
namespace tImage
{


// Unlike DirectX, which assumes all machines are little-endian, the enumeration below specifies the components in the
// order they appear in memory. To make matters worse, Microsoft's R8G8B8 format is B8R8G8 in memory, but their R5G6B5
// format is correctly the same order in memory. So a little inconsistent there. BC stands for Block Compression.
//
// A note regarding sRGB. We are _not_ indicating via the pixel format what space the colour encoded by the format is
// in. Tacent separates the encoding (the pixel format) from how the encoded data is to be interpreted. This is in
// contrast to all the MS DXGI formats where they effectively at least double the number of formats unnecessarily.
// See tColourSpace enum in tColour.h.
//
// A way to think of it is as follows -- You have some input data (Din) that gets encoded using a pixel format (Epf)
// resulting in some output data (Dout). Din -> Epf -> Dout.
// Without changing Din, if changing Epf would result in different Dout, it is correct to have separate formats (eg.
// BCH6_S vs BCH6_U. DXT1 vs DXT1BA). If changing Epf would not result in different Dout then the formats are not
// different and satellite info should be used if what's stored in Din (and Dout) has certain properties (eg. sRGB space
// vs Linear, premultiplied vs not, DXT2 and DXT3 are the same).
enum class tPixelFormat
{
	Invalid				= -1,
	Auto				= Invalid,

	FirstNormal,
	R8G8B8				= FirstNormal,	// 24  bit. Full colour. No alpha. Matches GL_RGB source ordering. Not efficient. Most drivers will swizzle to BGR.
	R8G8B8A8,							// 32  bit. Full alpha. Matches GL_RGBA source ordering. Not efficient. Most drivers will swizzle to ABGR.
	B8G8R8,								// 24  bit. Full colour. No alpha. Matches GL_BGR source ordering. Efficient. Most drivers do not need to swizzle.
	B8G8R8A8,							// 32  bit. Full alpha. Matches GL_BGRA source ordering. Most drivers do not need to swizzle.
	B5G6R5,								// 16  bit. No alpha. The truth is in memory this is actually G3B5R5G3, but no-one calls it that.
	B4G4R4A4,							// 16  bit. 12 colour bits. 4 bit alpha.
	B5G5R5A1,							// 16  bit. 15 colour bits. Binary alpha.
	A8L8,								// 16  bit. Alpha and Luminance.
	A8,									// 8   bit. Alpha only.
	L8,									// 8   bit. Luminance only.

	R16F,								// 16  bit. Half-float red/luminance channel only.
	R16G16F,							// 32  bit. Two half-floats per pixel. Red and green.
	R16G16B16A16F,						// 64  bit. Four half-floats per pixel. RGBA.
	R32F,								// 32  bit. Float red/luminance channel only.
	R32G32F,							// 64  bit. Two floats per pixel. Red and green.
	R32G32B32A32F,						// 128 bit. HDR format (linear-space), RGBA in 4 floats.
	LastNormal			= R32G32B32A32F,

	FirstBlock,
	BC1DXT1				= FirstBlock,	// BC 1, DXT1. No alpha.
	BC1DXT1A,							// BC 1, DXT1. Binary alpha.
	BC2DXT2DXT3,						// BC 2, DXT2 (premult-alpha) and DXT3 share the same format. Large alpha gradients (alpha banding).
	BC3DXT4DXT5,						// BC 3, DXT4 (premult-alpha) and DXT5 share the same format. Variable alpha (smooth).
	BC4ATI1,							// BC 4. One colour channel only. May not be HW supported.
	BC5ATI2,							// BC 5. Two colour channels only. May not be HW supported.
	BC6S,								// BC 6 HDR. No alpha. 3 x 16bit signed half-floats per pixel.
	BC6U,								// BC 6 HDR. No alpha. 3 x 16bit unsigned half-floats per pixel.
	BC7,								// BC 7. Full colour. Variable alpha 0 to 8 bits.
	LastBlock			= BC7,

	FirstVendor,
	RADIANCE			= FirstVendor,	// Radiance HDR.
	OPENEXR,							// OpenEXR HDR.
	LastVendor			= OPENEXR,

	FirstPAL,
	PAL8BIT				= FirstPAL,		// 8bit indexes to a Palette. ex. gif files.
	PAL4BIT,
	PAL1BIT,
	LastPAL				= PAL1BIT,

	NumPixelFormats,
	NumNormalFormats	= LastNormal - FirstNormal + 1,
	NumBlockFormats		= LastBlock - FirstBlock + 1,
	NumVendorFormats	= LastVendor - FirstVendor + 1,
	NumPALFormats		= LastPAL - FirstPAL + 1
};


bool tIsNormalFormat(tPixelFormat);
bool tIsBlockCompressedFormat(tPixelFormat);
bool tIsVendorFormat(tPixelFormat);
bool tIsPaletteFormat(tPixelFormat);
bool tIsAlphaFormat(tPixelFormat);
bool tIsOpaqueFormat(tPixelFormat);
bool tIsHDRFormat(tPixelFormat);
int tGetBitsPerPixel(tPixelFormat);				// Some formats (dxt1) are only half a byte per pixel, so we report bits.
int tGetBytesPer4x4PixelBlock(tPixelFormat);	// This function must be given a BC pixel format.
const char* tGetPixelFormatName(tPixelFormat);
tPixelFormat tGetPixelFormat(const char* name);	// Gets the pixel format from its name. Case insensitive. Slow. Use for testing only.


}


// Implementation below this line.


inline bool tImage::tIsNormalFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstNormal) && (format <= tPixelFormat::LastNormal))
		return true;

	return false;
}


inline bool tImage::tIsBlockCompressedFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstBlock) && (format <= tPixelFormat::LastBlock))
		return true;

	return false;
}


inline bool tImage::tIsVendorFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstVendor) && (format <= tPixelFormat::LastVendor))
		return true;

	return false;
}


inline bool tImage::tIsPaletteFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstPAL) && (format <= tPixelFormat::LastPAL))
		return true;

	return false;
}


inline bool tImage::tIsAlphaFormat(tPixelFormat format)
{
	switch (format)
	{
		case tPixelFormat::R8G8B8A8:
		case tPixelFormat::B8G8R8A8:
		case tPixelFormat::B4G4R4A4:
		case tPixelFormat::B5G5R5A1:
		case tPixelFormat::A8L8:
		case tPixelFormat::R16G16B16A16F:
		case tPixelFormat::R32G32B32A32F:
		case tPixelFormat::BC1DXT1A:
		case tPixelFormat::BC2DXT2DXT3:
		case tPixelFormat::BC3DXT4DXT5:
		case tPixelFormat::BC7:
		case tPixelFormat::OPENEXR:

		// For palettized the palette may have an entry that can be considered alpha. However for only 1-bit
		// palettes we consider it dithered (ColourA/ColourB) and not to have an alpha.
		case tPixelFormat::PAL8BIT:
		case tPixelFormat::PAL4BIT:
			return true;
	}

	return false;
}


inline bool tImage::tIsOpaqueFormat(tPixelFormat format)
{
	return !tImage::tIsAlphaFormat(format);
}


inline bool tImage::tIsHDRFormat(tPixelFormat format)
{
	switch (format)
	{
		case tPixelFormat::R16F:
		case tPixelFormat::R16G16F:
		case tPixelFormat::R16G16B16A16F:
		case tPixelFormat::R32F:
		case tPixelFormat::R32G32F:
		case tPixelFormat::R32G32B32A32F:
		case tPixelFormat::BC6S:
		case tPixelFormat::BC6U:
		case tPixelFormat::RADIANCE:
		case tPixelFormat::OPENEXR:
			return true;
	}

	return false;
}
