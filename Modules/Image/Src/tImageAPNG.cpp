// tImageAPNG.cpp
//
// This knows how to load/save animated PNGs (APNGs). It knows the details of the apng file format and loads the data
// into multiple tPixel arrays, one for each frame. These arrays may be 'stolen' by tPictures.
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

#include <Foundation/tStandard.h>
#include <Foundation/tString.h>
#include <System/tFile.h>
#include "Image/tImageAPNG.h"
#include "Image/tPicture.h"
#include "apngdis.h"
#include "apngasm.h"
using namespace tSystem;
namespace tImage
{


bool tImageAPNG::IsAnimatedPNG(const tString& pngFile)
{
	int numBytes = 2048;

	// Remember, tLoadFileHead modifies numBytes if the file is smaller than the head-size requested.
	uint8* headData = tSystem::tLoadFileHead(pngFile, numBytes);
	if (!headData)
		return false;

	uint8 acTL[] = { 'a', 'c', 'T', 'L' };
	uint8 IDAT[] = { 'I', 'D', 'A', 'T' };
	uint8* actlLoc = (uint8*)tStd::tMemsrch(headData, numBytes, acTL, sizeof(acTL));
	if (!actlLoc)
		return false;

	// Now for safety we also make sure there is an IDAT after the acTL.
	uint8* idatLoc = (uint8*)tStd::tMemsrch(actlLoc+sizeof(acTL), numBytes - (actlLoc-headData) - sizeof(acTL), IDAT, sizeof(IDAT));

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

	// We assume here that load_apng can hande UTF-8 filenames.
	int result = APngDis::load_apng(apngFile.Chr(), frames);
	if (result < 0)
		return false;

	// Now we load and populate the frames.
	for (int f = 0; f < frames.size(); f++)
	{
		APngDis::Image& srcFrame = frames[f];
		tFrame* newFrame = new tFrame;
		int width = srcFrame.w;
		int height = srcFrame.h;
		newFrame->Width = width;
		newFrame->Height = height;
		newFrame->Pixels = new tPixel4b[width * height];

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
			uint8* dstRowData = (uint8*)newFrame->Pixels + ((height-1)-r) * (width*4);
			tStd::tMemcpy(dstRowData, srcRowData, width*4);
		}

		Frames.Append(newFrame);
	}

	for (int f = 0; f < frames.size(); f++)
		frames[f].free();
	frames.clear();

	PixelFormatSrc		= tPixelFormat::R8G8B8A8;
	PixelFormat			= tPixelFormat::R8G8B8A8;

	// Assume colour profile is sRGB for source and current.
	ColourProfileSrc	= tColourProfile::sRGB;
	ColourProfile		= tColourProfile::sRGB;

	if (IsOpaque())
		PixelFormatSrc = tPixelFormat::R8G8B8;

	// Set every frame's source pixel format.
	for (tFrame* frame = Frames.Head(); frame; frame = frame->Next())
		frame->PixelFormatSrc = PixelFormatSrc;

	return true;
}


bool tImageAPNG::Set(tList<tFrame>& srcFrames, bool stealFrames)
{
	Clear();
	if (srcFrames.GetNumItems() <= 0)
		return false;

	PixelFormatSrc		= srcFrames.Head()->PixelFormatSrc;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume srcFrames must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

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


bool tImageAPNG::Set(tPixel4b* pixels, int width, int height, bool steal)
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

	PixelFormatSrc		= tPixelFormat::R8G8B8A8;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume pixels must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

	return true;
}


bool tImageAPNG::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	PixelFormatSrc		= frame->PixelFormatSrc;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume frame must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

	if (steal)
		Frames.Append(frame);
	else
		Frames.Append(new tFrame(*frame));
	return true;
}


bool tImageAPNG::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	PixelFormatSrc		= picture.PixelFormatSrc;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	// We don't know colour profile of tPicture.

	// This is worth some explanation. If steal is true the picture becomes invalid and the
	// 'set' call will steal the stolen pixels. If steal is false GetPixels is called and the
	// 'set' call will memcpy them out... which makes sure the picture is still valid after and
	// no-one is sharing the pixel buffer. We don't check the success of 'set' because it must
	// succeed if picture was valid.
	tPixel4b* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	bool success = Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
	tAssert(success);
	return true;
}


tFrame* tImageAPNG::GetFrame(bool steal)
{
	if (!IsValid())
		return nullptr;

	return steal ? Frames.Remove() : new tFrame( *Frames.First() );
}


tImageAPNG::tFormat tImageAPNG::Save(const tString& apngFile, tFormat format, int overrideFrameDuration) const
{
	SaveParams params;
	params.Format = format;
	params.OverrideFrameDuration = overrideFrameDuration;
	return Save(apngFile, params);
}


tImageAPNG::tFormat tImageAPNG::Save(const tString& apngFile, const SaveParams& params) const
{
	if (!IsValid())
		return tFormat::Invalid;

	if ((tSystem::tGetFileType(apngFile) != tSystem::tFileType::PNG) && (tSystem::tGetFileType(apngFile) != tSystem::tFileType::APNG))
		return tFormat::Invalid;

	// Currently apng_save will crash if all the images don't have the same size. We check that here.
	// Additionally it seems the frame sizes in an APNG must match in size. From the spec: "Each frame
	// is identical for each play, therefore it is safe for applications to cache the frames." This means
	// we'll need to keep this check in.
	int frameWidth = Frames.Head()->Width;
	int frameHeight = Frames.Head()->Height;
	if ((frameWidth <= 0) || (frameHeight <= 0))
		return tFormat::Invalid;
	for (tFrame* frame = Frames.Head(); frame; frame = frame->Next())
	{
		if ((frame->Width != frameWidth) || (frame->Height != frameHeight))
			return tFormat::Invalid;
	}

	int overrideFrameDuration = params.OverrideFrameDuration;
	tMath::tiClampMax(overrideFrameDuration, 65535);
	int bytesPerPixel = 0;
	switch (params.Format)
	{
		case tFormat::Auto:		bytesPerPixel = IsOpaque() ? 3 : 4;	break;
		case tFormat::BPP24:	bytesPerPixel = 3;					break;
		case tFormat::BPP32:	bytesPerPixel = 4;					break;
	}
	if (!bytesPerPixel)
		return tFormat::Invalid;

	std::vector<APngAsm::Image> images;
	images.resize(Frames.GetNumItems());

	int frameIndex = 0;
	for (tFrame* frame = Frames.Head(); frame; frame = frame->Next(), frameIndex++)
	{
		APngAsm::Image& img = images[frameIndex];
		int w = frame->Width;
		int h = frame->Height;

		// Use coltype = 6 for RGBA. Use coltype = 2 for RGB.
		int coltype = (bytesPerPixel == 4) ? 6 : 2;
		img.init(frame->Width, frame->Height, bytesPerPixel, coltype);

		// Initing does not populate the pixel data. We do that here.
		for (int r = 0; r < h; r++)
		{
			uint8* row = img.rows[h-r-1];
			for (int x = 0; x < w; x++)
			{
				int idx = r*w + x;
				row[x*bytesPerPixel + 0] = frame->Pixels[idx].R;
				row[x*bytesPerPixel + 1] = frame->Pixels[idx].G;
				row[x*bytesPerPixel + 2] = frame->Pixels[idx].B;
				if (bytesPerPixel == 4)
					row[x*bytesPerPixel + 3] = frame->Pixels[idx].A;
			}
		}

		// From the official apng spec:
		// The delay_num and delay_den parameters together specify a fraction indicating the time to display
		// the current frame, in seconds. If the denominator is 0, it is to be treated as if it were 100 (that
		// is, delay_num then specifies 1/100ths of a second). If the the value of the numerator is 0 the decoder
		// should render the next frame as quickly as possible, though viewers may impose a reasonable lower bound.
		// Default is numerator = 0. Fast as possible. Use when frameDur = 0.
		uint delayNumer = 0;
		uint delayDenom = 1000;
		if (overrideFrameDuration < 0)
		{
			// We use milliseconds here since the apng format uses 16bit unsigned (65535 max) for numerator and denominator.
			// A max numerator of 65535 gives us ~65 seconds max per frame, which seems reasonable.
			float frameDur = frame->Duration;
			if (frameDur > 0.0f)
				delayNumer = uint(frameDur * 1000.0f);
		}
		else
		{
			if (overrideFrameDuration > 0)
				delayNumer = overrideFrameDuration;
		}
		img.delay_num = delayNumer;
		img.delay_den = delayDenom;
	}

	int errCode = APngAsm::save_apng((char*)apngFile.Chr(), images, 0, 0, 0, 0);
	if (errCode)
		return tFormat::Invalid;

	return (bytesPerPixel == 3) ? tFormat::BPP24 : tFormat::BPP32;
}


}
