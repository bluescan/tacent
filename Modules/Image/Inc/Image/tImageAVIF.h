// tImageAVIF.h
//
// This class knows how to load AVIF files using libheif.
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

#pragma once
#include <Foundation/tString.h>
#include <Math/tColour.h>
#include "Image/tPixelFormat.h"
#include "Image/tMetaData.h"
#include "Image/tBaseImage.h"
#include "Image/tUtilHEIF.h"


// Opaque libheif image handle (defined in the global namespace by <libheif/heif.h>). It is forward-declared here so
// that PopulateMetaData can be declared without including the libheif headers.
struct heif_image_handle;
namespace tImage
{


class tImageAVIF : public tBaseImage
{
public:
	enum LoadFlags
	{
		LoadFlag_None			= 0,

		// Do not decompress image. Loads as a memory image only. Lossless flip/rotate transforms can only be called if
		// NoDecompress is set.
		LoadFlag_NoDecompress	= 1 << 0,
		LoadFlags_Default		= LoadFlag_None
	};

	struct LoadParams
	{
		LoadParams()																									{ Reset(); }
		LoadParams(const LoadParams& src)																				: Flags(src.Flags) { }
		void Reset()																									{ Flags = LoadFlags_Default; }
		LoadParams& operator=(const LoadParams& src)																	{ Flags = src.Flags; return *this; }
		uint32 Flags;
	};

	// Creates an invalid tImageAVIF. You must call Load manually.
	tImageAVIF()																										{ }
	tImageAVIF(const tString& avifFile, const LoadParams& params = LoadParams())										{ Load(avifFile, params); }

	// The data is copied out of avifFileInMemory. Go ahead and delete[] after if you want.
	tImageAVIF(const uint8* avifFileInMemory, int numBytes, const LoadParams& params = LoadParams())					{ Load(avifFileInMemory, numBytes, params); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out. Sets the colour space to sRGB. Call SetColourSpace after if you wanted linear.
	tImageAVIF(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageAVIF(tFrame* frame, bool steal = true)																		{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImageAVIF(tPicture& picture, bool steal = true)																	{ Set(picture, steal); }

	virtual ~tImageAVIF()																								{ Clear(); }

	// Clears the current tImageAVIF before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& avifFile, const LoadParams& params = LoadParams());
	bool Load(const uint8* avifFileInMemory, int numBytes, const LoadParams& params = LoadParams());

	bool Set(tPixel4b* pixels, int width, int height, bool steal = false) override;
	bool Set(tFrame*, bool steal = true) override;
	bool Set(tPicture& picture, bool steal = true) override;

	// Saves the tImageAVIF to the file specified. The file extension must be AVIF. If the tImageAVIF was loaded with
	// LoadFlag_NoDecompress, the original container bytes are written to disk (lossless). Otherwise this returns
	// false, since there is no AVIF encoder available.
	bool Save(const tString& avifFile) const;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return (Pixels || MemImage) ? true : false; }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }
	bool IsOpaque() const;

	// A lossless transform is one where the compressed container data is untouched and only the orientation metadata
	// is rewritten. This is only possible when the image was loaded with LoadFlag_NoDecompress (i.e. the original
	// bytes are still held in memory). All 8 D4 orientations (HEIFTransform) are supported; Identity is a no-op that
	// reports success. If false is returned the object is left unchanged and the image is invalid.
	bool LosslessTransform(HEIFTransform);

	// After this call you are the owner of the pixels and must eventually delete[] them. This tImageAVIF object is
	// invalid afterwards (unless it also holds a memory image, in which case it remains valid).
	tPixel4b* StealPixels();
	tFrame* GetFrame(bool steal = true) override;
	tPixel4b* GetPixels() const																							{ return Pixels; }

	// A place to store EXIF and XMP metadata. AVIF files often contain this metadata. This field is not populated if
	// the NoDecompress flag was used during load.
	tMetaData MetaData;

private:
	// Populates MetaData from the EXIF and XMP metadata blocks found inside the HEIF container. Returns true if at
	// least one metadata block was recognized and parsed.
	bool PopulateMetaData(struct heif_image_handle*);

	int Width			= 0;
	int Height			= 0;
	tPixel4b* Pixels	= nullptr;
	uint8* MemImage		= nullptr;
	int MemImageSize	= 0;
};


inline void tImageAVIF::Clear()
{
	Width = 0;
	Height = 0;
	delete[] Pixels;
	Pixels = nullptr;
	delete[] MemImage;
	MemImage = nullptr;
	MemImageSize = 0;
	MetaData.Clear();
	tBaseImage::Clear();
}


}
