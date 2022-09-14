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
#include <System/tThrow.h>
#include "Image/tLayer.h"
#include "Image/tPixelFormat.h"
namespace tImage
{


struct tDXT1Block;


// A tImageDDS object represents and knows how to load a dds file. In general a DirectDrawSurface is composed of
// multiple layers -- each one a mipmap. It loads the data into tLayers. It can either decode to RGBA layers, or leave
// the data as-is. Decode from BCn is supported. ASTC is not. The layers may be 'stolen' from a tImageDDS so that
// excessive memcpys are avoided. After they are stolen the tImageDDS is invalid. 1D and 3D textures are not supported.
// Cubemaps and mipmaps are.
class tImageDDS
{
public:
	// Creates an invalid tImageDDS. You must call Load manually.
	tImageDDS();

	enum LoadFlag
	{
		LoadFlag_Decode				= 1 << 0,	// Decode the dds texture data into RGBA 32 bit layers. If not set, the layer data will remain unmodified.
		LoadFlag_ReverseRowOrder	= 1 << 1,	// OpenGL uses the lower left as the orig DirectX uses the upper left. Set flag for OpenGL.
		LoadFlags_Default			= LoadFlag_Decode | LoadFlag_ReverseRowOrder
	};

	// If an error is encountered loading the resultant object will return false for IsValid. You can call GetLastError
	// to get more detailed information. When decoding _and_ reversing row order, BC 4x4 blocks can be massaged without
	// decompression to fix the row order.
	tImageDDS(const tString& ddsFile, uint32 loadFlags = LoadFlags_Default);

	// This load from memory constructor behaves a lot like the from-file version. The file image in memory is read from
	// and the caller may delete it immediately after if desired.
	tImageDDS(const uint8* ddsFileInMemory, int numBytes, uint32 loadFlags = LoadFlags_Default);
	virtual ~tImageDDS()																								{ Clear(); }

	enum class ErrorCode
	{
		NoError,
		Success									= NoError,

		FileNonexistent,
		IncorrectExtension,
		IncorrectFileSize,
		Magic,
		IncorrectHeaderSize,
		PitchOrLinearSize,
		VolumeTexturesNotSupported,
		IncorrectPixelFormatSize,
		InconsistentPixelFormat,
		UnsupportedFourCCPixelFormat,
		UnsupportedRGBPixelFormat,
		IncorrectDXTDataSize,
		UnsupportedDXTDimensions,
		LoaderSupportsPowerOfTwoDimsOnly,
		MaxNumMipmapLevelsExceeded,
		UnsuportedFloatingPointPixelFormat,
		Unknown,
		NumCodes
	};

	// After construction if the object is invalid you can call GetLastError() to find out what went wrong.
	ErrorCode GetLastError() const																						{ return Result; }
	static const char* GetErrorName(ErrorCode);
	const char* GetLastErrorName() const																				{ return GetErrorName(Result); }

	// Clears the current tImageDDS before loading. If the dds file failed to load for any reason it will result in an
	// invalid object. A dds may fail to load for a number of reasons: Volume textures are not supported, some
	// pixel-formats may not yet be supported, or inconsistent flags.
	ErrorCode Load(const tString& ddsFile, uint32 loadFlags = LoadFlags_Default);
	ErrorCode Load(const uint8* ddsFileInMemory, int numBytes, uint32 loadFlags = LoadFlags_Default);

	// After this call no memory will be consumed by the object and it will be invalid. Does not clear filename.
	void Clear();

	// Will return true if a dds file has been successfully loaded.
	bool IsValid() const																								{ return ((NumMipmapLayers > 0) && (NumImages > 0) && (PixelFormat != tPixelFormat::Invalid)) ? true : false; }
	bool IsMipmapped() const																							{ return (NumMipmapLayers > 1) ? true : false; }
	bool IsCubemap() const																								{ return IsCubeMap; }

	// The number of mipmap levels per image is always the same if there is more than one image in the direct texture
	// (like for cube maps). Same for the dimensions and pixel format.
	int GetNumMipmapLevels() const																						{ return NumMipmapLayers; }
	int GetNumImages() const																							{ return NumImages; }
	int GetWidth() const																								{ return IsValid() ? MipmapLayers[0][0]->Width : 0; }
	int GetHeight() const																								{ return IsValid() ? MipmapLayers[0][0]->Height : 0; }

	// Will return RGBA if you chose to decode the layers. Otherwise it will be whatever format the dds data was in.
	tPixelFormat GetPixelFormat() const																					{ return PixelFormat; }

	// The texture is considered to have alphas if it is in a pixel format that supports them. For DXT1, the data is
	// checked to see if any DXT1 blocks have a binary alpha index. We could check the data for the RGBA formats, but
	// we don't as it shouldn't have been saved in an alpha supporting format if an all opaque texture was desired.
	bool IsOpaque() const;

	// After calling StealTextureLayers the current object will be invalid. This call populates the passed in layer
	// list. If the current object is not valid the passed-in layer list is left unmodified. The layer list is appended
	// to. It is not emptied if there are layers on the list when passed in. This call gives management of the layers
	// to the caller. It does not memcpy and create new layers which is why the object becomes invalid afterwards. If
	// the tImageDDS is a cubemap, this function returns false and leaves the object unmodified. See StealCubemapLayers
	// if you want to steal cubemap layers.
	bool StealTextureLayers(tList<tLayer>&);

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

	// Similar to StealTextureLayers except it steals up to 6 layer-lists if the object is a cubemap. If the tImageDDS
	// is not a cubemap this function returns 0 and leaves the object unmodified. If you only steal a single cubemap
	// side, the object becomes completely invalid afterwards. The six passed in list pointers must all be non-zero
	// otherwise this function does nothing. The lists are appended to. Returns the number of layer-lists that were
	// populated.
	int StealCubemapLayers(tList<tLayer> layers[tSurfIndex_NumSurfaces], uint32 sideFlags = tSurfFlag_All);

	// You do not own the returned pointer.
	tLayer* GetLayer(int layerNum, int imageNum) const																	{ return MipmapLayers[layerNum][imageNum]; }

	// This is only for reporting the filename in case of errors.
	tString Filename;

private:
	bool DoDXT1BlocksHaveBinaryAlpha(tDXT1Block* blocks, int numBlocks);

	ErrorCode Result = ErrorCode::Success;

	// The surface is only valid if this is not PixelFormat_Invalid.
	tPixelFormat PixelFormat;
	bool IsCubeMap;

	// This will be 1 for textures. For cubemaps NumImages == 6.
	int NumImages;

	// If this is 1, you can consider the texture(s) to NOT be mipmapped. If there is more than a single image (like
	// with a cubemap), all images have the same number of mipmap layers.
	int NumMipmapLayers;
	const static int MaxMipmapLayers = 16;

	// Cubemaps are always specified using a left-handed coord system even when using the OpenGL functions.
	const static int MaxImages = 6;
	tLayer* MipmapLayers[MaxMipmapLayers][MaxImages];

public:
	static const char* ErrorDescriptions[];
};


}
