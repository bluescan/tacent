// tImageJXL.cpp
//
// This class knows how to load JPEG XL files using libjxl. JPEG XL (JXL) is an open image format. In its container form
// (the ".jxl" files this loads) the compressed codestream lives alongside optional EXIF and XMP boxes, both of which
// are extracted and stored in MetaData.
//
// A JPEG XL codestream is either a single still image or an animation. In the animation case it is a sequence of
// frames, each with its own duration. This class decodes every displayed frame into a separate tFrame and stores them,
// in order, in the Frames list (exactly like tImageAPNG) -- a still image simply ends up with a single-frame Frames
// list.
//
// Copyright (c) 2026 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Image/tImageJXL.h>
#include <System/tFile.h>
#include <Image/tFrame.h>
#include <Image/tPicture.h>
#include "jxl/decode.h"
namespace tImage
{


bool tImageJXL::Load(const tString& jxlFile)
{
	Clear();

	if (!tSystem::tFileExists(jxlFile))
		return false;

	int numBytes = 0;
	uint8* jxlFileInMemory = tSystem::tLoadFile(jxlFile, nullptr, &numBytes);
	bool success = Load(jxlFileInMemory, numBytes);
	delete[] jxlFileInMemory;

	return success;
}


bool tImageJXL::Load(const uint8* jxlFileInMemory, int numBytes)
{
	Clear();

	if ((numBytes <= 0) || !jxlFileInMemory)
		return false;

	// Pass 1: decode the frames. A still image yields one frame; an animation yields one per codestream frame, each with
	// its own duration. This is the critical step; if it fails the load fails.
	if (!DecodeFrames(jxlFileInMemory, numBytes))
		return false;

	// Pass 2: extract EXIF / XMP metadata. This is best-effort and a failure here is not a load failure -- the image is
	// already decoded. It runs on a second, independent decoder so that a box-reading hiccup can never affect pixels.
	PopulateMetaData(jxlFileInMemory, numBytes);

	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	PixelFormat = tPixelFormat::R8G8B8A8;
	ColourProfileSrc = tColourProfile::sRGB;
	ColourProfile = tColourProfile::sRGB;

	return true;
}


bool tImageJXL::DecodeFrames(const uint8* data, int numBytes)
{
	tAssert((numBytes > 0) && data);

	JxlDecoder* dec = JxlDecoderCreate(nullptr);
	if (!dec)
		return false;

	// Coalescing is on by default; request it explicitly so every displayed frame is handed to us as a full, alpha-blended
	// canvas. That means each animation frame maps onto a single, standalone tFrame with no reference-frame bookkeeping on
	// our side (this is the whole point of the Frames model).
	if (JxlDecoderSetCoalescing(dec, JXL_TRUE) != JXL_DEC_SUCCESS)
		;	// Ignore -- coalescing-on is the default and the only value we actually want.

	bool success = false;
	bool gotCanvas = false;
	int canvasWidth = 0;
	int canvasHeight = 0;
	float tickSeconds = 0.0f;	// Seconds per animation tick (derived from the codestream timescale). 0 => no durations.
	tFrame* pendingFrame = nullptr;	// The frame whose pixels libjxl is about to write.

	// Provides the RGBA8 output buffer for one frame. libjxl fills alpha with 255 for opaque images, so every JXL image
	// (grayscale, RGB, RGBA) maps cleanly onto tPixel4b (RGBA).
	auto offerOutBuffer = [&](tFrame& frame) -> bool
	{
		JxlPixelFormat format;
		format.num_channels	= 4;
		format.data_type		= JXL_TYPE_UINT8;
		format.endianness	= JXL_NATIVE_ENDIAN;
		format.align		= 0;
		return JxlDecoderSetImageOutBuffer(dec, &format, frame.Pixels, (size_t)frame.Width * (size_t)frame.Height * sizeof(tPixel4b)) == JXL_DEC_SUCCESS;
	};

	if ((JxlDecoderSetInput(dec, data, numBytes) == JXL_DEC_SUCCESS) &&
		(JxlDecoderSubscribeEvents(dec, JXL_DEC_BASIC_INFO | JXL_DEC_FRAME | JXL_DEC_FULL_IMAGE) == JXL_DEC_SUCCESS))
	{
		// We have the entire file in memory; tell the decoder no more input will arrive.
		JxlDecoderCloseInput(dec);

		for (;;)
		{
			JxlDecoderStatus status = JxlDecoderProcessInput(dec);

			if (status == JXL_DEC_SUCCESS)
			{
				success = (Frames.GetNumItems() >= 1);
				break;
			}
			if ((status == JXL_DEC_ERROR) || (status == JXL_DEC_NEED_MORE_INPUT))
				break;		// A real error (NEED_MORE_INPUT should not happen since we closed the input).

			if (status == JXL_DEC_BASIC_INFO)
			{
				JxlBasicInfo info;
				tStd::tMemset(&info, 0, sizeof(info));
				if (JxlDecoderGetBasicInfo(dec, &info) != JXL_DEC_SUCCESS)
					break;

				canvasWidth = (int)info.xsize;
				canvasHeight = (int)info.ysize;
				if ((canvasWidth <= 0) || (canvasHeight <= 0))
					break;
				gotCanvas = true;

				// The animation timescale is expressed as ticks-per-second (numerator / denominator). Convert it to
				// seconds-per-tick so a frame's tick duration (JxlFrameHeader::duration) can be scaled to seconds.
				if (info.have_animation && (info.animation.tps_numerator > 0))
					tickSeconds = (float)info.animation.tps_denominator / (float)info.animation.tps_numerator;
			}
			else if ((status == JXL_DEC_FRAME) || (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER))
			{
				// A new displayed frame is starting. Build it (full canvas) and work out its duration in seconds.
				if (!pendingFrame)
				{
					if (!gotCanvas)
						break;

					tFrame* frame = new tFrame();
					frame->Width = canvasWidth;
					frame->Height = canvasHeight;
					frame->PixelFormatSrc = tPixelFormat::R8G8B8A8;
					frame->Pixels = new tPixel4b[(size_t)canvasWidth * (size_t)canvasHeight];

					// JxlDecoderGetFrameHeader is valid once the JXL_DEC_FRAME event has occurred for this frame. If it is
					// not (yet) available we simply leave the duration at zero.
					JxlFrameHeader frameHeader;
					tStd::tMemset(&frameHeader, 0, sizeof(frameHeader));
					if (JxlDecoderGetFrameHeader(dec, &frameHeader) == JXL_DEC_SUCCESS)
						frame->Duration = tickSeconds * (float)frameHeader.duration;

					pendingFrame = frame;
				}

				if (!offerOutBuffer(*pendingFrame))
					break;
			}
			else if (status == JXL_DEC_FULL_IMAGE)
			{
				// The pending frame's pixels are now filled (coalesced, full canvas, rows top-to-bottom).
				if (!pendingFrame)
					break;

				ReverseRows(pendingFrame);	// Convert libjxl's top-to-bottom layout to Tacent's bottom-up storage.
				Frames.Append(pendingFrame);
				pendingFrame = nullptr;
			}
		}
	}
	JxlDecoderDestroy(dec);

	if (!success || pendingFrame)
	{
		if (pendingFrame)
			delete pendingFrame;
		Clear();
		return false;
	}

	return true;
}


void tImageJXL::ReverseRows(tFrame* frame)
{
	if ((frame->Width <= 0) || (frame->Height <= 1) || !frame->Pixels)
		return;

	int bytesPerRow = frame->Width * (int)sizeof(tPixel4b);
	uint8* tempRow = new uint8[bytesPerRow];
	for (int y = 0; y < (frame->Height / 2); y++)
	{
		uint8* top = (uint8*)frame->Pixels + (size_t)y * (size_t)bytesPerRow;
		uint8* bottom = (uint8*)frame->Pixels + (size_t)(frame->Height - 1 - y) * (size_t)bytesPerRow;
		tStd::tMemcpy(tempRow, top, bytesPerRow);
		tStd::tMemcpy(top, bottom, bytesPerRow);
		tStd::tMemcpy(bottom, tempRow, bytesPerRow);
	}
	delete[] tempRow;
}


bool tImageJXL::Set(tList<tFrame>& srcFrames, bool stealFrames)
{
	Clear();

	if (srcFrames.GetNumItems() <= 0)
		return false;

	PixelFormatSrc = srcFrames.Head()->PixelFormatSrc;
	PixelFormat = tPixelFormat::R8G8B8A8;
	ColourProfileSrc = tColourProfile::sRGB;	// We assume srcFrames must be sRGB.
	ColourProfile = tColourProfile::sRGB;

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


bool tImageJXL::Set(tPixel4b* pixels, int width, int height, bool steal)
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
	PixelFormat = tPixelFormat::R8G8B8A8;
	ColourProfileSrc = tColourProfile::sRGB;	// We assume pixels must be sRGB.
	ColourProfile = tColourProfile::sRGB;

	return true;
}


bool tImageJXL::Set(tFrame* frame, bool steal)
{
	Clear();

	if (!frame || !frame->IsValid())
		return false;

	PixelFormatSrc = frame->PixelFormatSrc;
	PixelFormat = tPixelFormat::R8G8B8A8;
	ColourProfileSrc = tColourProfile::sRGB;	// We assume frame must be sRGB.
	ColourProfile = tColourProfile::sRGB;

	if (steal)
		Frames.Append(frame);
	else
		Frames.Append(new tFrame(*frame));
	return true;
}


bool tImageJXL::Set(tPicture& picture, bool steal)
{
	if (!picture.IsValid())
		return false;

	// If steal is true the picture becomes invalid and 'Set' steals the pixels; otherwise GetPixels returns them and 'Set'
	// copies them out, so the picture remains valid and no buffer is shared.
	tPixel4b* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	tAssert(pixels);
	bool success = Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);

	if (success)
	{
		// We assume the picture is already in the correct pixel format. A tPicture does not record its colour profile.
		PixelFormatSrc = picture.PixelFormatSrc;
		ColourProfileSrc = tColourProfile::sRGB;
		ColourProfile = tColourProfile::sRGB;
	}
	return success;
}


bool tImageJXL::WalkBoxes(JxlDecoder* dec, tList<tMetaData::tMetaSegment>& exifSegments, tList<tMetaData::tMetaSegment>& xmpSegments)
{
	while (1)
	{
		JxlDecoderStatus status = JxlDecoderProcessInput(dec);

		if (status == JXL_DEC_SUCCESS)
			return true;

		// We only care about the box event; anything else (an error, or an unexpected status) ends the walk cleanly so we
		// never spin on a decoder state we cannot service.
		if (status != JXL_DEC_BOX)
			return false;

		JxlBoxType boxType = { 0, 0, 0, 0 };
		JxlDecoderGetBoxType(dec, boxType, JXL_FALSE);
		bool isExif = (memcmp((const char*)boxType, "Exif", 4) == 0);
		bool isXmp	= (memcmp((const char*)boxType, "xml ", 4) == 0);
		if ((!isExif) && (!isXmp))
			continue;			// Leave the box buffer unset to skip this (non-metadata) box.

		uint8* boxData = nullptr;
		int boxBytes = 0;
		if (!ReadBoxPayload(dec, boxData, boxBytes))
			return false;

		if (isExif)
		{
			// "Exif" box layout: [exif_tiff_header_offset: u32 BE][TIFF]. The offset is measured from the byte just past
			// the 4-byte field (per the JXL spec), so the TIFF header sits at 4 + off. In practice off is 0 (a bare TIFF).
			uint32 off = 0;
			if (boxBytes >= 4)
				off = (uint32(boxData[0]) << 24) | (uint32(boxData[1]) << 16) | (uint32(boxData[2]) << 8) | uint32(boxData[3]);
			size_t tiffStart = (size_t)4 + (size_t)off;
			if ((tiffStart + 8) <= (size_t)boxBytes)
			{
				// The segment points into boxData; boxData is recorded as the segment's UserData (the owner).
				exifSegments.Append(new tMetaData::tMetaSegment(boxData + tiffStart, (int)(boxBytes - tiffStart), boxData));
				continue;
			}
			// Malformed / too small EXIF box -- drop it (but keep walking for other boxes).
		}
		else
		{
			// "xml " box layout: raw XMP XML. tMetaData wants the raw XML, which is exactly what the box holds.
			if (boxBytes > 0)
			{
				xmpSegments.Append(new tMetaData::tMetaSegment(boxData, boxBytes, boxData));
				continue;
			}
		}

		// A box we did not adopt (or that was malformed) still owns its buffer -- free it now.
		delete[] boxData;
	}
}


bool tImageJXL::ReadBoxPayload(JxlDecoder* dec, uint8*& outData, int& outBytes)
{
	outData = nullptr;
	outBytes = 0;

	size_t capacity = 65536;	// Initial room offered to the decoder; doubled each time the box outgrows it.
	size_t collected = 0;		// Bytes of box content received so far.

	for (;;)
	{
		uint8* grown = new uint8[capacity];
		if (collected > 0)
			tStd::tMemcpy(grown, outData, collected);
		delete[] outData;
		outData = grown;

		const size_t room = capacity - collected;
		if (JxlDecoderSetBoxBuffer(dec, outData + collected, room) != JXL_DEC_SUCCESS)
			return false;

		const JxlDecoderStatus status = JxlDecoderProcessInput(dec);
		const size_t remaining = JxlDecoderReleaseBoxBuffer(dec);	// Portion of `room` the decoder left unfilled.
		collected += (room - remaining);

		if (status == JXL_DEC_BOX_COMPLETE)
		{
			outBytes = (int)collected;
			return true;
		}
		if (status == JXL_DEC_BOX_NEED_MORE_OUTPUT)
		{
			capacity *= 2;		// Offered buffer filled; make more room and continue.
			continue;
		}
		// JXL_DEC_ERROR / JXL_DEC_NEED_MORE_INPUT / anything unexpected -- the box could not be read.
		return false;
	}
}


bool tImageJXL::PopulateMetaData(const uint8* data, int numBytes)
{
	tList<tMetaData::tMetaSegment> exifSegments, xmpSegments;

	JxlDecoder* dec = JxlDecoderCreate(nullptr);
	if (!dec)
		return false;

	bool walked = false;
	if (JxlDecoderSetInput(dec, data, numBytes) == JXL_DEC_SUCCESS)
	{
		JxlDecoderCloseInput(dec);
		// Decompress "brob" boxes so that EXIF/XMP stored in them are surfaced under their real ("Exif"/"xml ") type.
		if (JxlDecoderSetDecompressBoxes(dec, JXL_TRUE) == JXL_DEC_SUCCESS)
		{
			if (JxlDecoderSubscribeEvents(dec, JXL_DEC_BOX | JXL_DEC_BOX_COMPLETE) == JXL_DEC_SUCCESS)
			{
				walked = WalkBoxes(dec, exifSegments, xmpSegments);
			}
		}
	}
	JxlDecoderDestroy(dec);

	if (!walked)
	{
		// The walk did not finish cleanly -- drop any segments we collected (they own their UserData buffers).
		for (tMetaData::tMetaSegment* s = exifSegments.First(); s; s = s->Next())
			delete[] s->UserData;
		for (tMetaData::tMetaSegment* s = xmpSegments.First(); s; s = s->Next())
			delete[] s->UserData;
		return false;
	}

	// Hand all collected segments to tMetaData in one call (which applies EXIF before XMP, so EXIF wins overlaps), then
	// free the underlying per-box buffers via the segment UserDatas (the segment objects are owned by the lists).
	bool found = MetaData.AddSegments(exifSegments, xmpSegments);
	for (tMetaData::tMetaSegment* s = exifSegments.First(); s; s = s->Next())
		delete[] s->UserData;
	for (tMetaData::tMetaSegment* s = xmpSegments.First(); s; s = s->Next())
		delete[] s->UserData;
	return found;
}


}