// tImagePKM.cpp
//
// This class knows how to load and save pkm (.pkm) files into tPixel arrays. These tPixels may be 'stolen' by the
// tPicture's constructor if a pkm file is specified. After the array is stolen the tImagePKM is invalid. This is
// purely for performance.
//
// Copyright (c) 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.
//
// Everything in the tPKM namespace is a modified version of the PKM loading code found in these two locations:
// https://android.googlesource.com/platform/frameworks/native/+/master/opengl/include/ETC1/etc1.h
// https://android.googlesource.com/platform/frameworks/native/+/master/opengl/libs/ETC1/etc1.cpp
// Both is these files are under the  Apache License, Version 2.0:
//
// Copyright 2009 Google Inc.
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
//
// Modifications are under the ISC licence like the rest of tImagePKM.cpp.
// Modifications consist of:
// a) Placing the header and definitions in the tPKM namespace.
// b) Using tacent datatypes.
// c) Indentation/formatting changes for readability.
// d) Using namespace scoped const int instead of #defines to reduce name pollution and gain type-safety.

#include <System/tFile.h>
#include "Image/tImagePKM.h"
#include "Image/tPicture.h"
using namespace tSystem;
namespace tImage
{


// Helper functions for encoding/decoding etc1 data inside a pkm.
// See http://www.khronos.org/registry/gles/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
namespace tPKM
{
	const int  ETC1_ENCODED_BLOCK_SIZE	= 8;
	const int  ETC1_DECODED_BLOCK_SIZE	= 48;
	const uint ETC1_RGB8_OES			= 0x8D64;

	// Encode a block of pixels.
	//
	// pIn is a pointer to a ETC_DECODED_BLOCK_SIZE array of bytes that represent a
	// 48 / 16 = 3. That is, 3 bytes per pixel. RGB.
	// 4 x 4 square of 3-byte pixels in form R, G, B. Byte (3 * (x + 4 * y) is the R
	// value of pixel (x, y).
	// validPixelMask is a 16-bit mask where bit (1 << (x + y * 4)) indicates whether
	// the corresponding (x,y) pixel is valid. Invalid pixel color values are ignored when compressing.
	// pOut is an ETC1 compressed version of the data.
	void etc1_encode_block(const uint8* blockIn, uint32 validPixelMask, uint8* pOut);

	// Decode a block of pixels.
	// pIn is an ETC1 compressed version of the data.
	// pOut is a pointer to a ETC_DECODED_BLOCK_SIZE array of bytes that represent a
	// 4 x 4 square of 3-byte pixels in form R, G, B. Byte (3 * (x + 4 * y) is the R
	// value of pixel (x, y).
	void etc1_decode_block(const uint8* blockIn, uint8* pOut);

	// Return the size of the encoded image data (does not include size of PKM header).
	uint32 etc1_get_encoded_data_size(uint32 width, uint32 height);

	// Encode an entire image.
	// pIn - pointer to the image data. Formatted such that
	//	   pixel (x,y) is at pIn + pixelSize * x + stride * y;
	// pOut - pointer to encoded data. Must be large enough to store entire encoded image.
	// pixelSize can be 2 or 3. 2 is an GL_UNSIGNED_SHORT_5_6_5 image, 3 is a GL_BYTE RGB image.
	// returns non-zero if there is an error.
	int etc1_encode_image(const uint8* pIn, uint32 width, uint32 height, uint32 pixelSize, uint32 stride, uint8* pOut);

	// Decode an entire image.
	// pIn - pointer to encoded data.
	// pOut - pointer to the image data. Will be written such that
	//		pixel (x,y) is at pIn + pixelSize * x + stride * y. Must be
	//		large enough to store entire image.
	// pixelSize can be 2 or 3. 2 is an GL_UNSIGNED_SHORT_5_6_5 image, 3 is a GL_BYTE RGB image.
	// returns non-zero if there is an error.
	int etc1_decode_image(const uint8* pIn, uint8* pOut, uint32 width, uint32 height, uint32 pixelSize, uint32 stride);

	// Size of a PKM header, in bytes.
	const int  ETC_PKM_HEADER_SIZE = 16;

	// Format a PKM header
	void etc1_pkm_format_header(uint8* pHeader, uint32 width, uint32 height);

	// Check if a PKM header is correctly formatted.
	bool etc1_pkm_is_valid(const uint8* pHeader);

	// Read the image width from a PKM header
	uint32 etc1_pkm_get_width(const uint8* pHeader);

	// Read the image height from a PKM header
	uint32 etc1_pkm_get_height(const uint8* pHeader);
}


bool tImagePKM::Load(const tString& pkmFile)
{
	Clear();

	if (tSystem::tGetFileType(pkmFile) != tSystem::tFileType::PKM)
		return false;

	if (!tFileExists(pkmFile))
		return false;

	int numBytes = 0;
	uint8* pkmFileInMemory = tLoadFile(pkmFile, nullptr, &numBytes);
	bool success = Load(pkmFileInMemory, numBytes);
	delete[] pkmFileInMemory;

	return success;
}


bool tImagePKM::Load(const uint8* pkmFileInMemory, int numBytes)
{
	Clear();
	if ((numBytes <= 0) || !pkmFileInMemory)
		return false;

	const uint8* header = pkmFileInMemory;
	const uint8* data = pkmFileInMemory + tPKM::ETC_PKM_HEADER_SIZE;

	bool valid = tPKM::etc1_pkm_is_valid(header);
	if (!valid)
		return false;

	Width = tPKM::etc1_pkm_get_width(header);
	Height = tPKM::etc1_pkm_get_height(header);
	if ((Width <= 0) || (Height <= 0))
		return false;

	tColour3i* rgbPixels = new tColour3i[Width*Height];

	int stride = Width;
	int errCode = tPKM::etc1_decode_image(data, (uint8*)rgbPixels, Width, Height, 3, 3*stride);
	if (errCode)
	{
		delete[] rgbPixels;
		return false;
	}

	PixelFormatSrc = tPixelFormat::R8G8B8;

	// Now we need to get it into RGBA, and not upside down.
	Pixels = new tPixel[Width*Height];
	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			int srcIdx = Width*(Height-y-1) + x;
			Pixels[y*Width + x].Set( rgbPixels[srcIdx], 255 );
		}
	}
	delete[] rgbPixels;
	return true;
}


bool tImagePKM::Set(tPixel* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Width = width;
	Height = height;

	if (steal)
	{
		Pixels = pixels;
	}
	else
	{
		Pixels = new tPixel[Width*Height];
		tStd::tMemcpy(Pixels, pixels, Width*Height*sizeof(tPixel));
	}

	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	return true;
}


bool tImagePKM::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	Set(frame->GetPixels(steal), frame->Width, frame->Height, steal);
	if (steal)
		delete frame;

	return true;
}


bool tImagePKM::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


tFrame* tImagePKM::GetFrame(bool steal)
{
	if (!IsValid())
		return nullptr;

	tFrame* frame = new tFrame();
	frame->PixelFormatSrc = PixelFormatSrc;

	if (steal)
	{
		frame->StealFrom(Pixels, Width, Height);
		Pixels = nullptr;
	}
	else
	{
		frame->Set(Pixels, Width, Height);
	}
	
	return frame;
}


bool tImagePKM::Save(const tString& pkmFile) const
{
	if (!IsValid())
		return false;

	if (tSystem::tGetFileType(pkmFile) != tSystem::tFileType::PKM)
		return false;

	// @wip Implement save.
	bool success = false;
	return success;
}


bool tImagePKM::IsOpaque() const
{
	for (int p = 0; p < (Width*Height); p++)
	{
		if (Pixels[p].A < 255)
			return false;
	}

	return true;
}


tPixel* tImagePKM::StealPixels()
{
	tPixel* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	return pixels;
}


static const int kModifierTable[] =
{
	2,    8,    -2,   -8,		// 0
	5,    17,   -5,   -17,		// 1
	9,    29,   -9,   -29,		// 2
	13,   42,   -13,  -42,		// 3
	18,   60,   -18,  -60,		// 4
	24,   80,   -24,  -80,		// 5
	33,   106,  -33,  -106,		// 6
	47,   183,  -47,  -183		// 7
};


static const int kLookup[8] = { 0, 1, 2, 3, -4, -3, -2, -1 };

static inline uint8 clamp(int x)
{
	return (uint8) (x >= 0 ? (x < 255 ? x : 255) : 0);
}

static inline int convert4To8(int b)
{
	int c = b & 0xf;
	return (c << 4) | c;
}

static inline int convert5To8(int b)
{
	int c = b & 0x1f;
	return (c << 3) | (c >> 2);
}

static inline int convert6To8(int b)
{
	int c = b & 0x3f;
	return (c << 2) | (c >> 4);
}

static inline int divideBy255(int d)
{
	return (d + 128 + (d >> 8)) >> 8;
}

static inline int convert8To4(int b)
{
	int c = b & 0xff;
	return divideBy255(c * 15);
}

static inline int convert8To5(int b)
{
	int c = b & 0xff;
	return divideBy255(c * 31);
}

static inline int convertDiff(int base, int diff)
{
	return convert5To8((0x1f & base) + kLookup[0x7 & diff]);
}

static void decode_subblock(uint8* pOut, int r, int g, int b, const int* table, uint32 low, bool second, bool flipped)
{
	int baseX = 0;
	int baseY = 0;
	if (second)
	{
		if (flipped)
		{
			baseY = 2;
		}
		else
		{
			baseX = 2;
		}
	}

	for (int i = 0; i < 8; i++)
	{
		int x, y;
		if (flipped)
		{
			x = baseX + (i >> 1);
			y = baseY + (i & 1);
		}
		else
		{
			x = baseX + (i >> 2);
			y = baseY + (i & 3);
		}
		int k = y + (x * 4);
		int offset = ((low >> k) & 1) | ((low >> (k + 15)) & 2);
		int delta = table[offset];
		uint8* q = pOut + 3 * (x + 4 * y);
		*q++ = clamp(r + delta);
		*q++ = clamp(g + delta);
		*q++ = clamp(b + delta);
	}
}

// Input is an ETC1 compressed version of the data.
// Output is a 4 x 4 square of 3-byte pixels in form R, G, B
void tPKM::etc1_decode_block(const uint8* pIn, uint8* pOut)
{
	uint32 high = (pIn[0] << 24) | (pIn[1] << 16) | (pIn[2] << 8) | pIn[3];
	uint32 low = (pIn[4] << 24) | (pIn[5] << 16) | (pIn[6] << 8) | pIn[7];
	int r1, r2, g1, g2, b1, b2;
	if (high & 2)
	{
		// differential
		int rBase = high >> 27;
		int gBase = high >> 19;
		int bBase = high >> 11;
		r1 = convert5To8(rBase);
		r2 = convertDiff(rBase, high >> 24);
		g1 = convert5To8(gBase);
		g2 = convertDiff(gBase, high >> 16);
		b1 = convert5To8(bBase);
		b2 = convertDiff(bBase, high >> 8);
	}
	else
	{
		// not differential
		r1 = convert4To8(high >> 28);
		r2 = convert4To8(high >> 24);
		g1 = convert4To8(high >> 20);
		g2 = convert4To8(high >> 16);
		b1 = convert4To8(high >> 12);
		b2 = convert4To8(high >> 8);
	}
	int tableIndexA = 7 & (high >> 5);
	int tableIndexB = 7 & (high >> 2);
	const int* tableA = kModifierTable + tableIndexA * 4;
	const int* tableB = kModifierTable + tableIndexB * 4;
	bool flipped = (high & 1) != 0;
	decode_subblock(pOut, r1, g1, b1, tableA, low, false, flipped);
	decode_subblock(pOut, r2, g2, b2, tableB, low, true, flipped);
}


typedef struct
{
	uint32 high;
	uint32 low;
	uint32 score; // Lower is more accurate
} etc_compressed;

static inline void take_best(etc_compressed* a, const etc_compressed* b)
{
	if (a->score > b->score)
	{
		*a = *b;
	}
}

static void etc_average_colors_subblock(const uint8* pIn, uint32 inMask, uint8* pColors, bool flipped, bool second)
{
	int r = 0;
	int g = 0;
	int b = 0;
	if (flipped)
	{
		int by = 0;
		if (second)
		{
			by = 2;
		}
		for (int y = 0; y < 2; y++)
		{
			int yy = by + y;
			for (int x = 0; x < 4; x++)
			{
				int i = x + 4 * yy;
				if (inMask & (1 << i))
				{
					const uint8* p = pIn + i * 3;
					r += *(p++);
					g += *(p++);
					b += *(p++);
				}
			}
		}
	}
	else
	{
		int bx = 0;
		if (second)
		{
			bx = 2;
		}
		for (int y = 0; y < 4; y++)
		{
			for (int x = 0; x < 2; x++)
			{
				int xx = bx + x;
				int i = xx + 4 * y;
				if (inMask & (1 << i))
				{
					const uint8* p = pIn + i * 3;
					r += *(p++);
					g += *(p++);
					b += *(p++);
				}
			}
		}
	}
	pColors[0] = (uint8)((r + 4) >> 3);
	pColors[1] = (uint8)((g + 4) >> 3);
	pColors[2] = (uint8)((b + 4) >> 3);
}

static inline int square(int x)
{
	return x * x;
}

static uint32 chooseModifier(const uint8* pBaseColors, const uint8* pIn, uint32 *pLow, int bitIndex, const int* pModifierTable)
{
	uint32 bestScore = ~0;
	int bestIndex = 0;
	int pixelR = pIn[0];
	int pixelG = pIn[1];
	int pixelB = pIn[2];
	int r = pBaseColors[0];
	int g = pBaseColors[1];
	int b = pBaseColors[2];
	for (int i = 0; i < 4; i++)
	{
		int modifier = pModifierTable[i];
		int decodedG = clamp(g + modifier);
		uint32 score = (uint32) (6 * square(decodedG - pixelG));
		if (score >= bestScore)
		{
			continue;
		}
		int decodedR = clamp(r + modifier);
		score += (uint32) (3 * square(decodedR - pixelR));
		if (score >= bestScore)
		{
			continue;
		}
		int decodedB = clamp(b + modifier);
		score += (uint32) square(decodedB - pixelB);
		if (score < bestScore)
		{
			bestScore = score;
			bestIndex = i;
		}
	}
	uint32 lowMask = (((bestIndex >> 1) << 16) | (bestIndex & 1)) << bitIndex;
	*pLow |= lowMask;
	return bestScore;
}

static void etc_encode_subblock_helper(const uint8* pIn, uint32 inMask, etc_compressed* pCompressed, bool flipped, bool second, const uint8* pBaseColors, const int* pModifierTable)
{
	int score = pCompressed->score;
	if (flipped)
	{
		int by = 0;
		if (second)
		{
			by = 2;
		}
		for (int y = 0; y < 2; y++)
		{
			int yy = by + y;
			for (int x = 0; x < 4; x++)
			{
				int i = x + 4 * yy;
				if (inMask & (1 << i))
				{
					score += chooseModifier(pBaseColors, pIn + i * 3, &pCompressed->low, yy + x * 4, pModifierTable);
				}
			}
		}
	}
	else
	{
		int bx = 0;
		if (second)
		{
			bx = 2;
		}
		for (int y = 0; y < 4; y++)
		{
			for (int x = 0; x < 2; x++)
			{
				int xx = bx + x;
				int i = xx + 4 * y;
				if (inMask & (1 << i))
				{
					score += chooseModifier(pBaseColors, pIn + i * 3, &pCompressed->low, y + xx * 4, pModifierTable);
				}
			}
		}
	}
	pCompressed->score = score;
}

static bool inRange4bitSigned(int color)
{
	return color >= -4 && color <= 3;
}

static void etc_encodeBaseColors(uint8* pBaseColors, const uint8* pColors, etc_compressed* pCompressed)
{
	int r1, g1, b1, r2, g2, b2; // 8 bit base colors for sub-blocks
	bool differential;
	int r51 = convert8To5(pColors[0]);
	int g51 = convert8To5(pColors[1]);
	int b51 = convert8To5(pColors[2]);
	int r52 = convert8To5(pColors[3]);
	int g52 = convert8To5(pColors[4]);
	int b52 = convert8To5(pColors[5]);
	r1 = convert5To8(r51);
	g1 = convert5To8(g51);
	b1 = convert5To8(b51);
	int dr = r52 - r51;
	int dg = g52 - g51;
	int db = b52 - b51;
	differential = inRange4bitSigned(dr) && inRange4bitSigned(dg) && inRange4bitSigned(db);
	if (differential)
	{
		r2 = convert5To8(r51 + dr);
		g2 = convert5To8(g51 + dg);
		b2 = convert5To8(b51 + db);
		pCompressed->high |= (r51 << 27) | ((7 & dr) << 24) | (g51 << 19) | ((7 & dg) << 16) | (b51 << 11) | ((7 & db) << 8) | 2;
	}
	else
	{
		int r41 = convert8To4(pColors[0]);
		int g41 = convert8To4(pColors[1]);
		int b41 = convert8To4(pColors[2]);
		int r42 = convert8To4(pColors[3]);
		int g42 = convert8To4(pColors[4]);
		int b42 = convert8To4(pColors[5]);
		r1 = convert4To8(r41);
		g1 = convert4To8(g41);
		b1 = convert4To8(b41);
		r2 = convert4To8(r42);
		g2 = convert4To8(g42);
		b2 = convert4To8(b42);
		pCompressed->high |= (r41 << 28) | (r42 << 24) | (g41 << 20) | (g42 << 16) | (b41 << 12) | (b42 << 8);
	}
	pBaseColors[0] = r1;
	pBaseColors[1] = g1;
	pBaseColors[2] = b1;
	pBaseColors[3] = r2;
	pBaseColors[4] = g2;
	pBaseColors[5] = b2;
}

static void etc_encode_block_helper(const uint8* pIn, uint32 inMask, const uint8* pColors, etc_compressed* pCompressed, bool flipped)
{
	pCompressed->score = ~0;
	pCompressed->high = (flipped ? 1 : 0);
	pCompressed->low = 0;
	uint8 pBaseColors[6];
	etc_encodeBaseColors(pBaseColors, pColors, pCompressed);
	int originalHigh = pCompressed->high;
	const int* pModifierTable = kModifierTable;
	for (int i = 0; i < 8; i++, pModifierTable += 4)
	{
		etc_compressed temp;
		temp.score = 0;
		temp.high = originalHigh | (i << 5);
		temp.low = 0;
		etc_encode_subblock_helper(pIn, inMask, &temp, flipped, false, pBaseColors, pModifierTable);
		take_best(pCompressed, &temp);
	}
	pModifierTable = kModifierTable;
	etc_compressed firstHalf = *pCompressed;
	for (int i = 0; i < 8; i++, pModifierTable += 4)
	{
		etc_compressed temp;
		temp.score = firstHalf.score;
		temp.high = firstHalf.high | (i << 2);
		temp.low = firstHalf.low;
		etc_encode_subblock_helper(pIn, inMask, &temp, flipped, true, pBaseColors + 3, pModifierTable);
		if (i == 0)
		{
			*pCompressed = temp;
		}
		else
		{
			take_best(pCompressed, &temp);
		}
	}
}

static void writeBigEndian(uint8* pOut, uint32 d)
{
	pOut[0] = (uint8)(d >> 24);
	pOut[1] = (uint8)(d >> 16);
	pOut[2] = (uint8)(d >> 8);
	pOut[3] = (uint8) d;
}
// Input is a 4 x 4 square of 3-byte pixels in form R, G, B
// inmask is a 16-bit mask where bit (1 << (x + y * 4)) tells whether the corresponding (x,y)
// pixel is valid or not. Invalid pixel color values are ignored when compressing.
// Output is an ETC1 compressed version of the data.
void etc1_encode_block(const uint8* pIn, uint32 inMask, uint8* pOut)
{
	uint8 colors[6];
	uint8 flippedColors[6];
	etc_average_colors_subblock(pIn, inMask, colors, false, false);
	etc_average_colors_subblock(pIn, inMask, colors + 3, false, true);
	etc_average_colors_subblock(pIn, inMask, flippedColors, true, false);
	etc_average_colors_subblock(pIn, inMask, flippedColors + 3, true, true);
	etc_compressed a, b;
	etc_encode_block_helper(pIn, inMask, colors, &a, false);
	etc_encode_block_helper(pIn, inMask, flippedColors, &b, true);
	take_best(&a, &b);
	writeBigEndian(pOut, a.high);
	writeBigEndian(pOut + 4, a.low);
}

// Return the size of the encoded image data (does not include size of PKM header).
uint32 etc1_get_encoded_data_size(uint32 width, uint32 height)
{
	return (((width + 3) & ~3) * ((height + 3) & ~3)) >> 1;
}

// Encode an entire image.
// pIn - pointer to the image data. Formatted such that the Red component of
//	   pixel (x,y) is at pIn + pixelSize * x + stride * y + redOffset;
// pOut - pointer to encoded data. Must be large enough to store entire encoded image.
int etc1_encode_image(const uint8* pIn, uint32 width, uint32 height, uint32 pixelSize, uint32 stride, uint8* pOut)
{
	if (pixelSize < 2 || pixelSize > 3) {
		return -1;
	}
	static const unsigned short kYMask[] = { 0x0, 0xf, 0xff, 0xfff, 0xffff };
	static const unsigned short kXMask[] = { 0x0, 0x1111, 0x3333, 0x7777, 0xffff };
	uint8 block[tPKM::ETC1_DECODED_BLOCK_SIZE];
	uint8 encoded[tPKM::ETC1_ENCODED_BLOCK_SIZE];
	uint32 encodedWidth = (width + 3) & ~3;
	uint32 encodedHeight = (height + 3) & ~3;
	for (uint32 y = 0; y < encodedHeight; y += 4)
	{
		uint32 yEnd = height - y;
		if (yEnd > 4)
		{
			yEnd = 4;
		}
		int ymask = kYMask[yEnd];
		for (uint32 x = 0; x < encodedWidth; x += 4)
		{
			uint32 xEnd = width - x;
			if (xEnd > 4)
			{
				xEnd = 4;
			}
			int mask = ymask & kXMask[xEnd];
			for (uint32 cy = 0; cy < yEnd; cy++)
			{
				uint8* q = block + (cy * 4) * 3;
				const uint8* p = pIn + pixelSize * x + stride * (y + cy);
				if (pixelSize == 3) {
					memcpy(q, p, xEnd * 3);
				}
				else
				{
					for (uint32 cx = 0; cx < xEnd; cx++)
					{
						int pixel = (p[1] << 8) | p[0];
						*q++ = convert5To8(pixel >> 11);
						*q++ = convert6To8(pixel >> 5);
						*q++ = convert5To8(pixel);
						p += pixelSize;
					}
				}
			}
			etc1_encode_block(block, mask, encoded);
			memcpy(pOut, encoded, sizeof(encoded));
			pOut += sizeof(encoded);
		}
	}
	return 0;
}

// Decode an entire image.
// pIn - pointer to encoded data.
// pOut - pointer to the image data. Will be written such that the Red component of
//	   pixel (x,y) is at pIn + pixelSize * x + stride * y + redOffset. Must be
//		large enough to store entire image.
int tPKM::etc1_decode_image(const uint8* pIn, uint8* pOut, uint32 width, uint32 height, uint32 pixelSize, uint32 stride)
{
	if (pixelSize < 2 || pixelSize > 3)
	{
		return -1;
	}

	uint8 block[tPKM::ETC1_DECODED_BLOCK_SIZE];
	uint32 encodedWidth = (width + 3) & ~3;
	uint32 encodedHeight = (height + 3) & ~3;
	for (uint32 y = 0; y < encodedHeight; y += 4)
	{
		uint32 yEnd = height - y;
		if (yEnd > 4)
		{
			yEnd = 4;
		}
		for (uint32 x = 0; x < encodedWidth; x += 4)
		{
			uint32 xEnd = width - x;
			if (xEnd > 4)
			{
				xEnd = 4;
			}
			etc1_decode_block(pIn, block);
			pIn += tPKM::ETC1_ENCODED_BLOCK_SIZE;
			for (uint32 cy = 0; cy < yEnd; cy++)
			{
				const uint8* q = block + (cy * 4) * 3;
				uint8* p = pOut + pixelSize * x + stride * (y + cy);
				if (pixelSize == 3)
				{
					memcpy(p, q, xEnd * 3);
				}
				else
				{
					for (uint32 cx = 0; cx < xEnd; cx++)
					{
						uint8 r = *q++;
						uint8 g = *q++;
						uint8 b = *q++;
						uint32 pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
						*p++ = (uint8) pixel;
						*p++ = (uint8) (pixel >> 8);
					}
				}
			}
		}
	}
	return 0;
}

static const char kMagic[] = { 'P', 'K', 'M', ' ', '1', '0' };
static const uint32 ETC1_PKM_FORMAT_OFFSET = 6;
static const uint32 ETC1_PKM_ENCODED_WIDTH_OFFSET = 8;
static const uint32 ETC1_PKM_ENCODED_HEIGHT_OFFSET = 10;
static const uint32 ETC1_PKM_WIDTH_OFFSET = 12;
static const uint32 ETC1_PKM_HEIGHT_OFFSET = 14;
static const uint32 ETC1_RGB_NO_MIPMAPS = 0;
static void writeBEUint16(uint8* pOut, uint32 data)
{
	pOut[0] = (uint8) (data >> 8);
	pOut[1] = (uint8) data;
}
static uint32 readBEUint16(const uint8* pIn)
{
	return (pIn[0] << 8) | pIn[1];
}

// Format a PKM header
void etc1_pkm_format_header(uint8* pHeader, uint32 width, uint32 height)
{
	memcpy(pHeader, kMagic, sizeof(kMagic));
	uint32 encodedWidth = (width + 3) & ~3;
	uint32 encodedHeight = (height + 3) & ~3;
	writeBEUint16(pHeader + ETC1_PKM_FORMAT_OFFSET, ETC1_RGB_NO_MIPMAPS);
	writeBEUint16(pHeader + ETC1_PKM_ENCODED_WIDTH_OFFSET, encodedWidth);
	writeBEUint16(pHeader + ETC1_PKM_ENCODED_HEIGHT_OFFSET, encodedHeight);
	writeBEUint16(pHeader + ETC1_PKM_WIDTH_OFFSET, width);
	writeBEUint16(pHeader + ETC1_PKM_HEIGHT_OFFSET, height);
}

// Check if a PKM header is correctly formatted.
bool tPKM::etc1_pkm_is_valid(const uint8* pHeader)
{
	if (memcmp(pHeader, kMagic, sizeof(kMagic)))
	{
		return false;
	}
	uint32 format = readBEUint16(pHeader + ETC1_PKM_FORMAT_OFFSET);
	uint32 encodedWidth = readBEUint16(pHeader + ETC1_PKM_ENCODED_WIDTH_OFFSET);
	uint32 encodedHeight = readBEUint16(pHeader + ETC1_PKM_ENCODED_HEIGHT_OFFSET);
	uint32 width = readBEUint16(pHeader + ETC1_PKM_WIDTH_OFFSET);
	uint32 height = readBEUint16(pHeader + ETC1_PKM_HEIGHT_OFFSET);
	return format == ETC1_RGB_NO_MIPMAPS && encodedWidth >= width && encodedWidth - width < 4 && encodedHeight >= height && encodedHeight - height < 4;
}

// Read the image width from a PKM header
uint32 tPKM::etc1_pkm_get_width(const uint8* pHeader)
{
	return readBEUint16(pHeader + ETC1_PKM_WIDTH_OFFSET);
}

// Read the image height from a PKM header
uint32 tPKM::etc1_pkm_get_height(const uint8* pHeader)
{
	return readBEUint16(pHeader + ETC1_PKM_HEIGHT_OFFSET);
}


}
