// tImageEXR.h
//
// This knows how to load and save OpenEXR images (.exr). It knows the details of the exr high dynamic range
// file format and loads the data into a tPixel array. These tPixels may be 'stolen' by the tPicture's constructor if
// an EXR file is specified. After the array is stolen the tImageEXR is invalid. This is purely for performance.
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
#include <Image/tBaseImage.h>
namespace tImage
{


class tImageEXR : public tBaseImage
{
public:
	struct LoadParams
	{
		LoadParams()																									{ Reset(); }
		void Reset();
		float Gamma;		// [   0.6, 3.0  ]
		float Exposure;		// [ -10.0, 10.0 ]
		float Defog;		// [   0.0, 0.1  ] Try to keep below 0.01.
		float KneeLow;		// [  -3.0, 3.0  ]
		float KneeHigh;		// [   3.5, 7.5  ]
	};

	// Creates an invalid tImageEXR. You must call Load manually.
	tImageEXR()																											{ }
	tImageEXR(const tString& exrFile, const LoadParams& loadParams = LoadParams())										{ Load(exrFile, loadParams); }

	// Creates a tImageEXR from a bunch of frames. If steal is true, the srcFrames will be empty after.
	tImageEXR(tList<tFrame>& srcFrames, bool stealFrames)																{ Set(srcFrames, stealFrames); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageEXR(tPixel4* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageEXR(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture. Single-frame.
	tImageEXR(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImageEXR()																								{ Clear(); }

	// Clears the current tImageEXR before loading. If false returned object is invalid.
	bool Load(const tString& exrFile, const LoadParams& = LoadParams());

	bool Set(tList<tFrame>& srcFrames, bool stealFrames);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return (GetNumFrames() >= 1); }
	int GetNumFrames() const																							{ return Frames.GetNumItems(); }

	// Returns true if ALL frames are opaque. Slow. Checks all pixels.
	bool IsOpaque() const;

	// After this call you are the owner of the frame and must eventually delete it. The frame you stole will no longer
	// be part of the tImageEXR, but the remaining ones will still be there. GetNumFrames will be one fewer.
	tFrame* StealFrame(int frameNum);
	tFrame* GetFrame(bool steal = true) override;

	// Similar to above but takes all the frames from the tImageEXR and appends them to the supplied frame list. The
	// object will be invalid after since it will have no frames.
	void StealFrames(tList<tFrame>&);

	// Returns a pointer to the frame, but it's not yours to delete. This object still owns it.
	tFrame* GetFrame(int frameNum);

	tPixelFormat GetPixelFormatSrc() const override																		{ return IsValid() ? PixelFormatSrc : tPixelFormat::Invalid; }
	tPixelFormat GetPixelFormat() const override																		{ return IsValid() ? tPixelFormat::R8G8B8A8 : tPixelFormat::Invalid; }

private:
	tPixelFormat PixelFormatSrc = tPixelFormat::Invalid;
	tList<tFrame> Frames;
};


// Implementation only below.


inline void tImageEXR::LoadParams::Reset()
{
	Gamma			= tMath::DefaultGamma;
	Exposure		= 1.0f;
	Defog			= 0.0f;
	KneeLow			= 0.0f;
	KneeHigh		= 3.5f;
}


inline bool tImageEXR::IsOpaque() const
{
	for (tFrame* frame = Frames.Head(); frame; frame = frame->Next())
		if (!frame->IsOpaque())
			return false;

	return true;
}


inline tFrame* tImageEXR::StealFrame(int frameNum)
{
	tFrame* f = GetFrame(frameNum);
	if (!f)
		return nullptr;

	return Frames.Remove(f);
}


inline void tImage::tImageEXR::StealFrames(tList<tFrame>& frames)
{
	while (tFrame* frame = Frames.Remove())
		frames.Append(frame);
}


inline tFrame* tImageEXR::GetFrame(int frameNum)
{
	if ((frameNum >= Frames.GetNumItems()) || (frameNum < 0))
		return nullptr;

	tFrame* f = Frames.First();
	while (frameNum--)
		f = f->Next();

	return f;
}


inline void tImageEXR::Clear()
{
	while (tFrame* frame = Frames.Remove())
		delete frame;

	PixelFormatSrc = tPixelFormat::Invalid;
}


}
