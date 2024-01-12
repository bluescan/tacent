// tImageXPM.h
//
// This class knows how to load and save an X-Windows Pix Map (.xpm) file. It knows the details of the xpm file format
// and loads the data into a tPixel array. These tPixels may be 'stolen' by the tPicture's constructor if a xpm file is
// specified. After the array is stolen the tImageXPM is invalid. This is purely for performance.
//
// Copyright (c) 2020, 2022-2024 Tristan Grimmer.
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


class tImageXPM : public tBaseImage
{
public:
	// Creates an invalid tImageXPM. You must call Load manually.
	tImageXPM()																											{ }

	tImageXPM(const tString& xpmFile)																					{ Load(xpmFile); }

	// The data is copied out of xpmFileInMemory. Go ahead and delete after if you want.
	tImageXPM(const uint8* xpmFileInMemory, int numBytes)																{ Load(xpmFileInMemory, numBytes); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageXPM(tPixel4* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageXPM(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImageXPM(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImageXPM()																								{ Clear(); }

	// Clears the current tImageXPM before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& xpmFile);
	bool Load(const uint8* xpmFileInMemory, int numBytes);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4*, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return Pixels ? true : false; }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }

	bool IsOpaque() const																								{ return true; }

	// After this call you are the owner of the pixels and must eventually delete[] them. This tImageXPM object is
	// invalid afterwards.
	tPixel4* StealPixels();
	tFrame* GetFrame(bool steal = true) override;
	tPixel4* GetPixels() const																							{ return Pixels; }

	tPixelFormat GetPixelFormatSrc() const override																		{ return IsValid() ? PixelFormatSrc : tPixelFormat::Invalid; }
	tPixelFormat GetPixelFormat() const override																		{ return IsValid() ? tPixelFormat::R8G8B8A8 : tPixelFormat::Invalid; }

private:
	tPixelFormat PixelFormatSrc	= tPixelFormat::Invalid;
	int Width					= 0;
	int Height					= 0;
	tPixel4* Pixels				= nullptr;
};


// Implementation below this line.


inline void tImageXPM::Clear()
{
	PixelFormatSrc	= tPixelFormat::Invalid;
	Width			= 0;
	Height			= 0;
	delete[] Pixels;
	Pixels = nullptr;
}


}
