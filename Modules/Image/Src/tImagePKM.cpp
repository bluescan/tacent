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

	// These figure out the pixel format and the colour-profile. tPixelFormat does not specify ancilllary
	// properties of the data -- it specified the encoding of the data. The extra information, like the colour-space it
	// was authored in, is stored in tColourProfile. In many cases this satellite information cannot be determined, in
	// which case colour-profile will be set to their 'unspecified' enumerant.
	void GetFormatInfo_FromPKMFormat(tPixelFormat&, tColourProfile&, uint32 pkmFmt, int version);
}


void tPKM::GetFormatInfo_FromPKMFormat(tPixelFormat& fmt, tColourProfile& pro, uint32 pkmFmt, int version)
{
	fmt = tPixelFormat::Invalid;
	pro = tColourProfile::sRGB;
	switch (PKMFMT(pkmFmt))
	{
		case PKMFMT::ETC1_RGB:		fmt = tPixelFormat::ETC1;		pro = tColourProfile::sRGB;		break;
		case PKMFMT::ETC2_RGB:		fmt = tPixelFormat::ETC2RGB;	pro = tColourProfile::lRGB;		break;
		case PKMFMT::ETC2_sRGB:		fmt = tPixelFormat::ETC2RGB;	pro = tColourProfile::sRGB;		break;
		case PKMFMT::ETC2_RGBA_OLD:
		case PKMFMT::ETC2_RGBA:		fmt = tPixelFormat::ETC2RGBA;	pro = tColourProfile::lRGB;		break;
		case PKMFMT::ETC2_sRGBA:	fmt = tPixelFormat::ETC2RGBA;	pro = tColourProfile::sRGB;		break;
		case PKMFMT::ETC2_RGBA1:	fmt = tPixelFormat::ETC2RGBA1;	pro = tColourProfile::lRGB;		break;
		case PKMFMT::ETC2_sRGBA1:	fmt = tPixelFormat::ETC2RGBA1;	pro = tColourProfile::sRGB;		break;
		case PKMFMT::ETC2_R:		fmt = tPixelFormat::EACR11U;	pro = tColourProfile::sRGB;		break;
		case PKMFMT::ETC2_RG:		fmt = tPixelFormat::EACRG11U;	pro = tColourProfile::sRGB;		break;
		case PKMFMT::ETC2_R_SIGNED:	fmt = tPixelFormat::EACR11S;	pro = tColourProfile::sRGB;		break;
		case PKMFMT::ETC2_RG_SIGNED:fmt = tPixelFormat::EACRG11S;	pro = tColourProfile::sRGB;		break;
	}

	// If the format is still invalid we encountered an invalid format in the PKM header.
	// In this case we base the format on the header version number only.
	if (fmt == tPixelFormat::Invalid)
	{
		pro = tColourProfile::sRGB;
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
	uint32 encodedWidth = header.GetEncodedWidth();
	uint32 encodedHeight = header.GetEncodedHeight();
	uint32 width = header.GetWidth();
	uint32 height = header.GetHeight();

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
	tColourProfile profile;
	tPKM::GetFormatInfo_FromPKMFormat(format, profile, header->GetFormat(), header->GetVersion());
	if (!tIsBCFormat(format))
		return false;

	PixelFormat = format;
	PixelFormatSrc = format;
	ColourProfile = profile;
	ColourProfileSrc = profile;

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
		if (tMath::tIsProfileLinearInRGB(ColourProfileSrc))
			params.Flags |= LoadFlag_SRGBCompression;
	}

	tColour4f* decoded4f = nullptr;
	tColour4i* decoded4i = nullptr;
	DecodeResult result = tImage::DecodePixelData_Block(format, (uint8*)pkmData, pkmDataSize, width, height, decoded4i, decoded4f);
	if (result != DecodeResult::Success)
		return false;

	// Apply any decode flags.
	tAssert(decoded4f || decoded4i);
	bool flagSRGB = (params.Flags & tImagePKM::LoadFlag_SRGBCompression) ? true : false;
	bool flagGama = (params.Flags & tImagePKM::LoadFlag_GammaCompression)? true : false;
	if (decoded4f && (flagSRGB || flagGama))
	{
		for (int p = 0; p < width*height; p++)
		{
			tColour4f& colour = decoded4f[p];
			if (flagSRGB)
				colour.LinearToSRGB(tCompBit_RGB);
			if (flagGama)
				colour.LinearToGamma(params.Gamma, tCompBit_RGB);
		}
	}
	if (decoded4i && (flagSRGB || flagGama))
	{
		for (int p = 0; p < width*height; p++)
		{
			tColour4f colour(decoded4i[p]);
			if (flagSRGB)
				colour.LinearToSRGB(tCompBit_RGB);
			if (flagGama)
				colour.LinearToGamma(params.Gamma, tCompBit_RGB);
			decoded4i[p].SetR(colour.R);
			decoded4i[p].SetG(colour.G);
			decoded4i[p].SetB(colour.B);
		}
	}

	// Converts to RGBA32 into the decoded4i array.
	if (decoded4f)
	{
		tAssert(!decoded4i);
		decoded4i = new tColour4i[width*height];
		for (int p = 0; p < width*height; p++)
			decoded4i[p].Set(decoded4f[p]);
		delete[] decoded4f;
	}

	// Possibly spread the L/Red channel.
	if (spread && tIsLuminanceFormat(PixelFormatSrc))
	{
		for (int p = 0; p < width*height; p++)
		{
			decoded4i[p].G = decoded4i[p].R;
			decoded4i[p].B = decoded4i[p].R;
		}
	}

	// Give decoded pixelData to layer.
	tAssert(!Layer);
	Layer = new tLayer(tPixelFormat::R8G8B8A8, width, height, (uint8*)decoded4i, true);
	tAssert(Layer->OwnsData);

	// We've got one more chance to reverse the rows here (if we still need to) because we were asked to decode.
	if (params.Flags & tImagePKM::LoadFlag_ReverseRowOrder)
	{
		// This shouldn't ever fail. Too easy to reverse RGBA 32-bit.
		uint8* reversedRowData = tImage::CreateReversedRowData(Layer->Data, Layer->PixelFormat, width, height);
		tAssert(reversedRowData);
		delete[] Layer->Data;
		Layer->Data = reversedRowData;
	}

	// Finally update the current pixel format -- but not the source format.
	PixelFormat = tPixelFormat::R8G8B8A8;
	tAssert(IsValid());
	return true;
}


bool tImagePKM::Set(tPixel* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Layer				= new tLayer(tPixelFormat::R8G8B8A8, width, height, (uint8*)pixels, steal);
	PixelFormatSrc		= tPixelFormat::R8G8B8A8;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::LDRsRGB_LDRlA;
	ColourProfile		= tColourProfile::LDRsRGB_LDRlA;
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

		case tPixelFormat::EACR11U:
		case tPixelFormat::EACR11S:
		case tPixelFormat::EACRG11U:
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
