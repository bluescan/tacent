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
		char Version[2];					// Often '1', '0' or '2', '0'.
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

		uint32 GetFormat() const			{ return (FormatMSB << 8) | FormatLSB; }
		uint32 GetEncodedWidth() const		{ return (EncodedWidthMSB << 8) | EncodedWidthLSB; }
		uint32 GetEncodedHeight() const		{ return (EncodedHeightMSB << 8) | EncodedHeightLSB; }
		uint32 GetWidth() const				{ return (WidthMSB << 8) | WidthLSB; }
		uint32 GetHeight() const			{ return (HeightMSB << 8) | HeightLSB; }
	};
	#pragma pack(pop)

	// Format codes.
	const uint32 PKMFMT_ETC1_RGB			= 0x0000;
	const uint32 PKMFMT_ETC1_RGB8_OES		= 0x8D64;		// OES just means the format internal ID was developed by the working group (Kronos I assume).

	bool IsHeaderValid(const Header&);
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

	const tPKM::Header* header = (const tPKM::Header*)pkmFileInMemory;
	const uint8* data = pkmFileInMemory + sizeof(tPKM::Header);

	bool valid = tPKM::IsHeaderValid(*header);
	if (!valid)
		return false;

	PixelFormatSrc = tPixelFormat::R8G8B8;

	// Now we need to get it into RGBA, and not upside down.
	Width = header->GetWidth();
	Height = header->GetHeight();

/*
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


}
