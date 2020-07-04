// tImageJPG.h
//
// This clas knows how to load and save a JPeg (.jpg and .jpeg) file. It does zero processing of image data. It knows
// the details of the jpg file format and loads the data into a tPixel array. These tPixels may be 'stolen' by the
// tPicture's constructor if a jpg file is specified. After the array is stolen the tImageJPG is invalid. This is
// purely for performance.
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


class tImageJPG
{
public:
	// Creates an invalid tImageJPG. You must call Load manually.
	tImageJPG()																											{ }
	tImageJPG(const tString& jpgFile)																					{ Load(jpgFile); }

	// The data is copied out of jpgFileInMemory. Go ahead and delete after if you want.
	tImageJPG(const uint8* jpgFileInMemory, int numBytes)																{ Set(jpgFileInMemory, numBytes); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageJPG(tPixel* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	virtual ~tImageJPG()																								{ Clear(); }

	// Clears the current tImageJPG before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& jpgFile);
	bool Set(const uint8* jpgFileInMemory, int numBytes);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel*, int width, int height, bool steal = false);

	// Saves the tImageJPG to the JPeg file specified. The extension of filename must be ".jpg" or ".jpeg".
	// The quality int is should be a percent in [1,100]. Returns true on success.
	const static int DefaultQuality = 95;
	bool Save(const tString& jpgFile, int quality = DefaultQuality) const;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear();
	bool IsValid() const																								{ return Pixels ? true : false; }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }

	// IsOpaque always return true for a JPeg.
	bool IsOpaque() const																								{ return true; }

	// After this call you are the owner of the pixels and must eventually delete[] them. This tImageJPG object is
	// invalid afterwards.
	tPixel* StealPixels();
	tPixel* GetPixels() const																							{ return Pixels; }
	tPixelFormat SrcPixelFormat = tPixelFormat::Invalid;

private:
	int Width = 0;
	int Height = 0;
	tPixel* Pixels = nullptr;
};


// Implementation below this line.


inline void tImageJPG::Clear()
{
	Width = 0;
	Height = 0;
	delete[] Pixels;
	Pixels = nullptr;
	SrcPixelFormat = tPixelFormat::Invalid;
}


}
