// tImageAPNG.h
//
// This knows how to load/save animated PNGs (APNGs). It knows the details of the apng file format and loads the data
// into multiple tPixel arrays, one for each frame. These arrays may be 'stolen' by tPictures.
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


class tImageAPNG : public tBaseImage
{
public:
	// Creates an invalid tImageAPNG. You must call Load manually.
	tImageAPNG()																										{ }
	tImageAPNG(const tString& apngFile)																					{ Load(apngFile); }

	// Creates a tImageAPNG from a bunch of frames. If steal is true, the srcFrames will be empty after.
	tImageAPNG(tList<tFrame>& srcFrames, bool stealFrames)																{ Set(srcFrames, stealFrames); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageAPNG(tPixel4* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageAPNG(tFrame* frame, bool steal = true)																		{ Set(frame, steal); }

	// Constructs from a tPicture. Single-frame.
	tImageAPNG(tPicture& picture, bool steal = true)																	{ Set(picture, steal); }

	virtual ~tImageAPNG()																								{ Clear(); }

	// Clears the current tImageAPNG before loading. If false returned object is invalid.
	bool Load(const tString& apngFile);

	bool Set(tList<tFrame>& srcFrames, bool stealFrames);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4* pixels, int width, int height, bool steal = false) override;

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
		SaveParams(const SaveParams& src)																				: Format(src.Format), OverrideFrameDuration(src.OverrideFrameDuration) { }
		void Reset()																									{ Format = tFormat::Auto; OverrideFrameDuration = -1; }
		SaveParams& operator=(const SaveParams& src)																	{ Format = src.Format; OverrideFrameDuration = src.OverrideFrameDuration; return *this; }

		tFormat Format;
		int OverrideFrameDuration;
	};

	// Saves the tImageAPNG to the APNG file specified. The type of filename must be PNG or APNG. PNG is allowed because
	// the way apng files are specified they can have the png extension and still be read by non-apng-aware loaders.
	// If tFormat is Auto, this function will decide the format. BPP24 if all image pixels are opaque and BPP32
	// otherwise. Returns the format that the file was saved in, or tFormat::Invalid if there was a problem. Since
	// Invalid is 0, you can use an 'if'. OverrideframeDuration is in milliseconds. Set to >= 0 to override all frames.
	tFormat Save(const tString& apngFile, tFormat, int overrideframeDuration = -1) const;
	tFormat Save(const tString& apngFile, const SaveParams& = SaveParams()) const;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return (GetNumFrames() >= 1); }
	int GetNumFrames() const																							{ return Frames.GetNumItems(); }

	// Returns true if ALL frames are opaque. Slow. Checks all pixels.
	bool IsOpaque() const;

	// After this call you are the owner of the frame and must eventually delete it. The frame you stole will no longer
	// be part of the tImageAPNG, but the remaining ones will still be there. GetNumFrames will be one fewer.
	tFrame* StealFrame(int frameNum);

	// Gets the first frame only.
	tFrame* GetFrame(bool steal = true) override;

	// Similar to above but takes all the frames from the tImageAPNG and appends them to the supplied frame list. The
	// object will be invalid after since it will have no frames.
	void StealFrames(tList<tFrame>&);

	// Returns a pointer to the frame, but it's not yours to delete. This object still owns it.
	tFrame* GetFrame(int frameNum);

	// Since some apng files may have a .png extension, it is hand to quickly be able to tell if a particular .png
	// file is an apng. Probably no one will ever read this comment, but the Mozilla apng people should probably not
	// have insisted that apngs be encoded in pngs. In any case, this slightly crappy code cannot guarantee that a
	// return value of true means it is an apng (although such a false positive is extremely unlikely). Even in these
	// cases, it just means the APNG reading code will be used -- it will still successfully extract the single frame.
	//
	// The preference is, however, that non-apng files be loaded by tImagePNG. It is faster and reads the src format
	// better than APngDis, which could be further modified but is unfamiliar code.
	static bool IsAnimatedPNG(const tString& pngFile);

	tPixelFormat GetPixelFormatSrc() const override																		{ return IsValid() ? PixelFormatSrc : tPixelFormat::Invalid; }
	tPixelFormat GetPixelFormat() const override																		{ return IsValid() ? tPixelFormat::R8G8B8A8 : tPixelFormat::Invalid; }

	tList<tFrame> Frames;

private:
	tPixelFormat PixelFormatSrc = tPixelFormat::Invalid;
};


// Implementation only below.


inline bool tImage::tImageAPNG::IsOpaque() const
{
	for (tFrame* frame = Frames.Head(); frame; frame = frame->Next())
		if (!frame->IsOpaque())
			return false;

	return true;
}


inline tFrame* tImage::tImageAPNG::StealFrame(int frameNum)
{
	tFrame* f = GetFrame(frameNum);
	if (!f)
		return nullptr;

	return Frames.Remove(f);
}


inline void tImage::tImageAPNG::StealFrames(tList<tFrame>& frames)
{
	while (tFrame* frame = Frames.Remove())
		frames.Append(frame);
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

	PixelFormatSrc = tPixelFormat::Invalid;
}


}
