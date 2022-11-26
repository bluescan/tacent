// tImageQOI.cpp
//
// This class knows how to load and save ARM's Adaptive Scalable Texture Compression (.astc) files into tPixel arrays.
// These tPixels may be 'stolen' by the tPicture's constructor if a targa file is specified. After the array is stolen
// the tImageASTC is invalid. This is purely for performance.
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

#include <System/tFile.h>
#include <System/tMachine.h>
#include "Image/tImageASTC.h"
#include "Image/tPixelUtil.h"
#include "Image/tPicture.h"
#include "astcenc.h"
namespace tImage
{


namespace tASTC
{
	struct Header
	{
		uint8_t Magic[4];
		uint8_t BlockW;
		uint8_t BlockH;
		uint8_t BlockD;
		uint8_t DimX[3];
		uint8_t DimY[3];
		uint8_t DimZ[3];
	};
	tStaticAssert(sizeof(Header) == 16);

	tPixelFormat GetFormatFromBlockDimensions(int blockW, int blockH);
	void ProcessHDRFlags(tColour4f& colour, tcomps channels, const tImageASTC::LoadParams& params);
};


tPixelFormat tASTC::GetFormatFromBlockDimensions(int blockW, int blockH)
{
	int pixelsPerBlock = blockW * blockH;
	switch (pixelsPerBlock)
	{
		case 16:	return tPixelFormat::ASTC4X4;
		case 20:	return tPixelFormat::ASTC5X4;
		case 25:	return tPixelFormat::ASTC5X5;
		case 30:	return tPixelFormat::ASTC6X5;
		case 36:	return tPixelFormat::ASTC6X6;
		case 40:	return tPixelFormat::ASTC8X5;
		case 48:	return tPixelFormat::ASTC8X6;
		case 50:	return tPixelFormat::ASTC10X5;
		case 60:	return tPixelFormat::ASTC10X6;
		case 64:	return tPixelFormat::ASTC8X8;
		case 80:	return tPixelFormat::ASTC10X8;
		case 100:	return tPixelFormat::ASTC10X10;
		case 120:	return tPixelFormat::ASTC12X10;
		case 144:	return tPixelFormat::ASTC12X12;
	}
	return tPixelFormat::Invalid;
}


void tASTC::ProcessHDRFlags(tColour4f& colour, tcomps channels, const tImageASTC::LoadParams& params)
{
	if (params.Flags & tImageASTC::LoadFlag_ToneMapExposure)
		colour.TonemapExposure(params.Exposure, channels);
	if (params.Flags & tImageASTC::LoadFlag_SRGBCompression)
		colour.LinearToSRGB(channels);
	if (params.Flags & tImageASTC::LoadFlag_GammaCompression)
		colour.LinearToGamma(params.Gamma, channels);
}


bool tImageASTC::Load(const tString& astcFile, const LoadParams& params)
{
	Clear();

	if (tSystem::tGetFileType(astcFile) != tSystem::tFileType::ASTC)
		return false;

	if (!tSystem::tFileExists(astcFile))
		return false;

	int numBytes = 0;
	uint8* astcFileInMemory = tSystem::tLoadFile(astcFile, nullptr, &numBytes);
	bool success = Load(astcFileInMemory, numBytes, params);
	delete[] astcFileInMemory;

	return success;
}


bool tImageASTC::Load(const uint8* astcInMemory, int numBytes, const LoadParams& params)
{
	Clear();

	// This will deal with zero-sized files properly as well. Basically we need
	// the header and at least one block of data. All ASTC blocks are 16 bytes.
	if (!astcInMemory || (numBytes < (sizeof(tASTC::Header)+16)))
		return false;

	const uint8* astcCurr = astcInMemory;
	tASTC::Header& header = *((tASTC::Header*)astcInMemory);

	// Check the magic.
	if ((header.Magic[0] != 0x13) || (header.Magic[1] != 0xAB) || (header.Magic[2] != 0xA1) || (header.Magic[3] != 0x5C))
		return false;

	int blockW = header.BlockW;
	int blockH = header.BlockH;
	int blockD = header.BlockD;

	// We only support block depth of 1 (2D blocks) -- for now.
	if (blockD != 1)
		return false;

	int width  = header.DimX[0] + (header.DimX[1] << 8) + (header.DimX[2] << 16);
	int height = header.DimY[0] + (header.DimY[1] << 8) + (header.DimY[2] << 16);
	int depth  = header.DimZ[0] + (header.DimZ[1] << 8) + (header.DimZ[2] << 16);

	// We only support depth of 1 (2D images) -- for now.
	if ((width <= 0) || (height <= 0) || (depth > 1))
		return false;

	tPixelFormat format = tASTC::GetFormatFromBlockDimensions(blockW, blockH);
	if (!tIsASTCFormat(format))
		return false;

	PixelFormat = format;
	PixelFormatSrc = format;
	const uint8* astcData = astcInMemory + sizeof(tASTC::Header);
	int astcDataSize = numBytes - sizeof(tASTC::Header);
	tAssert(!Layer);

	// If we were not asked to decode we just get the data over to the Layer and we're done.
	if (!(params.Flags & LoadFlag_Decode))
	{
		Layer = new tLayer(PixelFormat, width, height, (uint8*)astcData);
		return true;
	}

	//
	// We were asked to decode. Decode to RGBA data and then populate a new layer.
	//
	// ASTC Colour Profile.
	astcenc_profile profile = ASTCENC_PRF_LDR_SRGB;
	switch (params.Profile)
	{
		case ColourProfile::LDR:		profile = ASTCENC_PRF_LDR_SRGB;			break;
		case ColourProfile::LDR_FULL:	profile = ASTCENC_PRF_LDR;				break;
		case ColourProfile::HDR:		profile = ASTCENC_PRF_HDR_RGB_LDR_A;	break;
		case ColourProfile::HDR_FULL:	profile = ASTCENC_PRF_HDR;				break;
	}

	// ASTC Config.
	float quality = ASTCENC_PRE_MEDIUM;			// Only need for compression.
	astcenc_error result = ASTCENC_SUCCESS;
	astcenc_config config;
	astcenc_config_init(profile, blockW, blockH, blockD, quality, ASTCENC_FLG_DECOMPRESS_ONLY, &config);
	if (result != ASTCENC_SUCCESS)
		return false;

	// ASTC Context.
	astcenc_context* context = nullptr;
	int numThreads = tMath::tMax(tSystem::tGetNumCores(), 2);
	result = astcenc_context_alloc(&config, numThreads, &context);
	if (result != ASTCENC_SUCCESS)
		return false;

	// ASTC Image.
	tColour4f* uncompData = new tColour4f[width*height];
	astcenc_image image;
	image.dim_x = width;
	image.dim_y = height;
	image.dim_z = depth;
	image.data_type = ASTCENC_TYPE_F32;

	// ASTC Decode.
	tColour4f* slices = uncompData;
	image.data = reinterpret_cast<void**>(&slices);
	astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };
	result = astcenc_decompress_image(context, astcData, astcDataSize, &image, &swizzle, 0);
	if (result != ASTCENC_SUCCESS)
	{
		astcenc_context_free(context);
		delete[] uncompData;
		return false;
	}

	// Convert to 32-bit RGBA and apply any HDR flags.
	tPixel* pixelData = new tPixel[width*height];
	for (int p = 0; p < width*height; p++)
	{
		tColour4f col(uncompData[p]);
		tASTC::ProcessHDRFlags(col, tComp_RGB, params);
		pixelData[p].Set(col);
	}

	// If requested, reverse the row order of the decoded data.
	if (params.Flags & LoadFlag_ReverseRowOrder)
	{
		uint8* reversedRowData = tImage::CreateReversedRowData((uint8*)pixelData, tPixelFormat::R8G8B8A8, width, height);
		tAssert(reversedRowData);
		delete[] pixelData;
		pixelData = (tPixel*)reversedRowData;
	}

	// Give decoded pixelData to layer.
	tAssert(!Layer);
	Layer = new tLayer(tPixelFormat::R8G8B8A8, width, height, (uint8*)pixelData, true);
	tAssert(Layer->OwnsData);

	// ASTC Cleanup.
	astcenc_context_free(context);
	delete[] uncompData;

	// Finally update the current pixel format -- but not the source format.
	PixelFormat = tPixelFormat::R8G8B8A8;
	return true;
}


bool tImageASTC::Set(tPixel* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Layer = new tLayer(tPixelFormat::R8G8B8A8, width, height, (uint8*)pixels, steal);
	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	PixelFormat = tPixelFormat::R8G8B8A8;
	return true;
}


bool tImageASTC::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	tPixel* pixels = frame->GetPixels(steal);
	Set(pixels, frame->Width, frame->Height, steal);
	if (steal)
		delete frame;

	return true;
}


bool tImageASTC::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


tFrame* tImageASTC::StealFrame()
{
	// Data must be decoded for this to work.
	if (!IsValid() || (PixelFormat != tPixelFormat::R8G8B8A8))
		return nullptr;

	tFrame* frame = new tFrame();
	frame->Width = Layer->Width;
	frame->Height = Layer->Height;
	frame->PixelFormatSrc = PixelFormatSrc;
	frame->Pixels = (tPixel*)Layer->StealData();
	delete Layer;
	Layer = nullptr;

	return frame;
}


bool tImageASTC::Save(const tString& astcFile) const
{
	if (!IsValid())
		return false;

	if (tSystem::tGetFileType(astcFile) != tSystem::tFileType::ASTC)
		return false;

	bool success = false;

	// WIP

	return success;
}


bool tImageASTC::IsOpaque() const
{
	if (!IsValid())
		return false;
	
	if (Layer->PixelFormat == tPixelFormat::R8G8B8A8)
	{
		tPixel* pixels = (tPixel*)Layer->Data;
		for (int p = 0; p < (Layer->Width * Layer->Height); p++)
		{
			if (pixels[p].A < 255)
				return false;
		}
	}

	return false;
}


}
