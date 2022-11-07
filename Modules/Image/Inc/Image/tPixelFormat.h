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
// resulting in some output data (Dout). Din -> Epf -> Dout. Without changing Din, if changing Epf would result in
// different Dout, it is correct to have separate formats (eg. BCH6_S vs BCH6_U. DXT1 vs DXT1BA). If changing Epf would
// not result in different Dout then the formats are not different and satellite info should be used if what's stored in
// Din (and Dout) has certain properties (eg. sRGB space vs Linear, premultiplied vs not, DXT2 and DXT3 are the same).
// This is also why we don't distinguish between UNORM and UINT for example, as this is just a runtime distinction, not
// an encoding difference (UNORM gets converted to a float in [0.0, 1.0] in shaders, UINT doesn't).
//
// The only exception to this rule is the Tacent pixel format _does_ make distinctions between formats based on the
// colour components being represented. It's not ideal, but pixel formats do generally specify R, G, B, A, L etc and
// what order they appear in. In a perfect world (in my perfect world anyways), R8G8B8 would just be C8C8C8 (C8X3) and
// satellite info would describe what the data represented (RGB in this case). Anyway, that's too much of a divergence.
// This exception is why there is a tPixelFormat R8 (Vulkan has one of these), A8, and L8, all 3 with the same internal
// representation.
enum class tPixelFormat
{
	Invalid				= -1,
	Auto				= Invalid,

	FirstPacked,
	R8					= FirstPacked,	// 8   bit. Unsigned representing red. Some file-types not supporting A8 or L8 (eg ktx2) will export to this.
	R8G8,								// 16  bit. Unsigned representing red and green. Vulkan has an analagous format.
	R8G8B8,								// 24  bit. Full colour. No alpha. Matches GL_RGB source ordering. Not efficient. Most drivers will swizzle to BGR.
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
	LastPacked			= R32G32B32A32F,

	FirstBC,
	BC1DXT1				= FirstBC,		// BC 1, DXT1. No alpha.
	BC1DXT1A,							// BC 1, DXT1. Binary alpha.
	BC2DXT2DXT3,						// BC 2, DXT2 (premult-alpha) and DXT3 share the same format. Large alpha gradients (alpha banding).
	BC3DXT4DXT5,						// BC 3, DXT4 (premult-alpha) and DXT5 share the same format. Variable alpha (smooth).
	BC4ATI1,							// BC 4. One colour channel only. May not be HW supported.
	BC5ATI2,							// BC 5. Two colour channels only. May not be HW supported.
	BC6S,								// BC 6 HDR. No alpha. 3 x 16bit signed half-floats per pixel.
	BC6U,								// BC 6 HDR. No alpha. 3 x 16bit unsigned half-floats per pixel.
	BC7,								// BC 7. Full colour. Variable alpha 0 to 8 bits.
	LastBC				= BC7,

	FirstASTC,
	ASTC4X4				= FirstASTC,	// 128 bits per 16  pixels. 8    bpp. LDR UNORM.
	ASTC5X4,							// 128 bits per 20  pixels. 6.4  bpp. LDR UNORM.
	ASTC5X5,							// 128 bits per 25  pixels. 5.12 bpp. LDR UNORM.
	ASTC6X5,							// 128 bits per 30  pixels. 4.27 bpp. LDR UNORM.
	ASTC6X6,							// 128 bits per 36  pixels. 3.56 bpp. LDR UNORM.
	ASTC8X5,							// 128 bits per 40  pixels. 3.2  bpp. LDR UNORM.
	ASTC8X6,							// 128 bits per 48  pixels. 2.67 bpp. LDR UNORM.
	ASTC8X8,							// 128 bits per 64  pixels. 2.56 bpp. LDR UNORM.
	ASTC10X5,							// 128 bits per 50  pixels. 2.13 bpp. LDR UNORM.
	ASTC10X6,							// 128 bits per 60  pixels. 2    bpp. LDR UNORM.
	ASTC10X8,							// 128 bits per 80  pixels. 1.6  bpp. LDR UNORM.
	ASTC10X10,							// 128 bits per 100 pixels. 1.28 bpp. LDR UNORM.
	ASTC12X10,							// 128 bits per 120 pixels. 1.07 bpp. LDR UNORM.
	ASTC12X12,							// 128 bits per 144 pixels. 0.89 bpp. LDR UNORM.
	LastASTC			= ASTC12X12,

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
	NumPackedFormats	= LastPacked	- FirstPacked	+ 1,
	NumBCFormats		= LastBC		- FirstBC		+ 1,
	NumASTCFormats		= LastASTC		- FirstASTC		+ 1,
	NumVendorFormats	= LastVendor	- FirstVendor	+ 1,
	NumPALFormats		= LastPAL		- FirstPAL		+ 1
};


bool tIsPackedFormat	(tPixelFormat);				// Simple RGB and RGBA formats with different numbers of bits per component and different orderings.
bool tIsBCFormat		(tPixelFormat);				// Is the format an original 4x4 BC (Block Compression) format. These 4x4 blocks use various numbers of bits per block.
bool tIsASTCFormat		(tPixelFormat);				// Is it one of the ASTC (Adaptive Scalable Texture Compression) block formats. Block sizes are avail from 4x4 up to 12x12.
bool tIsVendorFormat	(tPixelFormat);
bool tIsPaletteFormat	(tPixelFormat);
bool tIsAlphaFormat		(tPixelFormat);
bool tIsOpaqueFormat	(tPixelFormat);
bool tSupportsHDR		(tPixelFormat);
bool tIsLuminanceFormat	(tPixelFormat);				// Single-channel luminance formats. Includes red-only formats. Does not include alpha only.

// Gets the pixel width/height of the block size specified by the pixel-format. BC blocks are all 4x4. ASTC blocks have
// varying width/height depending on specific ASTC format. Packed, Vendor, and Palette formats return 1 for width and
// height. Invalid pixel-formats return 0.
int tGetBlockWidth		(tPixelFormat);
int tGetBlockHeight		(tPixelFormat);

// Given a block=width or block-height and how may pixels you need to store (image-width or image-height), returns the
// number of blocks you will need in that dimension.
int tGetNumBlocks		(int blockWH, int imageWH);

// Only applies to formats that can guarantee an integer number of bits per pixel. In particular does not apply to ASTC
// formats (even if the particular ASTC format has an integer number of bits per pixel). We report in bits (not bytes)
// because some formats (i.e. BC1) are only half a byte per pixel. Returns -1 for non-integral bpp formats and all ASTC
// formats.
int tGetBitsPerPixel(tPixelFormat);

// This function must be given a BC format, an ASTC format, or a packed format.
// BC formats		: 4x4 with different number of bytes per block.
// ASTC formats		: Varying MxN but always 16 bytes.
// Packed Formats	: Considered 1x1 with varying number of bytes per pixel.
// Returns -1 otherwise.
int tGetBytesPerBlock(tPixelFormat);

const char* tGetPixelFormatName(tPixelFormat);

// Gets the pixel format from its name. Case insensitive. Slow. Use for testing/unit-tests only.
tPixelFormat tGetPixelFormat(const char* name);


}


// Implementation below this line.


inline bool tImage::tIsPackedFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstPacked) && (format <= tPixelFormat::LastPacked))
		return true;

	return false;
}


inline bool tImage::tIsBCFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstBC) && (format <= tPixelFormat::LastBC))
		return true;

	return false;
}


inline bool tImage::tIsASTCFormat(tPixelFormat format)
{
	if ((format >= tPixelFormat::FirstASTC) && (format <= tPixelFormat::LastASTC))
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

	// Not quite sure how to hadle ASTC formats, but they usually contain an alpha.
	if (tIsASTCFormat(format))
		return true;

	return false;
}


inline bool tImage::tIsOpaqueFormat(tPixelFormat format)
{
	return !tImage::tIsAlphaFormat(format);
}


inline bool tImage::tSupportsHDR(tPixelFormat format)
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

		// These can be LDR or HDR.
		case tPixelFormat::ASTC4X4:
		case tPixelFormat::ASTC5X4:
		case tPixelFormat::ASTC5X5:
		case tPixelFormat::ASTC6X5:
		case tPixelFormat::ASTC6X6:
		case tPixelFormat::ASTC8X5:
		case tPixelFormat::ASTC8X6:
		case tPixelFormat::ASTC8X8:
		case tPixelFormat::ASTC10X5:
		case tPixelFormat::ASTC10X6:
		case tPixelFormat::ASTC10X8:
		case tPixelFormat::ASTC10X10:
		case tPixelFormat::ASTC12X10:
		case tPixelFormat::ASTC12X12:
			return true;
	}

	return false;
}


inline bool tImage::tIsLuminanceFormat(tPixelFormat format)
{
	switch (format)
	{
		case tPixelFormat::L8:
		case tPixelFormat::R16F:
		case tPixelFormat::R32F:
			return true;
	}

	return false;
}


inline int tImage::tGetNumBlocks(int blockWH, int imageWH)
{
	tAssert(blockWH > 0);
	return (imageWH + blockWH - 1) / blockWH;
}
