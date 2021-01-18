// tImageAPNG.h
//
// This knows how to load/save animated PNGs (APNGs). It knows the details of the apng file format and loads the data
// into multiple tPixel arrays, one for each frame. These arrays may be 'stolen' by tPictures.
//
// Copyright (c) 2020, 2021 Tristan Grimmer.
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


class tImageAPNG
{
public:
	// Creates an invalid tImageAPNG. You must call Load manually.
	tImageAPNG()																										{ }
	tImageAPNG(const tString& apngFile)																					{ Load(apngFile); }

	// Creates a tImageAPNG from a bunch of frames. If steal is true, the srcFrames will be empty after.
	tImageAPNG(tList<tFrame>& srcFrames, bool stealFrames)																{ Set(srcFrames, stealFrames); }
	virtual ~tImageAPNG()																								{ Clear(); }

	// Clears the current tImageAPNG before loading. If false returned object is invalid.
	bool Load(const tString& apngFile);

	// OverrideframeDuration is in milliseconds. Set to >= 0 to override all frames.
	bool Save(const tString& apngFile, int overrideframeDuration = -1);
	bool Set(tList<tFrame>& srcFrames, bool stealFrames);

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear();
	bool IsValid() const																								{ return (GetNumFrames() >= 1); }
	int GetNumFrames() const																							{ return Frames.GetNumItems(); }

	// Returns true if ALL frames are opaque. Slow. Checks all pixels.
	bool IsOpaque() const;

	// After this call you are the owner of the frame and must eventually delete it. The frame you stole will no
	// longer be a valid frame of the tImageAPNG, but the remaining ones will still be valid.
	tFrame* StealFrame(int frameNum);
	tFrame* GetFrame(int frameNum);
	tPixelFormat SrcPixelFormat = tPixelFormat::Invalid;

	// Since some apng files may have a .png extension, it is hand to quickly be able to tell if a particular .png
	// file is an apng. Probably no one will ever read this comment, but the Mozilla apng people should probably not
	// have insisted that apngs be encoded in pngs. In any case, this slightly crappy code cannot guarantee that a
	// return value of true means it is an apng (although such a false positive is extremely unlikely). Even in these
	// cases, it just means the APNG reading code will be used -- it will still successfully extract the single frame.
	//
	// The preference is, however, that non-apng files be loaded by tImagePNG. It is faster and reads the src format
	// better than APngDis, which could be further modified but is unfamiliar code.
	static bool IsAnimatedPNG(const tString& pngFile);

	tList<tFrame> Frames;
};


// Implementation only below.


inline tFrame* tImage::tImageAPNG::StealFrame(int frameNum)
{
	tFrame* f = GetFrame(frameNum);
	if (!f)
		return nullptr;

	return Frames.Remove(f);
}


inline tFrame* tImage::tImageAPNG::GetFrame(int frameNum)
{
	if ((frameNum >= Frames.GetNumItems()) || (frameNum < 0))
		return nullptr;

	tFrame* f = Frames.First();
	while (frameNum--)
		f = f->Next();

	return f;
}


inline void tImageAPNG::Clear()
{
	while (tFrame* frame = Frames.Remove())
		delete frame;

	SrcPixelFormat = tPixelFormat::Invalid;
}


}
