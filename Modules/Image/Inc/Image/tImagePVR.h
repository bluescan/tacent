// tImagePVR.h
//
// This class knows how to load PowerVR (.pvr) files. It knows the details of the pvr file format and loads the data
// into tLayers, optionally decompressing them. Saving is not implemented yet. The layers may be 'stolen' from a
// tImagePVR so that excessive memcpys are avoided. After they are stolen the tImagePVR is invalid. The tImagePVR
// class supports V1, V2, and V3 pvr files.
//
// Copyright (c) 2023-2025 Tristan Grimmer.
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
#include <Image/tBaseImage.h>
namespace tImage
{


// A tImagePVR object represents and knows how to load a pvr file. A pvr file is a container format much like ktx or
// dds. It comes in 3 different versions: V1, V2, and V3. All three use the same pvr extension. This class loads the
// data into tLayers. It can either decode to R8G8B8A8 layers, or leave the data as-is. There are many pixel formats
// supported by pvr V3 files including ASTC, BCn, and the PVRTC formats. The PVRTC formats also come in two main
// versions: V1 and V2. If a V1 format is used, the file should be checked to ensure it is POT. The layers may be
// 'stolen' from a tImagePVR so that excessive memcpys are avoided. After they are stolen the tImagePVR is invalid.
// Cubemaps, mipmaps, texture arrays, and 3D textures are supported.
class tImagePVR : public tBaseImage
{
public:
	enum LoadFlag
	{
		LoadFlag_Decode				= 1 << 0,	// Decode the pvr texture data into RGBA 32-bit layers. If not set, the layer data will remain unmodified.
		LoadFlag_ReverseRowOrder	= 1 << 1,	// OpenGL uses the lower left as the orig DirectX uses the upper left. Set flag for OpenGL.

		// Gamma-correct. Gamma compression using an encoding gamma of 1/2.2. Flag only applies when decode flag set for
		// HDR / floating-point formats (BC6, rgb16f/32f, etc) images. Assumes (colour) data is linear and puts it in
		// gamma-space (brighter) for diaplay on a monitor.
		LoadFlag_GammaCompression	= 1 << 2,
		LoadFlag_SRGBCompression	= 1 << 3,	// Same as above but uses the official sRGB transformation. Linear -> sRGB. Approx encoding gamma of 1/2.4 for part of curve.
		LoadFlag_AutoGamma			= 1 << 4,	// Tries to determine whether to apply sRGB compression based on pixel format. Call GetColourSpace to see if it applied.
		LoadFlag_ToneMapExposure	= 1 << 5,	// Apply exposure value when loading. Only affects HDR (linear-colour) formats.
		LoadFlag_SpreadLuminance	= 1 << 6,	// For files with a single Red or Luminance component, spread it to all the RGB channels (otherwise red only). Does not spread single-channel Alpha formats. Applies only if decoding an R-only or L-only format.
		LoadFlag_CondMultFourDim	= 1 << 7,	// Produce conditional success if image dimension not a multiple of 4. Only checks BC formats,
		LoadFlag_StrictLoading		= 1 << 8,	// If set ill-formed files will not load. Specifically if format is PVRTC (not PVRTC2) the texture must be POT if this flag set.
		LoadFlag_MetaDataOrient		= 1 << 9,	// Undo orientation transformations in pvr3 image as indicated by pvr3 meta-data. Works iff decoding or already in RGBA.

		LoadFlags_Default			= LoadFlag_Decode | LoadFlag_ReverseRowOrder | LoadFlag_SpreadLuminance | LoadFlag_AutoGamma | LoadFlag_StrictLoading | LoadFlag_MetaDataOrient
	};

	// If an error is encountered loading the resultant object will return false for IsValid. You can call GetLastResult
	// to get more detailed information. There are some results that are not full-success that leave the object valid.
	// When decoding _and_ reversing row order, most BC 4x4 blocks can be massaged without decompression to fix the row
	// order. The more complex ones like BC6 and BC7 cannot be swizzled around like this (well, they probably could be,
	// but it's a non-trivial amount of work).
	//
	// A note on LoadFlag_ReverseRowOrder. tImagePVR tries to perform row-reversing before any decode operation. This is
	// often possible even if the PVR texture data is BC-compressed. However, for some of the more complex BC schemes
	// (eg. BC6 BC7) this reversal cannot be easily accomplished without a full decode and re-encode which would be
	// lossy. In these cases the row-reversal is done _after_ decoding. Unfortunalely decoding may not always be
	// requested (for example if you want to pass the image data directly to the GPU memory in OpenGL). In these cases
	// tImagePVR will be unable to reverse the rows. You will still get a valid object, but it will be a conditional
	// valid (GetStates() will have Conditional_CouldNotFlipRows flag set). You can call also call RowsReversed() to
	// see if row-reversal was performed. The conditional is only set if reversal was requested.
	//
	// Additional parameters may be processed during loading. Gamma is only used if GammaCompression flag is set.
	// Exposure >= 0 (black) and only used if ToneMapExposure flag set.
	struct LoadParams
	{
		LoadParams()																									{ Reset(); }
		LoadParams(const LoadParams& src)																				: Flags(src.Flags), Gamma(src.Gamma), Exposure(src.Exposure), MaxRange(src.MaxRange) { }
		void Reset()																									{ Flags = LoadFlags_Default; Gamma = tMath::fDefaultGamma; Exposure = 1.0f; MaxRange = 8.0f; }
		LoadParams& operator=(const LoadParams& src)																	{ Flags = src.Flags; Gamma = src.Gamma; Exposure = src.Exposure; MaxRange = src.MaxRange; return *this; }

		uint32 Flags;
		float Gamma;
		float Exposure;
		float MaxRange;		// Used for RGBM and RGBD only.
	};

	// Creates an invalid tImagePVR. You must call Load manually.
	tImagePVR()																											{ }
	tImagePVR(const tString& pvrFile, const LoadParams& params = LoadParams())											: Filename(pvrFile) { Load(pvrFile, params); }

	// This load from memory constructor behaves a lot like the from-file version. The file image in memory is read from
	// and the caller may delete it immediately after if desired.
	tImagePVR(const uint8* pvrMem, int numBytes, const LoadParams& params = LoadParams())								{ Load(pvrMem, numBytes, params); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out. Sets the colour space to sRGB.
	tImagePVR(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImagePVR(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImagePVR(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImagePVR()																								{ Clear(); }

	enum class StateBit
	{
		// The tImagePVR is considered valid. May be combined with the conditionals below.
		Valid,

		// Conditional valid. Valid bit still set.
		FirstConditional,
		Conditional_CouldNotFlipRows			= FirstConditional,
		Conditional_IncorrectPixelFormatSpec,	// Possible if strict loading not set.
		Conditional_V2IncorrectFourCC,
		Conditional_V1V2InvalidDimensionsPVRTC1,
		Conditional_V1V2MipmapFlagInconsistent,	// V1 V2 files have a mipmaps present flag. If set there should be more than one mipmap. If not set there should be one.
		LastConditional							= Conditional_V1V2MipmapFlagInconsistent,

		// Fatal. Load was uncuccessful and object is invalid. The valid flag will not be set.
		FirstFatal,
		Fatal_FileDoesNotExist					= FirstFatal,
		Fatal_IncorrectFileType,
		Fatal_IncorrectFileSize,
		Fatal_V2IncorrectFourCC,
		Fatal_IncorrectHeaderSize,
		Fatal_BadHeaderData,
		Fatal_UnsupportedPVRFileVersion,
		Fatal_V1V2InvalidDimensionsPVRTC1,
		Fatal_IncorrectPixelFormatHeaderSize,
		Fatal_IncorrectPixelFormatSpec,			// Possible if strict loading set.
		Fatal_PixelFormatNotSupported,
		Fatal_V1V2MipmapFlagInconsistent,		// Same as conditional but fatal for strict loading.
		Fatal_V1V2CubemapFlagInconsistent,		// If flag set there must be a mutiple of 6 surfaces.
		Fatal_V1V2TwiddlingUnsupported,			// We do not yet support V1V2 data twiddling.
		Fatal_PackedDecodeError,
		Fatal_BCDecodeError,
		Fatal_ASTCDecodeError,
		Fatal_PVRDecodeError,
		LastFatal								= Fatal_PVRDecodeError,

		// Since we store states as bits in a 32-bit uint, we need to make sure we don't too many.
		NumStateBits,
		MaxStateBits							= 32
	};

	// After this call no memory will be consumed by the object and it will be invalid. Does not clear filename.
	void Clear() override;

	// Will return true if a dds file has been successfully loaded or otherwise populated.
	// This includes conditional valid results.
	bool IsValid() const override																						{ return IsStateSet(StateBit::Valid); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4b* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame. After this is called the layer data will be in R8G8B8A8.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	// Clears the current tImagePVR before loading. If the pvr file failed to load for any reason it will result in an
	// invalid object. A pvr may fail to load for a number of reasons: Some pixel-formats may not yet be supported, or
	// inconsistent flags. Returns true on success or conditional-success.
	bool Load(const tString& pvrFile, const LoadParams& = LoadParams());
	bool Load(const uint8* pvrFileInMemory, int numBytes, const LoadParams& = LoadParams());

	// After a load you can call GetStates() to find out what, if anything, went wrong.
	uint32 GetStates() const																							{ return States; }
	bool IsStateSet(StateBit state) const																				{ return (States & (1<<int(state))); }
	static const char* GetStateDesc(StateBit);

	bool IsMipmapped() const override																					{ return (NumMipmaps > 1) ? true : false; }
	bool IsCubemap() const override																						{ return (NumFaces == 6); }

	// Returns the pvr container format version. If the tImagePVR is not valid -1 is returned. If the tImagePVR is valid
	// but was not loaded from a .pvr file, 0 is returned. Otherwise 1 is returned for V1, 2 is returned for V2, and 3
	// is returned for V3.
	int GetVersion() const																								{ return IsValid() ? PVRVersion : -1; }
	bool RowsReversed() const																							{ return RowReversalOperationPerformed; }

	int GetNumSurfaces() const																							{ return NumSurfaces; }
	int GetNumFaces() const		/* Should be 6 for cubemaps */															{ return NumFaces; }
	int GetNumMipmaps() const																							{ return NumMipmaps; }
	int GetNumLayersTotal() const																						{ return NumSurfaces * NumFaces * NumMipmaps; }

	int GetWidth() const																								{ return IsValid() ? Width : 0; }
	int GetHeight() const																								{ return IsValid() ? Height : 0; }

	// The texture is considered to have alphas if it is in a pixel format that supports them. For BC1, the data is
	// checked to see if any BC1 blocks have a binary alpha index. We could check the data for the RGBA formats, but
	// we don't as it shouldn't have been saved in an alpha supporting format if an all opaque texture was desired.
	bool IsOpaque() const;

	// After calling StealLayers the current object will be invalid. This call populates the passed in layer
	// list. If the current object is not valid the passed-in layer list is left unmodified. The layer list is appended
	// to. It is not emptied if there are layers on the list when passed in. This call gives management of the layers
	// to the caller. It does not memcpy and create new layers which is why the object becomes invalid afterwards. If
	// the tImagePVR is a cubemap, this function returns false and leaves the object (and list) unmodified. See
	// StealCubemapLayers if you want to steal cubemap layers.
	bool StealLayers(tList<tLayer>&);
	tFrame* GetFrame(bool steal = true) override;

	// Gets the layers but you're not allowed to delete them, they're not yours. Make sure the list you supply doesn't
	// delete them when it's destructed. Returns the number of items appended to the list.
	int GetLayers(tList<tLayer>&) const override;

	// Similar to StealLayers except it steals up to 6 layer-lists if the object is a cubemap. If the tImagePVR
	// is not a cubemap this function returns 0 and leaves the object (and list) unmodified. If you only steal a single
	// cubemap side, the object becomes completely invalid afterwards. The six passed in list pointers must all be
	// non-zero otherwise this function does nothing. The lists are appended to. Returns the number of layer-lists that
	// were populated.
	int StealCubemapLayers(tList<tLayer> layers[tFaceIndex_NumFaces], uint32 faceFlags = tFaceFlag_All);

	// Gets the layers but you're not allowed to delete them, they're not yours. Make sure the list you supply doesn't
	// delete them when it's destructed. Returns the number of items appended to the list.
	int GetCubemapLayers(tList<tLayer> layers[tFaceIndex_NumFaces], uint32 faceFlags = tFaceFlag_All) const override;

	// You do not own the returned pointer.
	tLayer* GetLayer(int layerNum, int imageNum) const;

	tAlphaMode GetAlphaMode() const override																			{ return AlphaMode; }
	tChannelType GetChannelType() const override																		{ return ChannelType; }

	tString Filename;

private:
	bool ParseMetaData(const uint8* metaData, int metaDataSize);
	int LayerIdx(int surf, int face = 0, int mip = 0, int depth = 0) const;
	tLayer* CreateNewLayer(const LoadParams&, const uint8* srcPixelData, int numBytes, int width, int height);
	void SetStateBit(StateBit state)		{ States |= 1 << int(state); }

	// The states are bits in this States member.
	uint32 States							= 0;
	int PVRVersion							= 0;

	// These are _not_ part of the pixel format in tacent.
	tAlphaMode AlphaMode					= tAlphaMode::Unspecified;
	tChannelType ChannelType				= tChannelType::Unspecified;

	bool RowReversalOperationPerformed		= false;

	// This is lifted from the PVR3 spec. It is a superset of the PVR1 and PVR2 structure.
	//
	// for each MIP-Map Level in MIP-Map Count
	//  for each Surface in Num. Surfaces
	//   for each Face in Num. Faces
	//    for each Slice in Depth
	//     for each Row in Height
	//      for each Pixel in Width
	//       Byte data[Size_Based_On_PixelFormat]
	//
	// The data ordering is different for PVR1/2 files but will fit info PVR3 structure
	// as listed above.
	//
	// for each Surface in Num. Surfaces
	//  for each Face in 6
	//   for each MIP-Map Level in MIP-Map Count
	//    for each Row in Height
	//     for each Pixel in Width
	//      Byte data[Size_Based_On_PixelFormat]
	//
	// In this loader we'll leverage tLayers which deal with the Width, Height, and Byte data.
	// We'll simply create enough tLayer slots for NumSurfaces*NumFaces*NumMipmaps*Depth.
	// Rather than either the V1/2 or V3 ordering we'll be using:
	//
	// Surfaces
	//  Faces
	//   Mipmaps
	//    Slices
	//     Layers
	int NumSurfaces							= 0;		// For storing arrays of image data.
	int NumFaces							= 0;		// For cubemaps. Cubemaps are always specified using a left-handed coord system even when using the OpenGL functions.
	int NumMipmaps							= 0;

	int Depth								= 0;		// Number of slices.
	int Width								= 0;
	int Height								= 0;

	int NumLayers							= 0;
	tLayer** Layers							= nullptr;	// Always nullptr if NumLayers is 0.

	// Parsed from MetaData.
	bool MetaData_Orientation_Flip_X		= false;
	bool MetaData_Orientation_Flip_Y		= false;

public:
	static const char* StateDescriptions[];
};


}
