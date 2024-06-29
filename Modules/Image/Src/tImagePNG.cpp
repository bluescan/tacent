// tImagePNG.cpp
//
// This class knows how to load and save PNG files. It does zero processing of image data. It knows the details of the
// png file format and loads the data into a tPixel array. These tPixels may be 'stolen' by the tPicture's constructor
// if a png file is specified. After the array is stolen the tImagePNG is invalid. This is purely for performance.
//
// Copyright (c) 2020, 2022-2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.
//
// The loading and saving code in here is roughly based on the example code from the LibPNG and SPNG libraries. The
// licences may be found in Licence_LibPNG.txt and Licence_LibSPNG.txt.

#include <Foundation/tArray.h>
#include <System/tFile.h>

// This define chooses between using LibPNG (the original png library) or the slightly cleaner/newer LibSPNG. While the
// interface for SPNG isn't that much different, I did notice SPNG loads 16-bpc PNG files without multiplying the alpha
// channel into the colours -- this is the desired behaviour and so SPNG is enabled. Note that apngasm and apngdis
// depend on LibPNG, so we are keeping LibPNG around until SPNG supports animated PNGs. This is apparently work in
// progress as of 2024.02.06.
#define USE_SPNG_LIBRARY
#ifdef USE_SPNG_LIBRARY
#include "spng.h"
#else
#include "png.h"
#endif

#include "Image/tImagePNG.h"
#include "Image/tImageJPG.h"		// Because some jpg/jfif files have a png extension in the wild. Scary but true.
#include "Image/tPicture.h"
using namespace tSystem;
namespace tImage
{


bool tImagePNG::Load(const tString& pngFile, const LoadParams& params)
{
	Clear();

	if (tSystem::tGetFileType(pngFile) != tSystem::tFileType::PNG)
		return false;

	if (!tFileExists(pngFile))
		return false;

	int numBytes = 0;
	uint8* pngFileInMemory = tLoadFile(pngFile, nullptr, &numBytes);
	bool success = Load(pngFileInMemory, numBytes, params);
	delete[] pngFileInMemory;

	return success;
}


#ifndef USE_SPNG_LIBRARY
bool tImagePNG::Load(const uint8* pngFileInMemory, int numBytes, const LoadParams& paramsIn)
{
	Clear();
	if ((numBytes <= 0) || !pngFileInMemory)
		return false;

	LoadParams params(paramsIn);	
	png_image pngImage;
	tStd::tMemset(&pngImage, 0, sizeof(pngImage));
	pngImage.version = PNG_IMAGE_VERSION;

	int successCode = png_image_begin_read_from_memory(&pngImage, pngFileInMemory, numBytes);
	if (!successCode)
	{
		png_image_free(&pngImage);
		if ((params.Flags & LoadFlag_AllowJPG))
		{
			tImageJPG jpg;
			bool success = jpg.Load(pngFileInMemory, numBytes);
			if (!success)
				return false;

			PixelFormatSrc		= tPixelFormat::R8G8B8;
			PixelFormat			= PixelFormatSrc;
			ColourProfileSrc	= tColourProfile::sRGB;
			ColourProfile		= ColourProfileSrc;
			Width				= jpg.GetWidth();
			Height				= jpg.GetHeight();
			Pixels8				= jpg.StealPixels();
			return true;
		}

		return false;
	}

	// This should only return 1 or 2. If 2 it means the data is in lRGB space (PNG_FORMAT_FLAG_LINEAR).
	int bytesPerComponent = PNG_IMAGE_SAMPLE_COMPONENT_SIZE(pngImage.format);
	if (bytesPerComponent == 2)
		PixelFormatSrc = (pngImage.format & PNG_FORMAT_FLAG_ALPHA) ? tPixelFormat::R16G16B16A16 : tPixelFormat::R16G16B16;
	else
		PixelFormatSrc = (pngImage.format & PNG_FORMAT_FLAG_ALPHA) ? tPixelFormat::R8G8B8A8 : tPixelFormat::R8G8B8;
	PixelFormat = PixelFormatSrc;
	ColourProfileSrc = (bytesPerComponent == 2) ? tColourProfile::lRGB : tColourProfile::sRGB;
	ColourProfile = ColourProfileSrc;

	// Are we being asked to do auto-gamma-compression?
	if (params.Flags & LoadFlag_AutoGamma)
	{
		// Clear all related flags.
		params.Flags &= ~(LoadFlag_AutoGamma | LoadFlag_SRGBCompression | LoadFlag_GammaCompression);
		if (ColourProfileSrc == tColourProfile::lRGB)
			params.Flags |= LoadFlag_SRGBCompression;
	}

	// We need to modify the format to specify what to decode to. If bytesPerComponent is 1 or we are forcing 8bpc we
	// decode into an 8-bpc buffer -- otherwise we decode into a 16-bpc buffer keeping the additional precision.
	pngImage.format = (bytesPerComponent == 1) ? PNG_FORMAT_RGBA : PNG_FORMAT_LINEAR_RGB_ALPHA;
	Width = pngImage.width;
	Height = pngImage.height;

	int numPixels = Width * Height;
	int destBytesPC = (pngImage.format == PNG_FORMAT_RGBA) ? 1 : 2;
	int rawPixelsSize = numPixels * 4 * destBytesPC;
	uint8* rawPixels = new uint8[rawPixelsSize];
	successCode = png_image_finish_read(&pngImage, nullptr, rawPixels, 0, nullptr);
	if (!successCode)
	{
		png_image_free(&pngImage);
		delete[] rawPixels;
		Clear();
		return false;
	}

	// Reverse rows as we copy into our final buffer.
	if (pngImage.format == PNG_FORMAT_RGBA)
	{
		Pixels8 = new tPixel4b[numPixels];
		int bytesPerRow = Width*sizeof(tPixel4b);
		for (int y = Height-1; y >= 0; y--)
			tStd::tMemcpy((uint8*)Pixels8 + ((Height-1)-y)*bytesPerRow, rawPixels + y*bytesPerRow, bytesPerRow);

		// For no reversal we would just do this:
		// tStd::tMemcpy((uint8*)Pixels8, rawPixels, rawPixelsSize);
	}
	else
	{
		Pixels16 = new tPixel4s[numPixels];
		int bytesPerRow = Width*sizeof(tPixel4s);
		for (int y = Height-1; y >= 0; y--)
			tStd::tMemcpy((uint8*)Pixels16 + ((Height-1)-y)*bytesPerRow, rawPixels + y*bytesPerRow, bytesPerRow);

		// For no reversal we would just do this:
		// tStd::tMemcpy((uint8*)Pixels16, rawPixels, rawPixelsSize);
	}
	png_image_free(&pngImage);
	delete[] rawPixels;

	if ((params.Flags & LoadFlag_ForceToBpc8) && Pixels16)
	{
		Pixels8 = new tPixel4b[Width*Height*sizeof(tPixel4b)];

		int dindex = 0; tColour4b c;
		for (int p = 0; p < Width*Height; p++)
		{
			c.Set(Pixels16[p]);
			Pixels8[p].Set(c);
		}
		delete[] Pixels16;
		Pixels16 = nullptr;
	}

	PixelFormat = Pixels8 ? tPixelFormat::R8G8B8A8 : tPixelFormat::R16G16B16A16;

	// Apply gamma or sRGB compression if necessary.
	tAssert(Pixels8 || Pixels16);
	bool flagSRGB = (params.Flags & LoadFlag_SRGBCompression) ? true : false;
	bool flagGama = (params.Flags & LoadFlag_GammaCompression)? true : false;
	if (Pixels8 && (flagSRGB || flagGama))
	{
		for (int p = 0; p < Width*Height; p++)
		{
			tColour4f colour(Pixels8[p]);
			if (flagSRGB)
				colour.LinearToSRGB(tCompBit_RGB);
			if (flagGama)
				colour.LinearToGamma(params.Gamma, tCompBit_RGB);
			Pixels8[p].SetR(colour.R);
			Pixels8[p].SetG(colour.G);
			Pixels8[p].SetB(colour.B);
		}
	}
	else if (Pixels16 && (flagSRGB || flagGama))
	{
		for (int p = 0; p < Width*Height; p++)
		{
			tColour4f colour(Pixels16[p]);
			if (flagSRGB)
				colour.LinearToSRGB(tCompBit_RGB);
			if (flagGama)
				colour.LinearToGamma(params.Gamma, tCompBit_RGB);
			Pixels16[p].SetR(colour.R);
			Pixels16[p].SetG(colour.G);
			Pixels16[p].SetB(colour.B);
		}
	}

	if (params.Flags & LoadFlag_SRGBCompression)  ColourProfile = tColourProfile::sRGB;
	if (params.Flags & LoadFlag_GammaCompression) ColourProfile = tColourProfile::gRGB;

	return true;
}
#endif


#ifdef USE_SPNG_LIBRARY
bool tImagePNG::Load(const uint8* pngFileInMemory, int numBytes, const LoadParams& paramsIn)
{
	Clear();
	if ((numBytes <= 0) || !pngFileInMemory)
		return false;

	LoadParams params(paramsIn);

	spng_ctx* ctx = spng_ctx_new(0);
	if (!ctx)
		return false;

	// Ignore and don't calculate chunk CRCs.
	spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);

	// Set memory usage limits for storing standard and unknown chunks. This is important when reading untrusted files.
	size_t limit = 1024 * 1024 * 64;
	spng_set_chunk_limits(ctx, limit, limit);

	// Tell the context about the source png image.
	spng_set_png_buffer(ctx, pngFileInMemory, numBytes);

	struct spng_ihdr ihdr;
	int errCode = spng_get_ihdr(ctx, &ihdr);
	if (errCode)
	{
		spng_ctx_free(ctx);
		if ((params.Flags & LoadFlag_AllowJPG))
		{
			tImageJPG jpg;
			bool success = jpg.Load(pngFileInMemory, numBytes);
			if (!success)
				return false;

			PixelFormatSrc		= tPixelFormat::R8G8B8;
			PixelFormat			= PixelFormatSrc;
			ColourProfileSrc	= tColourProfile::sRGB;
			ColourProfile		= ColourProfileSrc;
			Width				= jpg.GetWidth();
			Height				= jpg.GetHeight();
			Pixels8				= jpg.StealPixels();
			return true;
		}

		return false;
	}

	int bitDepth = ihdr.bit_depth;
	bool hasAlpha = (ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE_ALPHA) || (ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR_ALPHA);
	Width = ihdr.width;
	Height = ihdr.height;
	int numPixels = Width * Height;

	// If the src bit depth is 16, RGBA are all linear. Otherwise RGB are  sRGB and A is linear.
	if (bitDepth == 16)
		PixelFormatSrc = hasAlpha ? tPixelFormat::R16G16B16A16 : tPixelFormat::R16G16B16;
	else
		PixelFormatSrc = hasAlpha ? tPixelFormat::R8G8B8A8 : tPixelFormat::R8G8B8;
	PixelFormat = PixelFormatSrc;
	ColourProfileSrc = (bitDepth == 16) ? tColourProfile::lRGB : tColourProfile::sRGB;
	ColourProfile = ColourProfileSrc;

	// Are we being asked to do auto-gamma-compression?
	if (params.Flags & LoadFlag_AutoGamma)
	{
		// Clear all related flags.
		params.Flags &= ~(LoadFlag_AutoGamma | LoadFlag_SRGBCompression | LoadFlag_GammaCompression);
		if (ColourProfileSrc == tColourProfile::lRGB)
			params.Flags |= LoadFlag_SRGBCompression;
	}

	struct spng_plte plte = { 0 };
	errCode = spng_get_plte(ctx, &plte);
	if (errCode && (errCode != SPNG_ECHUNKAVAIL))
	{
		spng_ctx_free(ctx);
		Clear();
		return false;
	}

	// Output format, does not depend on source PNG format except for SPNG_FMT_PNG, which is the PNGs format in
	// host-endian (or big-endian for SPNG_FMT_RAW). Note that for these two formats <8-bit images are left byte-packed.
	// Here we decode to a 16 bit buffer if the src is 16 bit to keep full precision.
	int fmt = (bitDepth == 16) ? SPNG_FMT_RGBA16 : SPNG_FMT_RGBA8;

	// With SPNG_FMT_PNG indexed color images are output as palette indices, pick another format to expand them.
	// if (ihdr.color_type == SPNG_COLOR_TYPE_INDEXED)
	//	 fmt = SPNG_FMT_RGBA8;
	size_t rawPixelsSize = 0;
	errCode = spng_decoded_image_size(ctx, fmt, &rawPixelsSize);
	if (errCode)
	{
		spng_ctx_free(ctx);
		Clear();
		return false;
	}

	uint8* rawPixels = new uint8[rawPixelsSize];

	// Decode the image in one go.
	errCode = spng_decode_image(ctx, rawPixels, rawPixelsSize, fmt, 0);
	if (errCode)
	{
		delete[] rawPixels;
		spng_ctx_free(ctx);
		Clear();
		return false;
	}

	// Reverse rows as we copy into our final buffer.
	if (fmt == SPNG_FMT_RGBA8)
	{
		Pixels8 = new tPixel4b[numPixels];
		int bytesPerRow = Width*sizeof(tPixel4b);
		for (int y = Height-1; y >= 0; y--)
			tStd::tMemcpy((uint8*)Pixels8 + ((Height-1)-y)*bytesPerRow, rawPixels + y*bytesPerRow, bytesPerRow);
	}
	else
	{
		Pixels16 = new tPixel4s[numPixels];
		int bytesPerRow = Width*sizeof(tPixel4s);
		for (int y = Height-1; y >= 0; y--)
			tStd::tMemcpy((uint8*)Pixels16 + ((Height-1)-y)*bytesPerRow, rawPixels + y*bytesPerRow, bytesPerRow);
	}
	delete[] rawPixels;
	spng_ctx_free(ctx);

	if ((params.Flags & LoadFlag_ForceToBpc8) && Pixels16)
	{
		Pixels8 = new tPixel4b[Width*Height*sizeof(tPixel4b)];

		int dindex = 0; tColour4b c;
		for (int p = 0; p < Width*Height; p++)
		{
			c.Set(Pixels16[p]);
			Pixels8[p].Set(c);
		}
		delete[] Pixels16;
		Pixels16 = nullptr;
	}

	PixelFormat = Pixels8 ? tPixelFormat::R8G8B8A8 : tPixelFormat::R16G16B16A16;

	// Apply gamma or sRGB compression if necessary.
	tAssert(Pixels8 || Pixels16);
	bool flagSRGB = (params.Flags & LoadFlag_SRGBCompression) ? true : false;
	bool flagGama = (params.Flags & LoadFlag_GammaCompression)? true : false;
	if (Pixels8 && (flagSRGB || flagGama))
	{
		for (int p = 0; p < Width*Height; p++)
		{
			tColour4f colour(Pixels8[p]);
			if (flagSRGB)
				colour.LinearToSRGB(tCompBit_RGB);
			if (flagGama)
				colour.LinearToGamma(params.Gamma, tCompBit_RGB);
			Pixels8[p].SetR(colour.R);
			Pixels8[p].SetG(colour.G);
			Pixels8[p].SetB(colour.B);
		}
	}
	else if (Pixels16 && (flagSRGB || flagGama))
	{
		for (int p = 0; p < Width*Height; p++)
		{
			tColour4f colour(Pixels16[p]);
			if (flagSRGB)
				colour.LinearToSRGB(tCompBit_RGB);
			if (flagGama)
				colour.LinearToGamma(params.Gamma, tCompBit_RGB);
			Pixels16[p].SetR(colour.R);
			Pixels16[p].SetG(colour.G);
			Pixels16[p].SetB(colour.B);
		}
	}

	if (params.Flags & LoadFlag_SRGBCompression)  ColourProfile = tColourProfile::sRGB;
	if (params.Flags & LoadFlag_GammaCompression) ColourProfile = tColourProfile::gRGB;
	return true;
}
#endif


bool tImagePNG::Set(tPixel4b* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Width = width;
	Height = height;
	if (steal)
	{
		Pixels8 = pixels;
	}
	else
	{
		Pixels8 = new tPixel4b[Width*Height];
		tStd::tMemcpy(Pixels8, pixels, Width*Height*sizeof(tPixel4b));
	}

	PixelFormatSrc		= tPixelFormat::R8G8B8A8;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume 4-byte pixels must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

	return true;
}


bool tImagePNG::Set(tPixel4s* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Width = width;
	Height = height;
	if (steal)
	{
		Pixels16 = pixels;
	}
	else
	{
		Pixels16 = new tPixel4s[Width*Height];
		tStd::tMemcpy(Pixels16, pixels, Width*Height*sizeof(tPixel4s));
	}

	PixelFormatSrc		= tPixelFormat::R16G16B16A16;
	PixelFormat			= tPixelFormat::R16G16B16A16;
	ColourProfileSrc	= tColourProfile::HDRa;		// We assume 4-short pixels must be HDRa.
	ColourProfile		= tColourProfile::HDRa;

	return true;
}


bool tImagePNG::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	PixelFormatSrc		= frame->PixelFormatSrc;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume frame must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

	Set(frame->GetPixels(steal), frame->Width, frame->Height, steal);
	if (steal)
		delete frame;

	return true;
}


bool tImagePNG::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	PixelFormatSrc		= picture.PixelFormatSrc;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	// We don't know colour profile of tPicture.

	// This is worth some explanation. If steal is true the picture becomes invalid and the
	// 'set' call will steal the stolen pixels. If steal is false GetPixels is called and the
	// 'set' call will memcpy them out... which makes sure the picture is still valid after and
	// no-one is sharing the pixel buffer. We don't check the success of 'set' because it must
	// succeed if picture was valid.
	tPixel4b* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	bool success = Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
	tAssert(success);
	return true;
}


tFrame* tImagePNG::GetFrame(bool steal)
{
	if (!IsValid())
		return nullptr;

	tFrame* frame = new tFrame();
	frame->PixelFormatSrc = PixelFormatSrc;

	if (steal)
	{
		frame->StealFrom(Pixels8, Width, Height);
		Pixels8 = nullptr;
	}
	else
	{
		frame->Set(Pixels8, Width, Height);
	}

	return frame;
}


tImagePNG::tFormat tImagePNG::Save(const tString& pngFile, tFormat format) const
{
	SaveParams params;
	params.Format = format;
	return Save(pngFile, params);
}


#ifndef USE_SPNG_LIBRARY
tImagePNG::tFormat tImagePNG::Save(const tString& pngFile, const SaveParams& params) const
{
	if (!IsValid())
		return tFormat::Invalid;

	if (tSystem::tGetFileType(pngFile) != tSystem::tFileType::PNG)
		return tFormat::Invalid;

	int dstBytesPerPixel = 0;

	switch (params.Format)
	{
		case tFormat::BPP24_RGB_BPC8:	dstBytesPerPixel = 3;	break;
		case tFormat::BPP32_RGBA_BPC8:	dstBytesPerPixel = 4;	break;
		case tFormat::BPP48_RGB_BPC16:	dstBytesPerPixel = 6;	break;
		case tFormat::BPP64_RGBA_BPC16:	dstBytesPerPixel = 8;	break;
		case tFormat::Auto:
			dstBytesPerPixel = IsOpaque() ? 3 : 4;
			if (Pixels16) dstBytesPerPixel <<= 1;
			break;
	}
	if (!dstBytesPerPixel)
		return tFormat::Invalid;

	// Guard against integer overflow when saving.
	if (Height > PNG_SIZE_MAX / (Width * dstBytesPerPixel))
		return tFormat::Invalid;

	// If it's 3 or 6 bytes per pixel we make a no-alpha-channel buffer. This should not be
	// necessary but I can't figure out how to get libpng reading 32bit/64bit and writing 24/48.
	uint8* pixelData = new uint8[Width*Height*dstBytesPerPixel];

	switch (dstBytesPerPixel)
	{
		case 3:
		{
			int dindex = 0; tColour4b c;
			for (int p = 0; p < Width*Height; p++)
			{
				if (Pixels8) c.Set(Pixels8[p]); else c.Set(Pixels16[p]);
				pixelData[dindex++] = c.R;
				pixelData[dindex++] = c.G;
				pixelData[dindex++] = c.B;
			}
			break;
		}

		case 4:
			if (Pixels8)
			{
				tStd::tMemcpy(pixelData, Pixels8, Width*Height*4);
			}
			else
			{
				int dindex = 0; tColour4b c;
				for (int p = 0; p < Width*Height; p++)
				{
					c.Set(Pixels16[p]);
					pixelData[dindex++] = c.R;
					pixelData[dindex++] = c.G;
					pixelData[dindex++] = c.B;
					pixelData[dindex++] = c.A;
				}
			}
			break;

		case 6:
		{
			int dindex = 0; tColour4s c; uint16* pdata = (uint16*)pixelData;
			for (int p = 0; p < Width*Height; p++)
			{				
				if (Pixels16) c.Set(Pixels16[p]); else c.Set(Pixels8[p]);
				pdata[dindex++] = tSwapEndian16(c.R);
				pdata[dindex++] = tSwapEndian16(c.G);
				pdata[dindex++] = tSwapEndian16(c.B);
			}
			break;
		}

		case 8:
		{
			int dindex = 0; tColour4s c; uint16* pdata = (uint16*)pixelData;
			for (int p = 0; p < Width*Height; p++)
			{
				if (Pixels16) c.Set(Pixels16[p]); else c.Set(Pixels8[p]);
				pdata[dindex++] = tSwapEndian16(c.R);
				pdata[dindex++] = tSwapEndian16(c.G);
				pdata[dindex++] = tSwapEndian16(c.B);
				pdata[dindex++] = tSwapEndian16(c.A);
			}
			break;
		}
	}

	FILE* fp = fopen(pngFile.Chr(), "wb");
	if (!fp)
	{
		delete[] pixelData;
		return tFormat::Invalid;
	}

	// Create and initialize the png_struct with the desired error handler functions.
	png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!pngPtr)
	{
		fclose(fp);
		delete[] pixelData;
		return tFormat::Invalid;
	}

	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr)
	{
		fclose(fp);
		png_destroy_write_struct(&pngPtr, 0);
		delete[] pixelData;
		return tFormat::Invalid;
	}

	// Set up default error handling.
	if (setjmp(png_jmpbuf(pngPtr)))
	{
		fclose(fp);
		png_destroy_write_struct(&pngPtr, &infoPtr);
		delete[] pixelData;
		return tFormat::Invalid;
	}

	png_init_io(pngPtr, fp);

	// Supported depths are 1, 2, 4, 8, 16. We support 8 and 16.
	int bitDepth = (dstBytesPerPixel <= 4) ? 8 : 16;

	// We write either 24/32 or 48/64 bit images depending on whether we have an alpha channel.
	uint32 pngColourType = ((dstBytesPerPixel == 3) || (dstBytesPerPixel == 6)) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA;
	png_set_IHDR
	(
		pngPtr, infoPtr, Width, Height, bitDepth, pngColourType,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE
	);

	// The sBIT chunk tells the decoder the number of significant bits in the pixel data. It is optional
	// as the data is still stored as either 8 or 16 bits per component,
	png_color_8 sigBit;
	sigBit.red		= bitDepth;
	sigBit.green	= bitDepth;
	sigBit.blue		= bitDepth;
	sigBit.alpha	= ((dstBytesPerPixel == 3) || (dstBytesPerPixel == 6)) ? 0 : bitDepth;
	png_set_sBIT(pngPtr, infoPtr, &sigBit);

	png_write_info(pngPtr, infoPtr);

	// Shift the pixels up to a legal bit depth and fill in as appropriate to correctly scale the image.
	// png_set_shift(pngPtr, &sigBit);
	//
	// Pack pixels into bytes.
	// png_set_packing(pngPtr);
	//
	// Swap location of alpha bytes from ARGB to RGBA.
	// png_set_swap_alpha(pngPtr);
	//
	// Get rid of filler (OR ALPHA) bytes, pack XRGB/RGBX/ARGB/RGBA into RGB (4 channels -> 3 channels). The second parameter is not used.
	// png_set_filler(pngPtr, 0, PNG_FILLER_BEFORE);
	//
	// png_set_strip_alpha(pngPtr);
	//
	// Flip BGR pixels to RGB.
	// png_set_bgr(pngPtr);
	//
	// Swap bytes of 16-bit files to most significant byte first.
	// png_set_swap(pngPtr);
	tArray<png_bytep> rowPointers(Height);

	// Set up pointers into the src data.
	for (int r = 0; r < Height; r++)
		rowPointers[Height-1-r] = pixelData + r * Width * dstBytesPerPixel;

	// tArray has an implicit cast operator. rowPointers is equivalient to rowPointers.GetElements().
	png_write_image(pngPtr, rowPointers);

	// Finish writing the rest of the file.
	png_write_end(pngPtr, infoPtr);

	// Clear the srcPixels if we created the buffer.
	delete[] pixelData;
	pixelData = nullptr;

	// Clean up.
	png_destroy_write_struct(&pngPtr, &infoPtr);
	fclose(fp);

	switch (dstBytesPerPixel)
	{
		case 3:		return tFormat::BPP24_RGB_BPC8;
		case 4:		return tFormat::BPP32_RGBA_BPC8;
		case 6:		return tFormat::BPP48_RGB_BPC16;
		case 8:		return tFormat::BPP64_RGBA_BPC16;
	}

	return tFormat::Invalid;
}
#endif


#ifdef USE_SPNG_LIBRARY
tImagePNG::tFormat tImagePNG::Save(const tString& pngFile, const SaveParams& params) const
{
	if (!IsValid())
		return tFormat::Invalid;

	if (tSystem::tGetFileType(pngFile) != tSystem::tFileType::PNG)
		return tFormat::Invalid;

	int bytesPerPixel = 0;
	switch (params.Format)
	{
		case tFormat::BPP24_RGB_BPC8:	bytesPerPixel = 3;	break;
		case tFormat::BPP32_RGBA_BPC8:	bytesPerPixel = 4;	break;
		case tFormat::BPP48_RGB_BPC16:	bytesPerPixel = 6;	break;
		case tFormat::BPP64_RGBA_BPC16:	bytesPerPixel = 8;	break;
		case tFormat::Auto:
			bytesPerPixel = IsOpaque() ? 3 : 4;
			if (Pixels16) bytesPerPixel <<= 1;
			break;
	}
	if (!bytesPerPixel)
		return tFormat::Invalid;

	// If it's 3 or 6 bytes per pixel we make a no-alpha-channel buffer. Basically we need the data
	// in the correct layout before we save it. SPNG does not do it for us.
	// The pixels need to be reordered so that the first row is at the top.
	uint8* pixelData = new uint8[Width*Height*bytesPerPixel];
	switch (bytesPerPixel)
	{
		case 3:
		{
			int dindex = 0; tColour4b c;
			for (int y = Height-1; y >= 0; y--)
				for (int x = 0; x < Width; x++)
				{
					int p = y*Width + x;
					if (Pixels8) c.Set(Pixels8[p]); else c.Set(Pixels16[p]);
					pixelData[dindex++] = c.R;
					pixelData[dindex++] = c.G;
					pixelData[dindex++] = c.B;
				}
			break;
		}

		case 4:
		{
			int dindex = 0; tColour4b c;
			for (int y = Height-1; y >= 0; y--)
				for (int x = 0; x < Width; x++)
				{
					int p = y*Width + x;
					if (Pixels8) c.Set(Pixels8[p]); else c.Set(Pixels16[p]);
					pixelData[dindex++] = c.R;
					pixelData[dindex++] = c.G;
					pixelData[dindex++] = c.B;
					pixelData[dindex++] = c.A;
				}
			break;
		}

		case 6:
		{
			int dindex = 0; tColour4s c; uint16* pdata = (uint16*)pixelData;
			for (int y = Height-1; y >= 0; y--)
				for (int x = 0; x < Width; x++)
				{
					int p = y*Width + x;
					if (Pixels16) c.Set(Pixels16[p]); else c.Set(Pixels8[p]);
					pdata[dindex++] = c.R;
					pdata[dindex++] = c.G;
					pdata[dindex++] = c.B;
				}
			break;
		}

		case 8:
		{
			int dindex = 0; tColour4s c; uint16* pdata = (uint16*)pixelData;
			for (int y = Height-1; y >= 0; y--)
				for (int x = 0; x < Width; x++)
				{
					int p = y*Width + x;
					if (Pixels16) c.Set(Pixels16[p]); else c.Set(Pixels8[p]);
					pdata[dindex++] = c.R;
					pdata[dindex++] = c.G;
					pdata[dindex++] = c.B;
					pdata[dindex++] = c.A;
				}
			break;
		}
	}

	FILE* fp = fopen(pngFile.Chr(), "wb");
	if (!fp)
	{
		delete[] pixelData;
		return tFormat::Invalid;
	}

	// Creating an encoder context requires a flag.
	spng_ctx* ctx = spng_ctx_new(SPNG_CTX_ENCODER);

	// Don't encode to internal buffer managed by the library. We'll be writing to a file.
	spng_set_option(ctx, SPNG_ENCODE_TO_BUFFER, 0);
	spng_set_png_file(ctx, fp);
	spng_set_option(ctx, SPNG_FILTER_CHOICE, SPNG_DISABLE_FILTERING);

	// Set image properties, this determines the destination image format. Start by zero-initing ihdr.
	struct spng_ihdr ihdr = { 0 };
	ihdr.width = Width;
	ihdr.height = Height;

	// See https://www.w3.org/TR/2003/REC-PNG-20031110/#table111 for valid color-type/bit-depth combinations.
	switch (bytesPerPixel)
	{
		case 3: ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR;		ihdr.bit_depth = 8;		break;
		case 4: ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;	ihdr.bit_depth = 8;		break;
		case 6: ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR;		ihdr.bit_depth = 16;	break;
		case 8: ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;	ihdr.bit_depth = 16;	break;
	}
	spng_set_ihdr(ctx, &ihdr);

	// This is the source data format. SPNG_FMT_PNG is a special value that matches the format in ihdr
	// The encode call only works if the format is SPNG_FMT_PNG (machine-endian) or SPNG_FMT_RAW (big-endian).
	// SPNG_ENCODE_FINALIZE will finalize the PNG with the end-of-file marker.
	int errCode = spng_encode_image(ctx, pixelData, Width*Height*bytesPerPixel, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
	if (errCode)
	{
		fclose(fp);
		spng_ctx_free(ctx);
		delete[] pixelData;
		return tFormat::Invalid;
	}

	fclose(fp);
	spng_ctx_free(ctx);
	delete[] pixelData;

	switch (bytesPerPixel)
	{
		case 3:		return tFormat::BPP24_RGB_BPC8;
		case 4:		return tFormat::BPP32_RGBA_BPC8;
		case 6:		return tFormat::BPP48_RGB_BPC16;
		case 8:		return tFormat::BPP64_RGBA_BPC16;
	}

	return tFormat::Invalid;
}
#endif


bool tImagePNG::IsOpaque() const
{
	if (Pixels8)
	{
		for (int p = 0; p < (Width*Height); p++)
		{
			if (Pixels8[p].A < 255)
				return false;
		}
	}
	else if (Pixels16)
	{
		for (int p = 0; p < (Width*Height); p++)
		{
			if (Pixels16[p].A < 65535)
				return false;
		}
	}

	return true;
}


tPixel4b* tImagePNG::StealPixels8()
{
	if (!Pixels8)
		return nullptr;

	tPixel4b* pixels = Pixels8;
	Pixels8 = nullptr;
	Width = 0;
	Height = 0;
	return pixels;
}


tPixel4s* tImagePNG::StealPixels16()
{
	if (!Pixels16)
		return nullptr;

	tPixel4s* pixels = Pixels16;
	Pixels16 = nullptr;
	Width = 0;
	Height = 0;
	return pixels;
}


}
