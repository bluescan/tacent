// tImageTIFF.cpp
//
// This knows how to load/save TIFFs. It knows the details of the tiff file format and loads the data into multiple
// tPixel arrays, one for each frame (in a TIFF thay are called pages). These arrays may be 'stolen' by tPictures.
//
// Copyright (c) 2020-2024 Tristan Grimmer.
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
#include "Image/tPicture.h"
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
	tExprReader script(softwareStr, false);
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


bool tImageTIFF::WriteSoftwarePageDuration(TIFF* tiff, int milliseconds) const
{
	tString softwareDesc;
	tsPrintf(softwareDesc, "TacentLibrary V%d.%d.%d [PageDur %d]", tVersion::Major, tVersion::Minor, tVersion::Revision, milliseconds);
	int success = TIFFSetField(tiff, TIFFTAG_SOFTWARE, softwareDesc.Chr());
	return success ? true : false;
}


bool tImageTIFF::Load(const tString& tiffFile)
{
	Clear();

	if (tSystem::tGetFileType(tiffFile) != tSystem::tFileType::TIFF)
		return false;

	if (!tFileExists(tiffFile))
		return false;

	TIFF* tiff = TIFFOpen(tiffFile.Chr(), "rb");
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
		frame->Pixels = new tPixel4b[width*height];
		frame->PixelFormatSrc = tPixelFormat::R8G8B8A8;

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

	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	return true;
}


bool tImageTIFF::Set(tList<tFrame>& srcFrames, bool stealFrames)
{
	Clear();
	if (srcFrames.GetNumItems() <= 0)
		return false;

	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	if (stealFrames)
	{
		while (tFrame* frame = srcFrames.Remove())
			Frames.Append(frame);
	}
	else
	{
		for (tFrame* frame = srcFrames.Head(); frame; frame = frame->Next())
			Frames.Append(new tFrame(*frame));
	}

	return true;
}


bool tImageTIFF::Set(tPixel4b* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	tFrame* frame = new tFrame();
	if (steal)
		frame->StealFrom(pixels, width, height);
	else
		frame->Set(pixels, width, height);
	Frames.Append(frame);
	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	return true;
}


bool tImageTIFF::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	if (steal)
		Frames.Append(frame);
	else
		Frames.Append(new tFrame(*frame));
	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	return true;
}


bool tImageTIFF::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel4b* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


tFrame* tImageTIFF::GetFrame(bool steal)
{
	if (!IsValid())
		return nullptr;

	return steal ? Frames.Remove() : new tFrame( *Frames.First() );
}


bool tImageTIFF::IsOpaque() const
{
	for (tFrame* frame = Frames.Head(); frame; frame = frame->Next())
		if (!frame->IsOpaque())
			return false;

	return true;
}


bool tImageTIFF::Save(const tString& tiffFilename, tFormat format, bool useZLibComp, int overrideFrameDuration) const
{
	SaveParams params;
	params.Format = format;
	params.UseZLibCompression = useZLibComp;
	params.OverrideFrameDuration = overrideFrameDuration;
	return Save(tiffFilename, params);
}


bool tImageTIFF::Save(const tString& tiffFile, const SaveParams& params) const
{
	if (!IsValid() || (params.Format == tFormat::Invalid))
		return false;

	if (tSystem::tGetFileType(tiffFile) != tSystem::tFileType::TIFF)
		return false;

	TIFF* tiff = TIFFOpen(tiffFile.Chr(), "wb");
	if (!tiff)
		return false;

	int rowSize = 0;
	uint8* rowBuf = nullptr;
	for (tFrame* frame = Frames.First(); frame; frame = frame->Next())
	{
		// Writes image from last loop and starts a new directory.
		if (frame != Frames.First())
			TIFFWriteDirectory(tiff);

		bool isOpaque = frame->IsOpaque();
		int bytesPerPixel = 0;
		switch (params.Format)
		{
			case tFormat::Auto:		bytesPerPixel = isOpaque ? 3 : 4;	break;
			case tFormat::BPP24:	bytesPerPixel = 3;					break;
			case tFormat::BPP32:	bytesPerPixel = 4;					break;
		}
		tAssert(bytesPerPixel);

		int w = frame->Width;
		int h = frame->Height;

		TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, w);
		TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, h);
		TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, bytesPerPixel);
		TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
		TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		TIFFSetField(tiff, TIFFTAG_COMPRESSION, params.UseZLibCompression ? COMPRESSION_DEFLATE : COMPRESSION_NONE);
		TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		if (bytesPerPixel == 4)
		{
			// Unassociated alpha means the extra channel is not a premultiplied alpha channel.
			uint16 extraSampleTypes[] = { EXTRASAMPLE_UNASSALPHA };
			TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, tNumElements(extraSampleTypes), extraSampleTypes);
		}

		int pageDurMilliSec = (params.OverrideFrameDuration >= 0) ? params.OverrideFrameDuration : int(frame->Duration*1000.0f);
		WriteSoftwarePageDuration(tiff, pageDurMilliSec);

		int rowSizeNeeded = TIFFScanlineSize(tiff);
		tAssert(rowSizeNeeded == (w*bytesPerPixel));

		// Let's not reallocate the line buffer every frame. Often all frames will be the same size.
		if (rowSize != rowSizeNeeded)
		{
			if (rowBuf)
				_TIFFfree(rowBuf);
			rowBuf = (uint8*)_TIFFmalloc(rowSizeNeeded);
			rowSize = rowSizeNeeded;
		}

		TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff, w*bytesPerPixel));
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

			int errCode = TIFFWriteScanline(tiff, rowBuf, r, 0);
			if (errCode < 0)
				continue;
		}
    }

	if (rowBuf)
		_TIFFfree(rowBuf);
	TIFFClose(tiff);

	return true;
}


}
