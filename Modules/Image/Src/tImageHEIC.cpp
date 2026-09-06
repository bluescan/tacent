// tImageHEIC.cpp
//
// This class knows how to load HEIC files using libheif.
//
// Copyright (c) 2026 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Image/tImageHEIC.h>
#include <System/tFile.h>
#include <Image/tFrame.h>
#include <Image/tPicture.h>
#include <cstring>
#include <cstdint>
#ifdef TACENT_ENABLE_HEIF
#include "libheif/heif.h"
#endif
namespace tImage
{


bool tImageHEIC::Load(const tString& heicFile)
{
	Clear();

	if (!tSystem::tFileExists(heicFile))
		return false;

	int numBytes = 0;
	uint8* heicFileInMemory = tSystem::tLoadFile(heicFile, nullptr, &numBytes);
	bool success = Load(heicFileInMemory, numBytes);
	delete[] heicFileInMemory;

	return success;
}


bool tImageHEIC::Load(const uint8* heicFileInMemory, int numBytes)
{
	Clear();

	if ((numBytes <= 0) || !heicFileInMemory)
		return false;

#ifdef TACENT_ENABLE_HEIF
	// Create libheif context
	struct heif_context* ctx = heif_context_alloc();
	if (!ctx)
		return false;

	// Read the HEIF file
	struct heif_error error = heif_context_read_from_memory(ctx, heicFileInMemory, numBytes, nullptr);
	if (error.code != heif_error_Ok)
	{
		heif_context_free(ctx);
		return false;
	}

	// Get the primary image
	struct heif_image_handle* handle = nullptr;
	error = heif_context_get_primary_image_handle(ctx, &handle);
	if (error.code != heif_error_Ok)
	{
		heif_context_free(ctx);
		return false;
	}

	// Decode the image
	struct heif_image* image = nullptr;
	error = heif_decode_image(handle, &image, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, nullptr);
	if (error.code != heif_error_Ok)
	{
		heif_image_handle_release(handle);
		heif_context_free(ctx);
		return false;
	}

	// Get image properties
	Width = heif_image_get_width(image, heif_channel_interleaved);
	Height = heif_image_get_height(image, heif_channel_interleaved);
	if ((Width <= 0) || (Height <= 0))
	{
		heif_image_release(image);
		heif_image_handle_release(handle);
		heif_context_free(ctx);
		return false;
	}

	// Retrieve the decoded pixel data.
	//
	// Use the non-deprecated heif_image_get_plane2() together with its row
	// stride. The deprecated heif_image_get_plane() returns NULL for a channel
	// that is actually present (observed with the bundled libheif 1.23.2), and
	// the plane's row stride may exceed Width * 4 when the plane is padded, so
	// rows are copied using the stride rather than assuming a tightly packed
	// layout.
	size_t stride = 0;
	const uint8_t* pixel_data = heif_image_get_plane2(image, heif_channel_interleaved, &stride);
	if ((!pixel_data) || (stride < ((size_t)Width * 4u)))
	{
		heif_image_release(image);
		heif_image_handle_release(handle);
		heif_context_free(ctx);
		return false;
	}

	// Allocate memory for pixels
	Pixels = new tPixel4b[Width * Height];

	// Convert from libheif interleaved RGBA (8-bit per channel) to tPixel4b.
	// libheif returns the decoded rows top-to-bottom, but the resulting image
	// ends up upside down, so the rows are populated in reverse order
	// (bottom-up): destination row y receives source row (Height - 1 - y).
	for (int y = 0; y < Height; ++y)
	{
		const uint8_t* srcRow = pixel_data + (size_t)y * stride;
		tPixel4b* dstRow = Pixels + (Height - 1 - y) * Width;
		for (int x = 0; x < Width; ++x)
		{
			const uint8_t* srcPx = srcRow + x * 4u;
			dstRow[x].R = srcPx[0];
			dstRow[x].G = srcPx[1];
			dstRow[x].B = srcPx[2];
			dstRow[x].A = srcPx[3];
		}
	}

	// HEIC files are assumed to be in sRGB.
	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	PixelFormat = tPixelFormat::R8G8B8A8;
	ColourProfileSrc = tColourProfile::sRGB;
	ColourProfile = tColourProfile::sRGB;

	// Cleanup
	heif_image_release(image);
	heif_image_handle_release(handle);
	heif_context_free(ctx);

	return true;
#else
	return false;
#endif
}


bool tImageHEIC::Set(tPixel4b* pixels, int width, int height, bool steal)
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
		Pixels = new tPixel4b[Width * Height];
		memcpy(Pixels, pixels, Width * Height * sizeof(tPixel4b));
	}

	return true;
}


bool tImageHEIC::Set(tFrame* frame, bool steal)
{
	Clear();
	
	if (!frame)
		return false;

	if (!frame->IsValid())
		return false;

	// If steal is true the pixels are moved out of the frame; otherwise they are copied out.
	tPixel4b* pixels = steal ? frame->GetPixels(true) : frame->GetPixels();
	bool success = Set(pixels, frame->Width, frame->Height, steal);
	tAssert(success);
	if (success)
	{
		// We assume the frame is already in the correct pixel format and colour profile.
		PixelFormatSrc = frame->PixelFormatSrc;
		PixelFormat = PixelFormatSrc;
		ColourProfileSrc = tColourProfile::sRGB;
		ColourProfile = ColourProfileSrc;
	}
	return success;
}


bool tImageHEIC::Set(tPicture& picture, bool steal)
{
	Clear();

	if (!picture.IsValid())
		return false;

	// If steal is true the picture becomes invalid and 'Set' steals the pixels; otherwise GetPixels
	// returns them and 'Set' copies them out, so the picture remains valid and no buffer is shared.
	tPixel4b* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	bool success = Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
	tAssert(success);
	if (success)
	{
		// We assume the picture is already in the correct pixel format and colour profile.
		PixelFormatSrc = picture.PixelFormatSrc;
		PixelFormat = PixelFormatSrc;
		ColourProfileSrc = tColourProfile::sRGB;
		ColourProfile = ColourProfileSrc;
	}
	return success;
}


bool tImageHEIC::IsOpaque() const
{
	if (!Pixels)
		return false;

	for (int i = 0; i < Width * Height; ++i)
	{
		if (Pixels[i].A != 255)
			return false;
	}
	
	return true;
}


tFrame* tImageHEIC::GetFrame(bool steal)
{
	if (!Pixels || (Width <= 0) || (Height <= 0))
		return nullptr;

	tFrame* frame = new tFrame();

	if (steal)
	{
		frame->StealFrom(Pixels, Width, Height);
		Pixels = nullptr;
	}
	else
	{
		frame->Set(Pixels, Width, Height);
	}
	frame->PixelFormatSrc = PixelFormat;
	return frame;
}


tPixel4b* tImageHEIC::StealPixels()
{
	tPixel4b* stolenPixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	return stolenPixels;
}


}