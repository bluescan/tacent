// tImageTIFF.cpp
//
// This knows how to load/save TIFFs. It knows the details of the tiff file format and loads the data into multiple
// tPixel arrays, one for each frame (in a TIFF thay are called pages). These arrays may be 'stolen' by tPictures.
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

#include <Foundation/tVersion.cmake.h>
#include <Foundation/tStandard.h>
#include <Foundation/tString.h>
#include <System/tFile.h>
#include <System/tScript.h>
#include "Image/tImageTIFF.h"
#include "LibTIFF/include/tiff.h"
#include "LibTIFF/include/tiffio.h"
#include "LibTIFF/include/tiffvers.h"
using namespace tSystem;
namespace tImage
{


int tImageTIFF::ReadSoftwarePageDuration(TIFF* tiff) const
{
	void* data = nullptr;
	int durationMilliSec = -1;
	int success = TIFFGetField(tiff, TIFFTAG_SOFTWARE, &data);
	if (!success || !data)
		return durationMilliSec;

	tString softwareStr((char*)data);
	tScriptReader script(softwareStr, false);
	tExpression tacentView = script.First();
	if (tacentView.GetAtomString() == "TacentLibrary")
	{
		tExpression tacentVers = tacentView.Next();
		tExpression durationEx = tacentVers.Next();
		tExpression durCmd = durationEx.Item0();
		if (durCmd.GetAtomString() == "PageDur")
		{
			tExpression durVal = durationEx.Item1();
			durationMilliSec = durVal.GetAtomInt();
		}
	}

	return durationMilliSec;
}


bool tImageTIFF::WriteSoftwarePageDuration(TIFF* tiff, int milliseconds)
{
	tString softwareDesc;
	tsPrintf(softwareDesc, "TacentLibrary V%d.%d.%d [PageDur %d]", tVersion::Major, tVersion::Minor, tVersion::Revision, milliseconds);
	int success = TIFFSetField(tiff, TIFFTAG_SOFTWARE, softwareDesc.Chars());
	return success ? true : false;
}


bool tImageTIFF::Load(const tString& tiffFile)
{
	Clear();

	if (tSystem::tGetFileType(tiffFile) != tSystem::tFileType::TIFF)
		return false;

	if (!tFileExists(tiffFile))
		return false;

	TIFF* tiff = TIFFOpen(tiffFile.Chars(), "rb");
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
		int durationMilliSeconds = ReadSoftwarePageDuration(tiff);

		uint32* pixels = (uint32*)_TIFFmalloc(numPixels * sizeof(uint32));
		int successCode = TIFFReadRGBAImage(tiff, width, height, pixels, 0);
		if (!successCode)
		{
			_TIFFfree(pixels);
			break;
		}

		tFrame* frame = new tFrame;
		frame->Width = width;
		frame->Height = height;
		frame->Pixels = new tPixel[width*height];
		frame->SrcPixelFormat = tPixelFormat::R8G8B8A8;

		// If duration not set we use a default of 1 second.
		frame->Duration = (durationMilliSeconds >= 0) ? float(durationMilliSeconds)/1000.0f : 1.0f;

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


bool tImageTIFF::Set(tList<tFrame>& srcFrames, bool stealFrames)
{
	Clear();
	if (srcFrames.GetNumItems() <= 0)
		return false;

	tPixelFormat SrcPixelFormat = tPixelFormat::R8G8B8A8;
	if (stealFrames)
	{
		while (tFrame* frame = srcFrames.Remove())
			Frames.Append(frame);
		return true;
	}

	for (tFrame* frame = srcFrames.Head(); frame; frame = frame->Next())
		Frames.Append(new tFrame(*frame));

	return true;
}


bool tImageTIFF::IsOpaque() const
{
	for (tFrame* frame = Frames.Head(); frame; frame = frame->Next())
		if (!frame->IsOpaque())
			return false;

	return true;
}


bool tImageTIFF::Save(const tString& tiffFilename, bool useZLibComp, int overrideFrameDuration)
{
	if (!IsValid())
		return false;

	TIFF* tiffFile = TIFFOpen(tiffFilename.Chars(), "wb");
	if (!tiffFile)
		return false;

	int rowSize = 0;
	uint8* rowBuf = nullptr;
	for (tFrame* frame = Frames.First(); frame; frame = frame->Next())
	{
		// Writes image from last loop and starts a new directory.
		if (frame != Frames.First())
			TIFFWriteDirectory(tiffFile);

		bool isOpaque = frame->IsOpaque();
		int bytesPerPixel = isOpaque ? 3 : 4;
		int w = frame->Width;
		int h = frame->Height;

		TIFFSetField(tiffFile, TIFFTAG_IMAGEWIDTH, w);
		TIFFSetField(tiffFile, TIFFTAG_IMAGELENGTH, h);
		TIFFSetField(tiffFile, TIFFTAG_SAMPLESPERPIXEL, bytesPerPixel);
		TIFFSetField(tiffFile, TIFFTAG_BITSPERSAMPLE, 8);
		TIFFSetField(tiffFile, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		TIFFSetField(tiffFile, TIFFTAG_COMPRESSION, useZLibComp ? COMPRESSION_DEFLATE : COMPRESSION_NONE);
		TIFFSetField(tiffFile, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(tiffFile, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

		int pageDurMilliSec = (overrideFrameDuration >= 0) ? overrideFrameDuration : int(frame->Duration*1000.0f);
		WriteSoftwarePageDuration(tiffFile, pageDurMilliSec);

		int rowSizeNeeded = TIFFScanlineSize(tiffFile);
		tAssert(rowSizeNeeded == (w*bytesPerPixel));

		// Let's not reallocate the line buffer every frame. Often all frames will be the same size.
		if (rowSize != rowSizeNeeded)
		{
			if (rowBuf)
				_TIFFfree(rowBuf);
			rowBuf = (uint8*)_TIFFmalloc(rowSizeNeeded);
			rowSize = rowSizeNeeded;
		}

		TIFFSetField(tiffFile, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiffFile, w*bytesPerPixel));
		for (int r = 0; r < h; r++)
		{
			for (int x = 0; x < w; x++)
			{
				int idx = (h-r-1)*w + x;
				rowBuf[x*bytesPerPixel + 0] = frame->Pixels[idx].R;
				rowBuf[x*bytesPerPixel + 1] = frame->Pixels[idx].G;
				rowBuf[x*bytesPerPixel + 2] = frame->Pixels[idx].B;
				if (bytesPerPixel == 4)
					rowBuf[x*bytesPerPixel + 3] = frame->Pixels[idx].A;
			}

			int errCode = TIFFWriteScanline(tiffFile, rowBuf, r, 0);
			if (errCode < 0)
				continue;
		}
    }

	if (rowBuf)
		_TIFFfree(rowBuf);
	TIFFClose(tiffFile);

	return true;
}


}
