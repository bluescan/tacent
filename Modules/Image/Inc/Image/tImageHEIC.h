// tImageHEIC.h
//
// This class knows how to load HEIC files using libheif.
//
// Copyright (c) 2026 Tristan Grimmer.
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


class tImageHEIC : public tBaseImage
{
public:
	// Creates an invalid tImageHEIC. You must call Load manually.
	tImageHEIC()																										{ }
	tImageHEIC(const tString& heicFile)																					{ Load(heicFile); }

	// The data is copied out of heicFileInMemory. Go ahead and delete[] after if you want.
	tImageHEIC(const uint8* heicFileInMemory, int numBytes)																{ Load(heicFileInMemory, numBytes); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out. Sets the colour space to sRGB. Call SetColourSpace after if you wanted linear.
	tImageHEIC(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageHEIC(tFrame* frame, bool steal = true)																		{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImageHEIC(tPicture& picture, bool steal = true)																	{ Set(picture, steal); }

	virtual ~tImageHEIC()																								{ Clear(); }

	bool Load(const tString& heicFile);
	bool Load(const uint8* heicFileInMemory, int numBytes);

	bool Set(tPixel4b* pixels, int width, int height, bool steal = false) override;
	bool Set(tFrame*, bool steal = true) override;
	bool Set(tPicture& picture, bool steal = true) override;

	void Clear() override;
	bool IsValid() const override																						{ return Pixels ? true : false; }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }
	bool IsOpaque() const;

	tPixel4b* StealPixels();
	tFrame* GetFrame(bool steal = true) override;
	tPixel4b* GetPixels() const																							{ return Pixels; }

private:
	int Width			= 0;
	int Height			= 0;
	tPixel4b* Pixels	= nullptr;
};


inline void tImageHEIC::Clear()
{
	Width = 0;
	Height = 0;
	delete[] Pixels;
	Pixels = nullptr;
	tBaseImage::Clear();
}


}
