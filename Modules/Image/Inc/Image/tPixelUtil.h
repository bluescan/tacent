// tPixelUtil.h
//
// Helper functions for manipulating and parsing pixel-data in packed and compressed block formats.
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
#include <Foundation/tPlatform.h>
#include <Math/tColour.h>
#include <Image/tPixelFormat.h>
namespace tImage
{


// Given a pixel-format, pixel-data, and dimensions, this function decodes it into either an LDR buffer of tColour4b's
// or an HDR buffer of tColour4f's. If Success returned, the function populates a width*height array of tColours in
// either the dstLDR or dstHRD pointers you passed in. These decode buffers are now owned by you and you must delete[]
// them. Depending on the pixel format, either LDR or HDR buffers will be populated, but not both. For safety, this
// function expects dstLDR and dstHRD to be set to nullptr when you call. If they're not it returns BufferNotClear and
// leaves the dst buffer pointers unmodified. If you know the colour-space of the pixelData, pass it in. This is used
// by the ASTC decoder. @todo Support decode of PAL formats.
enum class DecodeResult
{
	Success,		// Must be 0.
	BuffersNotClear,
	UnsupportedFormat,
	InvalidInput,
	PackedDecodeError,
	BlockDecodeError,
	ASTCDecodeError,
	PVRDecodeError
};
DecodeResult DecodePixelData
(
	tPixelFormat, const uint8* data, int dataSize, int width, int height,
	tColour4b*& dstLDR, tColour4f*& dstHDR,
	tColourProfile = tColourProfile::Auto,		// Only used for ASTC decodes.
	float RGBM_RGBD_MaxRange = 8.0f				// Only used or RGBM and RGBD decodes.
);


// These do the same as above except for a subset of pixel formats. The above function ends up calling one of these.
DecodeResult DecodePixelData_Packed	(tPixelFormat, const uint8* data, int dataSize, int w, int h, tColour4b*&, tColour4f*&, float RGBM_RGBD_MaxRange = 8.0f);
DecodeResult DecodePixelData_Block	(tPixelFormat, const uint8* data, int dataSize, int w, int h, tColour4b*&, tColour4f*&);
DecodeResult DecodePixelData_ASTC	(tPixelFormat, const uint8* data, int dataSize, int w, int h, tColour4f*&, tColourProfile = tColourProfile::Auto);
DecodeResult DecodePixelData_PVR	(tPixelFormat, const uint8* data, int dataSize, int w, int h, tColour4b*&, tColour4f*&);


constexpr uint32 FourCC(uint8 ch0, uint8 ch1, uint8 ch2, uint8 ch3);


// These BC blocks are needed so that any tImage class that supports BC1 can re-order the rows by messing with each
// block's lookup table and alpha tables. This is because files have the rows of their textures upside down (texture
// origin in OpenGL is lower left, while in DirectX it is upper left). See: http://en.wikipedia.org/wiki/S3_Texture_Compression
// The BC1 block is used for both DXT1 and DXT1 with binary alpha. It's also used as the colour information block in the
// DXT 2, 3, 4 and 5 formats. Size is 64 bits.
#pragma pack(push, 1)
struct BC1Block
{
	uint16 Colour0;								// R5G6B5
	uint16 Colour1;								// R5G6B5
	uint8 LookupTableRows[4];
};
#pragma pack(pop)


bool DoBC1BlocksHaveBinaryAlpha(BC1Block* blocks, int numBlocks);


// Determine if row-reversal will succeed based on the pixel format and height.
bool CanReverseRowData(tPixelFormat, int height);


// This also works for packed formats which are considered to have a block width and height of 1.
uint8* CreateReversedRowData(const uint8* pixelData, tPixelFormat pixelDataFormat, int numBlocksW, int numBlocksH);


}


// Implementation only below this line.


inline constexpr uint32 tImage::FourCC(uint8 ch0, uint8 ch1, uint8 ch2, uint8 ch3)
{
	return (uint32(ch0) | (uint32(ch1) << 8) | (uint32(ch2) << 16) | (uint32(ch3) << 24));
}
