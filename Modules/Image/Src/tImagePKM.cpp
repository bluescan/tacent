// tImagePKM.cpp
//
// This class knows how to load and save Ericsson's ETC1/ETC2/EAC PKM (.pkm) files. The pixel data is stored in a
// tLayer. If decode was requested the layer will store raw pixel data. The layer may be 'stolen'. IF it is the
// tImagePKM is invalid afterwards. This is purely for performance.
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

#include <System/tFile.h>
#include "Image/tImagePKM.h"
#include "Image/tPixelUtil.h"
#include "Image/tPicture.h"
#include "etcdec/etcdec.h"
using namespace tSystem;
namespace tImage
{


namespace tPKM
{
	#pragma pack(push, 1)
	struct Header
	{
		char FourCCMagic[4];				// PKM files should have 'P', 'K', 'M', ' ' as the first four characters.
		char Version[2];					// Will be '1', '0' for ETC1 and '2', '0' for ETC2.
		uint8 FormatMSB;					// The header is big endian. This is the MSB of the format.
		uint8 FormatLSB;
		uint8 EncodedWidthMSB;				// This is the width in terms of number of 4x4 blocks used. It is always divisible by 4.
		uint8 EncodedWidthLSB;
		uint8 EncodedHeightMSB;				// This is the height in terms of number of 4x4 blocks used. It is always divisible by 4.
		uint8 EncodedHeightLSB;
		uint8 WidthMSB;						// This is the 'real' image width. Any value >= 1 works.
		uint8 WidthLSB;
		uint8 HeightMSB;					// This is the 'real' image height. Any value >= 1 works.
		uint8 HeightLSB;

		int GetVersion() const				{ return (Version[0] == '1') ? 1 : 2; }
		uint32 GetFormat() const			{ return (FormatMSB << 8) | FormatLSB; }
		uint32 GetEncodedWidth() const		{ return (EncodedWidthMSB << 8) | EncodedWidthLSB; }
		uint32 GetEncodedHeight() const		{ return (EncodedHeightMSB << 8) | EncodedHeightLSB; }
		uint32 GetWidth() const				{ return (WidthMSB << 8) | WidthLSB; }
		uint32 GetHeight() const			{ return (HeightMSB << 8) | HeightLSB; }
	};
	#pragma pack(pop)

	// Format codes. These are what will be found in the pkm header. The corresponding OpenGL texture format ID
	// is listed next to each one.
	// Note1: ETC1 pkm files should assume ETC1_RGB8 even if the format is not set to that.
	// Note2: The sRGB formats are decoded the same as the non-sRGB formats. It is only the interpretation
	//        of the pixel values that changes.
	// Note3: ETC1_RGB8 and ETC2_RGB8 and ETC2_sRGB8 are all decoded with the same RGB decode. This is
	//        because ETC2 is backwards compatible with ETC1.
	enum class PKMFMT
	{
		ETC1_RGB,				// GL_ETC1_RGB8_OES. OES just means the format internal ID was developed by the working group (Kronos I assume).
		ETC2_RGB,				// GL_COMPRESSED_RGB8_ETC2.
		ETC2_RGBA_OLD,			// GL_COMPRESSED_RGBA8_ETC2_EAC. Should not be encountered. Interpret as RGBA if it is.
		ETC2_RGBA,				// GL_COMPRESSED_RGBA8_ETC2_EAC.
		ETC2_RGBA1,				// GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2.
		ETC2_R,					// GL_COMPRESSED_R11_EAC.
		ETC2_RG,				// GL_COMPRESSED_RG11_EAC.
		ETC2_R_SIGNED,			// GL_COMPRESSED_SIGNED_R11_EAC.
		ETC2_RG_SIGNED,			// GL_COMPRESSED_SIGNED_RG11_EAC.
		ETC2_sRGB,				// GL_COMPRESSED_SRGB8_ETC2.
		ETC2_sRGBA,				// GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC.
		ETC2_sRGBA1				// GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2.
	};

	bool IsHeaderValid(const Header&);

	// These figure out the pixel format and the colour-space. tPixelFormat does not specify ancilllary
	// properties of the data -- it specified the encoding of the data. The extra information, like the colour-space it
	// was authored in, is stored in tColourSpace. In many cases this satellite information cannot be determined, in
	// which case colour-space will be set to their 'unspecified' enumerant.
	void GetFormatInfo_FromPKMFormat(tPixelFormat&, tColourSpace&, uint32 pkmFmt, int version);
}


void tPKM::GetFormatInfo_FromPKMFormat(tPixelFormat& fmt, tColourSpace& spc, uint32 pkmFmt, int version)
{
	fmt = tPixelFormat::Invalid;
	spc = tColourSpace::Invalid;
	switch (PKMFMT(pkmFmt))
	{
		case PKMFMT::ETC1_RGB:		fmt = tPixelFormat::ETC1;		spc = tColourSpace::sRGB;		break;
		case PKMFMT::ETC2_RGB:		fmt = tPixelFormat::ETC2RGB;	spc = tColourSpace::Linear;		break;
		case PKMFMT::ETC2_RGBA_OLD:
		case PKMFMT::ETC2_RGBA:		fmt = tPixelFormat::ETC2RGBA;	spc = tColourSpace::Linear;		break;
		case PKMFMT::ETC2_RGBA1:	fmt = tPixelFormat::ETC2RGBA1;	spc = tColourSpace::Linear;		break;
		case PKMFMT::ETC2_R:		fmt = tPixelFormat::EACR11;		spc = tColourSpace::Linear;		break;
		case PKMFMT::ETC2_RG:		fmt = tPixelFormat::EACRG11;	spc = tColourSpace::Linear;		break;
		case PKMFMT::ETC2_R_SIGNED:	fmt = tPixelFormat::EACR11S;	spc = tColourSpace::Linear;		break;
		case PKMFMT::ETC2_RG_SIGNED:fmt = tPixelFormat::EACRG11S;	spc = tColourSpace::Linear;		break;
		case PKMFMT::ETC2_sRGB:		fmt = tPixelFormat::ETC2RGB;	spc = tColourSpace::sRGB;		break;
		case PKMFMT::ETC2_sRGBA:	fmt = tPixelFormat::ETC2RGBA;	spc = tColourSpace::sRGB;		break;
		case PKMFMT::ETC2_sRGBA1:	fmt = tPixelFormat::ETC2RGBA1;	spc = tColourSpace::sRGB;		break;
	}

	// If the format is still invalid we encountered an invalid format in the PKM header.
	// In this case we base the format on the header version number only.
	if (fmt == tPixelFormat::Invalid)
	{
		spc = tColourSpace::sRGB;
		if (version == 2)
			fmt = tPixelFormat::ETC2RGB;
		else
			fmt = tPixelFormat::ETC1;
	}
}


bool tPKM::IsHeaderValid(const Header& header)
{
	if ((header.FourCCMagic[0] != 'P') || (header.FourCCMagic[1] != 'K') || (header.FourCCMagic[2] != 'M') || (header.FourCCMagic[3] != ' '))
		return false;

	uint32 format = header.GetFormat();
	tPrintf("Format %08x %d\n", format, format);

	uint32 encodedWidth = header.GetEncodedWidth();
	uint32 encodedHeight = header.GetEncodedHeight();
	tPrintf("Encoded width, height: %d %d\n", encodedWidth, encodedHeight);

	uint32 width = header.GetWidth();
	uint32 height = header.GetHeight();
	tPrintf("Width, height: %d %d\n", width, height);

	// I'm not sure why the header stores the encrypted sizes as they can be computed from
	// the width and height. They can, however, be used for validation.
	int blocksW = tImage::tGetNumBlocks(4, width);
	if (blocksW*4 != encodedWidth)
		return false;

	int blocksH = tImage::tGetNumBlocks(4, height);
	if (blocksH*4 != encodedHeight)
		return false;

	return true;
}


bool tImagePKM::Load(const tString& pkmFile, const LoadParams& params)
{
	Clear();

	if (tSystem::tGetFileType(pkmFile) != tSystem::tFileType::PKM)
		return false;

	if (!tFileExists(pkmFile))
		return false;

	int numBytes = 0;
	uint8* pkmFileInMemory = tLoadFile(pkmFile, nullptr, &numBytes);
	bool success = Load(pkmFileInMemory, numBytes, params);
	delete[] pkmFileInMemory;

	return success;
}


bool tImagePKM::Load(const uint8* pkmFileInMemory, int numBytes, const LoadParams& paramsIn)
{
	Clear();
	if ((numBytes <= 0) || !pkmFileInMemory)
		return false;

	const tPKM::Header* header = (const tPKM::Header*)pkmFileInMemory;
	bool valid = tPKM::IsHeaderValid(*header);
	if (!valid)
		return false;

	int width  = header->GetWidth();
	int height = header->GetHeight();
	if ((width <= 0) || (height <= 0))
		return false;

	tPixelFormat format;
	tColourSpace space;
	tPKM::GetFormatInfo_FromPKMFormat(format, space, header->GetFormat(), header->GetVersion());
	if (!tIsBCFormat(format))
		return false;

	PixelFormat = format;
	PixelFormatSrc = format;
	ColourSpace = space;
	ColourSpaceSrc = space;

	const uint8* pkmData = pkmFileInMemory + sizeof(tPKM::Header);
	int pkmDataSize = numBytes - sizeof(tPKM::Header);
	tAssert(!Layer);
	LoadParams params(paramsIn);

	// If we were not asked to decode we just get the data over to the Layer and we're done.
	if (!(params.Flags & LoadFlag_Decode))
	{
		Layer = new tLayer(PixelFormat, width, height, (uint8*)pkmData);
		return true;
	}

	// Decode to 32-bit RGBA.
	// Spread only applies to the single-channel (R-only) format.
	bool spread = params.Flags & LoadFlag_SpreadLuminance;

	// If the gamma mode is auto, we determine here whether to apply sRGB compression.
	// If the space is linear and a format that often encodes colours, we apply it.
	if (params.Flags & LoadFlag_AutoGamma)
	{
		// Clear all related flags.
		params.Flags &= ~(LoadFlag_AutoGamma | LoadFlag_SRGBCompression | LoadFlag_GammaCompression);
		if (ColourSpace == tColourSpace::Linear)
			params.Flags |= LoadFlag_SRGBCompression;
	}

	// We need extra room because the decompressor (bcdec) does not take an input for
	// the width and height, only the pitch (bytes per row). This means a texture that is 5
	// high will actually have row 6, 7, 8 written to.
	int wfull = 4 * tGetNumBlocks(4, width);
	int hfull = 4 * tGetNumBlocks(4, height);
	tPixel* uncompData = new tPixel[wfull*hfull];
	const uint8* src = pkmData;
	switch (PixelFormatSrc)
	{
		case tPixelFormat::ETC1:
		case tPixelFormat::ETC2RGB:				// Same decoder. backwards compatible.
		{
			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)uncompData + (y*wfull + x) * 4;

					// At first didn't understand the pitch (3rd) argument. It's cuz the block needs to be written into
					// multiple rows of the destination and we need to know how far to increment to the next row of 4.
					etcdec_etc_rgb(src, dst, wfull * 4);
					src += ETCDEC_ETC_RGB_BLOCK_SIZE;
				}
			break;
		}

		case tPixelFormat::ETC2RGBA:
		{
			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)uncompData + (y*wfull + x) * 4;
					etcdec_eac_rgba(src, dst, wfull * 4);
					src += ETCDEC_EAC_RGBA_BLOCK_SIZE;
				}
			break;
		}

		case tPixelFormat::ETC2RGBA1:
		{
			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint8* dst = (uint8*)uncompData + (y*wfull + x) * 4;
					etcdec_etc_rgb_a1(src, dst, wfull * 4);
					src += ETCDEC_ETC_RGB_A1_BLOCK_SIZE;
				}
			break;
		}

		case tPixelFormat::EACR11:
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
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				uint8 v = uint8( (255*rdata[xy]) / 65535 );
				tColour4i col(v, spread ? v : 0u, spread ? v : 0u, 255u);
				uncompData[xy].Set(col);
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
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				float vf = tMath::tSaturate((rdata[xy]+1.0f) / 2.0f);
				uint8 v = uint8( 255.0f * vf );
				tColour4i col(v, spread ? v : 0u, spread ? v : 0u, 255u);
				uncompData[xy].Set(col);
			}
			delete[] rdata;
			break;
		}

		case tPixelFormat::EACRG11:
		{
			struct RG { uint16 R; uint16 G; };
			// This format decompresses to RG uint8s.
			RG* rdata = new RG[wfull*hfull];

			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					uint16* dst = (uint16*)rdata + (y*wfull + x) * 2;
					etcdec_eac_rg11_u16(src, dst, wfull * sizeof(RG));
					src += ETCDEC_EAC_RG11_BLOCK_SIZE;
				}

			// Now convert to 32-bit RGBA.
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				uint8 r = uint8( (255*rdata[xy].R) / 65535 );
				uint8 g = uint8( (255*rdata[xy].G) / 65535 );
				tColour4i col(r, g, 0u, 255u);
				uncompData[xy].Set(col);
			}
			delete[] rdata;
			break;
		}

		case tPixelFormat::EACRG11S:
		{
			struct RG { float R; float G; };
			// This format decompresses to R float.
			RG* rdata = new RG[wfull*hfull];

			for (int y = 0; y < hfull; y += 4)
				for (int x = 0; x < wfull; x += 4)
				{
					float* dst = (float*)rdata + (y*wfull + x) * 2;
					etcdec_eac_rg11_float(src, dst, wfull * sizeof(RG), 1);
					src += ETCDEC_EAC_RG11_BLOCK_SIZE;
				}

			// Now convert to 32-bit RGBA.
			for (int xy = 0; xy < wfull*hfull; xy++)
			{
				float rf = tMath::tSaturate((rdata[xy].R+1.0f) / 2.0f);
				float gf = tMath::tSaturate((rdata[xy].G+1.0f) / 2.0f);
				uint8 r = uint8( 255.0f * rf );
				uint8 g = uint8( 255.0f * gf );
				tColour4i col(r, g, 0u, 255u);
				uncompData[xy].Set(col);
			}
			delete[] rdata;
			break;
		}

		default:
			delete[] uncompData;
			Clear();
			return false;
	}

	// Decode worked. We are now in RGBA 32-bit. Other params like width and height are already correct.
	// This isn't the most efficient because we don't have a stride in a tLayer, but correctness first.
	// Basically the uncompData may be too big if we needed extra room for w and h to do the decompression.
	// This happens when the image dimensions where not multiples of the block size. We deal with that here.
	// This is only inefficient if the dimensions were not a mult of 4, otherwise we can use the buffer directly.
	uint8* data = nullptr;
	if ((wfull == width) && (hfull == height))
	{
		data = (uint8*)uncompData;
	}
	else
	{
		data = new uint8[width*height*sizeof(tPixel)];
		uint8* s = (uint8*)uncompData;
		uint8* d = data;
		for (int r = 0; r < height; r++)
		{
			tStd::tMemcpy(d, s, width*sizeof(tPixel));
			s += wfull * sizeof(tPixel);
			d += width * sizeof(tPixel);
		}
		delete[] uncompData;
	}
	tAssert(data);

	// Apply gamma compression if requested.
	if ((params.Flags & LoadFlag_SRGBCompression) || (params.Flags & LoadFlag_GammaCompression))
	{
		tPixel* pixels = (tPixel*)data;
		for (int xy = 0; xy < width*height; xy++)
		{
			tColour4f colf(pixels[xy]);
			if (params.Flags & LoadFlag_SRGBCompression)
				colf.LinearToSRGB();
			else
				colf.LinearToGamma(params.Gamma);
			pixels[xy].SetR(colf.R); pixels[xy].SetG(colf.G); pixels[xy].SetB(colf.B);
		}
		ColourSpace = (params.Flags & LoadFlag_SRGBCompression) ? tColourSpace::sRGB : tColourSpace::Gamma;
	}

	bool reverseRowOrderRequested = params.Flags & LoadFlag_ReverseRowOrder;
	if (reverseRowOrderRequested)
	{
		// This shouldn't ever fail. Too easy to reverse RGBA 32-bit.
		uint8* reversedRowData = tImage::CreateReversedRowData(data, tPixelFormat::R8G8B8A8, width, height);
		tAssert(reversedRowData);
		delete[] data;
		data = reversedRowData;
	}

	PixelFormat = tPixelFormat::R8G8B8A8;
	Layer = new tLayer(PixelFormat, width, height, data, true);

	return true;
}


bool tImagePKM::Set(tPixel* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Layer			= new tLayer(tPixelFormat::R8G8B8A8, width, height, (uint8*)pixels, steal);
	PixelFormatSrc	= tPixelFormat::R8G8B8A8;
	PixelFormat		= tPixelFormat::R8G8B8A8;
	ColourSpaceSrc	= tColourSpace::sRGB;
	ColourSpace		= tColourSpace::sRGB;
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
	// Data must be decoded for this to work.
	if (!IsValid() || (PixelFormat != tPixelFormat::R8G8B8A8))
		return nullptr;

	tFrame* frame = new tFrame();
	frame->Width = Layer->Width;
	frame->Height = Layer->Height;
	frame->PixelFormatSrc = PixelFormatSrc;

	if (steal)
	{
		frame->Pixels = (tPixel*)Layer->StealData();
		delete Layer;
		Layer = nullptr;
	}
	else
	{
		frame->Pixels = new tPixel[frame->Width * frame->Height];
		tStd::tMemcpy(frame->Pixels, (tPixel*)Layer->Data, frame->Width * frame->Height * sizeof(tPixel));
	}

	return frame;
}


bool tImagePKM::IsOpaque() const
{
	if (!IsValid())
		return false;
	
	switch (Layer->PixelFormat)
	{
		case tPixelFormat::R8G8B8A8:
		{
			tPixel* pixels = (tPixel*)Layer->Data;
			for (int p = 0; p < (Layer->Width * Layer->Height); p++)
			{
				if (pixels[p].A < 255)
					return false;
			}
			break;
		}

		case tPixelFormat::EACR11:
		case tPixelFormat::EACR11S:
		case tPixelFormat::EACRG11:
		case tPixelFormat::EACRG11S:
		case tPixelFormat::ETC1:
		case tPixelFormat::ETC2RGB:
			return true;

		case tPixelFormat::ETC2RGBA1:
		case tPixelFormat::ETC2RGBA:
			return false;
	}

	return true;
}


}
