// tImageJXL.h
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

#pragma once
#include <Foundation/tString.h>
#include <Math/tColour.h>
#include <Image/tPixelFormat.h>
#include <Image/tFrame.h>
#include <Image/tMetaData.h>
#include <Image/tBaseImage.h>


// Opaque libjxl decoder handle (defined in the global namespace by <jxl/decode.h>). It is forward-declared here so that
// the metadata helpers below can be declared without pulling the libjxl headers into this public header.
struct JxlDecoder;


namespace tImage
{


class tImageJXL : public tBaseImage
{
public:
	// Creates an invalid tImageJXL. You must call Load manually.
	tImageJXL()																											{ }
	tImageJXL(const tString& jxlFile)																					{ Load(jxlFile); }

	// The data is copied out of jxlFileInMemory. Go ahead and delete[] after if you want.
	tImageJXL(const uint8* jxlFileInMemory, int numBytes)																{ Load(jxlFileInMemory, numBytes); }

	// Creates a tImageJXL from a bunch of frames. If steal is true, the srcFrames will be empty after.
	tImageJXL(tList<tFrame>& srcFrames, bool stealFrames)																{ Set(srcFrames, stealFrames); }

	// This one sets from a supplied pixel array (single frame). If steal is true it takes ownership of the pixels
	// pointer. Otherwise it just copies the data out.
	tImageJXL(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageJXL(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture. Single-frame.
	tImageJXL(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImageJXL()																								{ Clear(); }

	bool Load(const tString& jxlFile);
	bool Load(const uint8* jxlFileInMemory, int numBytes);

	// Parameters for saving to a JXL file.
	struct SaveParams
	{
		SaveParams()																									{ Reset(); }
		SaveParams(const SaveParams& src)																				: Lossless(src.Lossless), Distance(src.Distance), OverrideFrameDuration(src.OverrideFrameDuration) { }

		SaveParams& operator=(const SaveParams& src)																	{ Lossless = src.Lossless; Distance = src.Distance; OverrideFrameDuration = src.OverrideFrameDuration; return *this; }

		void Reset()																									{ Lossless = false; Distance = 1.0f; OverrideFrameDuration = -1; }

		// If true, encode losslessly (Distance is ignored). Lossless mode round-trips 8-bit RGBA exactly.
		// If you want compression, leave at the default of false.
		bool Lossless = false;

		// Target max Butteraugli distance for lossy encoding. Lower is higher quality. Max range is [0.0, 25.0].
		// Recommended range is [0.5, 3.0]. Only used when Lossless is false. Default is 1.0. At this setting you likely
		// won't perceive any compression. At 0.0 it is basically lossless, however it may not be bit-for-bit perfect so
		// use Lossless set to true if you really want lossless. It will encode faster too.
		float Distance = 1.0f;

		// In milliseconds. Set to >= 0 to override the duration of all animation frames.
		int OverrideFrameDuration = -1;
	};

	// Saves the frames to a JXL file. A single frame is written as a still image; more than one frame is written as an
	// animation (each frame's Duration, in seconds, is preserved). Set overrideFrameDuration to a value of 0 or greater
	// (in milliseconds) to override every frame's duration. Pixels are always written as 8-bit RGBA with straight
	// (non-premultiplied) alpha, matching how Tacent stores them. Returns true on success.
	bool Save(const tString& jxlFile, bool lossless, float distance = 0.0f, int overrideFrameDuration = -1) const;
	bool Save(const tString& jxlFile, const SaveParams& = SaveParams()) const;

	// Creates a tImageJXL from a bunch of frames. If steal is true, the srcFrames will be empty after.
	bool Set(tList<tFrame>& srcFrames, bool stealFrames);

	// This one sets from a supplied pixel array (single frame). If steal is true it takes ownership of the pixels
	// pointer. Otherwise it just copies the data out.
	bool Set(tPixel4b* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture. Single-frame.
	bool Set(tPicture& picture, bool steal = true) override;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;

	// Valid if at least one frame has been decoded.
	bool IsValid() const override																						{ return GetNumFrames() >= 1; }

	// Number of decoded frames. A still image has one; an animation has several.
	int GetNumFrames() const																							{ return Frames.GetNumItems(); }

	// Convenience accessors for the FIRST frame, so single-frame consumers keep working. Use the frame methods below to
	// iterate an animation.
	int GetWidth() const																								{ tFrame* f = Frames.Head(); return f ? f->Width : 0; }
	int GetHeight() const																								{ tFrame* f = Frames.Head(); return f ? f->Height : 0; }
	tPixel4b* GetPixels() const																							{ tFrame* f = Frames.Head(); return f ? f->Pixels : nullptr; }

	// Returns true if ALL frames are opaque. Slow. Checks all pixels.
	bool IsOpaque() const;

	// Steals the pixels of the FIRST frame. The frame is removed from Frames (so GetNumFrames drops by one and the
	// object may become invalid). You own the returned pixels and must delete[] them when done.
	tPixel4b* StealPixels();

	// After this call you are the owner of the frame and must eventually delete it. The frame you stole will no longer
	// be part of the tImageJXL, but the remaining ones will still be there. GetNumFrames will be one fewer.
	tFrame* StealFrame(int frameNum);

	// Gets the first frame only.
	tFrame* GetFrame(bool steal = true) override;

	// Similar to above but takes all the frames from the tImageJXL and appends them to the supplied frame list. The
	// object will be invalid after since it will have no frames.
	void StealFrames(tList<tFrame>&);

	// Returns a pointer to the frame, but it's not yours to delete. This object still owns it.
	tFrame* GetFrame(int frameNum);

	// A place to store EXIF and XMP metadata. JPEG XL container files (".jxl") often carry an "Exif" box (a 4-byte
	// offset followed by a bare TIFF/EXIF structure) and an "xml " box (raw XMP XML) alongside the codestream. Both are
	// extracted by the Load() calls and merged into this struct.
	tMetaData MetaData;

	// The decoded frames, in animation order. A still image yields exactly one frame; an animation yields several, each
	// with its own Duration (in seconds).
	tList<tFrame> Frames;

private:
	// Decodes the codestream (a still image, or an animation of many frames) and fills Frames in order. libjxl
	// coalesces each displayed frame (coalescing is on by default) so it arrives as a full, alpha-blended canvas that
	// maps onto a single tFrame. Each frame's Duration is computed from the codestream animation timescale
	// (ticks-per-second) and the frame's own duration (in ticks). libjxl emits rows top-to-bottom; Tacent stores them
	// bottom-up (matching PNG/TGA), so the rows are reversed before each frame is appended. Returns true if at least
	// one frame was decoded.
	bool DecodeFrames(const uint8* data, int numBytes);

	// Best-effort extraction of the EXIF and XMP boxes. A failure here is NOT a load failure -- the image may still
	// have decoded fine. Returns true if at least one metadata segment was recognized and parsed into MetaData.
	bool PopulateMetaData(const uint8* data, int numBytes);

	// Walks the ISOBMFF container boxes of a libjxl decoder (which must be subscribed to JXL_DEC_BOX) and appends the
	// bare EXIF-TIFF / raw XMP XML segments it finds to exifSegments / xmpSegments. Each segment's UserData records the
	// owning heap buffer (freed by the caller after AddSegments). Only JXL_DEC_SUCCESS and JXL_DEC_BOX are acted on;
	// any other status ends the walk cleanly.
	bool WalkBoxes(JxlDecoder*, tList<tMetaData::tMetaSegment>& exifSegments, tList<tMetaData::tMetaSegment>& xmpSegments);

	// Reads the full payload of the box that the decoder is currently presenting (the decoder just returned
	// JXL_DEC_BOX). Uses the incremental buffer protocol so that "brob" (Brotli-compressed) boxes of unknown
	// decompressed size are handled. On success *outData is heap-allocated (free with delete[]) and holds *outBytes of
	// box content.
	bool ReadBoxPayload(JxlDecoder*, uint8*& outData, int& outBytes);

	// Swaps rows in place to convert libjxl's top-to-bottom layout into Tacent's bottom-up storage, for one frame.
	void ReverseRows(tFrame* frame);
};


// Implementation only below.
inline bool tImageJXL::IsOpaque() const
{
	if (Frames.GetNumItems() < 1)
		return false;

	for (tFrame* frame = Frames.Head(); frame; frame = frame->Next())
		if (!frame->IsOpaque())
			return false;

	return true;
}


inline tFrame* tImageJXL::StealFrame(int frameNum)
{
	tFrame* f = GetFrame(frameNum);
	if (!f)
		return nullptr;

	return Frames.Remove(f);
}


inline void tImageJXL::StealFrames(tList<tFrame>& frames)
{
	while (tFrame* frame = Frames.Remove())
		frames.Append(frame);
}


inline tFrame* tImageJXL::GetFrame(int frameNum)
{
	if ((frameNum >= Frames.GetNumItems()) || (frameNum < 0))
		return nullptr;

	tFrame* f = Frames.First();
	while (frameNum--)
		f = f->Next();

	return f;
}


inline tFrame* tImageJXL::GetFrame(bool steal)
{
	if (steal)
		return StealFrame(0);

	return GetFrame(0);
}


inline tPixel4b* tImageJXL::StealPixels()
{
	tFrame* f = Frames.Head();
	if (!f)
		return nullptr;

	tPixel4b* pixels = f->GetPixels(true);
	Frames.Remove(f);
	delete f;
	return pixels;
}


inline void tImageJXL::Clear()
{
	tBaseImage::Clear();

	while (tFrame* frame = Frames.Remove())
		delete frame;

	MetaData.Clear();
}


}
