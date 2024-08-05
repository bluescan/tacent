// tLayer.h
//
// A tLayer is a data container for texture pixel data. It is used only by the tTexture class to store image data. It
// contains data that may be in a variety of hardware-ready formats, like dxt5. It is primarily used to store the
// multiple mipmap layers of a texture. Main members are width, height, pixel format, and a function to compute data
// size based on those three variables. It knows how to save and load itself in tChunk format.
//
// tLayers may have any width and height in [1, MaxLayerDimension]. If the pixel format is block-based (4x4 pixels)
// the tLayer still allows smaller than 4 width and height. However, a whole block is still needed so the number of
// bytes will be the block size for the particular BC format. For example, a 1x1 BC1 format layer still needs 8 bytes.
// A 5x5 BC format layer would need 4 blocks (same as an 8x8). The tLayer does not place further constraints on width
// and height. A higher level system, for example, may want to ensure power-of-two sizes, or multiple of 4, but that
// shouldn't and doesn't happen here.
//
// Copyright (c) 2006, 2017, 2022, 2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Foundation/tList.h>
#include <System/tChunk.h>
#include <Image/tPixelFormat.h>
namespace tImage
{


class tLayer : public tLink<tLayer>
{
public:
	tLayer()																											{ }

	// Constructs the layer from the chunk. Normally it copies the layer data from the chunk. However, if ownData is
	// false no mem-copy is performed and the layer just points into the chunk data. In this case it is vital that the
	// chunk object exists for the entire lifespan of the layer.
	tLayer(const tChunk& chunk, bool ownsData = true)																	{ Load(chunk, ownsData); }

	// Constructs a layer having the supplied width, height, and number of bytes. If steal memory is true, the data
	// array is passed off to this class and it will manage the array deleting it when necessary. If steal is false,
	// the data gets mem-copied into this object.
	tLayer(tPixelFormat fmt, int width, int height, uint8* data, bool steal = false)									{ Set(fmt, width, height, data, steal); }

	tLayer(const tLayer& src)																							{ Set(src); }
	virtual ~tLayer()																									{ Clear(); }

	bool IsValid() const																								{ return Data ? true : false; }

	void Set(tPixelFormat format, int width, int height, uint8* data, bool steal = false);
	void Set(const tLayer& layer);

	// Returns the size of the data in bytes by reading the Width, Height, and PixelFormat. For block-compressed format
	// the data size will be a multiple of the block size in bytes. BC 4x4 blocks may be different sizes, whereas ASTC
	// block size is always 16 bytes. eg. a 1x1 BC1 format layer still needs 8 bytes. A 5x5 BC1 format layer would need
	// a whole 4 blocks (same as an 8x8) and would yield 32 bytes.
	int GetDataSize() const;

	void Save(tChunkWriter&) const;

	// Loads the layer from a chunk. Any previous layer data gets destroyed. The bool ownData works the same as with
	// the constructor.
	void Load(const tChunk&, bool ownData);

	// Frees internal layer data and makes the layer invalid.
	void Clear()																										{ PixelFormat = tPixelFormat::Invalid; Width = Height = 0; if (OwnsData) delete[] Data; Data = nullptr; OwnsData = true; }

	// This just checks the pixel format to see if it supports alpha. It does NOT check the data.
	bool IsOpaqueFormat() const																							{ return tImage::tIsOpaqueFormat(PixelFormat); }

	uint8* StealData()																									{ if (!OwnsData) return nullptr; OwnsData = false; return Data; }

	// An invalid layer is never considered equal to another layer even if the other layer is also invalid. Whether the
	// layer owns the data is considered irrelevant for equivalency purposes.
	bool operator==(const tLayer&) const;
	bool operator!=(const tLayer& src) const																			{ return !(*this == src); }

	// The following functions are a convenience if the layer pixel format happens to be R8G8B8A8. For example,
	// block compressed images may have been told to decompress in which case the layers they contain will be R8G8B8A8.
	int GetIndex(int x, int y) const																					{ tAssert((x >= 0) && (y >= 0) && (x < Width) && (y < Height) && (PixelFormat == tPixelFormat::R8G8B8A8)); return y * Width + x; }
	tPixel4b GetPixel(int x, int y) const																				{ return ((tPixel4b*)Data)[ GetIndex(x, y) ]; }

	tPixelFormat PixelFormat	= tPixelFormat::Invalid;
	int Width					= 0;
	int Height					= 0;
	uint8* Data					= nullptr;
	bool OwnsData				= true;

	// 4096 x 4096 is pretty much a minimum requirement these days. 16Kx16k has good support. 32kx32k exists.
	const static int MaxLayerDimension = 32768;
	const static int MinLayerDimension = 1;
};


// Implementation below this line.


inline void tLayer::Set(tPixelFormat format, int width, int height, uint8* data, bool steal)
{
	Clear();
	if (!width || !height || !data)
		return;

	PixelFormat		= format;
	Width			= width;
	Height			= height;
	Data			= nullptr;
	OwnsData		= true;

	if (steal)
	{
		Data = data;
	}
	else
	{
		int dataSize = GetDataSize();
		Data = new uint8[dataSize];
		tStd::tMemcpy(Data, data, dataSize);
	}
}


inline void tLayer::Set(const tLayer& layer)
{
	if (&layer == this)
		return;

	PixelFormat		= layer.PixelFormat;
	Width			= layer.Width;
	Height			= layer.Height;
	OwnsData		= layer.OwnsData;

	if (OwnsData)
	{
		int dataSize = layer.GetDataSize();
		Data = new uint8[dataSize];
		tStd::tMemcpy(Data, layer.Data, dataSize);
	}
	else
	{
		Data = layer.Data;
	}
}


inline int tLayer::GetDataSize() const
{
	if (!Width || !Height || (PixelFormat == tPixelFormat::Invalid))
		return 0;

	// Non-block-compressed textures are considered as having a single pixel per block.
	int blockW = tGetBlockWidth(PixelFormat);
	int blockH = tGetBlockHeight(PixelFormat);
	tAssert((blockW > 0) && (blockH > 0));

	int numBlocks = tGetNumBlocks(blockW, Width) * tGetNumBlocks(blockH, Height);
	int bytesPerBlock = 0;

	// tGetBytesPerBlock _could_ also handle packed formats but for palettized formats
	// I think we still need to use tGetBitsPerPixel.
	if (tIsBCFormat(PixelFormat) || tIsASTCFormat(PixelFormat) || tIsPVRFormat(PixelFormat))
		bytesPerBlock = tGetBytesPerBlock(PixelFormat);
	else
		bytesPerBlock = tGetBitsPerPixel(PixelFormat) >> 3;

	// @todo I think we're currently in trouble here if we call tGetBitsPerPixel with palettized that
	// returns a non multiple of 8. The code below attempts to deal with this... but should be revisited.
	// It basically works out the total bits, and makes sure we have enough bytes to store them all even
	// if that total is not divisible by 8.
	if (bytesPerBlock == 0)
	{
		int totalBits = tGetBitsPerPixel(PixelFormat) * numBlocks;
		int totalBytesCeil = (totalBits/8) + ((totalBits%8) ? 1 : 0);
		return totalBytesCeil;
	}

	return numBlocks * bytesPerBlock;
}


}
