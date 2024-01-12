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
// The loading and saving code in here is roughly based on the example code from the LibPNG library. The licence may be
// found in the file Licence_LibPNG.txt.

#include <Foundation/tArray.h>
#include <System/tFile.h>
#include "png.h"
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


bool tImagePNG::Load(const uint8* pngFileInMemory, int numBytes, const LoadParams& params)
{
	Clear();
	if ((numBytes <= 0) || !pngFileInMemory)
		return false;

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

			PixelFormatSrc = tPixelFormat::R8G8B8;
			Width = jpg.GetWidth();
			Height = jpg.GetHeight();
			Pixels = jpg.StealPixels();
			return true;
		}

		return false;
	}

	// This should only return 1 or 2.
	int bytesPerComponent = PNG_IMAGE_SAMPLE_COMPONENT_SIZE(pngImage.format);
	if (bytesPerComponent == 2)
		PixelFormatSrc = (pngImage.format & PNG_FORMAT_FLAG_ALPHA) ? tPixelFormat::R16G16B16A16 : tPixelFormat::R16G16B16;
	else
		PixelFormatSrc = (pngImage.format & PNG_FORMAT_FLAG_ALPHA) ? tPixelFormat::R8G8B8A8 : tPixelFormat::R8G8B8;

	// We need to modify the format to match the type of the pixels we want to load into.
	pngImage.format = PNG_FORMAT_RGBA;
	Width = pngImage.width;
	Height = pngImage.height;

	int numPixels = Width * Height;
	tPixel4* reversedPixels = new tPixel4[numPixels];
	successCode = png_image_finish_read(&pngImage, nullptr, (uint8*)reversedPixels, 0, nullptr);
	if (!successCode)
	{
		png_image_free(&pngImage);
		delete[] reversedPixels;
		Clear();
		return false;
	}

	// Reverse rows.
	Pixels = new tPixel4[numPixels];
	int bytesPerRow = Width*4;
	for (int y = Height-1; y >= 0; y--)
		tStd::tMemcpy((uint8*)Pixels + ((Height-1)-y)*bytesPerRow, (uint8*)reversedPixels + y*bytesPerRow, bytesPerRow);
	delete[] reversedPixels;

	return true;
}


bool tImagePNG::Set(tPixel4* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Width = width;
	Height = height;
	if (steal)
	{
		Pixels = pixels;
	}
	else
	{
		Pixels = new tPixel4[Width*Height];
		tStd::tMemcpy(Pixels, pixels, Width*Height*sizeof(tPixel4));
	}

	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	return true;
}


bool tImagePNG::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

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

	tPixel4* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


tFrame* tImagePNG::GetFrame(bool steal)
{
	if (!IsValid())
		return nullptr;

	tFrame* frame = new tFrame();
	frame->PixelFormatSrc = PixelFormatSrc;

	if (steal)
	{
		frame->StealFrom(Pixels, Width, Height);
		Pixels = nullptr;
	}
	else
	{
		frame->Set(Pixels, Width, Height);
	}

	return frame;
}


tImagePNG::tFormat tImagePNG::Save(const tString& pngFile, tFormat format) const
{
	SaveParams params;
	params.Format = format;
	return Save(pngFile, params);
}


tImagePNG::tFormat tImagePNG::Save(const tString& pngFile, const SaveParams& params) const
{
	if (!IsValid())
		return tFormat::Invalid;

	if (tSystem::tGetFileType(pngFile) != tSystem::tFileType::PNG)
		return tFormat::Invalid;

	int srcBytesPerPixel = 0;
	switch (params.Format)
	{
		case tFormat::Auto:		srcBytesPerPixel = IsOpaque() ? 3 : 4;	break;
		case tFormat::BPP24:	srcBytesPerPixel = 3;					break;
		case tFormat::BPP32:	srcBytesPerPixel = 4;					break;
	}
	if (!srcBytesPerPixel)
		return tFormat::Invalid;

	// Guard against integer overflow.
	if (Height > PNG_SIZE_MAX / (Width * srcBytesPerPixel))
		return tFormat::Invalid;

	// If it's 3 bytes per pixel we use the alternate no-alpha buffer. This should not be necessary
	// but I can't figure out how to get libpng reading 32bit and writing 24.
	uint8* srcPixels = (uint8*)Pixels;
	if (srcBytesPerPixel == 3)
	{
		srcPixels = new uint8[Width*Height*srcBytesPerPixel];
		int dindex = 0;
		for (int p = 0; p < Width*Height; p++)
		{
			srcPixels[dindex++] = Pixels[p].R;
			srcPixels[dindex++] = Pixels[p].G;
			srcPixels[dindex++] = Pixels[p].B;
		}
	}

	FILE* fp = fopen(pngFile.Chr(), "wb");
	if (!fp)
	{
		if (srcBytesPerPixel == 3) delete srcPixels;
		return tFormat::Invalid;
	}

	// Create and initialize the png_struct with the desired error handler functions.
	png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!pngPtr)
	{
		fclose(fp);
		if (srcBytesPerPixel == 3) delete srcPixels;
		return tFormat::Invalid;
	}

	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr)
	{
		fclose(fp);
		png_destroy_write_struct(&pngPtr, 0);
		if (srcBytesPerPixel == 3) delete srcPixels;
		return tFormat::Invalid;
	}

	// Set up default error handling.
	if (setjmp(png_jmpbuf(pngPtr)))
	{
		fclose(fp);
		png_destroy_write_struct(&pngPtr, &infoPtr);
		if (srcBytesPerPixel == 3) delete srcPixels;
		return tFormat::Invalid;
	}

	png_init_io(pngPtr, fp);
	int bitDepth = 8;		// Supported depths are 1, 2, 4, 8, 16.

	// We write either 24 or 32 bit images depending on whether we have an alpha channel.
	uint32 pngColourType = (srcBytesPerPixel == 3) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA;
	png_set_IHDR
	(
		pngPtr, infoPtr, Width, Height, bitDepth, pngColourType,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE
	);

	// Optional significant bit (sBIT) chunk.
	png_color_8 sigBit;
	sigBit.red = 8;
	sigBit.green = 8;
	sigBit.blue = 8;
	sigBit.alpha = (srcBytesPerPixel == 3) ? 0 : 8;
	png_set_sBIT(pngPtr, infoPtr, &sigBit);

	// Optional gamma chunk is strongly suggested if you have any guess as to the correct gamma of the image.
	// png_set_gAMA(pngPtr, infoPtr, 2.2f);

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
		rowPointers[Height-1-r] = srcPixels + r * Width * srcBytesPerPixel;

	// tArray has an implicit cast operator. rowPointers is equivalient to rowPointers.GetElements().
	png_write_image(pngPtr, rowPointers);

	// Finish writing the rest of the file.
	png_write_end(pngPtr, infoPtr);

	// Clear the srcPixels if we created the buffer.
	if (srcBytesPerPixel == 3) delete srcPixels;
	srcPixels = nullptr;

	// Clean up.
	png_destroy_write_struct(&pngPtr, &infoPtr);
	fclose(fp);

	return (pngColourType == PNG_COLOR_TYPE_RGB_ALPHA) ? tFormat::BPP32 : tFormat::BPP24;
}


bool tImagePNG::IsOpaque() const
{
	for (int p = 0; p < (Width*Height); p++)
	{
		if (Pixels[p].A < 255)
			return false;
	}

	return true;
}


tPixel4* tImagePNG::StealPixels()
{
	tPixel4* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	return pixels;
}


}
