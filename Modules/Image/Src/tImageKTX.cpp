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

#include <Foundation/tString.h>
#include <Foundation/tHalf.h>
#include "Image/tImageKTX.h"
#include "Image/tPixelUtil.h"
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

	for (int image = 0; image < NumImages; image++)
	{
		int width = mainWidth;
		int height = mainHeight;
		for (int layer = 0; layer < NumMipmapLayers; layer++)
		{
			size_t offset;
			result = ktxTexture_GetImageOffset(texture, layer, 0, image, &offset);
			if (result != KTX_SUCCESS)
			{
				ktxTexture_Destroy(texture);
				Results |= 1 << int(ResultCode::Fatal_InvalidDataOffset);
				return false;
			}

			uint8* currPixelData = ktxTexture_GetData(texture) + offset;

			int numBytes = 0;
			if (tImage::tIsPackedFormat(PixelFormat))
			{
				numBytes = width*height*tImage::tGetBitsPerPixel(PixelFormat)/8;

				// Deal with reversing row order for RGB formats.
				if (reverseRowOrderRequested)
				{
					uint8* reversedPixelData = tImage::CreateReversedRowData_Packed(currPixelData, PixelFormat, width, height);
					if (reversedPixelData)
					{
						// We can simply get the layer to steal the memory (the last true arg).
						Layers[layer][image] = new tLayer(PixelFormat, width, height, reversedPixelData, true);
						RowReversalOperationPerformed = true;
					}
					else
					{
						// Row reversal failed. May be a conditional success if we don't convert to RGBA 32-bit later.
						Layers[layer][image] = new tLayer(PixelFormat, width, height, (uint8*)currPixelData);
					}
				}
				else
				{
					Layers[layer][image] = new tLayer(PixelFormat, width, height, (uint8*)currPixelData);
				}
				tAssert(Layers[layer][image]->GetDataSize() == numBytes);
			}

			else if (tImage::tIsBlockCompressedFormat(PixelFormat))
			{
				// It's a BC/DXTn format. Each block encodes a 4x4 square of pixels. DXT2,3,4,5 and BC 6,7 use 128
				// bits per block.  DXT1 and DXT1A (BC1) use 64bits per block.
				int bcBlockSize = tImage::tGetBytesPer4x4PixelBlock(PixelFormat);
				int numBlocksW = tMath::tMax(1, (width + 3) / 4);
				int numBlocksH = tMath::tMax(1, (height + 3) / 4);
				int numBlocks = numBlocksW*numBlocksH;
				numBytes = numBlocks * bcBlockSize;

				// Here's where we possibly modify the opaque DXT1 texture to be DXT1A if there are blocks with binary
				// transparency. We only bother checking the main layer. If it's opaque we assume all the others are too.
				if ((layer == 0) && (PixelFormat == tPixelFormat::BC1DXT1) && tImage::DoBC1BlocksHaveBinaryAlpha((tImage::BC1Block*)currPixelData, numBlocks))
					PixelFormat = PixelFormatSrc = tPixelFormat::BC1DXT1A;

				// DDS files store textures upside down. In the OpenGL RH coord system, the lower left of the texture
				// is the origin and consecutive rows go up. For this reason we need to read each row of blocks from
				// the top to the bottom row. We also need to flip the rows within the 4x4 block by flipping the lookup
				// tables. This should be fairly fast as there is no encoding or encoding going on. Width and height
				// will go down to 1x1, which will still use a 4x4 DXT pixel-block.
				if (reverseRowOrderRequested)
				{
					uint8* reversedPixelData = CreateReversedRowData_BC(currPixelData, PixelFormat, numBlocksW, numBlocksH);
					if (reversedPixelData)
					{
						// We can simply get the layer to steal the memory (the last true arg).
						Layers[layer][image] = new tLayer(PixelFormat, width, height, reversedPixelData, true);

						// If we can do one layer, we can do them all -- in all images.
						RowReversalOperationPerformed = true;
					}
					else
					{
						Layers[layer][image] = new tLayer(PixelFormat, width, height, (uint8*)currPixelData);
					}
				}
				else
				{
					// If reverseRowOrder is false we want the data to go straight in so we use the currPixelData directly.
					Layers[layer][image] = new tLayer(PixelFormat, width, height, (uint8*)currPixelData);
				}
				tAssert(Layers[layer][image]->GetDataSize() == numBytes);
			}
			else
			{
				// Upsupported pixel format.
				Clear();
				Results |= 1 << int(ResultCode::Fatal_PixelFormatNotSupported);
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

	// Decode to 32-bit RGBA if requested. If we're already in the correct R8G8B8A8 format, no need to do anything.
	// Note, gamma-correct load flag only applies when decoding HDR/floating-point formats, so never any need to do
	// it on R8G8B8A8. Likewise for spread-flag, never applies to R8G8B8A8 (only R-only or L-only formats)..
	if ((params.Flags & LoadFlag_Decode) && (PixelFormat != tPixelFormat::R8G8B8A8))
	{
		bool spread = params.Flags & LoadFlag_SpreadLuminance;
		bool didRowReversalAfterDecode = false;
		for (int image = 0; image < NumImages; image++)
		{
			for (int layerNum = 0; layerNum < NumMipmapLayers; layerNum++)
			{
				tLayer* layer = Layers[layerNum][image];
				int w = layer->Width;
				int h = layer->Height;
				uint8* src = layer->Data;

				if (tImage::tIsPackedFormat(PixelFormat))
				{
					tPixel* uncompData = new tPixel[w*h];
					switch (layer->PixelFormat)
					{
						case tPixelFormat::A8:
							// Convert to 32-bit RGBA with alpha in A and 0s for RGB.
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4i col(0u, 0u, 0u, src[ij]);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::L8:
						{
							// Convert to 32-bit RGBA with luminance in R and 255 for A. If SpreadLuminance flag set,
							// also set luminance in the GB channels, if not then GB get 0s.
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4i col(src[ij], spread ? src[ij] : 0u, spread ? src[ij] : 0u, 255u);
								uncompData[ij].Set(col);
							}
							break;
						}

						case tPixelFormat::B8G8R8:
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4i col(src[ij*3+2], src[ij*3+1], src[ij*3+0], 255u);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::B8G8R8A8:
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4i col(src[ij*4+2], src[ij*4+1], src[ij*4+0], src[ij*4+3]);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::B5G6R5:
							for (int ij = 0; ij < w*h; ij++)
							{
								// On an LE machine casting to a uint16 effectively swaps the bytes when doing bit ops.
								// This means red will be in the most significant bits -- that's why it looks backwards.
								uint16 u = *((uint16*)(src+ij*2));

								uint8 r = (u         ) >> 11;		// 1111 1000 0000 0000 >> 11.
								uint8 g = (u & 0x07E0) >> 5;		// 0000 0111 1110 0000 >> 5.
								uint8 b = (u & 0x001F)     ;		// 0000 0000 0001 1111 >> 0.

								// Normalize to range.
								// Careful here, you can't just do bit ops to get the components into position.
								// For example, a full red (11111) has to go to 255 (1.0f), and a zero red (00000) to 0(0.0f).
								// That is, the normalize has to divide by the range. At first I just masked and shifted the bits
								// to the right spot in an 8-bit type, but you don't know what to put in the LSBits. Putting 0s
								// would be bad (an 4 bit alpha of 1111 would go to 11110000... suddenly image not fully opaque)
								// and putting all 1s would add red (or alpha or whatever) when there was none. Half way won't
								// work either. You need the endpoints to work.
								float rf = (float(r) / 31.0f);		// Max is 2^5 - 1.
								float gf = (float(g) / 63.0f);		// Max is 2^6 - 1.
								float bf = (float(b) / 31.0f);		// Max is 2^5 - 1.
								tColour4i col(rf, gf, bf, 1.0f);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::B4G4R4A4:
							for (int ij = 0; ij < w*h; ij++)
							{
								uint16 u = *((uint16*)(src+ij*2));
								uint8 a = (u         ) >> 12;		// 1111 0000 0000 0000 >> 12.
								uint8 r = (u & 0x0F00) >> 8;		// 0000 1111 0000 0000 >> 8.
								uint8 g = (u & 0x00F0) >> 4;		// 0000 0000 1111 0000 >> 4.
								uint8 b = (u & 0x000F)     ;		// 0000 0000 0000 1111 >> 0.

								// Normalize to range.
								float af = float(a) / 15.0f;		// Max is 2^4 - 1.
								float rf = float(r) / 15.0f;
								float gf = float(g) / 15.0f;
								float bf = float(b) / 15.0f;

								tColour4i col(rf, gf, bf, af);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::B5G5R5A1:
							for (int ij = 0; ij < w*h; ij++)
							{
								uint16 u = *((uint16*)(src+ij*2));
								bool  a = (u & 0x8000);				// 1000 0000 0000 0000.
								uint8 r = (u & 0x7C00) >> 10;		// 0111 1100 0000 0000 >> 10.
								uint8 g = (u & 0x03E0) >> 5;		// 0000 0011 1110 0000 >> 5.
								uint8 b = (u & 0x001F)     ;		// 0000 0000 0001 1111 >> 0.

								// Normalize to range.
								float rf = float(r) / 31.0f;		// Max is 2^5 - 1.
								float gf = float(g) / 31.0f;
								float bf = float(b) / 31.0f;

								tColour4i col(rf, gf, bf, a ? 1.0f : 0.0f);
								uncompData[ij].Set(col);
							}
							break;

						case tPixelFormat::R16F:
						{
							// This HDR format has 1 red half-float channel.
							tHalf* hdata = (tHalf*)src;
							for (int ij = 0; ij < w*h; ij++)
							{
								float r = hdata[ij*1 + 0];
								tColour4f col(r, spread ? r : 0.0f, spread ? r : 0.0f, 1.0f);
								ProcessHDRFlags(col, spread ? tComp_RGB : tComp_R, params);
								uncompData[ij].Set(col);
							}
							break;
						}

						case tPixelFormat::R16G16F:
						{
							// This HDR format has 2 half-float channels. Red and green.
							tHalf* hdata = (tHalf*)src;
							for (int ij = 0; ij < w*h; ij++)
							{
								float r = hdata[ij*2 + 0];
								float g = hdata[ij*2 + 1];
								tColour4f col(r, g, 0.0f, 1.0f);
								ProcessHDRFlags(col, tComp_RG, params);
								uncompData[ij].Set(col);
							}
							break;
						}

						case tPixelFormat::R16G16B16A16F:
						{
							// This HDR format has 4 half-float channels. RGBA.
							tHalf* hdata = (tHalf*)src;
							for (int ij = 0; ij < w*h; ij++)
							{
								float r = hdata[ij*4 + 0];
								float g = hdata[ij*4 + 1];
								float b = hdata[ij*4 + 2];
								float a = hdata[ij*4 + 3];
								tColour4f col(r, g, b, a);
								ProcessHDRFlags(col, tComp_RGB, params);
								uncompData[ij].Set(col);
							}
							break;
						}

						case tPixelFormat::R32F:
						{
							// This HDR format has 1 red float channel.
							float* fdata = (float*)src;
							for (int ij = 0; ij < w*h; ij++)
							{
								float r = fdata[ij*1 + 0];
								tColour4f col(r, spread ? r : 0.0f, spread ? r : 0.0f, 1.0f);
								ProcessHDRFlags(col, spread ? tComp_RGB : tComp_R, params);
								uncompData[ij].Set(col);
							}
							break;
						}

						case tPixelFormat::R32G32F:
						{
							// This HDR format has 2 float channels. Red and green.
							float* fdata = (float*)src;
							for (int ij = 0; ij < w*h; ij++)
							{
								float r = fdata[ij*2 + 0];
								float g = fdata[ij*2 + 1];
								tColour4f col(r, g, 0.0f, 1.0f);
								ProcessHDRFlags(col, tComp_RG, params);
								uncompData[ij].Set(col);
							}
							break;
						}

						case tPixelFormat::R32G32B32A32F:
						{
							// This HDR format has 4 RGBA floats.
							float* fdata = (float*)src;
							for (int ij = 0; ij < w*h; ij++)
							{
								float r = fdata[ij*4 + 0];
								float g = fdata[ij*4 + 1];
								float b = fdata[ij*4 + 2];
								float a = fdata[ij*4 + 3];
								tColour4f col(r, g, b, a);
								ProcessHDRFlags(col, tComp_RGB, params);
								uncompData[ij].Set(col);
							}
							break;
						}

						default:
							delete[] uncompData;
							Clear();
							Results |= 1 << int(ResultCode::Fatal_PackedDecodeError);
							return false;
					}

					// Decode worked. We are now in RGBA 32-bit. Other params like width and height are already correct.
					delete[] layer->Data;
					layer->Data = (uint8*)uncompData;

					// We are now in in RGBA. In that order in memory.
					layer->PixelFormat = tPixelFormat::R8G8B8A8;
				}
				else if (tImage::tIsBlockCompressedFormat(PixelFormat))
				{
					// We need extra room because the decompressor (bcdec) does not take an input for
					// the width and height, only the pitch (bytes per row). This means a texture that is 5
					// high will actually have row 6, 7, 8 written to.
					int wextra = w + ((w%4) ? 4-(w%4) : 0);
					int hextra = h + ((h%4) ? 4-(h%4) : 0);
					tPixel* uncompData = new tPixel[wextra*hextra];
					switch (layer->PixelFormat)
					{
						case tPixelFormat::BC1DXT1:
						case tPixelFormat::BC1DXT1A:
						{
							for (int i = 0; i < h; i += 4)
								for (int j = 0; j < w; j += 4)
								{
									uint8* dst = (uint8*)uncompData + (i * w + j) * 4;

									// At first didn't understand the pitch (3rd) argument. It's cuz the block needs to be
									// written into multiple rows of the destination... and we need to know how far to get to the
									// next row for each pixel.
									bcdec_bc1(src, dst, w * 4);
									src += BCDEC_BC1_BLOCK_SIZE;
								}
							
							break;
						}

						case tPixelFormat::BC2DXT2DXT3:
						{
							for (int i = 0; i < h; i += 4)
								for (int j = 0; j < w; j += 4)
								{
									uint8* dst = (uint8*)uncompData + (i * w + j) * 4;
									bcdec_bc2(src, dst, w * 4);
									src += BCDEC_BC2_BLOCK_SIZE;
								}
							break;
						}

						case tPixelFormat::BC3DXT4DXT5:
						{
							for (int i = 0; i < h; i += 4)
								for (int j = 0; j < w; j += 4)
								{
									uint8* dst = (uint8*)uncompData + (i * w + j) * 4;
									bcdec_bc3(src, dst, w * 4);
									src += BCDEC_BC3_BLOCK_SIZE;
								}
							break;
						}

						case tPixelFormat::BC4ATI1:
						{
							// This HDR format decompresses to R uint8s.
							uint8* rdata = new uint8[wextra*hextra];

							for (int i = 0; i < h; i += 4)
								for (int j = 0; j < w; j += 4)
								{
									uint8* dst = (rdata + (i * w + j) * 1);
									bcdec_bc4(src, dst, w * 1);
									src += BCDEC_BC4_BLOCK_SIZE;
								}

							// Now convert to 32-bit RGBA.
							for (int ij = 0; ij < w*h; ij++)
							{
								uint8 v = rdata[ij];
								tColour4i col(v, spread ? v : 0u, spread ? v : 0u, 255u);
								uncompData[ij].Set(col);
							}
							delete[] rdata;
							break;
						}

						case tPixelFormat::BC5ATI2:
						{
							struct RG { uint8 R; uint8 G; };
							// This HDR format decompresses to RG uint8s.
							RG* rgData = new RG[wextra*hextra];

							for (int i = 0; i < h; i += 4)
								for (int j = 0; j < w; j += 4)
								{
									uint8* dst = (uint8*)rgData + (i * w + j) * 2;
									bcdec_bc5(src, dst, w * 2);
									src += BCDEC_BC5_BLOCK_SIZE;
								}

							// Now convert to 32-bit RGBA with 0,255 for B,A.
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4i col(rgData[ij].R, rgData[ij].G, 0u, 255u);
								uncompData[ij].Set(col);
							}
							delete[] rgData;
							break;
						}

						case tPixelFormat::BC6S:
						case tPixelFormat::BC6U:
						{
							// This HDR format decompresses to RGB floats.
							tColour3f* rgbData = new tColour3f[wextra*hextra];

							for (int i = 0; i < h; i += 4)
								for (int j = 0; j < w; j += 4)
								{
									uint8* dst = (uint8*)((float*)rgbData + (i * w + j) * 3);
									bool signedData = layer->PixelFormat == tPixelFormat::BC6S;
									bcdec_bc6h_float(src, dst, w * 3, signedData);
									src += BCDEC_BC6H_BLOCK_SIZE;
								}

							// Now convert to 32-bit RGBA with 255 alpha.
							for (int ij = 0; ij < w*h; ij++)
							{
								tColour4f col(rgbData[ij], 1.0f);
								ProcessHDRFlags(col, tComp_RGB, params);
								uncompData[ij].Set(col);
							}
							delete[] rgbData;
							break;
						}

						case tPixelFormat::BC7:
						{
							for (int i = 0; i < h; i += 4)
								for (int j = 0; j < w; j += 4)
								{
									uint8* dst = (uint8*)uncompData + (i * w + j) * 4;
									bcdec_bc7(src, dst, w * 4);
									src += BCDEC_BC7_BLOCK_SIZE;
								}
							break;
						}

						default:
							delete[] uncompData;
							Clear();
							Results |= 1 << int(ResultCode::Fatal_BlockDecodeError);
							return false;
					}

					// Decode worked. We are now in RGBA 32-bit. Other params like width and height are already correct.
					delete[] layer->Data;
					layer->Data = (uint8*)uncompData;
					layer->PixelFormat = tPixelFormat::R8G8B8A8;
				}
				else // Unsupported PixelFormat
				{
					// ASTC Would fall in this category. It's neither a BC format or a normal RGB format.
				}

				// We've got one more chance to reverse the rows here (if we still need to) because we were asked to decode.
				if (reverseRowOrderRequested && !RowReversalOperationPerformed && (layer->PixelFormat == tPixelFormat::R8G8B8A8))
				{
					// This shouldn't ever fail. Too easy to reverse RGBA 32-bit.
					uint8* reversedRowData = tImage::CreateReversedRowData_Packed(layer->Data, layer->PixelFormat, w, h);
					tAssert(reversedRowData);
					delete[] layer->Data;
					layer->Data = reversedRowData;
					didRowReversalAfterDecode = true;
				}
			}
		}

		if (reverseRowOrderRequested && !RowReversalOperationPerformed && didRowReversalAfterDecode)
			RowReversalOperationPerformed = true;

		// All images decoded. Can now set the object's pixel format. We do _not_ set the PixelFormatSrc here!
		PixelFormat = tPixelFormat::R8G8B8A8;
	}

	if (reverseRowOrderRequested && !RowReversalOperationPerformed)
		Results |= 1 << int(ResultCode::Conditional_CouldNotFlipRows);

	ktxTexture_Destroy(texture);
	tAssert(IsValid());

	Results |= 1 << int(ResultCode::Success);
	return true;
}


void tImageKTX::ProcessHDRFlags(tColour4f& colour, tcomps channels, const LoadParams& params)
{
	if (params.Flags & LoadFlag_ToneMapExposure)
		colour.TonemapExposure(params.Exposure, channels);
	if (params.Flags & LoadFlag_SRGBCompression)
		colour.LinearToSRGB(channels);
	if (params.Flags & LoadFlag_GammaCompression)
		colour.LinearToGamma(params.Gamma, channels);
}


const char* tImageKTX::GetResultDesc(ResultCode code)
{
	return ResultDescriptions[int(code)];
}


const char* tImageKTX::ResultDescriptions[] =
{
	"Success",
	"Conditional Success. Image rows could not be flipped.",
	"Conditional Success. Image has dimension not multiple of four.",
	"Conditional Success. Image has dimension not power of two.",
	"Fatal Error. File does not exist.",
	"Fatal Error. Incorrect file type. Must be a KTX or KTX2 file.",
	"Fatal Error. LibKTX could not parse file.",
	"Fatal Error. KTX file version not supported.",
	"Fatal Error. KTX file corrupted.",
	"Fatal Error. Incorrect Dimensions.",
	"Fatal Error. KTX volume textures not supported.",
	"Fatal Error. Unsupported pixel format.",
	"Fatal Error. Invalid pixel data offset.",
	"Fatal Error. Maximum number of mipmap levels exceeded.",
	"Fatal Error. Unable to decode BC pixels.",
	"Fatal Error. Unable to decode packed pixels."
};
tStaticAssert(tNumElements(tImageKTX::ResultDescriptions) == int(tImageKTX::ResultCode::NumCodes));
tStaticAssert(int(tImageKTX::ResultCode::NumCodes) <= int(tImageKTX::ResultCode::MaxCodes));


}