// tImageBMP.h
//
// This class knows how to load and save windows bitmap (.bmp) files into tPixel arrays. These tPixels may be 'stolen' by the
// tPicture's constructor if a targa file is specified. After the array is stolen the tImageBMP is invalid. This is
// purely for performance.
//
// The code in this module is a modification of code from https://github.com/phm97/bmp under the BSD 2-Clause License:
//
// Copyright (c) 2019, phm97
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
// following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
//    disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//    following disclaimer in the documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The modifications to use Tacent datatypes and conversion to C++ are under the ISC licence:
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


class tImageBMP : public tBaseImage
{
public:
	// Creates an invalid tImageBMP. You must call Load manually.
	tImageBMP()																											{ }
	tImageBMP(const tString& bmpFile)																					{ Load(bmpFile); }

	// This one sets from a supplied pixel array. It just reads the data (or steals the array if steal set).
	tImageBMP(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageBMP(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImageBMP(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImageBMP()																								{ Clear(); }

	// Clears the current tImageBMP before loading. Supports RGBA, RGB, R5G5B5A1, 8-bit indexed, 4-bit indexed, 1-bit
	// indexed, and run-length encoded RLE4 and RLE8. Returns success. If false returned, object is invalid.
	bool Load(const tString& bmpFile);

	// @todo No current in-memory loader.
	// bool Load(const uint8* bmpFileInMemory, int numBytes);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4b* pixels, int width, int height, bool steal = false) override;

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
		SaveParams(const SaveParams& src)																				: Format(src.Format) { }
		void Reset()																									{ Format = tFormat::Auto; }
		SaveParams& operator=(const SaveParams& src)																	{ Format = src.Format; return *this; }
		tFormat Format;
	};

	// Saves the tImageBMP to the bmp file specified. The filetype must be "bmp". If tFormat is Auto, this function
	// will decide the format. BPP24 if all image pixels are opaque and BPP32 otherwise. Returns the format that the
	// file was saved in, or tFormat::Invalid if there was a problem. Since Invalid is 0, you can use an 'if'.
	tFormat Save(const tString& bmpFile, tFormat) const;
	tFormat Save(const tString& bmpFile, const SaveParams& = SaveParams()) const;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return Pixels ? true : false; }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }

	// All pixels must be opaque (alpha = 1) for this to return true.
	bool IsOpaque() const;

	// After this call you are the owner of the pixels and must eventually delete[] them. This tImageBMP object is
	// invalid afterwards.
	tPixel4b* StealPixels();
	tFrame* GetFrame(bool steal = true) override;
	tPixel4b* GetPixels() const																							{ return Pixels; }

private:
	const uint16 FourCC				= 0x4D42;

	#pragma pack(push, r1, 1)
	struct Header
	{
		uint16 FourCC;
		int32 Size;
		int32 AppId;
		int32 Offset;
	};

	struct InfoHeader
	{
		int32 HeaderSize;
		int32 Width;
		int32 Height;
		int16 NumPlanes;
		int16 BPP;
		int32 Compression;
		int32 ImageSize;
		int32 HorizontalResolution;
		int32 VerticalResolution;
		int32 ColoursUsed;
		int32 ColoursImportant;
	};

	struct PaletteColour
	{
		uint8 B;
		uint8 G;
		uint8 R;
		uint8 A;
	};
	#pragma pack(pop, r1)

	void ReadRow_Pixels32	(tFileHandle, uint8* dest);
	void ReadRow_Pixels24	(tFileHandle, uint8* dest);
	void ReadRow_Pixels16	(tFileHandle, uint8* dest);
	void ReadRow_Indexed8	(tFileHandle, uint8* dest, PaletteColour* palette);
	void ReadRow_Indexed4	(tFileHandle, uint8* dest, PaletteColour* palette);
	void ReadRow_Indexed1	(tFileHandle, uint8* dest, PaletteColour* palette);
	void ReadRow_IndexedRLE8(tFileHandle, uint8* dest, PaletteColour* palette);
	void ReadRow_IndexedRLE4(tFileHandle, uint8* dest, PaletteColour* palette);

	// So this is a neat C++11 feature. Allows simplified constructors.
	int Width = 0;
	int Height = 0;
	tPixel4b* Pixels = nullptr;
};


// Implementation below this line.


inline void tImageBMP::Clear()
{
	tBaseImage::Clear();
	Width = 0;
	Height = 0;
	delete[] Pixels;
	Pixels = nullptr;
}


}
