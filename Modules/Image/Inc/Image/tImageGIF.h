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
#include <Image/tQuantize.h>
#include <Image/tFrame.h>
#include <Image/tBaseImage.h>
namespace tImage
{


class tImageGIF : public tBaseImage
{
public:
	// Creates an invalid tImageGIF. You must call Load manually.
	tImageGIF()																											{ }
	tImageGIF(const tString& gifFile)																					{ Load(gifFile); }

	// Creates a tImageGIF from a bunch of frames. If steal is true, the srcFrames will be empty after.
	tImageGIF(tList<tFrame>& srcFrames, bool stealFrames)																{ Set(srcFrames, stealFrames); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageGIF(tPixel* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageGIF(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture. Single-frame.
	tImageGIF(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImageGIF()																								{ Clear(); }

	// Clears the current tImageGIF before loading. If false returned object is invalid.
	bool Load(const tString& gifFile);

	bool Set(tList<tFrame>& srcFrames, bool stealFrames);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	// This function returns true on success. If any required condition is not met false is returned. gifFile is the
	// file to save to. It must end with a .gif extension.
	//
	// format must be one of the PALNBIT formats where N E [1,8]. i.e. Palette size 2, 4, 8, 16, 32, 64, 128, or 256.
	//
	// method should be set to one of the 4 available quantization methods: fixed, neuquant, wu, or scolorq.
	//
	// loop should be set to -1 for no looping. Single frame gifs always force loop to be set to -1. For multi-frame
	// gifs loop should be set to 0 to loop forever, and a value > 0 to loop a specific number of times.
	//
	// If alphaThreshold is -1, the gif is guaranteed to be opaque.
	// Otherwise alphaThreshold should be E [0, 255]. Any pixel with alpha <= alphaThreshold is considered transparent
	// (gif supports binary alpha only). Pixel alpha values > alphaThreshold are considered opaque. If PAL1BIT is chosen
	// as the pixel format (2 palette entries), alphaThreshold is forced to -1 (fully opaque image). This is because gif
	// transparency uses a palette entry, and colour quantization on a single colour is ill-defined.
	//
	// OverrideframeDuration is in 1/100 seconds. Set to >= 0 to override all frames. Note that values of 0 or 1 get
	// min-clamped to 2 during save since many viewers do not handle values below 2 properly. If overrideFrameDuration
	// is < 0, the individual frames' duration is used after being converted from seconds to 1/100th of seconds.
	bool Save
	(
		const tString& gifFile, tPixelFormat format = tPixelFormat::PAL8BIT,
		tQuantize::Method method = tQuantize::Method::Wu,
		int loop = -1, int alphaThreshold = -1, int overrideFrameDuration = -1
	);

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return (GetNumFrames() >= 1); }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }
	int GetNumFrames() const																							{ return Frames.GetNumItems(); }

	// After this call you are the owner of the frame and must eventually delete it. The frame you stole will no longer
	// be part of the tImageGIF, but the remaining ones will still be there. GetNumFrames will be one fewer.
	tFrame* StealFrame(int frameNum);

	// Similar to above but takes all the frames from the tImageGIF and appends them to the supplied frame list. The
	// object will be invalid after since it will have no frames.
	void StealFrames(tList<tFrame>&);
	tFrame* GetFrame(bool steal = true) override;

	// Returns a pointer to the frame, but it's not yours to delete. This object still owns it.
	tFrame* GetFrame(int frameNum);

	tPixelFormat PixelFormatSrc = tPixelFormat::Invalid;

private:
	static void FrameLoadCallbackBridge(void* imgGifRaw, struct GIF_WHDR*);
	void FrameLoadCallback(struct GIF_WHDR*);

	// Variables used during load callback processing.
	int FrmLast = 0;
	tPixel* FrmPict = nullptr;
	tPixel* FrmPrev = nullptr;

	int Width				= 0;
	int Height				= 0;
	tList<tFrame> Frames;
};


// Implementation only below.


inline void tImageGIF::FrameLoadCallbackBridge(void* imgGifRaw, struct GIF_WHDR* whdr)
{
	tImageGIF* imgGif = (tImageGIF*)imgGifRaw;
	imgGif->FrameLoadCallback(whdr);
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

	PixelFormatSrc = tPixelFormat::Invalid;
}


}
