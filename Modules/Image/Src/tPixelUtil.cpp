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
#include <Foundation/tSmallFloat.h>
#include <System/tMachine.h>
#include "Image/tPixelUtil.h"
#include "PVRTDecompress/PVRTDecompress.h"
#define BCDEC_IMPLEMENTATION
#include "bcdec/bcdec.h"
#define ETCDEC_IMPLEMENTATION
#include "etcdec/etcdec.h"
#include "astcenc.h"


namespace tImage
{
	bool CanReverseRowData_Packed		(tPixelFormat);
	bool CanReverseRowData_BC			(tPixelFormat, int height);

	uint8* CreateReversedRowData_Packed	(const uint8* pixelData, tPixelFormat pixelDataFormat, int width, int height);
	uint8* CreateReversedRowData_BC		(const uint8* pixelData, tPixelFormat pixelDataFormat, int numBlocksW, int numBlocksH);

	// The BC2 block is the same for DXT2 and DXT3, although we don't support 2 (premultiplied alpha). Size is 128 bits.
	#pragma pack(push, 1)
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
}


tImage::DecodeResult tImage::DecodePixelData(tPixelFormat fmt, const uint8* src, int srcSize, int w, int h, tColour4i*& decoded4i, tColour4f*& decoded4f, tColourProfile profile, float RGBM_RGBD_MaxRange)
{
	if (decoded4i || decoded4f)
		return DecodeResult::BuffersNotClear;

	if (!tIsPackedFormat(fmt) && !tIsBCFormat(fmt) && !tIsASTCFormat(fmt) && !tIsPVRFormat(fmt))
		return DecodeResult::UnsupportedFormat;

	if ((w <= 0) || (h <= 0) || !src)
		return DecodeResult::InvalidInput;

	if (tImage::tIsPackedFormat(fmt))
	{
		return DecodePixelData_Packed(fmt, src, srcSize, w, h, decoded4i, decoded4f, RGBM_RGBD_MaxRange);
	}
	else if (tImage::tIsBCFormat(fmt))
	{
		return DecodePixelData_Block(fmt, src, srcSize, w, h, decoded4i, decoded4f);
	}
	else if (tImage::tIsASTCFormat(fmt))
	{
		return DecodePixelData_ASTC(fmt, src, srcSize, w, h, decoded4f, profile);
	}
	else if (tImage::tIsPVRFormat(fmt))
	{
		return DecodePixelData_PVR(fmt, src, srcSize, w, h, decoded4i, decoded4f);
	}
	else // Unsupported PixelFormat
	{
		return DecodeResult::UnsupportedFormat;
	}

	return DecodeResult::Success;
}


tImage::DecodeResult tImage::DecodePixelData_Packed(tPixelFormat fmt, const uint8* src, int srcSize, int w, int h, tColour4i*& decoded4i, tColour4f*& decoded4f, float RGBM_RGBD_MaxRange)
{
	if (decoded4i || decoded4f)
		return DecodeResult::BuffersNotClear;

	if (!tIsPackedFormat(fmt))
		return DecodeResult::UnsupportedFormat;

	if ((w <= 0) || (h <= 0) || !src)
		return DecodeResult::InvalidInput;

	switch (fmt)
	{
		case tPixelFormat::A8:
			// Convert to 32-bit RGBA with alpha in A and 0s for RGB.
			decoded4i = new tColour4i[w*h];
			for (int ij = 0; ij < w*h; ij++)
			{
				tColour4i col(0u, 0u, 0u, src[ij]);
				decoded4i[ij].Set(col);
			}
			break;

		case tPixelFormat::L8:
		case tPixelFormat::R8:
		{
			// Convert to 32-bit RGBA with red or luminance in R and 255 for A. If SpreadLuminance flag set,
			// also set luminance or red in the GB channels, if not then GB get 0s.
			decoded4i = new tColour4i[w*h];
			for (int ij = 0; ij < w*h; ij++)
			{
				tColour4i col(src[ij], 0u, 0u, 255u);
				decoded4i[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R8G8:
			decoded4i = new tColour4i[w*h];
			for (int ij = 0; ij < w*h; ij++)
			{
				tColour4i col(src[ij*2+0], src[ij*2+1], 0u, 255u);
				decoded4i[ij].Set(col);
			}
			break;

		case tPixelFormat::R8G8B8:
			decoded4i = new tColour4i[w*h];
			for (int ij = 0; ij < w*h; ij++)
			{
				tColour4i col(src[ij*3+0], src[ij*3+1], src[ij*3+2], 255u);
				decoded4i[ij].Set(col);
			}
			break;
		
		case tPixelFormat::R8G8B8A8:
			decoded4i = new tColour4i[w*h];
			for (int ij = 0; ij < w*h; ij++)
			{
				tColour4i col(src[ij*4+0], src[ij*4+1], src[ij*4+2], src[ij*4+3]);
				decoded4i[ij].Set(col);
			}
			break;

		case tPixelFormat::B8G8R8:
			decoded4i = new tColour4i[w*h];
			for (int ij = 0; ij < w*h; ij++)
			{
				tColour4i col(src[ij*3+2], src[ij*3+1], src[ij*3+0], 255u);
				decoded4i[ij].Set(col);
			}
			break;

		case tPixelFormat::B8G8R8A8:
			decoded4i = new tColour4i[w*h];
			for (int ij = 0; ij < w*h; ij++)
			{
				tColour4i col(src[ij*4+2], src[ij*4+1], src[ij*4+0], src[ij*4+3]);
				decoded4i[ij].Set(col);
			}
			break;

		case tPixelFormat::G3B5R5G3:
			decoded4i = new tColour4i[w*h];
			for (int ij = 0; ij < w*h; ij++)
			{
				// On an LE machine casting to a uint16 effectively swaps the bytes when doing bit ops.
				// This means red will be in the most significant bits -- that's why it looks backwards.
				// GGGBBBBB RRRRRGGG in memory is RRRRRGGG GGGBBBBB as a uint16.
				uint16 u = *((uint16*)(src+ij*2));

				uint8 r = (u         ) >> 11;		// 1111 1000 0000 0000 >> 11.
				uint8 g = (u & 0x07E0) >> 5;		// 0000 0111 1110 0000 >> 5.
				uint8 b = (u & 0x001F)     ;		// 0000 0000 0001 1111 >> 0.

				// Normalize to range.
				// Careful here, you can't just do bit ops to get the components into position.
				// For example, a full red (11111) has to go to 255 (1.0f), and a zero red (00000) to 0(0.0f).
				// That is, the normalize has to divide by the range. At first I just masked and shifted the bits
				// to the right spot in an 8-bit type, but you don't know what to put in the LSBits. Putting 0s
				// would be bad (an 4 bit alpha of 1111 would go to 11110000... suddenly image not fully opaque)
				// and putting all 1s would add red (or alpha or whatever) when there was none. Half way won't
				// work either. You need the endpoints to work.
				float rf = (float(r) / 31.0f);		// Max is 2^5 - 1.
				float gf = (float(g) / 63.0f);		// Max is 2^6 - 1.
				float bf = (float(b) / 31.0f);		// Max is 2^5 - 1.
				tColour4i col(rf, gf, bf, 1.0f);
				decoded4i[ij].Set(col);
			}
			break;

		case tPixelFormat::G4B4A4R4:
			decoded4i = new tColour4i[w*h];
			for (int ij = 0; ij < w*h; ij++)
			{
				// GGGGBBBB AAAARRRR in memory is AAAARRRR GGGGBBBB as a uint16.
				uint16 u = *((uint16*)(src+ij*2));
				uint8 a = (u         ) >> 12;		// 1111 0000 0000 0000 >> 12.
				uint8 r = (u & 0x0F00) >> 8;		// 0000 1111 0000 0000 >> 8.
				uint8 g = (u & 0x00F0) >> 4;		// 0000 0000 1111 0000 >> 4.
				uint8 b = (u & 0x000F)     ;		// 0000 0000 0000 1111 >> 0.

				// Normalize to range. See note above.
				float af = float(a) / 15.0f;		// Max is 2^4 - 1.
				float rf = float(r) / 15.0f;
				float gf = float(g) / 15.0f;
				float bf = float(b) / 15.0f;

				tColour4i col(rf, gf, bf, af);
				decoded4i[ij].Set(col);
			}
			break;

		case tPixelFormat::B4A4R4G4:
			decoded4i = new tColour4i[w*h];
			for (int ij = 0; ij < w*h; ij++)
			{
				// BBBBAAAA RRRRGGGG in memory is RRRRGGGG BBBBAAAA as a uint16.
				uint16 u = *((uint16*)(src+ij*2));
				uint8 r = (u         ) >> 12;		// 1111 0000 0000 0000 >> 12.
				uint8 g = (u & 0x0F00) >> 8;		// 0000 1111 0000 0000 >> 8.
				uint8 b = (u & 0x00F0) >> 4;		// 0000 0000 1111 0000 >> 4.
				uint8 a = (u & 0x000F)     ;		// 0000 0000 0000 1111 >> 0.

				// Normalize to range. See note above.
				float af = float(a) / 15.0f;		// Max is 2^4 - 1.
				float rf = float(r) / 15.0f;
				float gf = float(g) / 15.0f;
				float bf = float(b) / 15.0f;

				tColour4i col(rf, gf, bf, af);
				decoded4i[ij].Set(col);
			}
			break;

		case tPixelFormat::G3B5A1R5G2:
			decoded4i = new tColour4i[w*h];
			for (int ij = 0; ij < w*h; ij++)
			{
				// GGGBBBBB ARRRRRGG in memory is ARRRRRGG GGGBBBBB as a uint16.
				uint16 u = *((uint16*)(src+ij*2));
				bool  a = (u & 0x8000);				// 1000 0000 0000 0000.
				uint8 r = (u & 0x7C00) >> 10;		// 0111 1100 0000 0000 >> 10.
				uint8 g = (u & 0x03E0) >> 5;		// 0000 0011 1110 0000 >> 5.
				uint8 b = (u & 0x001F)     ;		// 0000 0000 0001 1111 >> 0.

				// Normalize to range. See note above.
				float rf = float(r) / 31.0f;		// Max is 2^5 - 1.
				float gf = float(g) / 31.0f;
				float bf = float(b) / 31.0f;

				tColour4i col(rf, gf, bf, a ? 1.0f : 0.0f);
				decoded4i[ij].Set(col);
			}
			break;

		case tPixelFormat::G2B5A1R5G3:
			decoded4i = new tColour4i[w*h];
			for (int ij = 0; ij < w*h; ij++)
			{
				// GGBBBBBA RRRRRGGG in memory is RRRRRGGG GGBBBBBA as a uint16.
				uint16 u = *((uint16*)(src+ij*2));
				uint8 r = (u & 0xF800) >> 11;		// 1111 1000 0000 0000 >> 11.
				uint8 g = (u & 0x07C0) >> 6;		// 0000 0111 1100 0000 >> 6.
				uint8 b = (u & 0x003E) >> 1;		// 0000 0000 0011 1110 >> 1.
				bool  a = (u & 0x0001);				// 0000 0000 0000 0001.

				// Normalize to range. See note above.
				float rf = float(r) / 31.0f;		// Max is 2^5 - 1.
				float gf = float(g) / 31.0f;
				float bf = float(b) / 31.0f;

				tColour4i col(rf, gf, bf, a ? 1.0f : 0.0f);
				decoded4i[ij].Set(col);
			}
			break;

		case tPixelFormat::R16:
		{
			decoded4i = new tColour4i[w*h];
			uint16* udata = (uint16*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				uint8 r = udata[ij*1 + 0] >> 8;
				tColour4i col(r, 0u, 0u, 255u);
				decoded4i[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R16G16:
		{
			decoded4i = new tColour4i[w*h];
			uint16* udata = (uint16*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				uint8 r = udata[ij*2 + 0] >> 8;
				uint8 g = udata[ij*2 + 1] >> 8;
				tColour4i col(r, g, 0u, 255u);
				decoded4i[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R16G16B16:
		{
			decoded4i = new tColour4i[w*h];
			uint16* udata = (uint16*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				uint8 r = udata[ij*3 + 0] >> 8;
				uint8 g = udata[ij*3 + 1] >> 8;
				uint8 b = udata[ij*3 + 2] >> 8;
				tColour4i col(r, g, b, 255u);
				decoded4i[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R16G16B16A16:
		{
			decoded4i = new tColour4i[w*h];
			uint16* udata = (uint16*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				uint8 r = udata[ij*4 + 0] >> 8;
				uint8 g = udata[ij*4 + 1] >> 8;
				uint8 b = udata[ij*4 + 2] >> 8;
				uint8 a = udata[ij*4 + 3] >> 8;
				tColour4i col(r, g, b, a);
				decoded4i[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R32:
		{
			decoded4i = new tColour4i[w*h];
			uint32* udata = (uint32*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				uint8 r = udata[ij*1 + 0] >> 24;
				tColour4i col(r, 0u, 0u, 255u);
				decoded4i[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R32G32:
		{
			decoded4i = new tColour4i[w*h];
			uint32* udata = (uint32*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				uint8 r = udata[ij*2 + 0] >> 24;
				uint8 g = udata[ij*2 + 1] >> 24;
				tColour4i col(r, g, 0u, 255u);
				decoded4i[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R32G32B32:
		{
			decoded4i = new tColour4i[w*h];
			uint32* udata = (uint32*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				uint8 r = udata[ij*3 + 0] >> 24;
				uint8 g = udata[ij*3 + 1] >> 24;
				uint8 b = udata[ij*3 + 2] >> 24;
				tColour4i col(r, g, b, 255u);
				decoded4i[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R32G32B32A32:
		{
			decoded4i = new tColour4i[w*h];
			uint32* udata = (uint32*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				uint8 r = udata[ij*4 + 0] >> 24;
				uint8 g = udata[ij*4 + 1] >> 24;
				uint8 b = udata[ij*4 + 2] >> 24;
				uint8 a = udata[ij*4 + 3] >> 24;
				tColour4i col(r, g, b, a);
				decoded4i[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R16f:
		{
			// This HDR format has 1 red half-float channel.
			decoded4f = new tColour4f[w*h];
			tHalf* hdata = (tHalf*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				float r = hdata[ij*1 + 0];
				tColour4f col(r, 0.0f, 0.0f, 1.0f);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R16G16f:
		{
			// This HDR format has 2 half-float channels. Red and green.
			decoded4f = new tColour4f[w*h];
			tHalf* hdata = (tHalf*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				float r = hdata[ij*2 + 0];
				float g = hdata[ij*2 + 1];
				tColour4f col(r, g, 0.0f, 1.0f);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R16G16B16f:
		{
			// This HDR format has 3 half-float channels. RGB.
			decoded4f = new tColour4f[w*h];
			tHalf* hdata = (tHalf*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				float r = hdata[ij*3 + 0];
				float g = hdata[ij*3 + 1];
				float b = hdata[ij*3 + 2];
				tColour4f col(r, g, b, 1.0f);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R16G16B16A16f:
		{
			// This HDR format has 4 half-float channels. RGBA.
			decoded4f = new tColour4f[w*h];
			tHalf* hdata = (tHalf*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				float r = hdata[ij*4 + 0];
				float g = hdata[ij*4 + 1];
				float b = hdata[ij*4 + 2];
				float a = hdata[ij*4 + 3];
				tColour4f col(r, g, b, a);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R32f:
		{
			// This HDR format has 1 red float channel.
			decoded4f = new tColour4f[w*h];
			float* fdata = (float*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				float r = fdata[ij*1 + 0];
				tColour4f col(r, 0.0f, 0.0f, 1.0f);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R32G32f:
		{
			// This HDR format has 2 float channels. Red and green.
			decoded4f = new tColour4f[w*h];
			float* fdata = (float*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				float r = fdata[ij*2 + 0];
				float g = fdata[ij*2 + 1];
				tColour4f col(r, g, 0.0f, 1.0f);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R32G32B32f:
		{
			// This HDR format has 3 float channels. RGB.
			decoded4f = new tColour4f[w*h];
			float* fdata = (float*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				float r = fdata[ij*3 + 0];
				float g = fdata[ij*3 + 1];
				float b = fdata[ij*3 + 2];
				tColour4f col(r, g, b, 1.0f);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R32G32B32A32f:
		{
			// This HDR format has 4 RGBA floats.
			decoded4f = new tColour4f[w*h];
			float* fdata = (float*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				float r = fdata[ij*4 + 0];
				float g = fdata[ij*4 + 1];
				float b = fdata[ij*4 + 2];
				float a = fdata[ij*4 + 3];
				tColour4f col(r, g, b, a);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R11G11B10uf:
		{
			// This HDR format has 3 RGB floats packed into 32-bits.
			decoded4f = new tColour4f[w*h];
			uint32* fdata = (uint32*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				tPackedF11F11F10 packed(fdata[ij]);
				float r, g, b;
				packed.Get(r, g, b);
				tColour4f col(r, g, b, 1.0f);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::B10G11R11uf:
		{
			// This HDR format has 3 RGB floats packed into 32-bits.
			decoded4f = new tColour4f[w*h];
			uint32* fdata = (uint32*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				tPackedF10F11F11 packed(fdata[ij]);
				float r, g, b;
				packed.Get(b, g, r);
				tColour4f col(r, g, b, 1.0f);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R9G9B9E5uf:
		{
			// This HDR format has 3 RGB floats packed into 32-bits.
			decoded4f = new tColour4f[w*h];
			uint32* fdata = (uint32*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				tPackedM9M9M9E5 packed(fdata[ij]);
				float r, g, b;
				packed.Get(r, g, b);
				tColour4f col(r, g, b, 1.0f);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::E5B9G9R9uf:
		{
			// This HDR format has 3 RGB floats packed into 32-bits.
			decoded4f = new tColour4f[w*h];
			uint32* fdata = (uint32*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				tPackedE5M9M9M9 packed(fdata[ij]);
				float r, g, b;
				packed.Get(b, g, r);
				tColour4f col(r, g, b, 1.0f);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R8G8B8M8:
		{
			// This HDR format has 8-bit RGB components and a shared 8-bit multiplier.
			decoded4f = new tColour4f[w*h];
			uint8* udata = (uint8*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				float r = float(src[ij*4+0]) / 255.0f;
				float g = float(src[ij*4+1]) / 255.0f;
				float b = float(src[ij*4+2]) / 255.0f;
				float m = float(src[ij*4+3]) / 255.0f;
				r *= m * RGBM_RGBD_MaxRange;
				g *= m * RGBM_RGBD_MaxRange;
				b *= m * RGBM_RGBD_MaxRange;
				tColour4f col(r, g, b, 1.0f);
				decoded4f[ij].Set(col);
			}
			break;
		}

		case tPixelFormat::R8G8B8D8:
		{
			// This HDR format has 8-bit RGB components and a shared 8-bit divisor.
			decoded4f = new tColour4f[w*h];
			uint8* udata = (uint8*)src;
			for (int ij = 0; ij < w*h; ij++)
			{
				float r = float(src[ij*4+0]) / 255.0f;
				float g = float(src[ij*4+1]) / 255.0f;
				float b = float(src[ij*4+2]) / 255.0f;
				float d = float(src[ij*4+3]) / 255.0f;
				if (d == 0.0f)
				{
					r = 0.0f;
					g = 0.0f;
					b = 0.0f;
					d = 1.0f;
				}
				r *= (RGBM_RGBD_MaxRange/255.0f) / d;
				g *= (RGBM_RGBD_MaxRange/255.0f) / d;
				b *= (RGBM_RGBD_MaxRange/255.0f) / d;
				tColour4f col(r, g, b, 1.0f);
				decoded4f[ij].Set(col);
			}
			break;
		}

		default:
			return DecodeResult::PackedDecodeError;
	}

	return DecodeResult::Success;
}


tImage::DecodeResult tImage::DecodePixelData_Block(tPixelFormat fmt, const uint8* src, int srcSize, int w, int h, tColour4i*& decoded4i, tColour4f*& decoded4f)
{
	if (decoded4i || decoded4f)
		return DecodeResult::BuffersNotClear;

	if (!tIsBCFormat(fmt))
		return DecodeResult::UnsupportedFormat;

	if ((w <= 0) || (h <= 0) || !src)
		return DecodeResult::InvalidInput;

	// We need extra room because the decompressor (bcdec) does not take an input for
	// the width and height, only the pitch (bytes per row). This means a texture that is 5
	// high will actually have row 6, 7, 8 written to.
	int wfull = 4 * tGetNumBlocks(4, w);
	int hfull = 4 * tGetNumBlocks(4, h);
	tColour4i* decodedFull4i = nullptr;
	tColour4f* decodedFull4f = nullptr;
	switch (fmt)
	{
		case tPixelFormat::BC1DXT1:
		case tPixelFormat::BC1DXT1A:
		{
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)decodedFull4i + (y*wfull + x) * 4;

					// At first didn't understand the pitch (3rd) argument. It's cuz the block needs to be written into
					// multiple rows of the destination and we need to know how far to increment to the next row of 4.
					bcdec_bc1(src, dst, wfull * 4);
					src += BCDEC_BC1_BLOCK_SIZE;
				}

			break;
		}

		case tPixelFormat::BC2DXT2DXT3:
		{
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)decodedFull4i + (y*wfull + x) * 4;
					bcdec_bc2(src, dst, wfull * 4);
					src += BCDEC_BC2_BLOCK_SIZE;
				}
			break;
		}

		case tPixelFormat::BC3DXT4DXT5:
		{
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)decodedFull4i + (y*wfull + x) * 4;
					bcdec_bc3(src, dst, wfull * 4);
					src += BCDEC_BC3_BLOCK_SIZE;
				}
			break;
		}

		case tPixelFormat::BC4ATI1U:
		{
			// This HDR format decompresses to R uint8s.
			uint8* rdata = new uint8[wfull*hfull];
			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (rdata + (y*wfull + x) * 1);
					bcdec_bc4(src, dst, wfull * 1);
					src += BCDEC_BC4_BLOCK_SIZE;
				}

			// Now convert to 32-bit RGBA.
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				uint8 v = rdata[xy];
				tColour4i col(v, 0u, 0u, 255u);
				decodedFull4i[xy].Set(col);
			}
			delete[] rdata;
			break;
		}

		case tPixelFormat::BC4ATI1S:
		{
			// This HDR format decompresses to R uint8s.
			uint8* rdata = new uint8[wfull*hfull];
			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (rdata + (y*wfull + x) * 1);
					bcdec_bc4(src, dst, wfull * 1);
					src += BCDEC_BC4_BLOCK_SIZE;
				}

			// Now convert to 32-bit RGBA.
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				int vi = *((int8*)&rdata[xy]);
				vi += 128;
				uint8 v = uint8(vi);
				tColour4i col(v, 0u, 0u, 255u);
				decodedFull4i[xy].Set(col);
			}
			delete[] rdata;
			break;
		}

		case tPixelFormat::BC5ATI2U:
		{
			// This HDR format decompresses to RG uint8s.
			struct RG { uint8 R; uint8 G; };
			RG* rgData = new RG[wfull*hfull];

			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)rgData + (y*wfull + x) * 2;
					bcdec_bc5(src, dst, wfull * 2);
					src += BCDEC_BC5_BLOCK_SIZE;
				}

			// Now convert to 32-bit RGBA with 0,255 for B,A.
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				tColour4i col(rgData[xy].R, rgData[xy].G, 0u, 255u);
				decodedFull4i[xy].Set(col);
			}
			delete[] rgData;
			break;
		}

		case tPixelFormat::BC5ATI2S:
		{
			// This HDR format decompresses to RG uint8s.
			struct RG { uint8 R; uint8 G; };
			RG* rgData = new RG[wfull*hfull];

			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)rgData + (y*wfull + x) * 2;
					bcdec_bc5(src, dst, wfull * 2);
					src += BCDEC_BC5_BLOCK_SIZE;
				}

			// Now convert to 32-bit RGBA with 0,255 for B,A.
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				int iR = *((int8*)&rgData[xy].R);
				int iG = *((int8*)&rgData[xy].G);
				iR += 128;
				iG += 128;
				tColour4i col(uint8(iR), uint8(iG), 0u, 255u);
				decodedFull4i[xy].Set(col);
			}
			delete[] rgData;
			break;
		}

		case tPixelFormat::BC6U:
		case tPixelFormat::BC6S:
		{
			// This HDR format decompresses to RGB floats.
			tColour3f* rgbData = new tColour3f[wfull*hfull];

			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)((float*)rgbData + (y*wfull + x) * 3);
					bool signedData = fmt == tPixelFormat::BC6S;
					bcdec_bc6h_float(src, dst, wfull * 3, signedData);
					src += BCDEC_BC6H_BLOCK_SIZE;
				}

			// Now convert to 4-float (128-bit) RGBA with 1.0f alpha.
			decodedFull4f = new tColour4f[wfull*hfull];
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				tColour4f col(rgbData[xy], 1.0f);
				decodedFull4f[xy].Set(col);
			}
			delete[] rgbData;
			break;
		}

		case tPixelFormat::BC7:
		{
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)decodedFull4i + (y*wfull + x) * 4;
					bcdec_bc7(src, dst, wfull * 4);
					src += BCDEC_BC7_BLOCK_SIZE;
				}
			break;
		}

		case tPixelFormat::ETC1:
		case tPixelFormat::ETC2RGB:				// Same decoder. Backwards compatible.
		{
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)decodedFull4i + (y*wfull + x) * 4;
					etcdec_etc_rgb(src, dst, wfull * 4);
					src += ETCDEC_ETC_RGB_BLOCK_SIZE;
				}
			break;
		}

		case tPixelFormat::ETC2RGBA:
		{
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)decodedFull4i + (y*wfull + x) * 4;
					etcdec_eac_rgba(src, dst, wfull * 4);
					src += ETCDEC_EAC_RGBA_BLOCK_SIZE;
				}
			break;
		}

		case tPixelFormat::ETC2RGBA1:
		{
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)decodedFull4i + (y*wfull + x) * 4;
					etcdec_etc_rgb_a1(src, dst, wfull * 4);
					src += ETCDEC_ETC_RGB_A1_BLOCK_SIZE;
				}
			break;
		}

		case tPixelFormat::EACR11U:
		{
			// This format decompresses to R uint16.
			uint16* rdata = new uint16[wfull*hfull];

			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint16* dst = (rdata + (y*wfull + x) * 1);
					etcdec_eac_r11_u16(src, dst, wfull * sizeof(uint16));
					src += ETCDEC_EAC_R11_BLOCK_SIZE;
				}

			// Now convert to 32-bit RGBA.
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				uint8 v = uint8( (255*rdata[xy]) / 65535 );
				tColour4i col(v, 0u, 0u, 255u);
				decodedFull4i[xy].Set(col);
			}
			delete[] rdata;
			break;
		}

		case tPixelFormat::EACR11S:
		{
			// This format decompresses to R float.
			float* rdata = new float[wfull*hfull];

			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					float* dst = (rdata + (y*wfull + x) * 1);
					etcdec_eac_r11_float(src, dst, wfull * sizeof(float), 1);
					src += ETCDEC_EAC_R11_BLOCK_SIZE;
				}

			// Now convert to 32-bit RGBA.
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				float vf = tMath::tSaturate((rdata[xy]+1.0f) / 2.0f);
				uint8 v = uint8( 255.0f * vf );
				tColour4i col(v, 0u, 0u, 255u);
				decodedFull4i[xy].Set(col);
			}
			delete[] rdata;
			break;
		}

		case tPixelFormat::EACRG11U:
		{
			struct RG { uint16 R; uint16 G; };
			// This format decompresses to RG uint16s.
			RG* rdata = new RG[wfull*hfull];

			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint16* dst = (uint16*)rdata + (y*wfull + x) * 2;
					etcdec_eac_rg11_u16(src, dst, wfull * sizeof(RG));
					src += ETCDEC_EAC_RG11_BLOCK_SIZE;
				}

			// Now convert to 32-bit RGBA.
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				uint8 r = uint8( (255*rdata[xy].R) / 65535 );
				uint8 g = uint8( (255*rdata[xy].G) / 65535 );
				tColour4i col(r, g, 0u, 255u);
				decodedFull4i[xy].Set(col);
			}
			delete[] rdata;
			break;
		}

		case tPixelFormat::EACRG11S:
		{
			struct RG { float R; float G; };
			// This format decompresses to RG floats.
			RG* rdata = new RG[wfull*hfull];

			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					float* dst = (float*)rdata + (y*wfull + x) * 2;
					etcdec_eac_rg11_float(src, dst, wfull * sizeof(RG), 1);
					src += ETCDEC_EAC_RG11_BLOCK_SIZE;
				}

			// Now convert to 32-bit RGBA.
			decodedFull4i = new tColour4i[wfull*hfull];
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				float rf = tMath::tSaturate((rdata[xy].R+1.0f) / 2.0f);
				float gf = tMath::tSaturate((rdata[xy].G+1.0f) / 2.0f);
				uint8 r = uint8( 255.0f * rf );
				uint8 g = uint8( 255.0f * gf );
				tColour4i col(r, g, 0u, 255u);
				decodedFull4i[xy].Set(col);
			}
			delete[] rdata;
			break;
		}

		default:
			return DecodeResult::BlockDecodeError;
	}

	// Decode worked. We are now in RGBA 32-bit or float 128-bit. Width and height are already correct.
	// This isn't the most efficient because we don't have a stride in a tLayer, but correctness first.
	// Basically the decodedFull data may be too big if we needed extra room for w and h to do the decompression.
	// This happens when the image dimensions where not multiples of the block size. We deal with that here.
	// This is only inefficient if the dimensions were not a mult of 4, otherwise we can use the buffer directly.
	tAssert((decodedFull4i || decodedFull4f) && !(decodedFull4i && decodedFull4f));

	// At this point the job is to get decoded4i or decoded4f to be valid. First check if sizes match exactly.
	if ((wfull == w) && (hfull == h))
	{
		decoded4i = decodedFull4i;
		decoded4f = decodedFull4f;
	}
	else
	{
		if (decodedFull4i)
		{
			decoded4i = new tColour4i[w*h];
			tColour4i* src = decodedFull4i;
			tColour4i* dst = decoded4i;
			for (int r = 0; r < h; r++)
			{
				tStd::tMemcpy(dst, src, w*sizeof(tColour4i));
				src += wfull;
				dst += w;
			}
			delete[] decodedFull4i;
		}
		else if (decodedFull4f)
		{
			decoded4f = new tColour4f[w*h];
			tColour4f* src = decodedFull4f;
			tColour4f* dst = decoded4f;
			for (int r = 0; r < h; r++)
			{
				tStd::tMemcpy(dst, src, w*sizeof(tColour4f));
				src += wfull;
				dst += w;
			}
			delete[] decodedFull4f;
		}
	}

	return DecodeResult::Success;
}


tImage::DecodeResult tImage::DecodePixelData_ASTC(tPixelFormat fmt, const uint8* src, int srcSize, int w, int h, tColour4f*& decoded4f, tColourProfile profile)
{
	if (decoded4f)
		return DecodeResult::BuffersNotClear;

	if (!tIsASTCFormat(fmt))
		return DecodeResult::UnsupportedFormat;

	if ((w <= 0) || (h <= 0) || !src)
		return DecodeResult::InvalidInput;

	int blockW = 0;
	int blockH = 0;
	int blockD = 1;

	// Convert source colour profile to astc colour profile.
	astcenc_profile profileastc = ASTCENC_PRF_LDR_SRGB;
	switch (profile)
	{
		case tColourProfile::Auto:			profileastc = ASTCENC_PRF_HDR_RGB_LDR_A;break;	// Works for LDR also.
		case tColourProfile::LDRsRGB_LDRlA:	profileastc = ASTCENC_PRF_LDR_SRGB;		break;
		case tColourProfile::LDRgRGB_LDRlA:	profileastc = ASTCENC_PRF_LDR_SRGB;		break;	// Best approximation.
		case tColourProfile::LDRlRGBA:		profileastc = ASTCENC_PRF_LDR;			break;
		case tColourProfile::HDRlRGB_LDRlA:	profileastc = ASTCENC_PRF_HDR_RGB_LDR_A;break;
		case tColourProfile::HDRlRGBA:		profileastc = ASTCENC_PRF_HDR;			break;
	}

	switch (fmt)
	{
		case tPixelFormat::ASTC4X4:		blockW = 4;		blockH = 4;		break;
		case tPixelFormat::ASTC5X4:		blockW = 5;		blockH = 4;		break;
		case tPixelFormat::ASTC5X5:		blockW = 5;		blockH = 5;		break;
		case tPixelFormat::ASTC6X5:		blockW = 6;		blockH = 5;		break;
		case tPixelFormat::ASTC6X6:		blockW = 6;		blockH = 6;		break;
		case tPixelFormat::ASTC8X5:		blockW = 8;		blockH = 5;		break;
		case tPixelFormat::ASTC8X6:		blockW = 8;		blockH = 6;		break;
		case tPixelFormat::ASTC8X8:		blockW = 8;		blockH = 8;		break;
		case tPixelFormat::ASTC10X5:	blockW = 10;	blockH = 5;		break;
		case tPixelFormat::ASTC10X6:	blockW = 10;	blockH = 6;		break;
		case tPixelFormat::ASTC10X8:	blockW = 10;	blockH = 8;		break;
		case tPixelFormat::ASTC10X10:	blockW = 10;	blockH = 10;	break;
		case tPixelFormat::ASTC12X10:	blockW = 12;	blockH = 10;	break;
		case tPixelFormat::ASTC12X12:	blockW = 12;	blockH = 12;	break;
		default:														break;
	}

	if (!blockW || !blockH)
		return DecodeResult::ASTCDecodeError;

	float quality = ASTCENC_PRE_MEDIUM;			// Only need for compression.
	astcenc_error result = ASTCENC_SUCCESS;
	astcenc_config config;
	astcenc_config_init(profileastc, blockW, blockH, blockD, quality, ASTCENC_FLG_DECOMPRESS_ONLY, &config);

	// astcenc_get_error_string(status) can be called for details.
	if (result != ASTCENC_SUCCESS)
		return DecodeResult::ASTCDecodeError;

	astcenc_context* context = nullptr;
	int numThreads = tMath::tMax(tSystem::tGetNumCores(), 2);
	result = astcenc_context_alloc(&config, numThreads, &context);
	if (result != ASTCENC_SUCCESS)
		return DecodeResult::ASTCDecodeError;

	decoded4f = new tColour4f[w*h];
	astcenc_image image;
	image.dim_x = w;
	image.dim_y = h;
	image.dim_z = 1;
	image.data_type = ASTCENC_TYPE_F32;

	tColour4f* slices = decoded4f;
	image.data = reinterpret_cast<void**>(&slices);
	astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

	result = astcenc_decompress_image(context, src, srcSize, &image, &swizzle, 0);
	if (result != ASTCENC_SUCCESS)
	{
		astcenc_context_free(context);
		delete[] decoded4f;
		decoded4f = nullptr;
		return DecodeResult::ASTCDecodeError;
	}
	astcenc_context_free(context);

	return DecodeResult::Success;
}


tImage::DecodeResult tImage::DecodePixelData_PVR(tPixelFormat fmt, const uint8* src, int srcSize, int w, int h, tColour4i*& decoded4i, tColour4f*& decoded4f)
{
	if (decoded4i || decoded4f)
		return DecodeResult::BuffersNotClear;

	if (!tIsPVRFormat(fmt))
		return DecodeResult::UnsupportedFormat;

	if ((w <= 0) || (h <= 0) || !src)
		return DecodeResult::InvalidInput;

	// The PVRTDecompress calls expect the decoded destination array to be bug enough to handle w*h tColour4i pixels.
	// The function handles cases where the min width and height are too small, so even a 1x1 image can be handed off.
	switch (fmt)
	{
		case tPixelFormat::PVRBPP4:
		{
			decoded4i = new tColour4i[w*h];
			uint32_t do2bitMode = 0;
			uint32_t numSrcBytesDecompressed = pvr::PVRTDecompressPVRTC(src, do2bitMode, w, h, (uint8_t*)decoded4i);
			if (numSrcBytesDecompressed == 0)
			{
				delete[] decoded4i;
				decoded4i = nullptr;
				return DecodeResult::PVRDecodeError;
			}
			break;
		}

		case tPixelFormat::PVRBPP2:
		{
			decoded4i = new tColour4i[w*h];
			uint32_t do2bitMode = 1;
			uint32_t numSrcBytesDecompressed = pvr::PVRTDecompressPVRTC(src, do2bitMode, w, h, (uint8_t*)decoded4i);
			if (numSrcBytesDecompressed == 0)
			{
				delete[] decoded4i;
				decoded4i = nullptr;
				return DecodeResult::PVRDecodeError;
			}
			break;
		}

		case tPixelFormat::PVR2BPP4:
		case tPixelFormat::PVR2BPP2:
			return DecodeResult::UnsupportedFormat;

		case tPixelFormat::PVRHDRBPP8:
		case tPixelFormat::PVRHDRBPP6:
			return DecodeResult::UnsupportedFormat;

		case tPixelFormat::PVR2HDRBPP8:
		case tPixelFormat::PVR2HDRBPP6:
			return DecodeResult::UnsupportedFormat;

		default:
			return DecodeResult::PVRDecodeError;
	}

	return DecodeResult::Success;
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

	return false;
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
