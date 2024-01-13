// tImageTIFF.h
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

#pragma once
#include <Foundation/tString.h>
#include <Math/tColour.h>
#include <Image/tPixelFormat.h>
#include <Image/tFrame.h>
#include <LibTIFF/include/tiffio.h>
#include <Image/tBaseImage.h>
namespace tImage
{


class tImageTIFF : public tBaseImage
{
public:
	// Creates an invalid tImageTIFF. You must call Load manually.
	tImageTIFF()																										{ }
	tImageTIFF(const tString& tiffFile)																					{ Load(tiffFile); }

	// Creates a tImageAPNG from a bunch of frames. If steal is true, the srcFrames will be empty after.
	tImageTIFF(tList<tFrame>& srcFrames, bool stealFrames)																{ Set(srcFrames, stealFrames); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageTIFF(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageTIFF(tFrame* frame, bool steal = true)																		{ Set(frame, steal); }

	// Constructs from a tPicture. Single-frame.
	tImageTIFF(tPicture& picture, bool steal = true)																	{ Set(picture, steal); }

	virtual ~tImageTIFF()																								{ Clear(); }

	// Clears the current tImageTIFF before loading. If false returned object is invalid.
	bool Load(const tString& tiffFile);

	bool Set(tList<tFrame>& srcFrames, bool stealFrames);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4b* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	enum class tFormat
	{
		Invalid,	// Invalid must be 0.
		BPP24,		// RGB.  24 bit colour.
		BPP32,		// RGBA. 24 bit colour and 8 bits opacity in the alpha channel.
		Auto		// Save function will decide format. BPP24 if all image pixels are opaque and BPP32 otherwise.
	};

	struct SaveParams
	{
		SaveParams()																									{ Reset(); }
		SaveParams(const SaveParams& src)																				: Format(src.Format), UseZLibCompression(src.UseZLibCompression), OverrideFrameDuration(src.OverrideFrameDuration) { }
		void Reset()																									{ Format = tFormat::Auto; UseZLibCompression = true; OverrideFrameDuration = -1; }
		SaveParams& operator=(const SaveParams& src)																	{ Format = src.Format; UseZLibCompression = src.UseZLibCompression; OverrideFrameDuration = src.OverrideFrameDuration; return *this; }

		tFormat Format;
		bool UseZLibCompression;
		int OverrideFrameDuration;
	};

	// Saves the tImageTIFF to the TIFF file specified. The type of filename must be TIFF (tif or tiff extension).
	// If tFormat is Auto, this function will decide the format. BPP24 if all image pixels are opaque and BPP32
	// otherwise. Since each frame (page in tiff parlance) may be stored in a different pixel format, we cannot return
	// the chosen pixel format as they mey be different between frames. Returns true on success. OverrideframeDuration
	// is in milliseconds. Set to >= 0 to override all frames.
	bool Save(const tString& tiffFile, tFormat, bool useZLibComp = true, int overrideFrameDuration = -1) const;
	bool Save(const tString& tiffFile, const SaveParams& = SaveParams()) const;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return (GetNumFrames() >= 1); }
	int GetNumFrames() const																							{ return Frames.GetNumItems(); }

	// Returns true if ALL frames are opaque. Slow. Checks all pixels.
	bool IsOpaque() const;

	// After this call you are the owner of the frame and must eventually delete it. The frame you stole will no longer
	// be part of the tImageTIFF, but the remaining ones will still be there. GetNumFrames will be one fewer.
	tFrame* StealFrame(int frameNum);
	tFrame* GetFrame(bool steal = true) override;

	// Similar to above but takes all the frames from the tImageTIFF and appends them to the supplied frame list. The
	// object will be invalid after since it will have no frames.
	void StealFrames(tList<tFrame>&);

	// Returns a pointer to the frame, but it's not yours to delete. This object still owns it.
	tFrame* GetFrame(int frameNum);

	tPixelFormat GetPixelFormatSrc() const override																		{ return IsValid() ? PixelFormatSrc : tPixelFormat::Invalid; }
	tPixelFormat GetPixelFormat() const override																		{ return IsValid() ? tPixelFormat::R8G8B8A8 : tPixelFormat::Invalid; }

private:
	int ReadSoftwarePageDuration(TIFF*) const;
	bool WriteSoftwarePageDuration(TIFF*, int milliseconds) const;

	tPixelFormat PixelFormatSrc = tPixelFormat::Invalid;
	tList<tFrame> Frames;
};


// Implementation only below.


inline tFrame* tImage::tImageTIFF::StealFrame(int frameNum)
{
	tFrame* f = GetFrame(frameNum);
	if (!f)
		return nullptr;

	return Frames.Remove(f);
}


inline void tImage::tImageTIFF::StealFrames(tList<tFrame>& frames)
{
	while (tFrame* frame = Frames.Remove())
		frames.Append(frame);
}


inline tFrame* tImage::tImageTIFF::GetFrame(int frameNum)
{
	if ((frameNum >= Frames.GetNumItems()) || (frameNum < 0))
		return nullptr;

	tFrame* f = Frames.First();
	while (frameNum--)
		f = f->Next();

	return f;
}


inline void tImageTIFF::Clear()
{
	while (tFrame* frame = Frames.Remove())
		delete frame;

	PixelFormatSrc = tPixelFormat::Invalid;
}


}
