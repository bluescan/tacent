// tImageXPM.cpp
//
// This class knows how to load and save an X-Windows Pix Map (.xpm) file. It knows the details of the xpm file format
// and loads the data into a tPixel array. These tPixels may be 'stolen' by the tPicture's constructor if a xpm file is
// specified. After the array is stolen the tImageXPM is invalid. This is purely for performance.
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
#include "Image/tImageXPM.h"


using namespace tSystem;
namespace tImage
{


bool tImageXPM::Load(const tString& xpmFile)
{
	Clear();

	if (tSystem::tGetFileType(xpmFile) != tSystem::tFileType::XPM)
		return false;

	if (!tFileExists(xpmFile))
		return false;

	int numBytes = 0;
	uint8* xpmFileInMemory = tLoadFile(xpmFile, nullptr, &numBytes);
	bool success = Set(xpmFileInMemory, numBytes);
	delete[] xpmFileInMemory;

	return success;
}


bool tImageXPM::Set(const uint8* xpmFileInMemory, int numBytes)
{
	Clear();
	if ((numBytes <= 0) || !xpmFileInMemory)
		return false;

	// Set Width, Height, Pixels, and SrcPixelFormat.
	// @todo I'm going to build my own xpm parser here. I really found the official libxpm
	// very akward and it required unnecessary platform headers that are not needed for text file parsing.
	Width = 256;
	Height = 256;
	int numPixels = Width * Height;
	Pixels = new tPixel[numPixels];
	for (int p = 0; p < Width*Height; p++)
	{
		Pixels[p] = tPixel::blue;
	}
	SrcPixelFormat = tPixelFormat::R8G8B8A8;

	return true;
}


bool tImageXPM::Set(tPixel* pixels, int width, int height, bool steal)
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


tPixel* tImageXPM::StealPixels()
{
	tPixel* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	return pixels;
}


}
