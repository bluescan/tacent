// tImageGIF.h
//
// This knows how to load gifs. It knows the details of the gif file format and loads the data into multiple tPixel
// arrays, one for each frame (gifs may be animated). These arrays may be 'stolen' by tPictures.
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
namespace tImage
{


class tImageGIF
{
public:
	// Creates an invalid tImageGIF. You must call Load manually.
	tImageGIF()																											{ }
	tImageGIF(const tString& gifFile)																					{ Load(gifFile); }

	// Creates a tImageGIF from a bunch of frames. If steal is true, the srcFrames will be empty after.
	tImageGIF(tList<tFrame>& srcFrames, bool stealFrames)																{ Set(srcFrames, stealFrames); }

	virtual ~tImageGIF()																								{ Clear(); }

	// Clears the current tImageGIF before loading. If false returned object is invalid.
	bool Load(const tString& gifFile);

	// OverrideframeDuration is in 1/100 seconds. Set to >= 0 to override all frames. Note that values of 0 or 1 get
	// min-clamped to 2 during save since many viewers do not handle values below 2 properly.
	bool Save(const tString& gifFile, int overrideFrameDuration = -1);
	bool Set(tList<tFrame>& srcFrames, bool stealFrames);

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear();
	bool IsValid() const																								{ return (GetNumFrames() >= 1); }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }
	int GetNumFrames() const																							{ return Frames.GetNumItems(); }

	// After this call you are the owner of the frame and must eventually delete it. The frame you stole will no longer
	// be part of the tImageGIF, but the remaining ones will still be there. GetNumFrames will be one fewer.
	tFrame* StealFrame(int frameNum);

	// Similar to above but takes all the frames from the tImageGIF and appends them to the supplied frame list. The
	// object will be invalid after since it will have no frames.
	void StealFrames(tList<tFrame>&);

	// Returns a pointer to the frame, but it's not yours to delete. This object still owns it.
	tFrame* GetFrame(int frameNum);

	tPixelFormat SrcPixelFormat = tPixelFormat::Invalid;

private:
	static void FrameCallbackBridge(void* imgGifRaw, struct GIF_WHDR*);
	void FrameCallback(struct GIF_WHDR*);

	// Variables used during callback processing.
	int FrmLast = 0;
	tPixel* FrmPict = nullptr;
	tPixel* FrmPrev = nullptr;

	int Width				= 0;
	int Height				= 0;
	tList<tFrame> Frames;
};


// Implementation only below.


inline void tImageGIF::FrameCallbackBridge(void* imgGifRaw, struct GIF_WHDR* whdr)
{
	tImageGIF* imgGif = (tImageGIF*)imgGifRaw;
	imgGif->FrameCallback(whdr);
}


inline tFrame* tImage::tImageGIF::StealFrame(int frameNum)
{
	tFrame* f = GetFrame(frameNum);
	if (!f)
		return nullptr;

	return Frames.Remove(f);
}


inline void tImage::tImageGIF::StealFrames(tList<tFrame>& frames)
{
	while (tFrame* frame = Frames.Remove())
		frames.Append(frame);
}


inline tFrame* tImage::tImageGIF::GetFrame(int frameNum)
{
	if ((frameNum >= Frames.GetNumItems()) || (frameNum < 0))
		return nullptr;

	tFrame* f = Frames.First();
	while (frameNum--)
		f = f->Next();

	return f;
}


inline void tImageGIF::Clear()
{
	Width = 0;
	Height = 0;
	while (tFrame* frame = Frames.Remove())
		delete frame;

	SrcPixelFormat = tPixelFormat::Invalid;
}


}
