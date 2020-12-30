// tImageTIFF.cpp
//
// This knows how to load TIFFs. It knows the details of the tiff file format and loads the data into multiple tPixel
// arrays, one for each frame (in a TIFF thay are called pages). These arrays may be 'stolen' by tPictures.
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

#include <Foundation/tStandard.h>
#include <Foundation/tString.h>
#include <System/tFile.h>
#include "Image/tImageTIFF.h"
#include "LibTIFF/include/tiff.h"
#include "LibTIFF/include/tiffio.h"
#include "LibTIFF/include/tiffvers.h"
using namespace tSystem;
namespace tImage
{


bool tImageTIFF::Load(const tString& tiffFile)
{
	Clear();

	if (tSystem::tGetFileType(tiffFile) != tSystem::tFileType::TIFF)
		return false;

	if (!tFileExists(tiffFile))
		return false;

	TIFF* tiff = TIFFOpen(tiffFile.Chars(), "r");
	if (!tiff)
		return false;

	// Create all frames.
	do
	{
		int width = 0; int height = 0;
		TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
		TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
		if ((width <= 0) || (height <= 0))
			break;
		int numPixels = width*height;
			
		uint32* pixels = (uint32*)_TIFFmalloc(numPixels * sizeof(uint32));
		int successCode = TIFFReadRGBAImage(tiff, width, height, pixels, 0);
		if (!successCode)
		{
			_TIFFfree(pixels);
			break;
		}

		Frame* frame = new Frame;
		frame->Width = width;
		frame->Height = height;
		frame->Pixels = new tPixel[width*height];
		frame->SrcPixelFormat = tPixelFormat::R8G8B8A8;

		for (int p = 0; p < width*height; p++)
			frame->Pixels[p] = pixels[p];

		_TIFFfree(pixels);
		Frames.Append(frame);
	} while (TIFFReadDirectory(tiff));

	TIFFClose(tiff);
	if (Frames.GetNumItems() == 0)
		return false;

	SrcPixelFormat = tPixelFormat::R8G8B8A8;
	return true;
}


}
