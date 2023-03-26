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


	// @wip
	// Set Width, Height, PixelFormatSrc, Pixels.
	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	Width = 0;
	Height = 0;
	Pixels = nullptr;

	return false;
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
//	for (int p = 0; p < (Width*Height); p++)
//	{
//		if (Pixels[p].A < 255)
//			return false;
//	}

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


}
