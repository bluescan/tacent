// tImagePNG.h
//
// This class knows how to load and save PNG files. It does zero processing of image data. It knows the details of the
// png file format and loads the data into a tPixel array. These tPixels may be 'stolen' by the tPicture's constructor
// if a png file is specified. After the array is stolen the tImagePNG is invalid. This is purely for performance.
//
// Copyright (c) 2020, 2022 Tristan Grimmer.
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
#include <Image/tBaseImage.h>
namespace tImage
{


class tImagePNG : public tBaseImage
{
public:
	// Creates an invalid tImagePNG. You must call Load manually.
	tImagePNG()																											{ }
	tImagePNG(const tString& pngFile)																					{ Load(pngFile); }

	// The data is copied out of pngFileInMemory. Go ahead and delete after if you want.
	tImagePNG(const uint8* pngFileInMemory, int numBytes)																{ Load(pngFileInMemory, numBytes); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImagePNG(tPixel* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImagePNG(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImagePNG(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImagePNG()																								{ Clear(); }

	// Clears the current tImagePNG before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& pngFile);
	bool Load(const uint8* pngFileInMemory, int numBytes);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel*, int width, int height, bool steal = false);

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	enum class tFormat
	{
		Auto,		// Save function will decide format. BPP24 if all image pixels are opaque and BPP32 otherwise.
		BPP24,		// RGB.  24 bit colour.
		BPP32		// RGBA. 24 bit colour and 8 bits opacity in the alpha channel.
	};

	// Saves the tImagePNG to the PNG file specified. The type of filename must be "png". Returns true on success.
	bool Save(const tString& pngFile, tFormat = tFormat::Auto) const;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return Pixels ? true : false; }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }
	bool IsOpaque() const;

	// After this call you are the owner of the pixels and must eventually delete[] them. This tImagePNG object is
	// invalid afterwards.
	tPixel* StealPixels();
	tFrame* GetFrame(bool steal = true) override;

	tPixel* GetPixels() const																							{ return Pixels; }
	tPixelFormat PixelFormatSrc		= tPixelFormat::Invalid;

private:

	// @todo We could just use a single tFrame here instead of the 3 members below. Might simplify it a bit.
	int Width						= 0;
	int Height						= 0;
	tPixel* Pixels					= nullptr;
};


// Implementation below this line.


inline void tImagePNG::Clear()
{
	Width = 0;
	Height = 0;
	delete[] Pixels;
	Pixels = nullptr;
	PixelFormatSrc = tPixelFormat::Invalid;
}


}
