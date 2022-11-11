// tImageQOI.cpp
//
// This class knows how to load and save Quite OK Images (.qoi) files into tPixel arrays. These tPixels may be 'stolen'
// by the tPicture's constructor if a targa file is specified. After the array is stolen the tImageQOI is invalid. This
// is purely for performance.
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
#include "Image/tImageQOI.h"
#define QOI_NO_STDIO
#define QOI_IMPLEMENTATION
#include <QOI/qoi.h>
namespace tImage
{


bool tImageQOI::Load(const tString& qoiFile)
{
	Clear();

	if (tSystem::tGetFileType(qoiFile) != tSystem::tFileType::QOI)
		return false;

	if (!tSystem::tFileExists(qoiFile))
		return false;

	int numBytes = 0;
	uint8* qoiFileInMemory = tSystem::tLoadFile(qoiFile, nullptr, &numBytes);
	bool success = Set(qoiFileInMemory, numBytes);
	delete[] qoiFileInMemory;

	return success;
}


bool tImageQOI::Set(const uint8* qoiFileInMemory, int numBytes)
{
	Clear();
	if ((numBytes <= 0) || !qoiFileInMemory)
		return false;

	// WIP

	return true;
}


bool tImageQOI::Set(tPixel* pixels, int width, int height, bool steal)
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


tImageQOI::tFormat tImageQOI::Save(const tString& qoiFile, tFormat format) const
{
	if (!IsValid() || (format == tFormat::Invalid))
		return tFormat::Invalid;

	if (tSystem::tGetFileType(qoiFile) != tSystem::tFileType::QOI)
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


bool tImageQOI::IsOpaque() const
{
	for (int p = 0; p < (Width*Height); p++)
	{
		if (Pixels[p].A < 255)
			return false;
	}

	return true;
}


tPixel* tImageQOI::StealPixels()
{
	tPixel* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	
	return pixels;
}


}
