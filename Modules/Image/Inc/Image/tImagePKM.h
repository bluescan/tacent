// tImagePKM.h
//
// This class knows how to load and save pkm (.pkm) files into tPixel arrays. These tPixels may be 'stolen' by the
// tPicture's constructor if a pkm file is specified. After the array is stolen the tImagePKM is invalid. This is
// purely for performance.
//
// Copyright (c) 2023 Tristan Grimmer.
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
class tPicture;


class tImagePKM : public tBaseImage
{
public:
	// Creates an invalid tImagePMK. You must call Load manually.
	tImagePKM()																											{ }
	tImagePKM(const tString& pkmFile)																					{ Load(pkmFile); }

	// The data is copied out of pkmFileInMemory. Go ahead and delete after if you want.
	tImagePKM(const uint8* pkmFileInMemory, int numBytes)																{ Load(pkmFileInMemory, numBytes); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImagePKM(tPixel* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImagePKM(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImagePKM(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImagePKM()																								{ Clear(); }

	// Clears the current tImagePKM before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& pkmFile);
	bool Load(const uint8* pkmFileInMemory, int numBytes);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	// Saves the tImagePKM to the pkm file specified. The type of filename must be pkm. Returns false if problem.
	bool Save(const tString& pkmFile) const;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return Pixels ? true : false; }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }

	// PKM files are always opaque. No alpha support.
	bool IsOpaque() const;

	// After this call you are the owner of the pixels and must eventually delete[] them. This tImagePKM object is
	// invalid afterwards.
	tPixel* StealPixels();
	tFrame* GetFrame(bool steal = true) override;

	tPixel* GetPixels() const																							{ return Pixels; }
	tPixelFormat PixelFormatSrc = tPixelFormat::Invalid;

private:
	// So this is a neat C++11 feature. Allows simplified constructors.
	int Width = 0;
	int Height = 0;
	tPixel* Pixels = nullptr;
};


// Implementation below this line.


inline void tImagePKM::Clear()
{
	Width = 0;
	Height = 0;
	delete[] Pixels;
	Pixels = nullptr;
	PixelFormatSrc = tPixelFormat::Invalid;
}


}