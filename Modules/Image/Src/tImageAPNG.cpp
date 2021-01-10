// tImageAPNG.cpp
//
// This knows how to load animated PNGs (APNGs). It knows the details of the apng file format and loads the data into
// multiple tPixel arrays, one for each frame. These arrays may be 'stolen' by tPictures.
//
// Copyright (c) 2020, 2021 Tristan Grimmer.
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
#include "apngdis.h"
using namespace tSystem;
namespace tImage
{


bool tImageAPNG::IsAnimatedPNG(const tString& pngFile)
{
	int numBytes = 1024;
	uint8* headData = tSystem::tLoadFileHead(pngFile, numBytes);
	if (!headData)
		return false;

	uint8 acTL[] = { 'a', 'c', 'T', 'L' };
	uint8 IDAT[] = { 'I', 'D', 'A', 'T' };
	uint8* actlLoc = (uint8*)tStd::tMemmem(headData, numBytes, acTL, sizeof(acTL));
	if (!actlLoc)
		return false;

	// Now for safety we also make sure there is an IDAT after the acTL.
	uint8* idatLoc = (uint8*)tStd::tMemmem(actlLoc+sizeof(acTL), numBytes - (actlLoc-headData) - sizeof(acTL), IDAT, sizeof(IDAT));

	bool found = idatLoc ? true : false;
	delete[] headData;
	return found;
}


bool tImageAPNG::Load(const tString& apngFile)
{
	Clear();

	// Note that many apng files still have a .png extension/filetype, so we support both here.
	tSystem::tFileType filetype = tSystem::tGetFileType(apngFile);
	if ((filetype != tSystem::tFileType::APNG) && (filetype != tSystem::tFileType::PNG))
		return false;

	if (!tFileExists(apngFile))
		return false;

	std::vector<APngDis::Image> frames;
	int result = APngDis::load_apng(apngFile.Chars(), frames);
	if (result < 0)
		return false;

	// Now we load and populate the frames.
	SrcPixelFormat = tPixelFormat::R8G8B8A8;
	for (int f = 0; f < frames.size(); f++)
	{
		APngDis::Image& srcFrame = frames[f];
		tFrame* newFrame = new tFrame;
		newFrame->SrcPixelFormat = tPixelFormat::R8G8B8A8;
		int width = srcFrame.w;
		int height = srcFrame.h;
		newFrame->Width = width;
		newFrame->Height = height;
		newFrame->Pixels = new tPixel[width * height];

		// From the official apng spec:
		// The delay_num and delay_den parameters together specify a fraction indicating the time to display
		// the current frame, in seconds. If the denominator is 0, it is to be treated as if it were 100 (that
		// is, delay_num then specifies 1/100ths of a second). If the the value of the numerator is 0 the decoder
		// should render the next frame as quickly as possible, though viewers may impose a reasonable lower bound.
		uint dnum = srcFrame.delay_num;
		uint dden = srcFrame.delay_den;
		if (dden == 0)
			dden = 100;
		newFrame->Duration = (dnum == 0) ? 1.0f/60.0f : float(dnum) / float(dden);

		tAssert(srcFrame.bpp == 4);
		for (int r = 0; r < height; r++)
		{
			uint8* srcRowData = srcFrame.rows[r];
			// uint8* dstRowData = (uint8*)newFrame->Pixels + r * (width*4);
			uint8* dstRowData = (uint8*)newFrame->Pixels + ((height-1)-r) * (width*4);
			tStd::tMemcpy(dstRowData, srcRowData, width*4);
		}

		Frames.Append(newFrame);
	}

	for (int f = 0; f < frames.size(); f++)
		frames[f].free();
	frames.clear();

	return true;
}


}
