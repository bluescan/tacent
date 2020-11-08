// tImageAPNG.cpp
//
// This knows how to load animated PNGs (APNGs). It knows the details of the apng file format and loads the data into
// multiple tPixel arrays, one for each frame. These arrays may be 'stolen' by tPictures.
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
#include "Image/tImageAPNG.h"
using namespace tSystem;
namespace tImage
{


bool tImageAPNG::Load(const tString& apngFile)
{
	Clear();

	if (tSystem::tGetFileType(apngFile) != tSystem::tFileType::APNG)
		return false;

	if (!tFileExists(apngFile))
		return false;

	int numBytes = 0;
	uint8* apngFileInMemory = tLoadFile(apngFile, nullptr, &numBytes);

	// Now we load and populate the frames.
	SrcPixelFormat = tPixelFormat::R8G8B8;
	for (int f = 0; f < 1; f++)
	{
		Frame* newFrame = new Frame;
		newFrame->SrcPixelFormat = tPixelFormat::R8G8B8A8;
		int width = 64;
		int height = 64;
		newFrame->Width = width;
		newFrame->Height = height;
		newFrame->Pixels = new tPixel[width * height];
		newFrame->Duration = 0.1f;
		for (int p = 0; p < width*height; p++)
			newFrame->Pixels[p] = tPixel::black;

		Frames.Append(newFrame);
	}

	return true;
}


}
