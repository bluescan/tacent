// tImageTIFF.h
//
// This knows how to load/save TIFFs. It knows the details of the tiff file format and loads the data into multiple
// tPixel arrays, one for each frame (in a TIFF thay are called pages). These arrays may be 'stolen' by tPictures.
//
// Copyright (c) 2020-2022 Tristan Grimmer.
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
namespace tImage
{


class tImageTIFF
{
public:
	// Creates an invalid tImageTIFF. You must call Load manually.
	tImageTIFF()																										{ }
	tImageTIFF(const tString& tiffFile)																					{ Load(tiffFile); }

	// Creates a tImageAPNG from a bunch of frames. If steal is true, the srcFrames will be empty after.
	tImageTIFF(tList<tFrame>& srcFrames, bool stealFrames)																{ Set(srcFrames, stealFrames); }

	virtual ~tImageTIFF()																								{ Clear(); }

	// Clears the current tImageTIFF before loading. If false returned object is invalid.
	bool Load(const tString& tiffFile);

	// OverrideframeDuration is in milliseconds. Set to >= 0 to override all frames.
	bool Save(const tString& tiffFile, bool useZLibComp = true, int overrideFrameDuration = -1);
	bool Set(tList<tFrame>& srcFrames, bool stealFrames);

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear();
	bool IsValid() const																								{ return (GetNumFrames() >= 1); }
	int GetNumFrames() const																							{ return Frames.GetNumItems(); }

	// Returns true if ALL frames are opaque. Slow. Checks all pixels.
	bool IsOpaque() const;

	// After this call you are the owner of the frame and must eventually delete it. The frame you stole will no longer
	// be part of the tImageTIFF, but the remaining ones will still be there. GetNumFrames will be one fewer.
	tFrame* StealFrame(int frameNum);

	// Similar to above but takes all the frames from the tImageTIFF and appends them to the supplied frame list. The
	// object will be invalid after since it will have no frames.
	void StealFrames(tList<tFrame>&);

	// Returns a pointer to the frame, but it's not yours to delete. This object still owns it.
	tFrame* GetFrame(int frameNum);

	tPixelFormat SrcPixelFormat = tPixelFormat::Invalid;

private:
	int ReadSoftwarePageDuration(TIFF* tiff) const;
	bool WriteSoftwarePageDuration(TIFF* tiff, int milliseconds);

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

	SrcPixelFormat = tPixelFormat::Invalid;
}


}
