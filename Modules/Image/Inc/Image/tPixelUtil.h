// tPixelUtil.h
//
// Helper functions for manipulating and parsing pixel-data in packed and compressed block formats.
//
// Copyright (c) 2022, 2023 Tristan Grimmer.
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
#include <Image/tPixelFormat.h>
namespace tImage
{


// These BC blocks are needed so that the tImageDDS class can re-order the rows by messing with each block's lookup
// table and alpha tables. This is because DDS files have the rows of their textures upside down (texture origin in
// OpenGL is lower left, while in DirectX it is upper left). See: http://en.wikipedia.org/wiki/S3_Texture_Compression
#pragma pack(push, 1)

// The BC1 block is used for both DXT1 and DXT1 with binary alpha. It's also used as the colour information block in the
// DXT 2, 3, 4 and 5 formats. Size is 64 bits.
struct BC1Block
{
	uint16 Colour0;								// R5G6B5
	uint16 Colour1;								// R5G6B5
	uint8 LookupTableRows[4];
};

// The BC2 block is the same for DXT2 and DXT3, although we don't support 2 (premultiplied alpha). Size is 128 bits.
struct BC2Block
{
	uint16 AlphaTableRows[4];					// Each alpha is 4 bits.
	BC1Block ColourBlock;
};

// The BC3 block is the same for DXT4 and 5, although we don't support 4 (premultiplied alpha). Size is 128 bits.
struct BC3Block
{
	uint8 Alpha0;
	uint8 Alpha1;
	uint8 AlphaTable[6];						// Each of the 4x4 pixel entries is 3 bits.
	BC1Block ColourBlock;

	// These accessors are needed because of the unusual alignment of the 3bit alpha indexes. They each return or set a
	// value in [0, 2^12) which represents a single row. The row variable should be in [0, 3]
	uint16 GetAlphaRow(int row);
	void SetAlphaRow(int row, uint16 val);

};
#pragma pack(pop)


constexpr uint32 FourCC(uint8 ch0, uint8 ch1, uint8 ch2, uint8 ch3);
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
