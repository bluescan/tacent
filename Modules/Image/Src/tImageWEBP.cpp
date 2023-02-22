// tImageWEBP.cpp
//
// This knows how to load/save WebPs. It knows the details of the webp file format and loads the data into multiple
// tPixel arrays, one for each frame (WebPs may be animated). These arrays may be 'stolen' by tPictures.
//
// Copyright (c) 2020-2023 Tristan Grimmer.
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

	// Now we load and populate the frames.
	WebPData webpData;
	webpData.bytes = webpFileInMemory;
	webpData.size = numBytes;

	WebPDemuxer* demux = WebPDemux(&webpData);
	uint32 width = WebPDemuxGetI(demux, WEBP_FF_CANVAS_WIDTH);
	uint32 height = WebPDemuxGetI(demux, WEBP_FF_CANVAS_HEIGHT);
	uint32 flags = WebPDemuxGetI(demux, WEBP_FF_FORMAT_FLAGS);
	uint32 numFrames = WebPDemuxGetI(demux, WEBP_FF_FRAME_COUNT);
	
	if ((width <= 0) || (height <= 0) || (numFrames <= 0))
	{
		WebPDemuxDelete(demux);
		delete[] webpFileInMemory;
		return false;
	}

	// Iterate over all frames.
	PixelFormatSrc = tPixelFormat::R8G8B8;
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

			tFrame* newFrame = new tFrame;
			newFrame->PixelFormatSrc = iter.has_alpha ? tPixelFormat::R8G8B8A8 : tPixelFormat::R8G8B8;
			if (iter.has_alpha)
				PixelFormatSrc = tPixelFormat::R8G8B8A8;
			newFrame->Width = width;
			newFrame->Height = height;
			newFrame->Pixels = new tPixel[width * height];
			newFrame->Duration = float(iter.duration) / 1000.0f;

			tStd::tMemcpy(newFrame->Pixels, config.output.u.RGBA.rgba, width * height * sizeof(tPixel));
			WebPFreeDecBuffer(&config.output);
			Frames.Append(newFrame);
		}
		while (WebPDemuxNextFrame(&iter));

		WebPDemuxReleaseIterator(&iter);
	}

	WebPDemuxDelete(demux);
	delete[] webpFileInMemory;
	return true;
}


bool tImageWEBP::Set(tList<tFrame>& srcFrames, bool stealFrames)
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


bool tImageWEBP::Set(tPixel* pixels, int width, int height, bool steal)
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


bool tImageWEBP::Set(tFrame* frame, bool steal)
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


bool tImageWEBP::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
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
		success = WebPPictureImportRGBA(&pic, (uint8*)normFrame.Pixels, normFrame.Width*sizeof(tPixel));
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
