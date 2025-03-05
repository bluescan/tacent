// tImageTGA.h
//
// This class knows how to load and save targa (.tga) files into tPixel arrays. These tPixels may be 'stolen' by the
// tPicture's constructor if a targa file is specified. After the array is stolen the tImageTGA is invalid. This is
// purely for performance.
//
// Copyright (c) 2006, 2017, 2019, 2020, 2022-2024 Tristan Grimmer.
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


class tImageTGA : public tBaseImage
{
public:
	enum LoadFlags
	{
		LoadFlag_None			= 0,

		// The most common way to interpret the alpha channel is as opacity (0.0 is fully transarent and 1.0 is fully
		// opaque). However there are some 16-bit TGAs (5551 with 1-bit alpha) in the wild that are saved with a 0 in
		// the alpha channel and are expected to be visible. The TGA specification is a bit vague on this point:
		// "If the pixel depth is 16 bits, the topmost bit is reserved for transparency." This statement was probably
		// intended to mean the topmpst bit was for the 'attribute'/alpha channel and should be interpreted as opacity.
		// In any case, these files exist, so the LoadFlag_AlphaOpacity flag is available to be disabled if necessary.
		// LoadFlag_AlphaOpacity Present    : Interpret alpha normally (as opacity).      0 = transparent. 1 = opaque.
		// LoadFlag_AlphaOpacity Not Present: Interpret alpha reversed (as transparency). 0 = opaque. 1 = transparent.
		LoadFlag_AlphaOpacity	= 1 << 0,
		LoadFlags_Default		= LoadFlag_AlphaOpacity
	};

	struct LoadParams
	{
		LoadParams()																									{ Reset(); }
		LoadParams(const LoadParams& src)																				: Flags(src.Flags) { }
		void Reset()																									{ Flags = LoadFlags_Default; }
		LoadParams& operator=(const LoadParams& src)																	{ Flags = src.Flags; return *this; }
		uint32 Flags;
	};

	// Creates an invalid tImageTGA. You must call Load or Set manually.
	tImageTGA()																											{ }
	tImageTGA(const tString& tgaFile, const LoadParams& params = LoadParams())											{ Load(tgaFile, params); }

	// The data is copied out of tgaFileInMemory. Go ahead and delete after if you want.
	tImageTGA(const uint8* tgaFileInMemory, int numBytes, const LoadParams& params = LoadParams())						{ Load(tgaFileInMemory, numBytes, params); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageTGA(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageTGA(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImageTGA(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImageTGA()																								{ Clear(); }

	// Clears the current tImageTGA before loading. 16, 24, or 32 bit targas can be loaded. The tga may be uncompressed
	// or RLE compressed. Other compression methods are rare and unsupported. Returns success. If false returned,
	// object is invalid.
	bool Load(const tString& tgaFile, const LoadParams& params = LoadParams());
	bool Load(const uint8* tgaFileInMemory, int numBytes, const LoadParams& params = LoadParams());

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4b* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	enum class tFormat
	{
		Invalid,	// Invalid must be 0.
		BPP24,		// 24 bit colour.
		BPP32,		// 24 bit colour with 8 bits opacity in the alpha channel.
		Auto		// Save function will decide format. BPP24 if all image pixels are opaque and BPP32 otherwise.
	};

	enum class tCompression
	{
		None,		// No compression.
		RLE			// Run Length Encoding.
	};

	struct SaveParams
	{
		SaveParams()																									{ Reset(); }
		SaveParams(const SaveParams& src)																				: Format(src.Format), Compression(src.Compression) { }
		void Reset()																									{ Format = tFormat::Auto; Compression = tCompression::None; }
		SaveParams& operator=(const SaveParams& src)																	{ Format = src.Format; Compression = src.Compression; return *this; }

		tFormat Format;
		tCompression Compression;
	};

	// Saves the tImageTGA to the Targa file specified. The type of filename must be "tga". If tFormat is Auto, this
	// function will decide the format. BPP24 if all image pixels are opaque and BPP32 otherwise. Returns the format
	// that the file was saved in, or tFormat::Invalid if there was a problem. Since Invalid is 0, you can use an 'if'.
	tFormat Save(const tString& tgaFile, tFormat, tCompression = tCompression::RLE) const;
	tFormat Save(const tString& tgaFile, const SaveParams& = SaveParams()) const;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return Pixels ? true : false; }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }

	// All pixels must be opaque (alpha = 1) for this to return true.
	bool IsOpaque() const;

	// After this call you are the owner of the pixels and must eventually delete[] them. This tImageTGA object is
	// invalid afterwards.
	tPixel4b* StealPixels();
	tFrame* GetFrame(bool steal = true) override;
	tPixel4b* GetPixels() const																							{ return Pixels; }

private:
	bool SaveUncompressed(const tString& tgaFile, tFormat) const;
	bool SaveCompressed(const tString& tgaFile, tFormat) const;
	void ReadColourBytes(tColour4b& dest, const uint8* src, int bitDepth, bool alphaOpacity);

	int Width					= 0;
	int Height					= 0;
	tPixel4b* Pixels			= nullptr;
};


// Implementation below this line.


inline void tImageTGA::Clear()
{
	Width = 0;
	Height = 0;
	delete[] Pixels;
	Pixels = nullptr;

	tBaseImage::Clear();
}


}



