// tImageGIF.h
//
// This knows how to load gifs. It knows the details of the gif file format and loads the data into multiple tPixel
// arrays, one for each frame (gifs may be animated). These arrays may be 'stolen' by tPictures.
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

	// The data is copied out of gifFileInMemory. Go ahead and delete after if you want.
	tImageGIF(const uint8* gifFileInMemory, int numBytes)																{ Load(gifFileInMemory, numBytes); }

	// Creates a tImageGIF from a bunch of frames. If steal is true, the srcFrames will be empty after.
	tImageGIF(tList<tFrame>& srcFrames, bool stealFrames)																{ Set(srcFrames, stealFrames); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageGIF(tPixel4* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageGIF(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture. Single-frame.
	tImageGIF(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImageGIF()																								{ Clear(); }

	// Clears the current tImageGIF before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& gifFile);
	bool Load(const uint8* gifFileInMemory, int numBytes);

	bool Set(tList<tFrame>& srcFrames, bool stealFrames);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	// This function returns true on success. If any required condition is not met false is returned. gifFile is the
	// file to save to. It must end with a .gif extension.
	//
	// Format must be one of the PALNBIT formats where N E [1,8]. i.e. Palette sizes 2, 4, 8, 16, 32, 64, 128, or 256.
	//
	// Method should be set to one of the 4 available quantization methods: fixed, neuquant, wu, or scolorq.
	//
	// Loop only applies to multi-frame/animated gifs. 0 to loop forever. >0 to loop a specific number of times.
	//
	// Gif files support binary alpha only.
	// If AlphaThreshold is 255 (special case), the saved gif will be be opaque even if not all pixel alphas are max.
	// If AlphaThreshold is E [0, 255), any pixel with alpha <= AlphaThreshold is considered transparent. Pixel alpha
	// values > AlphaThreshold are considered opaque. If PAL1BIT is chosen as the pixel format (2 palette entries),
	// AlphaThreshold is forced to 255 (fully opaque). This is because gif transparency uses a palette entry, and colour
	// quantization on a single colour is not useful.
	// If AlphaThreshold is -1 (auto), then the frame pixels are inspected for transparency. If all frames are fully
	// opaque, an opaque gif will be saved (AlphaThreshold at 255). If any frame has a non-max pixel alpha, an
	// AlphaThreshold of 127 is used.
	//
	// OverrideframeDuration is in 1/100 seconds. Set to >= 0 to override all frames. Note that values of 0 or 1 get
	// min-clamped to 2 during save since many viewers do not handle values below 2 properly. If OverrideFrameDuration
	// is < 0, the individual frames' duration is used after being converted from seconds to 1/100th of seconds.
	struct SaveParams
	{
		SaveParams()																									{ Reset(); }
		SaveParams(const SaveParams& src)																				: Format(src.Format), Method(src.Method), Loop(src.Loop), AlphaThreshold(src.AlphaThreshold), OverrideFrameDuration(src.OverrideFrameDuration), DitherLevel(src.DitherLevel), FilterSize(src.FilterSize), SampleFactor(src.SampleFactor) { }
		void Reset()																									{ Format = tPixelFormat::PAL8BIT; Method = tQuantize::Method::Wu; Loop = 0; AlphaThreshold = -1; OverrideFrameDuration = -1; DitherLevel = 0.0; FilterSize = 3; SampleFactor = 1; }
		SaveParams& operator=(const SaveParams& src)																	{ Format = src.Format; Method = src.Method; Loop = src.Loop; AlphaThreshold = src.AlphaThreshold; OverrideFrameDuration = src.OverrideFrameDuration; DitherLevel = src.DitherLevel; FilterSize = src.FilterSize; SampleFactor = src.SampleFactor; return *this; }

		tPixelFormat Format;		// See comment above. Must be one of the PALNBIT formats wher N is E [1, 8].
		tQuantize::Method Method;	// See comment above. Choose one of the 4 available colour quantization methods.
		int Loop;					// See comment above. Animated only. 0 = infinite (default). >0 = that many times.
		int AlphaThreshold;			// See comment above. -1 = auto. 255 = opaque. else A<=threshold = transp pixel.
		int OverrideFrameDuration;	// See comment above. -1 = use frame duration. >=0 = Set all to this many 1/100 sec.

		double DitherLevel;			// For Method::Spatial only. 0.0 = auto. >0.0 = manual dither amount.
		int FilterSize;				// For Method::Spatial only. Must be 1, 3, or 5. Default is 3.

		int SampleFactor;			// For Method::Neu only. 1 = whole image learning. 10 = 1/10th image used. Max is 30.
	};

	bool Save(const tString& gifFile, const SaveParams& = SaveParams()) const;

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

	tPixelFormat GetPixelFormatSrc() const override																		{ return IsValid() ? PixelFormatSrc : tPixelFormat::Invalid; }
	tPixelFormat GetPixelFormat() const override																		{ return IsValid() ? tPixelFormat::R8G8B8A8 : tPixelFormat::Invalid; }

private:
	static void FrameLoadCallbackBridge(void* imgGifRaw, struct GIF_WHDR*);
	void FrameLoadCallback(struct GIF_WHDR*);

	// Variables used during load callback processing.
	int FrmLast					= 0;
	tPixel4* FrmPict			= nullptr;
	tPixel4* FrmPrev			= nullptr;

	tPixelFormat PixelFormatSrc	= tPixelFormat::Invalid;
	int Width					= 0;
	int Height					= 0;
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
