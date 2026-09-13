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

	// Read the HEIF file. The _without_copy variant parses directly from the caller's buffer (avoiding a whole-file
	// copy) -- valid here because the buffer is guaranteed to outlive the context (freed after Load() returns).
	struct heif_error error = heif_context_read_from_memory_without_copy(ctx, heicFileInMemory, numBytes, nullptr);
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

	// Extract EXIF and XMP metadata from the container. Failing to parse the metadata is not a load failure -- the
	// image may still decode perfectly.
	PopulateMetaData(handle);

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
	// Use the non-deprecated heif_image_get_plane2() together with its row stride. The deprecated
	// heif_image_get_plane() returns NULL for a channel that is actually present (observed with the bundled
	// libheif 1.23.2), and the plane's row stride may exceed Width * 4 when the plane is padded, so rows are copied
	// using the stride rather than assuming a tightly packed layout.
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

	// Convert from libheif interleaved RGBA (8-bit per channel) to tPixel4b. libheif returns the decoded rows
	// top-to-bottom, but the resulting image ends up upside down, so the rows are populated in reverse order
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


bool tImageHEIC::PopulateMetaData(struct heif_image_handle* handle)
{
#ifdef TACENT_ENABLE_HEIF
	tAssert(handle);

	// Find all metadata blocks attached to the primary image. HEIF containers store EXIF in "Exif" items and XMP in
	// "mime" or "meta" items with the content type "application/rdf+xml". Other items (e.g. ICC colour profiles inside
	// "mime" items) are ignored.
	int numMetaBlocks = heif_image_handle_get_number_of_metadata_blocks(handle, nullptr);
	if (numMetaBlocks <= 0)
		return false;

	heif_item_id* metaBlockIDs = new heif_item_id[numMetaBlocks];
	int numIDs = heif_image_handle_get_list_of_metadata_block_IDs(handle, nullptr, metaBlockIDs, numMetaBlocks);

	// Collect the bare EXIF TIFF / raw XMP XML segments here. Each segment's SegData points directly into the
	// per-block buffer filled by libheif (no copy), and that buffer is recorded as the segment's UserData (the
	// owner), so after the single AddSegments() call below we simply walk the two lists and delete[] each UserData.
	// The lists own the segment objects but not the bytes.
	tList<tMetaData::tMetaSegment> exifSegments, xmpSegments;
	for (int i = 0; i < numIDs; i++)
	{
		const char* itemType = heif_image_handle_get_metadata_type(handle, metaBlockIDs[i]);
		bool isExif = (strcmp(itemType, "Exif") == 0);
		bool isXmp = false;
		if ((!isExif) && (strcmp(itemType, "mime") == 0 || strcmp(itemType, "meta") == 0 || strcmp(itemType, "xmp ") == 0))
		{
			const char* contentType = heif_image_handle_get_metadata_content_type(handle, metaBlockIDs[i]);
			isXmp = (contentType && (strstr(contentType, "rdf+xml") != nullptr));
		}
		if ((!isExif) && (!isXmp))
			continue;

		size_t numBytes = heif_image_handle_get_metadata_size(handle, metaBlockIDs[i]);
		if ((numBytes <= 0) || (numBytes > ((size_t)0x7FFFFFFF)))
			continue;

		uint8* metaBytes = new uint8[numBytes];
		struct heif_error metaError = heif_image_handle_get_metadata(handle, metaBlockIDs[i], metaBytes);
		const uint8* segmentData = nullptr;
		size_t segmentNumBytes = 0;
		if (metaError.code == heif_error_Ok)
		{
			if (isExif)
			{
				// "Exif" item layout: [exif_tiff_header_offset: u32][optional "Exif\0\0"][TIFF]. The offset is measured from the
				// byte just past the 4-byte field (per the EXIF spec), so the TIFF header sits at 4 + off. In practice off is 0
				// for AVIF and 6 for HEIC (whose item carries an "Exif\0\0" prefix). tMetaData wants the bare TIFF.
				uint32 off = (uint32(metaBytes[0]) << 24) | (uint32(metaBytes[1]) << 16) | (uint32(metaBytes[2]) << 8) | uint32(metaBytes[3]);
				size_t tiffStart = (size_t)4 + (size_t)off;
				if ((tiffStart + 8) <= numBytes)
				{
					segmentData = metaBytes + tiffStart;
					segmentNumBytes = numBytes - tiffStart;
				}
			}
			else
			{
				// XMP item layout: [29-byte "http://ns.adobe.com/xap/1.0/\0" namespace prefix][xpacket]. tMetaData wants the
				// raw XML, so strip the namespace prefix when present.
				segmentData = metaBytes;
				segmentNumBytes = numBytes;
				if ((segmentNumBytes > 29) && (tStd::tMemcmp(metaBytes, "http://ns.adobe.com/xap/1.0/\0", 29) == 0))
				{
					segmentData += 29;
					segmentNumBytes -= 29;
				}
			}
		}
		if (segmentNumBytes > 0)
		{
			// No copy is needed: the segment's SegData points directly into the metaBytes buffer, and the same
			// buffer is recorded as the segment's UserData (the owner) so it can be freed after AddSegments() below.
			(isExif ? exifSegments : xmpSegments).Append(new tMetaData::tMetaSegment(segmentData, (int)segmentNumBytes, metaBytes));
		}
		else
		{
			// No segment takes ownership of the buffer -- free it now.
			delete[] metaBytes;
		}
	}
	delete[] metaBlockIDs;

	// Hand all collected segments to tMetaData in one call (which applies EXIF before XMP, so EXIF wins overlaps),
	// then free the underlying per-block buffers via the segment UserDatas (the segment objects are owned by the lists).
	bool found = MetaData.AddSegments(exifSegments, xmpSegments);
	for (tMetaData::tMetaSegment* s = exifSegments.First(); s; s = s->Next())
		delete[] s->UserData;
	for (tMetaData::tMetaSegment* s = xmpSegments.First(); s; s = s->Next())
		delete[] s->UserData;
	return found;

#else
	(void)handle;
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
		tStd::tMemcpy(Pixels, pixels, Width * Height * sizeof(tPixel4b));
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