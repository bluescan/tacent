// tImageDDS.h
//
// This class knows how to load Direct Draw Surface (.dds) files.
//
// Copyright (c) 2006, 2017, 2019, 2020, 2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include "Image/tLayer.h"
#include "Image/tPixelFormat.h"
namespace tImage
{


// A tImageDDS object represents and knows how to load a dds file. In general a DirectDrawSurface is composed of
// multiple layers -- each one a mipmap. It loads the data into tLayers. It can either decode to R8G8B8A8 layers, or leave
// the data as-is. Decode from BCn is supported. The layers may be 'stolen' from a tImageDDS so that excessive memcpys
// are avoided. After they are stolen the tImageDDS is invalid. Cubemaps and mipmaps are supported.
// @todo 1D and 3D textures are not supported yet.
// @todo ASTC is not supported yet.
class tImageDDS
{
public:
	enum LoadFlag
	{
		LoadFlag_Decode				= 1 << 0,	// Decode the dds texture data into RGBA 32 bit layers. If not set, the layer data will remain unmodified.
		LoadFlag_ReverseRowOrder	= 1 << 1,	// OpenGL uses the lower left as the orig DirectX uses the upper left. Set flag for OpenGL.

		// Gamma-correct. Gamma compression using an encoding gamma of 1/2.2. Flag only applies when decode flag set for
		// HDR / floating-point formats (BC6, rgb16f/32f, etc) images. Assumes (colour) data is linear and puts it in
		// gamma-space (brighter) for diaplay on a monitor.
		LoadFlag_GammaCompression	= 1 << 2,
		LoadFlag_SRGBCompression	= 1 << 3,	// Same as above but uses the official sRGB transformation. Linear -> sRGB. Approx encoding gamma of 1/2.4 for part of curve.
		LoadFlag_AutoGamma			= 1 << 4,	// Tries to determine whether to apply sRGB compression based on pixel format. Call GetColourSpace to see if it applied.
		LoadFlag_ToneMapExposure	= 1 << 5,	// Apply exposure value when loading the dds. Only affects HDR (linear-colour) formats.
		LoadFlag_SpreadLuminance	= 1 << 6,	// For DDS files with a single Red or Luminance component, spread it to all the RGB channels (otherwise red only). Does not spread single-channel Alpha formats. Applies only if decoding a dds is an R-only or L-only format.
		LoadFlag_CondMultFourDim	= 1 << 7,	// Produce conditional success if image dimension not a multiple of 4. Only checks BC formats,
		LoadFlag_CondPowerTwoDim	= 1 << 8,	// Produce conditional success if image dimension not a power of 2. Only checks BC formats.
		LoadFlags_Default			= LoadFlag_Decode | LoadFlag_ReverseRowOrder | LoadFlag_SpreadLuminance | LoadFlag_AutoGamma
	};

	// If an error is encountered loading the resultant object will return false for IsValid. You can call GetLastResult
	// to get more detailed information. There are some results that are not full-success that leave the object valid.
	// When decoding _and_ reversing row order, most BC 4x4 blocks can be massaged without decompression to fix the row
	// order. The more complex ones like BC6 and BC7 cannot be swizzled around like this (well, they probably could be,
	// but it's a non-trivial amount of work).
	//
	// A note on LoadFlag_ReverseRowOrder. tImageDDS tries to perform row-reversing before any decode operation. This is
	// often possible even if the DDS texture data is BC-compressed. However, for some of the more complex BC schemes
	// (eg. BC6 BC7) this reversal cannot be easily accomplished without a full decode and re-encode which would be
	// lossy. In these cases the row-reversal is done _after_ decoding. Unfortunalely decoding may not always be
	// requested (for example if you want to pass the image data directly to the GPU memory in OpenGL). In these cases
	// tImageDDS will be unable to reverse the rows. You will still get a valid object, but it will be a conditional
	// success (GetResults() will have Conditional_CouldNotFlipRows flag set). You can call also call RowsReversed() to
	// see if row-reversal was performed. The conditional is only set if reversal was requested.
	//
	// Additional parameters may be processed during dds-loading. Gamma is only used if GammaCompression flag is set.
	// Exposure >= 0 (black) and only used if ToneMapExposure flag set.
	struct LoadParams
	{
		LoadParams()																									{ Reset(); }
		LoadParams(const LoadParams& src)																				: Flags(src.Flags), Gamma(src.Gamma), Exposure(src.Exposure) { }
		void Reset()																									{ Flags = LoadFlags_Default; Gamma = tMath::DefaultGamma; Exposure = 1.0f; }
		LoadParams& operator=(const LoadParams& src)																	{ Flags = src.Flags; Gamma = src.Gamma; Exposure = src.Exposure; }

		uint32 Flags;
		float Gamma;
		float Exposure;
	};

	// Creates an invalid tImageDDS. You must call Load manually.
	tImageDDS();

	tImageDDS(const tString& ddsFile, const LoadParams& = LoadParams());

	// This load from memory constructor behaves a lot like the from-file version. The file image in memory is read from
	// and the caller may delete it immediately after if desired.
	tImageDDS(const uint8* ddsMem, int numBytes, const LoadParams& = LoadParams());
	virtual ~tImageDDS()																								{ Clear(); }

	enum class ResultCode
	{
		// Success. The tImageDDS is considered valid. May be combined with the conditionals below.
		Success,

		// Conditional success. Object is valid, but not all load flags applied.
		Conditional_CouldNotFlipRows,
		Conditional_PitchXORLinearSize,
		Conditional_DimNotMultFourBC,
		Conditional_DimNotPowerTwoBC,

		// Fatal. Load was uncuccessful and object is invalid. The success flag will not be set.
		Fatal_FileDoesNotExist,
		Fatal_IncorrectFileType,
		Fatal_IncorrectFileSize,
		Fatal_IncorrectMagicNumber,
		Fatal_IncorrectHeaderSize,
		Fatal_InvalidDimensions,
		Fatal_VolumeTexturesNotSupported,
		Fatal_IncorrectPixelFormatHeaderSize,
		Fatal_IncorrectPixelFormatSpec,
		Fatal_PixelFormatNotSupported,
		Fatal_MaxNumMipmapLevelsExceeded,
		Fatal_DX10HeaderSizeIncorrect,
		Fatal_DX10DimensionNotSupported,
		Fatal_PackedDecodeError,
		Fatal_BCDecodeError,
		Fatal_ASTCDecodeError,
		NumCodes,

		// Since we store result codes as bits in a 32-bit uint, we need to make sure we don't have too many codes.
		MaxCodes							= 32,
		FirstValid							= Success,
		LastValid							= Conditional_DimNotPowerTwoBC,
		FirstFatal							= Fatal_FileDoesNotExist,
		LastFatal							= Fatal_DX10DimensionNotSupported
	};

	// Clears the current tImageDDS before loading. If the dds file failed to load for any reason it will result in an
	// invalid object. A dds may fail to load for a number of reasons: Volume textures are not supported, some
	// pixel-formats may not yet be supported, or inconsistent flags. Returns true on success or conditional-success.
	bool Load(const tString& ddsFile, const LoadParams& = LoadParams());
	bool Load(const uint8* ddsMem, int numBytes, const LoadParams& = LoadParams());

	// After a load you can call GetResults() to find out what, if anything, went wrong.
	uint32 GetResults() const																							{ return Results; }
	bool IsResultSet(ResultCode code) const																				{ return (Results & (1<<int(code))); }
	static const char* GetResultDesc(ResultCode);

	// Will return true if a dds file has been successfully loaded. This includes conditional success results.
	bool IsValid() const																								{ return (Results < (1 << int(ResultCode::FirstFatal))); }

	// After this call no memory will be consumed by the object and it will be invalid. Does not clear filename.
	void Clear();
	bool IsMipmapped() const																							{ return (NumMipmapLayers > 1) ? true : false; }
	bool IsCubemap() const																								{ return IsCubeMap; }
	bool RowsReversed() const																							{ return RowReversalOperationPerformed; }

	// The number of mipmap levels per image is always the same if there is more than one image in the direct texture
	// (like for cube maps). Same for the dimensions and pixel format.
	int GetNumMipmapLevels() const																						{ return NumMipmapLayers; }
	int GetNumImages() const																							{ return NumImages; }
	int GetWidth() const																								{ return IsValid() ? Layers[0][0]->Width : 0; }
	int GetHeight() const																								{ return IsValid() ? Layers[0][0]->Height : 0; }

	// Will return R8G8B8A8 if you chose to decode the layers. Otherwise it will be whatever format the dds data was in.
	tPixelFormat GetPixelFormat() const																					{ return PixelFormat; }

	// Will return the format the dds data was in, even if you chose to decode.
	tPixelFormat GetPixelFormatSrc() const																				{ return PixelFormatSrc; }

	// Returns the current colour space.
	tColourSpace GetColourSpace() const																					{ return ColourSpace; }

	// Returns the colour space of the source file that was loaded. This may not match the current if, say, gamma
	// correction was requested on load.
	tColourSpace GetColourSpaceSrc() const																				{ return ColourSpaceSrc; }

	tAlphaMode GetAlphaMode() const																						{ return AlphaMode; }

	// The texture is considered to have alphas if it is in a pixel format that supports them. For BC1, the data is
	// checked to see if any BC1 blocks have a binary alpha index. We could check the data for the RGBA formats, but
	// we don't as it shouldn't have been saved in an alpha supporting format if an all opaque texture was desired.
	bool IsOpaque() const;

	// After calling StealLayers the current object will be invalid. This call populates the passed in layer
	// list. If the current object is not valid the passed-in layer list is left unmodified. The layer list is appended
	// to. It is not emptied if there are layers on the list when passed in. This call gives management of the layers
	// to the caller. It does not memcpy and create new layers which is why the object becomes invalid afterwards. If
	// the tImageDDS is a cubemap, this function returns false and leaves the object (and list) unmodified. See
	// StealCubemapLayers if you want to steal cubemap layers.
	bool StealLayers(tList<tLayer>&);

	// Alternative to StealLayers. Gets the layers but you're not allowed to delete them, they're not yours. Make
	// sure the list you supply doesn't delete them when it's destructed.
	bool GetLayers(tList<tLayer>&) const;

	enum tSurfIndex
	{
		tSurfIndex_Default,
		tSurfIndex_PosX					= tSurfIndex_Default,
		tSurfIndex_NegX,
		tSurfIndex_PosY,
		tSurfIndex_NegY,
		tSurfIndex_PosZ,
		tSurfIndex_NegZ,
		tSurfIndex_NumSurfaces
	};

	// Cubemaps are always specified using a left-handed coord system even when using the OpenGL functions.
	enum tSurfFlag
	{
		tSurfFlag_PosX					= 1 << tSurfIndex_PosX,
		tSurfFlag_NegX					= 1 << tSurfIndex_NegX,
		tSurfFlag_PosY					= 1 << tSurfIndex_PosY,
		tSurfFlag_NegY					= 1 << tSurfIndex_NegY,
		tSurfFlag_PosZ					= 1 << tSurfIndex_PosZ,
		tSurfFlag_NegZ					= 1 << tSurfIndex_NegZ,
		tSurfFlag_All					= 0xFFFFFFFF
	};

	// Similar to StealLayers except it steals up to 6 layer-lists if the object is a cubemap. If the tImageDDS
	// is not a cubemap this function returns 0 and leaves the object (and list) unmodified. If you only steal a single
	// cubemap side, the object becomes completely invalid afterwards. The six passed in list pointers must all be
	// non-zero otherwise this function does nothing. The lists are appended to. Returns the number of layer-lists that
	// were populated.
	int StealCubemapLayers(tList<tLayer> layers[tSurfIndex_NumSurfaces], uint32 sideFlags = tSurfFlag_All);

	// Alternative to StealCubemapLayers. Gets the layers but you're not allowed to delete them, they're not yours. Make
	// sure the list you supply doesn't delete them when it's destructed.
	int GetCubemapLayers(tList<tLayer> layers[tSurfIndex_NumSurfaces], uint32 sideFlags = tSurfFlag_All) const;

	// You do not own the returned pointer.
	tLayer* GetLayer(int layerNum, int imageNum) const																	{ return Layers[layerNum][imageNum]; }

	tString Filename;

private:
	// The result codes are bits in this Results member.
	uint32 Results							= 0;

	tPixelFormat PixelFormat				= tPixelFormat::Invalid;
	tPixelFormat PixelFormatSrc				= tPixelFormat::Invalid;

	// These two _not_ part of the pixel format in tacent.
	tColourSpace ColourSpace				= tColourSpace::Unspecified;
	tColourSpace ColourSpaceSrc				= tColourSpace::Unspecified;
	tAlphaMode AlphaMode					= tAlphaMode::Unspecified;

	bool IsCubeMap							= false;
	bool RowReversalOperationPerformed		= false;

	// This will be 1 for textures and 6 for cubemaps.
	int NumImages							= 0;

	// If this is 1, you can consider the texture(s) to NOT be mipmapped. If there is more than a single image (like
	// with a cubemap), all images have the same number of mipmap layers.
	int NumMipmapLayers						= 0;
	const static int MaxMipmapLayers		= 16;		// Max dimension 32768.

	// Cubemaps are always specified using a left-handed coord system even when using the OpenGL functions.
	const static int MaxImages				= 6;
	tLayer* Layers[MaxMipmapLayers][MaxImages];

public:
	static const char* ResultDescriptions[];
};


}
