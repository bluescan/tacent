// tImageQOI.h
//
// This class knows how to load and save Quite OK Images (.qoi) files into tPixel arrays. These tPixels may be 'stolen'
// by the tPicture's constructor if a qoi file is specified. After the array is stolen the tImageQOI is invalid. This
// is purely for performance.
//
// Copyright (c) 2022 Tristan Grimmer.
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


class tImageQOI : public tBaseImage
{
public:
	// Creates an invalid tImageQOI. You must call Load manually.
	tImageQOI()																											{ }
	tImageQOI(const tString& qoiFile)																					{ Load(qoiFile); }

	// The data is copied out of qoiFileInMemory. Go ahead and delete[] after if you want.
	tImageQOI(const uint8* qoiFileInMemory, int numBytes)																{ Load(qoiFileInMemory, numBytes); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out. Sets the colour space to sRGB. Call SetColourSpace after if you wanted linear.
	tImageQOI(tPixel* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageQOI(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImageQOI(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImageQOI()																								{ Clear(); }

	// Clears the current tImageQOI before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& qoiFile);
	bool Load(const uint8* qoiFileInMemory, int numBytes);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out. After this call the objects ColourSpace is set to sRGB. If the data was all linear
	// you can call SetColourSpace() manually afterwards.
	bool Set(tPixel* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	enum class tFormat
	{
		Invalid,	// Invalid must be 0.
		Auto,		// Save function will decide format. Bit24 if all image pixels are opaque and Bit32 otherwise.
		Bit24,		// 24 bit colour.
		Bit32		// 24 bit colour with 8 bits opacity in the alpha channel.
	};
	enum class tSpace
	{
		sRGB,		// sRGB (RGB in sRGB and A linear).
		Linear		// RGB(A) all linear.
	};

	// Saves the tImageQOI to the file specified. The extension of filename must be ".qoi". If tFormat is Auto, this
	// function will decide the format. Bit24 if all image pixels are opaque and Bit32 otherwise. Returns the format
	// that the file was saved in, or tFormat::Invalid if there was a problem. Since Invalid is 0, you can use an 'if'.
	// The colour-space is also saved with the file and can be retrieved on load. Optionally call SetColourSpace before
	// saving if you need to (although usually the default sRGB is the correct one).
	tFormat Save(const tString& qoiFile, tFormat = tFormat::Auto) const;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return Pixels ? true : false; }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }

	// All pixels must be opaque (alpha = 255) for this to return true.
	bool IsOpaque() const;

	tSpace GetColourSpace() const																						{ return ColourSpace; }
	void SetColourSpace(tSpace space)																					{ ColourSpace = space; }

	// After this call you are the owner of the pixels and must eventually delete[] them. This tImageQOI object is
	// invalid afterwards.
	tPixel* StealPixels();
	tFrame* StealFrame() override;

	tPixel* GetPixels() const																							{ return Pixels; }
	tPixelFormat PixelFormatSrc = tPixelFormat::Invalid;

private:

	tSpace ColourSpace		= tSpace::sRGB;
	int Width				= 0;
	int Height				= 0;
	tPixel* Pixels			= nullptr;
};


// Implementation below this line.


inline void tImageQOI::Clear()
{
	ColourSpace		= tSpace::sRGB;
	Width			= 0;
	Height			= 0;
	delete[]		Pixels;
	Pixels			= nullptr;
	PixelFormatSrc	= tPixelFormat::Invalid;
}


}
