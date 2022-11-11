// tImageQOI.cpp
//
// This class knows how to load and save ARM's Adaptive Scalable Texture Compression (.astc) files into tPixel arrays.
// These tPixels may be 'stolen' by the tPicture's constructor if a targa file is specified. After the array is stolen
// the tImageASTC is invalid. This is purely for performance.
//
// Copyright (c) 2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <System/tFile.h>
#include "Image/tImageASTC.h"
using namespace tSystem;
namespace tImage
{


bool tImageASTC::Load(const tString& astcFile)
{
	Clear();

	if (tSystem::tGetFileType(astcFile) != tSystem::tFileType::ASTC)
		return false;

	if (!tFileExists(astcFile))
		return false;

	int numBytes = 0;
	uint8* astcFileInMemory = tLoadFile(astcFile, nullptr, &numBytes);
	bool success = Set(astcFileInMemory, numBytes);
	delete[] astcFileInMemory;

	return success;
}


bool tImageASTC::Set(const uint8* astcFileInMemory, int numBytes)
{
	Clear();
	if ((numBytes <= 0) || !astcFileInMemory)
		return false;

	// WIP

	return true;
}


bool tImageASTC::Set(tPixel* pixels, int width, int height, bool steal)
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

	SrcPixelFormat = tPixelFormat::R8G8B8A8;
	return true;
}


tImageASTC::tFormat tImageASTC::Save(const tString& astcFile, tFormat format) const
{
	if (!IsValid() || (format == tFormat::Invalid))
		return tFormat::Invalid;

	if (tSystem::tGetFileType(astcFile) != tSystem::tFileType::ASTC)
		return tFormat::Invalid;

	if (format == tFormat::Auto)
	{
		if (IsOpaque())
			format = tFormat::Bit24;
		else
			format = tFormat::Bit32;
	}

	bool success = false;

	// WIP
	
	if (!success)
		return tFormat::Invalid;

	return format;
}


bool tImageASTC::IsOpaque() const
{
	for (int p = 0; p < (Width*Height); p++)
	{
		if (Pixels[p].A < 255)
			return false;
	}

	return true;
}


tPixel* tImageASTC::StealPixels()
{
	tPixel* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	
	return pixels;
}


}
