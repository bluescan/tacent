// tImagePNG.cpp
//
// This class knows how to load and save PNG files. It does zero processing of image data. It knows the details of the
// png file format and loads the data into a tPixel array. These tPixels may be 'stolen' by the tPicture's constructor
// if a png file is specified. After the array is stolen the tImagePNG is invalid. This is purely for performance.
//
// Copyright (c) 2020 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <System/tFile.h>
#include "Image/tImagePNG.h"
// include libpng.

using namespace tSystem;
namespace tImage
{


bool tImagePNG::Load(const tString& pngFile)
{
	Clear();

	if (tSystem::tGetFileType(pngFile) != tSystem::tFileType::PNG)
		return false;

	if (!tFileExists(pngFile))
		return false;

	int numBytes = 0;
	uint8* pngFileInMemory = tLoadFile(pngFile, nullptr, &numBytes);
	bool success = Set(pngFileInMemory, numBytes);
	delete[] pngFileInMemory;

	return success;
}


bool tImagePNG::Set(const uint8* pngFileInMemory, int numBytes)
{
	Clear();
	if ((numBytes <= 0) || !pngFileInMemory)
		return false;

	// Load the png and figure out width and height.
	int numPixels = Width * Height;
	Pixels = new tPixel[numPixels];
	for (int p = 0; p < numPixels; p++)
		Pixels[p] = tColouri::black;

	SrcPixelFormat = tPixelFormat::R8G8B8;
	return true;
}


bool tImagePNG::Set(tPixel* pixels, int width, int height, bool steal)
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


bool tImagePNG::Save(const tString& pngFile) const
{
	if (!IsValid())
		return false;

	if (tSystem::tGetFileType(pngFile) != tSystem::tFileType::PNG)
		return false;

	tFileHandle fileHandle = tOpenFile(pngFile.Chars(), "wb");
	if (!fileHandle)
	{
		return false;
	}

	uint8 pngBuf[64];
	int pngBufSize = sizeof(pngBuf) / sizeof(*pngBuf);
	for (int a = 0; a < pngBufSize; a++)
		pngBuf[a] = 0;

	bool success = tWriteFile(fileHandle, pngBuf, pngBufSize);
	tCloseFile(fileHandle);

	return true;
}


tPixel* tImagePNG::StealPixels()
{
	tPixel* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	return pixels;
}


}
