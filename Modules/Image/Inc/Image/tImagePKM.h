// tImagePKM.h
//
// This class knows how to load and save Ericsson's ETC1/ETC2/EAC PKM (.pkm) files. The pixel data is stored in a
// tLayer. If decode was requested the layer will store raw pixel data. The layer may be 'stolen'. IF it is the
// tImagePKM is invalid afterwards. This is purely for performance.
//
// Copyright (c) 2023, 2024 Tristan Grimmer.
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


class tImagePKM : public tBaseImage
{
public:
	enum LoadFlag
	{
		LoadFlag_Decode				= 1 << 0,	// Decode the pkm texture data into RGBA 32 bit. If not set, the pixel data will remain unmodified.

		// The remaining flags only apply when decode flag set. ReverseRowOrder is guaranteed to work if decoding, and
		// guaranteed to not work if not decoding.
		LoadFlag_ReverseRowOrder	= 1 << 1,	// OpenGL uses the lower left as the orig DirectX uses the upper left. Set flag for OpenGL.
		LoadFlag_GammaCompression	= 1 << 2,	// Gamma-correct. Gamma compression using an encoding gamma of 1/2.2.
		LoadFlag_SRGBCompression	= 1 << 3,	// Same as above but uses the official sRGB transformation. Linear -> sRGB. Approx encoding gamma of 1/2.4 for part of curve.
		LoadFlag_AutoGamma			= 1 << 4,	// Tries to determine whether to apply sRGB compression based on pixel format. Call GetColourSpace to see if it applied.
		LoadFlag_SpreadLuminance	= 1 << 5,	// For PKM files with a single Red, spread it to all the RGB channels (otherwise red only). Applies only if decoding a pkm that is an R-only format.
		LoadFlags_Default			= LoadFlag_Decode | LoadFlag_ReverseRowOrder
	};

	struct LoadParams
	{
		LoadParams()																									{ Reset(); }
		LoadParams(const LoadParams& src)																				: Flags(src.Flags), Gamma(src.Gamma) { }

		void Reset()																									{ Flags = LoadFlags_Default; Gamma = tMath::DefaultGamma; }
		LoadParams& operator=(const LoadParams& src)																	{ Flags = src.Flags; Gamma = src.Gamma; return *this; }

		uint32 Flags;
		float Gamma;				// Used iff decoding.
	};

	// Creates an invalid tImagePMK. You must call Load manually.
	tImagePKM()																											{ }
	tImagePKM(const tString& pkmFile, const LoadParams& params = LoadParams())											{ Load(pkmFile, params); }

	// The data is copied out of pkmFileInMemory. Go ahead and delete after if you want.
	tImagePKM(const uint8* pkmFileInMemory, int numBytes, const LoadParams& params = LoadParams())						{ Load(pkmFileInMemory, numBytes, params); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImagePKM(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImagePKM(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImagePKM(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImagePKM()																								{ Clear(); }

	// Clears the current tImagePKM before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& pkmFile, const LoadParams& params = LoadParams());
	bool Load(const uint8* pkmFileInMemory, int numBytes, const LoadParams& params = LoadParams());

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4b* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return (Layer && Layer->IsValid()); }

	int GetWidth() const																								{ return Layer ? Layer->Width  : 0; }
	int GetHeight() const																								{ return Layer ? Layer->Height : 0; }

	// If decoded all pixels must be opaque (alpha = 255) for this to return true.
	// If not decoded it returns false if the pixel format supports transparency.
	bool IsOpaque() const;

	// After the steal call you are the owner of the layer and must eventually delete it. This tImageASTC object is
	// invalid afterwards.
	tLayer* StealLayer()																								{ tLayer* layer = Layer; Layer = nullptr; return layer; }
	tLayer* GetLayer() const																							{ return Layer; }
	tFrame* GetFrame(bool steal = true) override;

private:
	// We store the data in a tLayer because that's the container we use for pixel data than may be in any format.
	// The user of tImagePKM is not required to decode, so we can't just use a tPixel array.
	tLayer* Layer							= nullptr;
};


// Implementation below this line.


inline void tImagePKM::Clear()
{
	delete									Layer;
	Layer									= nullptr;
	tBaseImage::Clear();
}


}
