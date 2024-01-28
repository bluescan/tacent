// tImagePNG.h
//
// This class knows how to load and save PNG files. It does zero processing of image data. It knows the details of the
// png file format and loads the data into a tPixel array. These tPixels may be 'stolen' by the tPicture's constructor
// if a png file is specified. After the array is stolen the tImagePNG is invalid. This is purely for performance.
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


class tImagePNG : public tBaseImage
{
public:
	enum LoadFlags
	{
		// Gamma-correct. Gamma compression using an encoding gamma of 1/2.2. Assumes (colour) data is linear and puts
		// it in gamma-space (brighter) for diaplay on a monitor. Png files at 16 bpc are in linear space. Png files at
		// 8 bpc abd sRGB.
		LoadFlag_GammaCompression	= 1 << 0,
		LoadFlag_SRGBCompression	= 1 << 1,	// Same as above but uses the official sRGB transformation. Linear -> sRGB. Approx encoding gamma of 1/2.4 for part of curve.
		LoadFlag_AutoGamma			= 1 << 2,	// Applies sRGB compression for 16 bpc png images. Call GetColourSpace to see final colour profile.

		// If a png is 16 bpc you can force it to load into an 8 bpc buffer with this flag.
		LoadFlag_ForceToBpc8		= 1 << 3,
		LoadFlag_ReverseRowOrder	= 1 << 4,	// OpenGL uses the lower left as the orig DirectX uses the upper left. Set flag for OpenGL.

		// Crazily some PNG files are actually JPG/JFIF files inside. I don't much like supporting this, but some
		// software (ms-paint for example), will happily load such an invalid png. The world would be better if app
		// developers wouldn't save things with the wrong extension, but they get away with it because other software
		// loads this junk... and now this library is yet another.
		LoadFlag_AllowJPG			= 1 << 5,
		LoadFlags_Default			= LoadFlag_AutoGamma | LoadFlag_ForceToBpc8 | LoadFlag_AllowJPG | LoadFlag_ReverseRowOrder
	};

	struct LoadParams
	{
		LoadParams()																									{ Reset(); }
		LoadParams(const LoadParams& src)																				: Flags(src.Flags), Gamma(src.Gamma) { }
		void Reset()																									{ Flags = LoadFlags_Default; Gamma = tMath::DefaultGamma; }
		LoadParams& operator=(const LoadParams& src)																	{ Flags = src.Flags; Gamma = src.Gamma; return *this; }
		uint32 Flags;
		float Gamma;
	};

	// Creates an invalid tImagePNG. You must call Load manually.
	tImagePNG()																											{ }
	tImagePNG(const tString& pngFile, const LoadParams& params = LoadParams())											{ Load(pngFile, params); }

	// The data is copied out of pngFileInMemory. Go ahead and delete after if you want.
	tImagePNG(const uint8* pngFileInMemory, int numBytes, const LoadParams& params = LoadParams())						{ Load(pngFileInMemory, numBytes, params); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImagePNG(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Same as above except using a 16-bit-per-component tPixel4s array.
	tImagePNG(tPixel4s* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImagePNG(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImagePNG(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImagePNG()																								{ Clear(); }

	// Clears the current tImagePNG before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& pngFile, const LoadParams& params = LoadParams());
	bool Load(const uint8* pngFileInMemory, int numBytes, const LoadParams& params = LoadParams());

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4b*, int width, int height, bool steal = false) override;

	// Set from a 16-bpc buffer.
	bool Set(tPixel4s*, int width, int height, bool steal = false);

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	enum class tFormat
	{
		Invalid,			// Invalid must be 0.
		BPP24_RGB_BPC8,		// 24-bit RGB.  3 8-bit  components.
		BPP32_RGBA_BPC8,	// 32-bit RGBA. 4 8-bit  components.
		BPP48_RGB_BPC16,	// 48-bit RGB.  3 16-bit components.
		BPP64_RGBA_BPC16,	// 64-bit RGBA. 4 16-bit components.
		Auto				// Save function will decide format. RGB_24BIT_8BPC if all image pixels are opaque and RGBA_32BIT_8BPC otherwise.
	};

	struct SaveParams
	{
		SaveParams()																									{ Reset(); }
		SaveParams(const SaveParams& src)																				: Format(src.Format) { }
		void Reset()																									{ Format = tFormat::Auto; }
		SaveParams& operator=(const SaveParams& src)																	{ Format = src.Format; return *this; }
		tFormat Format;
	};

	// Saves the tImagePNG to the PNG file specified. The type of filename must be PNG. If tFormat is Auto, this
	// function will decide the format. If the internal buffer is 8-bpc it will choose between BPP24 and BPP32 depending
	// on opacity (BPP24 is all pixels are opaque). In the internal buffer is 16-bpc it chooses between BPP48 and BPP64.
	// When tFormat is explicitly one of the BPPNN choices, it may need to convert the data. For example, if the
	// internal buffer is 8-bpc and you choose to save BPP48 or BPP64 (both 16-bpc formats), a conversion must take
	// place. Returns the format that the file was saved in, or tFormat::Invalid if there was a problem. Since Invalid
	// is 0, you can use an 'if' to check success.
	tFormat Save(const tString& pngFile, tFormat) const;
	tFormat Save(const tString& pngFile, const SaveParams& = SaveParams()) const;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return (Pixels8 || Pixels16) ? true : false; }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }
	bool IsOpaque() const;

	// After this call you are the owner of the pixels and must eventually delete[] them. This call only returns the
	// stolen pixel array if it was present. If it was, the tImagePNG object will be invalid afterwards.
	tPixel4b* StealPixels8();
	tPixel4s* StealPixels16();

	tFrame* GetFrame(bool steal = true) override;
	tPixel4b* GetPixels8() const																						{ return Pixels8; }
	tPixel4s* GetPixels16() const																						{ return Pixels16; }

	tPixelFormat GetPixelFormatSrc() const override																		{ return IsValid() ? PixelFormatSrc : tPixelFormat::Invalid; }
	tPixelFormat GetPixelFormat() const override																		{ return IsValid() ? PixelFormat : tPixelFormat::Invalid; }

	// Returns the colour profile of the source file that was loaded. This may not match the current if, say, gamma
	// correction was requested on load.
	tColourProfile GetColourProfileSrc() const override																	{ return ColourProfileSrc; }

	// Returns the current colour profile.
	tColourProfile GetColourProfile() const override																	{ return ColourProfile; }

private:
	tPixelFormat PixelFormatSrc		= tPixelFormat::Invalid;
	tPixelFormat PixelFormat		= tPixelFormat::Invalid;

	// These are _not_ part of the pixel format in tacent.
	tColourProfile ColourProfileSrc	= tColourProfile::Unspecified;
	tColourProfile ColourProfile	= tColourProfile::Unspecified;

	int Width						= 0;
	int Height						= 0;

	// Only one of these may be valid at a time depending on if the pixels are 8 or 16 bpc.
	tPixel4b* Pixels8				= nullptr;
	tPixel4s* Pixels16				= nullptr;
};


// Implementation below this line.


inline void tImagePNG::Clear()
{
	PixelFormatSrc					= tPixelFormat::Invalid;
	PixelFormat						= tPixelFormat::Invalid;
	ColourProfileSrc				= tColourProfile::Unspecified;
	ColourProfile					= tColourProfile::Unspecified;

	Width = 0;
	Height = 0;
	delete[] Pixels8;
	Pixels8 = nullptr;
	delete[] Pixels16;
	Pixels16 = nullptr;
}


}
