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
#include "Image/tPicture.h"
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

	// Format codes. These are what will be found in the pkm header. They match the OpenGL texture format IDs.
	// Note1: ETC1 pkm files should assume ETC1_RGB8_OES even if the format is not set to that.
	// Note2: The sRGB formats are decoded the same as the non-sRGB formats. It is only the interpretation
	//        of the pixel values that changes.
	// Note3: ETC1_RGB8 and ETC2_RGB8 and ETC2_sRGB8 are all decoded with the same RGB decode. This is
	//        because ETC2 is backwards compatible with ETC1.
	enum class PKMFMT
	{
		ETC1_RGB8		= 0x8D64,		// GL_ETC1_RGB8_OES. OES just means the format internal ID was developed by the working group (Kronos I assume).
		ETC2_R11		= 0x9270,		// GL_COMPRESSED_R11_EAC
		ETC2_R11s		= 0x9271,		// GL_COMPRESSED_SIGNED_R11_EAC
		ETC2_RG11		= 0x9272,		// GL_COMPRESSED_RG11_EAC
		ETC2_RG11s		= 0x9273,		// GL_COMPRESSED_SIGNED_RG11_EAC
		ETC2_RGB8		= 0x9274,		// GL_COMPRESSED_RGB8_ETC2
		ETC2_sRGB8		= 0x9275,		// GL_COMPRESSED_SRGB8_ETC2
		ETC2_RGB8A1		= 0x9276,		// GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
		ETC2_sRGB8A1	= 0x9277,		// GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2
		ETC2_RGBA8		= 0x9278,		// GL_COMPRESSED_RGBA8_ETC2_EAC
		ETC2_sRGBA8		= 0x9279		// GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
	};
	bool IsHeaderValid(const Header&);

	// These figure out the pixel format and the colour-space. tPixelFormat does not specify ancilllary
	// properties of the data -- it specified the encoding of the data. The extra information, like the colour-space it
	// was authored in, is stored in tColourSpace. In many cases this satellite information cannot be determined, in
	// which case colour-space will be set to their 'unspecified' enumerant.
	void GetFormatInfo_FromPKMFormat(tPixelFormat&, tColourSpace&, uint32 pkmFmt);
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


bool tImagePKM::Load(const uint8* pkmFileInMemory, int numBytes, const LoadParams& params)
{
	Clear();
	if ((numBytes <= 0) || !pkmFileInMemory)
		return false;

	const tPKM::Header* header = (const tPKM::Header*)pkmFileInMemory;
	const uint8* data = pkmFileInMemory + sizeof(tPKM::Header);

	bool valid = tPKM::IsHeaderValid(*header);
	if (!valid)
		return false;

/*
	PixelFormatSrc = tPixelFormat::R8G8B8;

	// Now we need to get it into RGBA, and not upside down.
	Width = header->GetWidth();
	Height = header->GetHeight();

	Pixels = new tPixel[Width*Height];
	tColour3i* rgbPixels = new tColour3i[Width*Height];
	int stride = Width;
	bool ok = tPKM::Uncompress
	if (!ok)
	{
		delete[] rgbPixels;
		return false;
	}

	// Reverse rows and update Pixels.
	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			int srcIdx = Width*(Height-y-1) + x;
			Pixels[y*Width + x].Set( rgbPixels[srcIdx], 255 );
		}
	}
	delete[] rgbPixels;
*/

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
