// tImageAPNG.h
//
// This knows how to load animated PNGs (APNGs). It knows the details of the apng file format and loads the data into
// multiple tPixel arrays, one for each frame. These arrays may be 'stolen' by tPictures.
//
// Copyright (c) 2020 Tristan Grimmer.
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
namespace tImage
{


class tImageAPNG
{
public:
	// Creates an invalid tImageAPNG. You must call Load manually.
	tImageAPNG()																										{ }
	tImageAPNG(const tString& apngFile)																					{ Load(apngFile); }

	virtual ~tImageAPNG()																								{ Clear(); }

	// Clears the current tImageAPNG before loading. If false returned object is invalid.
	bool Load(const tString& apngFile);

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear();
	bool IsValid() const																								{ return (GetNumFrames() >= 1); }
	int GetNumFrames() const																							{ return Frames.GetNumItems(); }

	struct Frame : public tLink<Frame>
	{
		int Width = 0;
		int Height = 0;
		tPixel* Pixels = nullptr;
		float Duration = 0.0f;			// Frame duration in seconds.
		tPixelFormat SrcPixelFormat = tPixelFormat::Invalid;
	};

	// After this call you are the owner of the frame and must eventually delete it. The frame you stole will no
	// longer be a valid frame of the tImageAPNG, but the remaining ones will still be valid.
	Frame* StealFrame(int frameNum);
	Frame* GetFrame(int frameNum);
	tPixelFormat SrcPixelFormat = tPixelFormat::Invalid;

private:
	tList<Frame> Frames;
};


// Implementation only below.


inline tImageAPNG::Frame* tImage::tImageAPNG::StealFrame(int frameNum)
{
	Frame* f = GetFrame(frameNum);
	if (!f)
		return nullptr;

	return Frames.Remove(f);
}


inline tImageAPNG::Frame* tImage::tImageAPNG::GetFrame(int frameNum)
{
	if ((frameNum >= Frames.GetNumItems()) || (frameNum < 0))
		return nullptr;

	Frame* f = Frames.First();
	while (frameNum--)
		f = f->Next();

	return f;
}


inline void tImageAPNG::Clear()
{
	while (Frame* frame = Frames.Remove())
	{
		delete[] frame->Pixels;
		delete frame;
	}

	SrcPixelFormat = tPixelFormat::Invalid;
}


}
