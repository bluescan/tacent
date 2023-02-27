// tTexture.cpp
//
// A tTexture is a 'hardware-ready' format. tTextures contain functionality for creating mipmap layers in a variety of
// block-compressed and uncompressed formats. A tTexture stores each mipmap layer in a tLayer. A tTexture can be
// created from either a tPicture or a dds file. The purpose of a dds file is so that content-creators have control
// over the authoring of each mipmap level and the exact pixel format used. Basically if you've created a dds file,
// you're saying you want the final hardware to use the image data unchanged and as authored -- same mip levels, same
// pixel format, same dimensions. For this reason, dds files should not be loaded into tPictures where image
// manipulation occurs and possibly lossy block-compressed dds images would be decompressed. A dds file may contain more
// than one image if it is a cubemap, but a tTexture only ever represents a single image. The tTexture dds constructor
// allows you to decide which one gets loaded. tTextures can save and load to a tChunk-based format, and are therefore
// useful at both pipeline and for runtime loading. To save to a tChunk file format a tTexture will call the Save
// method of all the tLayers.
//
// Copyright (c) 2006, 2016, 2017, 2020 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Image/tTexture.h>
#define RGBCX_IMPLEMENTATION
#include <BC7Enc/rgbcx.h>
namespace tImage
{


bool tTexture::BC7EncInitialized = false;


bool tTexture::Set(tList<tLayer>& layers)
{
	Clear();
	if (layers.GetNumItems() == 0)
		return false;

	while (!layers.IsEmpty())
		Layers.Append(layers.Remove());

	Opaque = Layers.First()->IsOpaqueFormat();
	return true;
}


bool tTexture::Load(const tString& ddsFile, tImageDDS::tSurfIndex surface, bool correctRowOrder)
{
	Clear();
	if ((tSystem::tGetFileType(ddsFile) != tSystem::tFileType::DDS) || !tSystem::tFileExists(ddsFile))
		return false;
	tImageDDS::LoadParams params;
	params.Flags = tImageDDS::LoadFlag_ReverseRowOrder;
	tImageDDS dds(ddsFile, params);
	if (!dds.IsValid())
		return false;

	return Set(dds, surface);
}


bool tTexture::Set(tImageDDS& dds, tImageDDS::tSurfIndex surface)
{
	Clear();
	if (!dds.IsValid())
		return false;

	if (!dds.IsCubemap())
	{
		dds.StealLayers(Layers);
	}
	else
	{
		tList<tLayer> layerSets[tImageDDS::tSurfIndex_NumSurfaces];
		dds.StealCubemapLayers(layerSets);
		while (!layerSets[surface].IsEmpty())
			Layers.Append(layerSets[surface].Remove());
	}

	if (Layers.GetNumItems() == 0)
		return false;

	Opaque = Layers.First()->IsOpaqueFormat();

	tLayer* mainLayer = Layers.First();
	int width = mainLayer->Width;
	int height = mainLayer->Height;

	if
	(
		!tMath::tInRange(width, tLayer::MinLayerDimension, tLayer::MaxLayerDimension) ||
		!tMath::tInRange(height, tLayer::MinLayerDimension, tLayer::MaxLayerDimension)
	)
	{
		Clear();
		return false;
	}

	return true;
}


/*
bool tTexture::Load(const tString& imageFile, bool generateMipMaps, tPixelFormat format, tQuality quality, int forceWidth, int forceHeight)
{
	Clear();
	tPicture image(imageFile);
	if (!image.IsValid())
		return false;

	return Set(image, generateMipMaps, format, quality, forceWidth, forceHeight);
}
*/


bool tTexture::Set(tPicture& image, bool generateMipmaps, tPixelFormat pixelFormat, tQuality quality, int forceWidth, int forceHeight)
{
	Clear();

	// Sanity check force arguments.
	if (forceWidth && !tMath::tIsPower2(forceWidth))
		throw tError("Texture forceWidth was specified but is not a power of 2.");

	if (forceHeight && !tMath::tIsPower2(forceHeight))
		throw tError("Texture forceHeight was specified but is not a power of 2.");

	// If the dimensions are incorrect we choose the closest power of 2 to resample to. Eg. If the value is 54 we can
	// choose from 32 and 64, but since 32 is 22 away and 64 is only 10, we choose 64.
	int origWidth = image.GetWidth();
	int newWidth = forceWidth ? forceWidth : tMath::tClosestPower2(origWidth);
	tMath::tiClamp(newWidth, tLayer::MinLayerDimension, tLayer::MaxLayerDimension);

	int origHeight = image.GetHeight();
	int newHeight = forceHeight ? forceHeight : tMath::tClosestPower2(origHeight);
	tMath::tiClamp(newHeight, tLayer::MinLayerDimension, tLayer::MaxLayerDimension);

	if ((origWidth != newWidth) || (origHeight != newHeight))
	{
		// Might want to let user know that we're resampling here. This resize happens when the artist didn't submit
		// proper power-of-2-sized images or if dimensions were forced.
		bool ok = image.Resize(newWidth, newHeight, DetermineFilter(quality));
		if (!ok)
			throw tError("Problem resampling texture '%s' to %dx%d.", tSystem::tGetFileBaseName(image.Filename).Pod(), newWidth, newHeight);
	}

	// This must be set before AutoDeterminePixelFormat is called.
	Opaque = image.IsOpaque();

	// Are we supposed to automatically determine the pixel format?
	if (pixelFormat == tPixelFormat::Auto)
		pixelFormat = DeterminePixelFormat(image);

	switch (pixelFormat)
	{
		case tPixelFormat::R8G8B8:
		case tPixelFormat::R8G8B8A8:
			ProcessImageTo_R8G8B8_Or_R8G8B8A8(image, pixelFormat, generateMipmaps, quality);
			break;

		case tPixelFormat::B5G6R5:
			ProcessImageTo_B5G6R5(image, generateMipmaps, quality);
			break;

		case tPixelFormat::BC1DXT1A:
		case tPixelFormat::BC1DXT1:
		case tPixelFormat::BC2DXT2DXT3:
		case tPixelFormat::BC3DXT4DXT5:
			ProcessImageTo_BCTC(image, pixelFormat, generateMipmaps, quality);
			break;

		default:
			throw tError("Conversion of image to pixel format %d failed.", int(pixelFormat));
	}

	// Since the convert functions may or may not modify the source tPicture image, we guarantee invalidness here.
	image.Clear();
	return true;
}


void tTexture::ProcessImageTo_R8G8B8_Or_R8G8B8A8(tPicture& image, tPixelFormat format, bool generateMipmaps, tQuality quality)
{
	tAssert((format == tPixelFormat::R8G8B8) || (format == tPixelFormat::R8G8B8A8));
	int width = image.GetWidth();
	int height = image.GetHeight();
	int bytesPerPixel = (format == tPixelFormat::R8G8B8) ? 3 : 4;
	tResampleFilter filter = DetermineFilter(quality);

	// This loop resamples (reduces) the image multiple times for mipmap generation. In general we should start with
	// the original image every time so that we're not applying interpolations to interpolations (better quality).
	// However, since we are only using a box-filter (pixel averaging) there is no benefit to having a fresh src
	// image each time. The math is equivalent: (a+b/2 + c+d/2)/2 = (a+b+c+d)/4. For now we are saving the extra
	// effort to start with an original every time. If we ever use a more advanced filter we'll need to change this
	// behaviour. Note: we're now using bilinear as the lower quality filter. Should probably make the change.
	while (1)
	{
		int numDataBytes = width*height*bytesPerPixel;
		uint8* layerData = new uint8[numDataBytes];

		// We can just extract the data out directly from RGBA to either RGB or RGBA.
		uint8* srcPixel = (uint8*)image.GetPixelPointer();
		uint8* dstPixel = layerData;
		for (int p = 0; p < width*height; p++)
		{
			tStd::tMemcpy(dstPixel, srcPixel, bytesPerPixel);
			srcPixel += 4;									// Src is always RGBA.
			dstPixel += bytesPerPixel;						// Dst is RGB or RGBA.
		}

		tLayer* layer = new tLayer(format, width, height, layerData, true);
		tAssert(numDataBytes == layer->GetDataSize());
		Layers.Append(layer);

		// Was this the last one?
		if (((width == 1) && (height == 1)) || !generateMipmaps)
			break;

		// Remember, width and height are not necessarily the same. As soon as one reaches 1 it needs to stay there until
		// the other gets there too.
		if (width != 1)
			width >>= 1;

		if (height != 1)
			height >>= 1;

		image.Resize(width, height, filter);
	}
}


void tTexture::ProcessImageTo_B5G6R5(tPicture& image, bool generateMipmaps, tQuality quality)
{
	int width = image.GetWidth();
	int height = image.GetHeight();
	int bytesPerPixel = 2;
	tResampleFilter filter = DetermineFilter(quality);

	// This loop resamples (reduces) the image multiple times for mipmap generation. In general we should start with
	// the original image every time so that we're not applying interpolations to interpolations (better quality).
	// However, since we are only using a box-filter (pixel averaging) there is no benefit to having a fresh src
	// image each time. The math is equivalent: (a+b/2 + c+d/2)/2 = (a+b+c+d)/4. For now we are saving the extra
	// effort to start with an original every time. If we ever use a more advanced filter we'll need to change this
	// behaviour. Note: we're now using bilinear as the lower quality filter. Should probably make the change.
	while (1)
	{
		int numDataBytes = width*height*bytesPerPixel;
		uint8* layerData = new uint8[numDataBytes];

		// We need to change the src data (RGBA) into 16bits.
		tPixel* srcPixel = image.GetPixelPointer();
		uint8* dstPixel = layerData;
		for (int p = 0; p < width*height; p++)
		{
			// In memory. Each letter a bit: GGGBBBBB RRRRRGGG
			dstPixel[0] = (srcPixel->G & 0x1C << 3) | (srcPixel->B >> 3);
			dstPixel[1] = (srcPixel->R & 0xF8) | (srcPixel->G >> 5);
			srcPixel++;
			dstPixel += bytesPerPixel;
		}

		tLayer* layer = new tLayer(tPixelFormat::B5G6R5, width, height, layerData, true);
		tAssert(numDataBytes == layer->GetDataSize());
		Layers.Append(layer);

		// Was this the last one?
		if (((width == 1) && (height == 1)) || !generateMipmaps)
			break;

		// Remember, width and height are not necessarily the same. As soon as one reaches 1 it needs to stay there until
		// the other gets there too.
		if (width != 1)
			width >>= 1;

		if (height != 1)
			height >>= 1;

		image.Resize(width, height, filter);
	}
}


void tTexture::ProcessImageTo_BCTC(tPicture& image, tPixelFormat pixelFormat, bool generateMipmaps, tQuality quality)
{
	int width = image.GetWidth();
	int height = image.GetHeight();
	tResampleFilter filter = DetermineFilter(quality);
	if (!tMath::tIsPower2(width) || !tMath::tIsPower2(height))
		throw tError("Texture must be power-of-2 to be compressed to a BC format.");

	if (!BC7EncInitialized)
	{
		rgbcx::init(rgbcx::bc1_approx_mode::cBC1Ideal);
		BC7EncInitialized = true;
	}

	// This loop resamples (reduces) the image multiple times for mipmap generation. In general we should start with
	// the original image every time so that we're not applying interpolations to interpolations (better quality).
	// However, since we are only using a box-filter (pixel averaging) there is no benefit to having a fresh src
	// image each time. The math is equivalent: (a+b/2 + c+d/2)/2 = (a+b+c+d)/4. For now we are saving the extra
	// effort to start with an original every time. If we ever use a more advanced filter we'll need to change this
	// behaviour. Note: we're now using bilinear as the lower quality filter. Should probably make the change.
	while (1)
	{
		// Setup the layer data to receive the compressed data.
		int numBlocks = tMath::tMax(1, width/4) * tMath::tMax(1, height/4);
		int blockSize = (pixelFormat == tPixelFormat::BC1DXT1) ? 8 : 16;
		int outputSize = numBlocks * blockSize;
		uint8* outputData = new uint8[outputSize];

		int encoderQualityLevel = DetermineBlockEncodeQualityLevel(quality);
		bool allow3colour = true;
		bool useTransparentTexelsForBlack = false;

		uint8* blockDest = outputData;
		uint8* pixelSrc = (uint8*)image.GetPixelPointer();
		for (int block = 0; block < numBlocks; block++)
		{
			switch (pixelFormat)
			{
				case tPixelFormat::BC1DXT1:
					rgbcx::encode_bc1(encoderQualityLevel, blockDest, pixelSrc, allow3colour, useTransparentTexelsForBlack);
					break;

				case tPixelFormat::BC3DXT4DXT5:
					rgbcx::encode_bc3(encoderQualityLevel, blockDest, pixelSrc);
					break;

				default:
					throw tError("Unsupported BC pixel format %d.", int(pixelFormat));
			}
			blockDest += blockSize;
			pixelSrc += sizeof(tPixel);
		}

		// The last true in this call allows the layer constructor to steal the outputData pointer. Avoids extra memcpys.
		tLayer* layer = new tLayer(pixelFormat, width, height, outputData, true);
		tAssert(layer->GetDataSize() == outputSize);
		Layers.Append(layer);

		// Was this the last one?
		if (((width == 1) && (height == 1)) || !generateMipmaps)
			break;

		if (width != 1)
			width >>= 1;

		if (height != 1)
			height >>= 1;

		// When using BC compression we don't ever want to scale lower than 4x4 as that is the individual block size.
		// we need at least that much data so the compressor can do it's job. Consider a 128x4 texture: Ideally we want
		// that to rescale to 64x4, rather than 64x2. So it's reasonable to just stop once either dimension reaches 4
		// because otherwise non-uniform scale issues come into play. In short, we either have to deal with this
		// distortion, or the cropping issue of just stopping. We do the latter because it's just easier.
		//
		// Just because we stop downscaling doesn't mean that we don't generate all the mipmap levels! We still
		// generate all the way to 1x1. It's only the src data that stops being down-sampled.
		if ((image.GetWidth() >= 8) && (image.GetHeight() >= 8))
		{
			// This code scales by half using the correct quality filter.
			int newWidth = image.GetWidth() / 2;
			int newHeight = image.GetHeight() / 2;
			image.Resize(newWidth, newHeight, filter);
		}
	}
}


int tTexture::ComputeMaxNumberOfMipmaps() const
{
	if (!IsValid())
		return 0;

	int maxDim = tMath::tMax(GetWidth(), GetHeight());
	int count = 0;
	while (maxDim > 0)
	{
		maxDim >>= 1;
		count++;
	}

	return count;
}


void tTexture::Save(tChunkWriter& chunk) const
{
	chunk.Begin(tChunkID::Image_Texture);
	{
		chunk.Begin(tChunkID::Image_TextureProperties);
		{
			chunk.Write(Opaque);
		}
		chunk.End();

		chunk.Begin(tChunkID::Image_TextureLayers);
		{
			for (tLayer* layer = Layers.First(); layer; layer = layer->Next())
				layer->Save(chunk);
		}
		chunk.End();
	}
	chunk.End();
}


void tTexture::Load(const tChunk& chunk)
{
	Clear();
	if (chunk.ID() != tChunkID::Image_Texture)
		return;

	int numLayers = 0;
	for (tChunk ch = chunk.First(); ch.IsValid(); ch = ch.Next())
	{
		switch (ch.ID())
		{
			case tChunkID::Image_TextureProperties:
			{
				ch.GetItem(Opaque);
				break;
			}

			case tChunkID::Image_TextureLayers:
			{
				for (tChunk layerChunk = ch.First(); layerChunk.IsValid(); layerChunk = layerChunk.Next())
					Layers.Append(new tLayer(layerChunk));
				break;
			}
		}
	}
}


bool tTexture::operator==(const tTexture& src) const
{
	if (!IsValid() || !src.IsValid())
		return false;

	if (Opaque != src.Opaque)
		return false;

	if (Layers.GetNumItems() != Layers.GetNumItems())
		return false;

	tLayer* srcLayer = Layers.First();
	for (tLayer* layer = Layers.First(); layer; layer = layer->Next(), srcLayer = srcLayer->Next())
		if (*layer != *srcLayer)
			return false;

	return true;
}


}
