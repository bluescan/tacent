// tImageXPM.h
//
// This class knows how to load and save an X-Windows Pix Map (.xpm) file. It knows the details of the xpm file format
// and loads the data into a tPixel array. These tPixels may be 'stolen' by the tPicture's constructor if a xpm file is
// specified. After the array is stolen the tImageXPM is invalid. This is purely for performance.
//
// Copyright (c) 2020, 2022-2024, 2026 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Foundation/tArray.h>
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

	// Creates from an XPM image on the filesystem.
	tImageXPM(const tString& xpmFile)																					{ Load(xpmFile); }

	// The supplied XPM data is parsed and decoded into pixel data.
	tImageXPM(const tArray<uint8>& xpmFileInMemory)																		{ Load(xpmFileInMemory); }

	// Moves (takes ownership of) the array. It is consumed (cleared) only if the load succeeds; on failure the array is
	// left intact so the caller can still use it.
	tImageXPM(tArray<uint8>&& xpmFileInMemory)																			{ Load(tStd::rVal(xpmFileInMemory)); }

	// The supplied XPM data is parsed and decoded into pixel data.
	tImageXPM(const uint8* xpmFileInMemory, int numBytes)																{ Load(xpmFileInMemory, numBytes); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageXPM(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageXPM(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImageXPM(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImageXPM()																								{ Clear(); }

	// Clears the current tImageXPM before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& xpmFile);

	// Reads the array (the source is left unchanged).
	bool Load(const tArray<uint8>& xpmFileInMemory);

	// Consumes the array only on success. On failure the array is left intact for the caller to retry or inspect.
	bool Load(tArray<uint8>&& xpmFileInMemory);

	// The supplied XPM data is parsed and decoded into pixel data.
	bool Load(const uint8* xpmFileInMemory, int numBytes);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4b*, int width, int height, bool steal = false) override;

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
	tPixel4b* StealPixels();
	tFrame* GetFrame(bool steal = true) override;
	tPixel4b* GetPixels() const																							{ return Pixels; }

private:
	// Decodes the in-memory XPM bytes. Read-only: never mutates the caller's buffer.
	bool Decode(const uint8* xpmFileInMemory, int numBytes);

	int Width					= 0;
	int Height					= 0;
	tPixel4b* Pixels			= nullptr;
};


// Implementation below this line.


inline void tImageXPM::Clear()
{
	Width			= 0;
	Height			= 0;

	delete[] Pixels;
	Pixels = nullptr;

	tBaseImage::Clear();
}


}

