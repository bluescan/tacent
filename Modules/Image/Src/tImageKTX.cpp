// tImageKTX.cpp
//
// This knows how to load/save KTX files. It knows the details of the ktx and ktx2 file format and loads the data into
// multiple tPixel arrays, one for each frame (KTKs may be animated). These arrays may be 'stolen' by tPictures.
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

// WIP #include <Foundation/tStandard.h>
#include <Foundation/tString.h>
#include <Foundation/tHalf.h>
// WIP #include <System/tFile.h>
#include "Image/tImageKTX.h"
#include "bcdec/bcdec.h"
#define KHRONOS_STATIC
#include "LibKTX/include/ktx.h"
#include "LibKTX/include/vulkan_core.h"
namespace tImage
{


// Helper functions, enums, and types for parsing KTX files.
namespace tKTX
{
	// These figure out the pixel format, the colour-space, and the alpha mode. tPixelFormat does not specify ancilllary
	// properties of the data -- it specified the encoding of the data. The extra information, like the colour-space it
	// was authored in, is stored in tColourSpace. In many cases this satellite information cannot be determined, in
	// which case colour-space will be set to their 'unspecified' enumerant.
	void GetFormatInfo_FromGLFormat(tPixelFormat&, tColourSpace&, uint32 glFormat);
	void GetFormatInfo_FromVKFormat(tPixelFormat&, tColourSpace&, uint32 vkFormat);
}


void tKTX::GetFormatInfo_FromGLFormat(tPixelFormat& format, tColourSpace& space, uint32 glFormat)
{
	format = tPixelFormat::Invalid;
	space = tColourSpace::Unspecified;
	// WIP
}


void tKTX::GetFormatInfo_FromVKFormat(tPixelFormat& format, tColourSpace& space, uint32 vkFormat)
{
	format = tPixelFormat::Invalid;
	space = tColourSpace::Unspecified;

	switch (vkFormat)
	{
		// The VK formats conflate the format with the data. The colour-space is not part of the format in tacent and is
		// returned in a separate variable.
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
			space = tColourSpace::sRGB;

		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
			format = tPixelFormat::BC1DXT1;
			break;

		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
			space = tColourSpace::sRGB;

		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			format = tPixelFormat::BC1DXT1A;
			break;
	}
}


tImageKTX::tImageKTX()
{
	tStd::tMemset(Layers, 0, sizeof(Layers));
}


tImageKTX::tImageKTX(const tString& ktxFile, const LoadParams& loadParams) :
	Filename(ktxFile)
{
	tStd::tMemset(Layers, 0, sizeof(Layers));
	Load(ktxFile, loadParams);
}


tImageKTX::tImageKTX(const uint8* ktxFileInMemory, int numBytes, const LoadParams& loadParams)
{
	tStd::tMemset(Layers, 0, sizeof(Layers));
	Load(ktxFileInMemory, numBytes, loadParams);
}


void tImageKTX::Clear()
{
	for (int image = 0; image < NumImages; image++)
	{
		for (int layer = 0; layer < NumMipmapLayers; layer++)
		{
			delete Layers[layer][image];
			Layers[layer][image] = nullptr;
		}
	}

	Results							= 0;							// This means no results.
	PixelFormat						= tPixelFormat::Invalid;
	PixelFormatSrc					= tPixelFormat::Invalid;
	ColourSpace						= tColourSpace::Unspecified;
	AlphaMode						= tAlphaMode::Unspecified;
	IsCubeMap						= false;
	RowReversalOperationPerformed	= false;
	NumImages						= 0;
	NumMipmapLayers					= 0;
}


bool tImageKTX::IsOpaque() const
{
	return tImage::tIsOpaqueFormat(PixelFormat);
}


bool tImageKTX::StealLayers(tList<tLayer>& layers)
{
	if (!IsValid() || IsCubemap() || (NumImages <= 0))
		return false;

	for (int mip = 0; mip < NumMipmapLayers; mip++)
	{
		layers.Append(Layers[mip][0]);
		Layers[mip][0] = nullptr;
	}

	Clear();
	return true;
}


bool tImageKTX::GetLayers(tList<tLayer>& layers) const
{
	if (!IsValid() || IsCubemap() || (NumImages <= 0))
		return false;

	for (int mip = 0; mip < NumMipmapLayers; mip++)
		layers.Append(Layers[mip][0]);

	return true;
}


int tImageKTX::StealCubemapLayers(tList<tLayer> layerLists[tSurfIndex_NumSurfaces], uint32 sideFlags)
{
	if (!IsValid() || !IsCubemap() || !sideFlags)
		return 0;

	int sideCount = 0;
	for (int side = 0; side < tSurfIndex_NumSurfaces; side++)
	{
		uint32 sideFlag = 1 << side;
		if (!(sideFlag & sideFlags))
			continue;

		tList<tLayer>& layers = layerLists[side];
		for (int mip = 0; mip < NumMipmapLayers; mip++)
		{
			layers.Append( Layers[mip][side] );
			Layers[mip][side] = nullptr;
		}
		sideCount++;
	}

	Clear();
	return sideCount;
}


int tImageKTX::GetCubemapLayers(tList<tLayer> layerLists[tSurfIndex_NumSurfaces], uint32 sideFlags) const
{
	if (!IsValid() || !IsCubemap() || !sideFlags)
		return 0;

	int sideCount = 0;
	for (int side = 0; side < tSurfIndex_NumSurfaces; side++)
	{
		uint32 sideFlag = 1 << side;
		if (!(sideFlag & sideFlags))
			continue;

		tList<tLayer>& layers = layerLists[side];
		for (int mip = 0; mip < NumMipmapLayers; mip++)
			layers.Append( Layers[mip][side] );

		sideCount++;
	}

	return sideCount;
}


bool tImageKTX::Load(const tString& ktxFile, const LoadParams& loadParams)
{
	Clear();
	Filename = ktxFile;
	tSystem::tFileType fileType = tSystem::tGetFileType(ktxFile);
	if ((fileType != tSystem::tFileType::KTX) && (fileType != tSystem::tFileType::KTX2))
	{
		Results |= 1 << int(ResultCode::Fatal_IncorrectFileType);
		return false;
	}

	if (!tSystem::tFileExists(ktxFile))
	{
		Results |= 1 << int(ResultCode::Fatal_FileDoesNotExist);
		return false;
	}

	int ktxSizeBytes = 0;
	uint8* ktxData = (uint8*)tSystem::tLoadFile(ktxFile, 0, &ktxSizeBytes);
	bool success = Load(ktxData, ktxSizeBytes, loadParams);
	delete[] ktxData;

	return success;
}


bool tImageKTX::Load(const uint8* ktxData, int ktxSizeBytes, const LoadParams& params)
{
	Clear();

	ktx_error_code_e result = KTX_SUCCESS;
	ktxTexture* texture = nullptr;
	result = ktxTexture_CreateFromMemory(ktxData, ktxSizeBytes, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
	if (!texture || (result != KTX_SUCCESS))
	{
		Results |= 1 << int(ResultCode::Fatal_CouldNotParseFile);
		return false;
	}

	NumImages			= texture->numFaces;		// Number of faces. 1 or 6 for cubemaps.
	int numLayers		= texture->numLayers;		// Number of array layers. I believe this will be > 1 for 3D textures that are made of an array of layers.
	NumMipmapLayers		= texture->numLevels;		// Mipmap levels.
	int numDims			= texture->numDimensions;	// 1D, 2D, or 3D.
	int mainWidth		= texture->baseWidth;
	int mainHeight		= texture->baseHeight;
	bool isArray		= texture->isArray;
	bool isCompressed	= texture->isCompressed;

	if ((NumMipmapLayers <= 0) || (numDims != 2) || (mainWidth <= 0) || (mainHeight <= 0))
	{
		ktxTexture_Destroy(texture);
		Results |= 1 << int(ResultCode::Fatal_InvalidDimensions);
		return false;
	}

	if (NumMipmapLayers > MaxMipmapLayers)
	{
		ktxTexture_Destroy(texture);
		Results |= 1 << int(ResultCode::Fatal_MaxNumMipmapLevelsExceeded);
		return false;
	}

	IsCubeMap = (NumImages == 6);

	// We need to determine the pixel-format of the data. To do this we first need to know if we are dealing with
	// a ktx1 (OpenGL-style) or ktx2 (Vulkan-style) file.
	ktxTexture1* ktx1 = (texture->classId == ktxTexture1_c) ? (ktxTexture1*)texture : nullptr;
	ktxTexture2* ktx2 = (texture->classId == ktxTexture2_c) ? (ktxTexture2*)texture : nullptr;
	if (!ktx1 && !ktx2)
	{
		ktxTexture_Destroy(texture);
		Results |= 1 << int(ResultCode::Fatal_CorruptedFile);
		return false;
	}

	// WIP For now we only support ktx2 files. Try using toktx to gen ktx (1) test files.
	if (!ktx2)
	{
		ktxTexture_Destroy(texture);
		Results |= 1 << int(ResultCode::Fatal_FileVersionNotSupported);
		return false;
	}

	if (ktx1)
		tKTX::GetFormatInfo_FromGLFormat(PixelFormat, ColourSpace, ktx1->glInternalformat);
	else if (ktx2)
		tKTX::GetFormatInfo_FromVKFormat(PixelFormat, ColourSpace, ktx2->vkFormat);
	PixelFormatSrc = PixelFormat;

	// From now on we should just be using the PixelFormat to decide what to do next.
	if (PixelFormat == tPixelFormat::Invalid)
	{
		ktxTexture_Destroy(texture);
		Results |= 1 << int(ResultCode::Fatal_PixelFormatNotSupported);
		return false;
	}

	if (tIsBlockCompressedFormat(PixelFormat))
	{
		if ((params.Flags & LoadFlag_CondMultFourDim) && ((mainWidth%4) || (mainHeight%4)))
			Results |= 1 << int(ResultCode::Conditional_DimNotMultFourBC);
		if ((params.Flags & LoadFlag_CondPowerTwoDim) && (!tMath::tIsPower2(mainWidth) || !tMath::tIsPower2(mainHeight)))
			Results |= 1 << int(ResultCode::Conditional_DimNotMultFourBC);
	}

	bool reverseRowOrderRequested = params.Flags & LoadFlag_ReverseRowOrder;
	RowReversalOperationPerformed = false;

/*
	// Retrieve a data pointer to the image for a specific miplevel, arraylayer, and slice (face or depth.
	for (int mip = 0; mip < numLevels; mip++)
	{
		int layerNum = 0;
		int faceNum = 0;
		size_t offset;
		result = ktxTexture_GetImageOffset(texture, mip, layerNum, faceNum, &offset);
		if (result != KTX_SUCCESS)
			break;

		uint8* pixelData = ktxTexture_GetData(texture) + offset;

		tPrintf("KTX. numFaces:%d  numLayers:%d  numLevels:%d  width:%d  height:%d  isArray:%B  iscompressed:%B\n", numFaces, numLayers, numLevels, width, height, isArray, isCompressed);
		// WIP Convert / decode the pixelData based on the pixelFormat into R8G8B8A8.
	}
*/

/*
	for (int image = 0; image < NumImages; image++)
	{
		int width = baseWidth;
		int height = baseHeight;
		for (int layer = 0; layer < NumMipmaps; layer++)
		{
			int numBytes = 0;
			if (tImage::tIsBlockCompressedFormat(PixelFormat))
			{
				// It's a BC/DXTn format. Each block encodes a 4x4 square of pixels. DXT2,3,4,5 and BC 6,7 use 128
				// bits per block.  DXT1 and DXT1A (BC1) use 64bits per block.
				int bcBlockSize = tImage::tGetBytesPer4x4PixelBlock(PixelFormat);
				int numBlocksW = tMath::tMax(1, (width + 3) / 4);
				int numBlocksH = tMath::tMax(1, (height + 3) / 4);
				int numBlocks = numBlocksW*numBlocksH;
				numBytes = numBlocks * bcBlockSize;

				// If reverseRowOrder is false we want the data to go straight in so we use the currPixelData directly.
				Layers[layer][image] = new tLayer(PixelFormat, width, height, (uint8*)currPixelData);

				tAssert(Layers[layer][image]->GetDataSize() == numBytes);
			}
			else
			{
				// Upsupported pixel format.
				Clear();
				Results |= 1 << int(ResultCode::Fatal_UnsupportedPixelFormat);
				return false;
			}

			currPixelData += numBytes;
			width /= 2;
			if (width < 1)
				width = 1;

			height /= 2;
			if (height < 1)
				height = 1;
		}
	}
*/

	ktxTexture_Destroy(texture);
	return true;
}


}
