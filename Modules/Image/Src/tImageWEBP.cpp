// tImageWEBP.cpp
//
// This knows how to load/save WebPs. It knows the details of the webp file format and loads the data into multiple
// tPixel arrays, one for each frame (WebPs may be animated). These arrays may be 'stolen' by tPictures.
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
#include "Image/tImageWEBP.h"
#include "Image/tPicture.h"
#include "WebP/include/mux.h"
#include "WebP/include/demux.h"
#include "WebP/include/encode.h"
using namespace tSystem;
namespace tImage
{


bool tImageWEBP::Load(const tString& webpFile)
{
	Clear();

	if (tSystem::tGetFileType(webpFile) != tSystem::tFileType::WEBP)
		return false;

	if (!tFileExists(webpFile))
		return false;

	int numBytes = 0;
	uint8* webpFileInMemory = tLoadFile(webpFile, nullptr, &numBytes);
	bool success = Load(webpFileInMemory, numBytes);
	delete[] webpFileInMemory;

	return success;
}


bool tImageWEBP::Load(const uint8* webpFileInMemory, int numBytes)
{
	Clear();
	if ((numBytes <= 0) || !webpFileInMemory)
		return false;

	// Now we load and populate the frames.
	WebPData webpData;
	webpData.bytes = webpFileInMemory;
	webpData.size = numBytes;

	WebPDemuxer* demux = WebPDemux(&webpData);
	uint32 canvasWidth = WebPDemuxGetI(demux, WEBP_FF_CANVAS_WIDTH);
	uint32 canvasHeight = WebPDemuxGetI(demux, WEBP_FF_CANVAS_HEIGHT);
	uint32 flags = WebPDemuxGetI(demux, WEBP_FF_FORMAT_FLAGS);
	uint32 numFrames = WebPDemuxGetI(demux, WEBP_FF_FRAME_COUNT);
	
	if ((canvasWidth <= 0) || (canvasHeight <= 0) || (numFrames <= 0))
	{
		WebPDemuxDelete(demux);
		return false;
	}

	bool animated = (numFrames > 1);
	if (animated)
	{
		// Bits 00 to 07: Alpha. Bits 08 to 15: Red. Bits 16 to 23: Green. Bits 24 to 31: Blue.
		uint32 col = WebPDemuxGetI(demux, WEBP_FF_BACKGROUND_COLOR);
		BackgroundColour.R = (col >> 8 ) & 0xFF;
		BackgroundColour.G = (col >> 16) & 0xFF;
		BackgroundColour.B = (col >> 24) & 0xFF;
		BackgroundColour.A = (col >> 0 ) & 0xFF;
	}

	// We start by creatng the initial canvas in memory set to the background colour.
	// This is our 'working area' where we put the decoded frames. See CopyRegion below.
	tPixel4b* canvas = new tPixel4b[canvasWidth*canvasHeight];
	for (int p = 0; p < canvasWidth*canvasHeight; p++)
		canvas[p] = tColour4b::transparent;

	// Iterate over all frames.
	tPixelFormat srcFormat = tPixelFormat::R8G8B8;
	WebPIterator iter;
	if (WebPDemuxGetFrame(demux, 1, &iter))
	{
		do
		{
			WebPDecoderConfig config;
			WebPInitDecoderConfig(&config);

			config.output.colorspace = MODE_RGBA;
			config.output.is_external_memory = 0;
			config.options.flip = 1;
			int result = WebPDecode(iter.fragment.bytes, iter.fragment.size, &config);
			if (result != VP8_STATUS_OK)
				continue;

			// What do we do with the canvas? If not animated it's not going to matter. From WebP source:
			// Dispose method (animation only). Indicates how the area used by the current
			// frame is to be treated before rendering the next frame on the canvas.
			bool dispose = (iter.dispose_method == WEBP_MUX_DISPOSE_BACKGROUND);
			if (dispose)
			{
				for (int p = 0; p < canvasWidth*canvasHeight; p++)
					canvas[p] = tColour4b::transparent;
			}

			int fragWidth = config.output.width;
			int fragHeight = config.output.height;
			if ((fragWidth <= 0) || (fragHeight <= 0))
				continue;

			// All frames in tacent are canvas-sized.
			tFrame* newFrame = new tFrame;
			newFrame->PixelFormatSrc = iter.has_alpha ? tPixelFormat::R8G8B8A8 : tPixelFormat::R8G8B8;

			// If any frame has alpha we set the main src format to have alpha.
			if (iter.has_alpha)
				srcFormat = tPixelFormat::R8G8B8A8;
			newFrame->Width = canvasWidth;
			newFrame->Height = canvasHeight;
			newFrame->Pixels = new tPixel4b[newFrame->Width * newFrame->Height];
			newFrame->Duration = float(iter.duration) / 1000.0f;

			// Next we need to grab the decoded pixels (which may be a sub-region of the canvas) and stick them in the canvas.
			// How we stick the pixels in depends on the anim-blend. If not animated, force simple overwrite.
			bool blend = false;
			if (iter.blend_method == WEBP_MUX_BLEND)
				blend = true;

			// The flip flag doesn't fix the offsets for WebP so we need the canvasHeight - iter.y_offset - frameHeight.
			bool copied = CopyRegion(canvas, canvasWidth, canvasHeight, (tPixel4b*)config.output.u.RGBA.rgba, fragWidth, fragHeight, iter.x_offset, canvasHeight - iter.y_offset - fragHeight, blend);
			if (!copied)
			{
				delete newFrame;
				continue;
			}

			// Now the canvas is updated. Put the canvas in the new frame.
			tStd::tMemcpy(newFrame->Pixels, canvas, canvasWidth * canvasHeight * sizeof(tPixel4b));

			WebPFreeDecBuffer(&config.output);
			Frames.Append(newFrame);
		}
		while (WebPDemuxNextFrame(&iter));

		WebPDemuxReleaseIterator(&iter);
	}

	delete[] canvas;
	WebPDemuxDelete(demux);
	if (Frames.GetNumItems() <= 0)
		return false;

	PixelFormatSrc		= srcFormat;
	PixelFormat			= tPixelFormat::R8G8B8A8;

	// WEBP files are assumed to be in sRGB.
	ColourProfileSrc	= tColourProfile::sRGB;
	ColourProfile		= tColourProfile::sRGB;

	return true;
}


bool tImageWEBP::CopyRegion(tPixel4b* dst, int dstW, int dstH, tPixel4b* src, int srcW, int srcH, int offsetX, int offsetY, bool blend)
{
	// Do nothing if anything wrong.
	if (!dst || !src || (dstW*dstH <= 0) || (srcW*srcH <= 0))
		return false;

	// Also check that the entire src region fits inside the dst canvas.
	if ((offsetX < 0) || (offsetX >= dstW) || (offsetY < 0) || (offsetY >= dstH))
		return false;
	if ((offsetX+srcW > dstW) || (offsetY+srcH > dstH))
		return false;

	// for each row of the src put it in dest.
	for (int sy = 0; sy < srcH; sy++)
	{
		int rowWidth = srcW;
		tPixel4b* dstRow = dst + ((offsetY+sy)*dstW + offsetX);
		tPixel4b* srcRow = src + (sy*srcW);
		for (int sx = 0; sx < rowWidth; sx++)
		{
			if (blend)
			{
				tColour4f scol(srcRow[sx]);
				tColour4f dcol(dstRow[sx]);
				float alpha = scol.A;
				float oneMinusAlpha = 1.0f - alpha;

				tColour4f pixelCol = scol;
				pixelCol.R = pixelCol.R*alpha + dcol.R*oneMinusAlpha;
				pixelCol.G = pixelCol.G*alpha + dcol.G*oneMinusAlpha;
				pixelCol.B = pixelCol.B*alpha + dcol.B*oneMinusAlpha;
				pixelCol.A = alpha > 0.0f ? alpha : dcol.A;

				dstRow[sx].Set(pixelCol);
			}
			else
			{
				dstRow[sx] = srcRow[sx];
			}
		}
	}

	return true;
}


bool tImageWEBP::Set(tList<tFrame>& srcFrames, bool stealFrames)
{
	Clear();
	if (srcFrames.GetNumItems() <= 0)
		return false;

	// This assumes the srcFrames all have the same format.
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


bool tImageWEBP::Set(tPixel4b* pixels, int width, int height, bool steal)
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


bool tImageWEBP::Set(tFrame* frame, bool steal)
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


bool tImageWEBP::Set(tPicture& picture, bool steal)
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


tFrame* tImageWEBP::GetFrame(bool steal)
{
	if (!IsValid())
		return nullptr;

	return steal ? Frames.Remove() : new tFrame( *Frames.First() );
}


bool tImageWEBP::Save(const tString& webpFile, bool lossy, float qualityCompstr, int overrideFrameDuration) const
{
	SaveParams params;
	params.Lossy = lossy;
	params.QualityCompstr = qualityCompstr;
	params.OverrideFrameDuration = overrideFrameDuration;
	return Save(webpFile, params);
}


bool tImageWEBP::Save(const tString& webpFile, const SaveParams& params) const
{
	if (!IsValid())
		return false;

	if (tSystem::tGetFileType(webpFile) != tSystem::tFileType::WEBP)
		return false;

	WebPConfig config;
	int success = WebPConfigPreset(&config, WEBP_PRESET_PHOTO, tMath::tClamp(params.QualityCompstr, 0.0f, 100.0f));
	if (!success)
		return false;

	// config.method is the quality/speed trade-off (0=fast, 6=slower-better).
	config.lossless = params.Lossy ? 0 : 1;

	// Additional config parameters in lossy mode.
	if (params.Lossy)
	{
		config.sns_strength = 90;
		config.filter_sharpness = 6;
		config.alpha_quality = 90;
	}

	success = WebPValidateConfig(&config);
	if (!success)
		return false;

	// Setup the muxer so we can put more than one image in a file.
	WebPMux* mux = WebPMuxNew();

	WebPMuxAnimParams animParams;
	animParams.bgcolor = 0x00000000;
	animParams.loop_count = 0;
    WebPMuxSetAnimationParams(mux, &animParams);

	bool animated = Frames.GetNumItems() > 1;
	for (tFrame* frame = Frames.First(); frame; frame = frame->Next())
	{
		WebPPicture pic;
		success = WebPPictureInit(&pic);
		if (!success)
			continue;

		// This is inefficient here. I'm reversing the rows so I can use the simple
		// WebPPictureImportRGBA. But this is a waste of memory and time.
		tFrame normFrame(*frame);
		normFrame.ReverseRows();

		// Let's get one frame going first.
		//tFrame* frame = Frames.Head();
		pic.width = normFrame.Width;
		pic.height = normFrame.Height;
		success = WebPPictureImportRGBA(&pic, (uint8*)normFrame.Pixels, normFrame.Width*sizeof(tPixel4b));
		if (!success)
			continue;

		WebPMemoryWriter writer;
		WebPMemoryWriterInit(&writer);
		pic.writer = WebPMemoryWrite;
		pic.custom_ptr = &writer;

		success = WebPEncode(&config, &pic);
		if (!success)
			continue;

		// Done with pic.
		WebPPictureFree(&pic);

		WebPData webpData;		
		webpData.bytes = writer.mem;
		webpData.size = writer.size;

		int copyData = 1;
		if (animated)
		{
			WebPMuxFrameInfo frameInfo;
			tStd::tMemset(&frameInfo, 0, sizeof(WebPMuxFrameInfo));

			// Frame duration is an integer in milliseconds.
			frameInfo.duration = (params.OverrideFrameDuration >= 0) ? params.OverrideFrameDuration : int(frame->Duration * 1000.0f);
			frameInfo.bitstream = webpData;
			frameInfo.id = WEBP_CHUNK_ANMF;
			frameInfo.blend_method = WEBP_MUX_NO_BLEND;
			frameInfo.dispose_method = WEBP_MUX_DISPOSE_BACKGROUND;
			WebPMuxPushFrame(mux, &frameInfo, copyData);
		}
		else
		{
			// One frame. Not animated.
			WebPMuxSetImage(mux, &webpData, copyData);
		}

		WebPMemoryWriterClear(&writer);
	}

	// Get data from mux in WebP RIFF format.
	WebPData assembledData;
	tStd::tMemset(&assembledData, 0, sizeof(WebPData));
	WebPMuxAssemble(mux, &assembledData);
	WebPMuxDelete(mux);

	bool ok = tCreateFile(webpFile, (uint8*)assembledData.bytes, assembledData.size);
	WebPDataClear(&assembledData);
	return ok;
}


}
