// tImageWEBP.h
//
// This knows how to load/save WebPs. It knows the details of the webp file format and loads the data into multiple
// tPixel arrays, one for each frame (WebPs may be animated). These arrays may be 'stolen' by tPictures.
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


class tImageWEBP : public tBaseImage
{
public:
	// Creates an invalid tImageWEBP. You must call Load manually.
	tImageWEBP()																										{ }
	tImageWEBP(const tString& webpFile)																					{ Load(webpFile); }

	// Creates a tImageWEBP from a bunch of frames. If steal is true, the srcFrames will be empty after.
	tImageWEBP(tList<tFrame>& srcFrames, bool stealFrames)																{ Set(srcFrames, stealFrames); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageWEBP(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageWEBP(tFrame* frame, bool steal = true)																		{ Set(frame, steal); }

	// Constructs from a tPicture. Single-frame.
	tImageWEBP(tPicture& picture, bool steal = true)																	{ Set(picture, steal); }

	virtual ~tImageWEBP()																								{ Clear(); }

	// Clears the current tImageWEBP before loading. If false returned object is invalid.
	bool Load(const tString& webpFile);
	bool Load(const uint8* webpFileInMemory, int numBytes);
	
	bool Set(tList<tFrame>& srcFrames, bool stealFrames);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4b* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	struct SaveParams
	{
		SaveParams()																									{ Reset(); }
		SaveParams(const SaveParams& src)																				: Lossy(src.Lossy), QualityCompstr(src.QualityCompstr), OverrideFrameDuration(src.OverrideFrameDuration) { }
		void Reset()																									{ Lossy = false; QualityCompstr = 90.0f; OverrideFrameDuration = -1; }
		SaveParams& operator=(const SaveParams& src)																	{ Lossy = src.Lossy; QualityCompstr = src.QualityCompstr; OverrideFrameDuration = src.OverrideFrameDuration; return *this; }

		bool Lossy;

		// QualityCompstr E [0.0, 100.0]. Interpret as quality for lossy images. Bigger looks better but bigger file size.
		// Interpret as compression strength for non-lossy.	Bigger values compress more (smaller size) but takes longer.
		float QualityCompstr;

		// In milliseconds. Set to >= 0 to override all frames.
		int OverrideFrameDuration;
	};

	// Saves the tImageWEBP to the WEBP file specified. The type of filename must be WEBP. If lossy is true it will
	// generate smaller files (think jpg with alpha) at cost of non-exact pixel colours. If lossy is false, generates
	// pixel-perfect images that are compressed. OverrideframeDuration is in milliseconds. Set to >= 0 to override all
	// frames. Returns true on success.
	bool Save(const tString& webpFile, bool lossy, float qualityCompstr = 90.0f, int overrideFrameDuration = -1) const;
	bool Save(const tString& webpFile, const SaveParams& = SaveParams()) const;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return (GetNumFrames() >= 1); }
	int GetNumFrames() const																							{ return Frames.GetNumItems(); }

	// After this call you are the owner of the frame and must eventually delete it. The frame you stole will no longer
	// be part of the tImageWEBP, but the remaining ones will still be there. GetNumFrames will be one fewer.
	tFrame* StealFrame(int frameNum);

	// Similar to above but takes all the frames from the tImageWEBP and appends them to the supplied frame list. The
	// object will be invalid after since it will have no frames.
	void StealFrames(tList<tFrame>&);
	tFrame* GetFrame(bool steal = true) override;

	// Returns a pointer to the frame, but it's not yours to delete. This object still owns it.
	tFrame* GetFrame(int frameNum);

	tList<tFrame> Frames;

	// The background colour of webp files defaults to white only because that is what popular browsers default to.
	// Animated webp files can override this colour. They store a background colour in the file itself.
	// The Load function above sets this colour every time it is called.
	tColour4b BackgroundColour = tColour4b::white;

private:
	bool CopyRegion(tPixel4b* dst, int dstW, int dstH, tPixel4b* src, int srcW, int srcH, int offsetX, int offsetY, bool blend);
};


// Implementation only below.


inline tFrame* tImage::tImageWEBP::StealFrame(int frameNum)
{
	tFrame* f = GetFrame(frameNum);
	if (!f)
		return nullptr;

	return Frames.Remove(f);
}


inline void tImage::tImageWEBP::StealFrames(tList<tFrame>& frames)
{
	while (tFrame* frame = Frames.Remove())
		frames.Append(frame);
}


inline tFrame* tImage::tImageWEBP::GetFrame(int frameNum)
{
	if ((frameNum >= Frames.GetNumItems()) || (frameNum < 0))
		return nullptr;

	tFrame* f = Frames.First();
	while (frameNum--)
		f = f->Next();

	return f;
}


inline void tImageWEBP::Clear()
{
	while (tFrame* frame = Frames.Remove())
		delete frame;
	BackgroundColour = tColour4b::white;	

	tBaseImage::Clear();
}


}
