// tPixelUtil.cpp
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

#include <Foundation/tAssert.h>
#include <Foundation/tStandard.h>
#include "Image/tPixelUtil.h"
#define BCDEC_IMPLEMENTATION
#include "bcdec/bcdec.h"
#define ETCDEC_IMPLEMENTATION
#include "etcdec/etcdec.h"


namespace tImage
{
	bool CanReverseRowData_Packed		(tPixelFormat);
	bool CanReverseRowData_BC			(tPixelFormat, int height);

	uint8* CreateReversedRowData_Packed	(const uint8* pixelData, tPixelFormat pixelDataFormat, int width, int height);
	uint8* CreateReversedRowData_BC		(const uint8* pixelData, tPixelFormat pixelDataFormat, int numBlocksW, int numBlocksH);
}


uint16 tImage::BC3Block::GetAlphaRow(int row)
{
	tAssert(row < 4);
	switch (row)
	{
		case 1:
			return (AlphaTable[2] << 4) | (0x0F & (AlphaTable[1] >> 4));

		case 0:
			return ((AlphaTable[1] & 0x0F) << 8) | AlphaTable[0];

		case 3:
			return (AlphaTable[5] << 4) | (0x0F & (AlphaTable[4] >> 4));

		case 2:
			return ((AlphaTable[4] & 0x0F) << 8) | AlphaTable[3];
	}
	return 0;
}


void tImage::BC3Block::SetAlphaRow(int row, uint16 val)
{
	tAssert(row < 4);
	tAssert(val < 4096);
	switch (row)
	{
		case 1:
			AlphaTable[2] = val >> 4;
			AlphaTable[1] = (AlphaTable[1] & 0x0F) | ((val & 0x000F) << 4);
			break;

		case 0:
			AlphaTable[1] = (AlphaTable[1] & 0xF0) | (val >> 8);
			AlphaTable[0] = val & 0x00FF;
			break;

		case 3:
			AlphaTable[5] = val >> 4;
			AlphaTable[4] = (AlphaTable[4] & 0x0F) | ((val & 0x000F) << 4);
			break;

		case 2:
			AlphaTable[4] = (AlphaTable[4] & 0xF0) | (val >> 8);
			AlphaTable[3] = val & 0x00FF;
			break;
	}
}


bool tImage::DoBC1BlocksHaveBinaryAlpha(tImage::BC1Block* block, int numBlocks)
{
	// The only way to check if the DXT1 format has alpha is by checking each block individually. If the block uses
	// alpha, the min and max colours are ordered in a particular order.
	for (int b = 0; b < numBlocks; b++)
	{
		if (block->Colour0 <= block->Colour1)
		{
			// It seems that at least the nVidia DXT compressor can generate an opaque DXT1 block with the colours in the order for a transparent one.
			// This forces us to check all the indexes to see if the alpha index (11 in binary) is used -- if not then it's still an opaque block.
			for (int row = 0; row < 4; row++)
			{
				uint8 bits = block->LookupTableRows[row];
				if
				(
					((bits & 0x03) == 0x03) ||
					((bits & 0x0C) == 0x0C) ||
					((bits & 0x30) == 0x30) ||
					((bits & 0xC0) == 0xC0)
				)
				{
					return true;
				}
			}
		}

		block++;
	}

	return false;
}


bool tImage::CanReverseRowData(tPixelFormat format, int height)
{
	if (tIsPackedFormat(format))
		return CanReverseRowData_Packed(format);

	if (tIsBCFormat(format))
		return CanReverseRowData_BC(format, height);

	return false;
}


uint8* tImage::CreateReversedRowData(const uint8* pixelData, tPixelFormat pixelDataFormat, int numBlocksW, int numBlocksH)
{
	if (tIsPackedFormat(pixelDataFormat))
		return CreateReversedRowData_Packed(pixelData, pixelDataFormat, numBlocksW, numBlocksH);

	if (tIsBCFormat(pixelDataFormat))
		return CreateReversedRowData_BC(pixelData, pixelDataFormat, numBlocksW, numBlocksH);

	return nullptr;
}


bool tImage::CanReverseRowData_Packed(tPixelFormat format)
{
	int bitsPerPixel = tImage::tGetBitsPerPixel(format);
	if ((bitsPerPixel % 8) == 0)
		return true;
}


uint8* tImage::CreateReversedRowData_Packed(const uint8* pixelData, tPixelFormat pixelDataFormat, int width, int height)
{
	// We only support pixel formats that contain a whole number of bytes per pixel.
	// That will cover all reasonable RGB and RGBA formats, but not ASTC formats.
	if (!CanReverseRowData_Packed(pixelDataFormat))
		return nullptr;

	int bitsPerPixel = tImage::tGetBitsPerPixel(pixelDataFormat);
	int bytesPerPixel = bitsPerPixel/8;
	int numBytes = width*height*bytesPerPixel;

	uint8* reversedPixelData = new uint8[numBytes];
	uint8* dstData = reversedPixelData;
	for (int row = height-1; row >= 0; row--)
	{
		for (int col = 0; col < width; col++)
		{
			const uint8* srcData = pixelData + row*bytesPerPixel*width + col*bytesPerPixel;
			for (int byte = 0; byte < bytesPerPixel; byte++, dstData++, srcData++)
				*dstData = *srcData;
		}
	}
	return reversedPixelData;
}


bool tImage::CanReverseRowData_BC(tPixelFormat format, int height)
{
	switch (format)
	{
		case tPixelFormat::BC1DXT1A:
		case tPixelFormat::BC1DXT1:
		case tPixelFormat::BC2DXT2DXT3:
		case tPixelFormat::BC3DXT4DXT5:
			if ((height % tGetBlockHeight(format)) == 0)
				return true;
			break;
	}

	return false;
}


uint8* tImage::CreateReversedRowData_BC(const uint8* pixelData, tPixelFormat pixelDataFormat, int numBlocksW, int numBlocksH)
{
	// We do not support all BC formats for this..
	if (!CanReverseRowData_BC(pixelDataFormat, numBlocksH*tGetBlockHeight(pixelDataFormat)))
		return nullptr;

	int bcBlockSize = tImage::tGetBytesPerBlock(pixelDataFormat);
	int numBlocks = numBlocksW*numBlocksH;
	int numBytes = numBlocks * bcBlockSize;

	uint8* reversedPixelData = new uint8[numBytes];
	uint8* dstData = reversedPixelData;
	for (int row = numBlocksH-1; row >= 0; row--)
	{
		for (int col = 0; col < numBlocksW; col++)
		{
			const uint8* srcData = pixelData + row*bcBlockSize*numBlocksW + col*bcBlockSize;
			for (int byte = 0; byte < bcBlockSize; byte++, dstData++, srcData++)
				*dstData = *srcData;
		}
	}

	// Now we flip the inter-block rows by messing with the block's lookup-table.  We handle three types of
	// blocks: BC1, BC2, and BC3. BC4/5 probably could be handled, and BC6/7 are too complex.
	switch (pixelDataFormat)
	{
		case tPixelFormat::BC1DXT1A:
		case tPixelFormat::BC1DXT1:
		{
			tImage::BC1Block* block = (tImage::BC1Block*)reversedPixelData;
			for (int b = 0; b < numBlocks; b++, block++)
			{
				// Reorder each row's colour indexes.
				tStd::tSwap(block->LookupTableRows[0], block->LookupTableRows[3]);
				tStd::tSwap(block->LookupTableRows[1], block->LookupTableRows[2]);
			}
			break;
		}

		case tPixelFormat::BC2DXT2DXT3:
		{
			tImage::BC2Block* block = (tImage::BC2Block*)reversedPixelData;
			for (int b = 0; b < numBlocks; b++, block++)
			{
				// Reorder the explicit alphas AND the colour indexes.
				tStd::tSwap(block->AlphaTableRows[0], block->AlphaTableRows[3]);
				tStd::tSwap(block->AlphaTableRows[1], block->AlphaTableRows[2]);
				tStd::tSwap(block->ColourBlock.LookupTableRows[0], block->ColourBlock.LookupTableRows[3]);
				tStd::tSwap(block->ColourBlock.LookupTableRows[1], block->ColourBlock.LookupTableRows[2]);
			}
			break;
		}

		case tPixelFormat::BC3DXT4DXT5:
		{
			tImage::BC3Block* block = (tImage::BC3Block*)reversedPixelData;
			for (int b = 0; b < numBlocks; b++, block++)
			{
				// Reorder the alpha indexes AND the colour indexes.
				uint16 orig0 = block->GetAlphaRow(0);
				block->SetAlphaRow(0, block->GetAlphaRow(3));
				block->SetAlphaRow(3, orig0);

				uint16 orig1 = block->GetAlphaRow(1);
				block->SetAlphaRow(1, block->GetAlphaRow(2));
				block->SetAlphaRow(2, orig1);

				tStd::tSwap(block->ColourBlock.LookupTableRows[0], block->ColourBlock.LookupTableRows[3]);
				tStd::tSwap(block->ColourBlock.LookupTableRows[1], block->ColourBlock.LookupTableRows[2]);
			}
			break;
		}

		default:
			// We should not get here. Should have early returned already.
			tAssert(!"Should be unreachable.");
			delete[] reversedPixelData;
			return nullptr;
	}

	return reversedPixelData;
}
