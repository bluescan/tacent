// tImageKTX.h
//
// This knows how to load/save KTX and KTX2 files. It knows the details of the ktx and ktx2 file format and loads the
// data into multiple layers.
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
#include <Image/tLayer.h>
namespace tImage
{


class tImageKTX
{
public:
	// Creates an invalid tImageKTX. You must call Load manually.
	tImageKTX()																											{ }
	tImageKTX(const tString& ktxFile)																					{ Load(ktxFile); }

	// ktxFileInMemory can be deleted after this runs.
	tImageKTX(uint8* ktxFileInMemory, int numBytes)																		{ Set(ktxFileInMemory, numBytes); }

	virtual ~tImageKTX()																								{ Clear(); }

	// Clears the current tImageKTX before loading. If false returned object is invalid.
	bool Load(const tString& ktxFile);
	bool Set(uint8* ktxFileInMemory, int numBytes);

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear();
	bool IsValid() const																								
	{ return false; }

	tPixelFormat SrcPixelFormat = tPixelFormat::Invalid;

private:
	tPixelFormat GetPixelFormatFromGLFormat(uint32 glFormat);
	tPixelFormat GetPixelFormatFromVKFormat(uint32 vkFormat);
};


// Implementation only below.


inline void tImageKTX::Clear()
{
	// @todo Delete data.
	SrcPixelFormat = tPixelFormat::Invalid;
}


}
