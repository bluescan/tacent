// tImageASTC.h
//
// This class knows how to load and save ARM's Adaptive Scalable Texture Compression (.astc) files. The pixel data is
// stored in a tLayer. If decode was requested the layer will store raw pixel data. The layer may be 'stolen'. IF it
// is the tImageASTC is invalid afterwards. This is purely for performance.
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
#include <Image/tLayer.h>
#include <Image/tBaseImage.h>
namespace tImage
{


class tImageASTC : public tBaseImage
{
public:
	enum LoadFlag
	{
		LoadFlag_Decode				= 1 << 0,	// Decode the astc texture data into RGBA 32 bit. If not set, the pixel data will remain unmodified.

		// The remaining flags only apply when decode flag set. ReverseRowOrder is guaranteed to work if decoding, and
		// guaranteed to not work if not decoding.
		LoadFlag_ReverseRowOrder	= 1 << 1,	// OpenGL uses the lower left as the orig DirectX uses the upper left. Set flag for OpenGL.
		LoadFlag_GammaCompression	= 1 << 2,	// Gamma-correct. Gamma compression using an encoding gamma of 1/2.2.
		LoadFlag_SRGBCompression	= 1 << 3,	// Same as above but uses the official sRGB transformation. Linear -> sRGB. Approx encoding gamma of 1/2.4 for part of curve.
		LoadFlag_AutoGamma			= 1 << 4,	// Determines whether to apply sRGB compression based on colour profile. Call GetColourProfile to see if it applied.
		LoadFlag_ToneMapExposure	= 1 << 5,	// Apply exposure value when loading the astc.
		LoadFlags_Default			= LoadFlag_Decode | LoadFlag_ReverseRowOrder | LoadFlag_AutoGamma
	};

	struct LoadParams
	{
		LoadParams()																									{ Reset(); }
		LoadParams(const LoadParams& src)																				: Flags(src.Flags), Profile(src.Profile), Gamma(src.Gamma), Exposure(src.Exposure) { }

		// We chose HDR as the default profile because it can load LDR blocks. The other way around doesn't work with
		// with the tests images -- the LDR profile doesn't appear capable of loading HDR blocks (they become magenta).
		void Reset()																									{ Flags = LoadFlags_Default; Profile = tColourProfile::HDRlRGB_LDRlA; Gamma = tMath::DefaultGamma; Exposure = 1.0f; }
		LoadParams& operator=(const LoadParams& src)																	{ Flags = src.Flags; Profile = src.Profile; Gamma = src.Gamma; Exposure = src.Exposure; return *this; }

		uint32 Flags;
		tColourProfile Profile;		// Used iff decoding.
		float Gamma;				// Used iff decoding.
		float Exposure;				// Used iff decoding.
	};

	// Creates an invalid tImageASTC. You must call Load manually.
	tImageASTC()																										{ }
	tImageASTC(const tString& astcFile, const LoadParams& params = LoadParams())										{ Load(astcFile, params); }

	// The data is copied out of astcFileInMemory. Go ahead and delete[] after if you want.
	tImageASTC(const uint8* astcFileInMemory, int numBytes, const LoadParams& params = LoadParams())					{ Load(astcFileInMemory, numBytes, params); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageASTC(tPixel* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageASTC(tFrame* frame, bool steal = true)																		{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImageASTC(tPicture& picture, bool steal = true)																	{ Set(picture, steal); }

	virtual ~tImageASTC()																								{ Clear(); }

	// Clears the current tImageASTC before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& astcFile, const LoadParams& params = LoadParams());
	bool Load(const uint8* astcFileInMemory, int numBytes, const LoadParams& params = LoadParams());

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame. After this is called the layer data will be in R8G8B8A8.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return (Layer && Layer->IsValid()); }

	int GetWidth() const																								{ return Layer ? Layer->Width  : 0; }
	int GetHeight() const																								{ return Layer ? Layer->Height : 0; }

	// All pixels must be opaque (alpha = 255) for this to return true. Always returns false if the object is not in the
	// R8G8B8A8 pixel-format (i.e. not decoded) since all ASTC pixel formats support alpha.
	bool IsOpaque() const;

	// Will return R8G8B8A8 if you chose to decode the layers. Otherwise it will be whatever format the astc data is in.
	tPixelFormat GetPixelFormat() const																					{ return PixelFormat; }

	// Will return the format the astc data was originally in, even if you chose to decode.
	tPixelFormat GetPixelFormatSrc() const																				{ return PixelFormatSrc; }

	// After the steal call you are the owner of the layer and must eventually delete it. This tImageASTC object is
	// invalid afterwards.
	tLayer* StealLayer()																								{ tLayer* layer = Layer; Layer = nullptr; return layer; }
	tLayer* GetLayer() const																							{ return Layer; }
	tFrame* GetFrame(bool steal = true) override;

private:
	tPixelFormat PixelFormat	= tPixelFormat::Invalid;
	tPixelFormat PixelFormatSrc	= tPixelFormat::Invalid;

	// We store the data in a tLayer because that's the container we use for pixel data than may be in any format.
	// The user of tImageASTC is not required to decode, so we can't just use a tPixel array.
	tLayer* Layer				= nullptr;
};


// Implementation below this line.


inline void tImageASTC::Clear()
{
	PixelFormat					= tPixelFormat::Invalid;
	PixelFormatSrc				= tPixelFormat::Invalid;
	delete						Layer;
	Layer						= nullptr;
}


}
