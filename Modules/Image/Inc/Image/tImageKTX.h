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
	enum LoadFlag
	{
		LoadFlag_Decode				= 1 << 0,	// Decode the dds texture data into RGBA 32 bit layers. If not set, the layer data will remain unmodified.
		LoadFlag_ReverseRowOrder	= 1 << 1,	// OpenGL uses the lower left as the orig DirectX uses the upper left. Set flag for OpenGL.

		// Gamma-correct. Gamma compression using a encoding gamma of 1/2.2. Flag only applies when decode flag set for
		// HDR / floating-point formats (BC6, rgb16f/32f, etc) images. Assumes (colour) data is linear and puts it in
		// gamma-space (brighter) for diaplay on a monitor.
		LoadFlag_GammaCompression	= 1 << 2,
		LoadFlag_SRGBCompression	= 1 << 3,	// Same as above but uses the official sRGB transformation. Linear -> sRGB. Approx encoding gamma of 1/2.4 for part of curve.
		LoadFlag_ToneMapExposure	= 1 << 4,	// Apply exposure value when loading the dds. Only affects HDR (linear-colour) formats.
		LoadFlag_SpreadLuminance	= 1 << 5,	// For DDS files with a single Red or Luminance component, spread it to all the RGB channels (otherwise red only). Does not spread single-channel Alpha formats. Applies only if decoding a dds is an R-only or L-only format.
		LoadFlag_CondMultFourDim	= 1 << 6,	// Produce conditional success if image dimension not a multiple of 4. Only checks BC formats,
		LoadFlag_CondPowerTwoDim	= 1 << 7,	// Produce conditional success if image dimension not a power of 2. Only checks BC formats.
		LoadFlags_Default			= LoadFlag_Decode | LoadFlag_ReverseRowOrder | LoadFlag_SpreadLuminance | LoadFlag_SRGBCompression
	};

	// See comments in tImageDDS.h for reference.
	struct LoadParams
	{
		LoadParams()																									{ Reset(); }
		void Reset()																									{ Flags = LoadFlags_Default; Gamma = tMath::DefaultGamma; Exposure = 1.0f; }

		uint32 Flags;
		float Gamma;
		float Exposure;
	};

	// Creates an invalid tImageKTX. You must call Load manually.
	tImageKTX()																											{ }

	tImageKTX(const tString& ktxFile, const LoadParams& = LoadParams())													{ Load(ktxFile); }

	// ktxFileInMemory can be deleted after this runs.
	tImageKTX(uint8* ktxFileInMemory, int numBytes)																		{ Load(ktxFileInMemory, numBytes); }

	virtual ~tImageKTX()																								{ Clear(); }

	enum class ResultCode
	{
		// Full success. All load flags applied successfully.
		Success,

		// Conditional success. Object is valid, but not all load flags applied.
		Conditional_CouldNotFlipRows,
		Conditional_DimNotMultFourBC,

		// Fatal. Load was uncuccessful and object is invalid.
		Fatal_DefaultInitialized,
		Fatal_FileDoesNotExist,
		Fatal_IncorrectFileType,
		NumCodes,

		// Since we store result codes as bits in a 32-bit uint, we need to make sure we don't have too many codes.
		MaxCodes							= 32,
		FirstValid							= Success,
		LastValid							= Conditional_DimNotMultFourBC,
		FirstFatal							= Fatal_DefaultInitialized,
		LastFatal							= Fatal_IncorrectFileType
	};

	// Clears the current tImageKTX before loading. If false returned object is invalid.
	bool Load(const tString& ktxFile);
	bool Load(uint8* ktxFileInMemory, int numBytes);

	// After a load you can call GetResults() to find out what, if anything, went wrong.
	uint32 GetResults() const																							{ return Results; }
	bool IsResultSet(ResultCode code) const																				{ return (Results & (1<<int(code))); }

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear();
	bool IsValid() const { return true; }

	// Will return R8G8B8A8 if you chose to decode the layers. Otherwise it will be whatever format the ktx data was in.
	tPixelFormat GetPixelFormat() const																					{ return PixelFormat; }

	// Will return the format the ktx data was in, even if you chose to decode.
	tPixelFormat GetPixelFormatSrc() const																				{ return PixelFormatSrc; }

	// After calling StealLayers the current object will be invalid. This call populates the passed in layer list. If
	// the current object is not valid the passed-in layer list is left unmodified. The layer list is appended to. It is
	// not emptied if there are layers on the list when passed in. This call gives management of the layers to the
	// caller. It does not memcpy and create new layers which is why the object becomes invalid afterwards. If the
	// tImageKTX is a cubemap, this function returns false and leaves the object (and list) unmodified. See
	// StealCubemapLayers if you want to steal cubemap layers.
	bool StealLayers(tList<tLayer>&) { return false; }

	// Alternative to StealLayers. Gets the layers but you're not allowed to delete them, they're not yours. Make
	// sure the list you supply doesn't delete them when it's destructed.
	bool GetLayers(tList<tLayer>&) const { return false; }

private:
	tPixelFormat GetPixelFormatFromGLFormat(uint32 glFormat);
	tPixelFormat GetPixelFormatFromVKFormat(uint32 vkFormat);

	// The result codes are bits in this Results member.
	uint32 Results =  1 << uint32(ResultCode::Success);

	tPixelFormat PixelFormat = tPixelFormat::R8G8B8A8;
	tPixelFormat PixelFormatSrc = tPixelFormat::Invalid;
};


// Implementation only below.


inline void tImageKTX::Clear()
{
	// @todo Delete data.
	PixelFormatSrc = tPixelFormat::Invalid;
}


}
