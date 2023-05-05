// tImageJPG.h
//
// This clas knows how to load and save a JPeg (.jpg and .jpeg) file. It does zero processing of image data. It knows
// the details of the jpg file format and loads the data into a tPixel array. These tPixels may be 'stolen' by the
// tPicture's constructor if a jpg file is specified. After the array is stolen the tImageJPG is invalid. This is
// purely for performance.
//
// Copyright (c) 2020, 2022, 2023 Tristan Grimmer.
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
#include <Image/tPixelFormat.h>
#include <Image/tMetaData.h>
#include <Image/tBaseImage.h>
namespace tImage
{


class tImageJPG : public tBaseImage
{
public:
	enum LoadFlags
	{
		LoadFlag_None			= 0,
		LoadFlag_Strict			= 1 << 0,	// If the file is ill-formed even in a non-fatal way, the image will be invalid.
		LoadFlag_ExifOrient		= 1 << 1,	// Undo orientation transformations in jpg image as indicated by Exif meta-data.
		LoadFlag_NoDecompress	= 1 << 2,	// Do not decompress image. Loads as a memory image only. Flip and rotate functions can only be called if NoDecompress is set.
		LoadFlags_Default		= LoadFlag_ExifOrient
	};

	// Creates an invalid tImageJPG. You must call Load or Set manually.
	tImageJPG()																											{ }
	
	tImageJPG(const tString& jpgFile)																					{ Load(jpgFile, LoadFlags_Default); }
	tImageJPG(const tString& jpgFile, uint32 loadFlags)																	{ Load(jpgFile, loadFlags); }

	// The data is copied out of jpgFileInMemory. Go ahead and delete after if you want.
	tImageJPG(const uint8* jpgFileInMemory, int numBytes, uint32 loadFlags = LoadFlags_Default)							{ Load(jpgFileInMemory, numBytes, loadFlags); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageJPG(tPixel* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageJPG(tFrame* frame, bool steal = true)																			{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImageJPG(tPicture& picture, bool steal = true)																		{ Set(picture, steal); }

	virtual ~tImageJPG()																								{ Clear(); }

	// Clears the current tImageJPG before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& jpgFile, uint32 loadFlags = LoadFlags_Default);
	bool Load(const uint8* jpgFileInMemory, int numBytes, uint32 loadFlags = LoadFlags_Default);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel*, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	const static int DefaultQuality = 95;

	struct SaveParams
	{
		SaveParams()																									{ Reset(); }
		SaveParams(const SaveParams& src)																				: Quality(src.Quality) { }
		void Reset()																									{ Quality = DefaultQuality; }
		SaveParams& operator=(const SaveParams& src)																	{ Quality = src.Quality; return *this; }
		int Quality;
	};

	// Saves the tImageJPG to the JPeg file specified. The type of filename must be JPG (jpg or jpeg extension).
	// The quality int is should be a percent in [1,100]. If the tImageJPG was loaded with LoadFlag_NoDecompress,
	// the quality setting is ignored. Returns true on success.
	bool Save(const tString& jpgFile, int quality) const;
	bool Save(const tString& jpgFile, const SaveParams& = SaveParams()) const;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return Pixels || MemImage ? true : false; }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }

	// IsOpaque always returns true for a JPeg.
	bool IsOpaque() const																								{ return true; }

	enum class Transform
	{
		Rotate90ACW,
		Rotate90CW,
		FlipH,
		FlipV
	};

	// A perfect lossless transfrom is one where the area of the image is the same before and after the transform.
	// An imperfect lossless transform is still lossless, but some edges of the image need to be culled. For all
	// lossless transforms (flips/rotates) to be perfect two things must be true:
	//
	// a) The NoDecompress load-flag must have been used.
	// b) The image's width and height must be evenly divisible by the MCU block size.
	//
	// For b) if both width and height are divisible, all transforms are possible. If one is divisible then the
	// transform may be possible or it may not be (depending on the transform). This is why the specific transform must
	// be supplied. If false you can still perform a LosslessTransform, but one or two outer edges will be culled.
	bool CanDoPerfectLosslessTransform(Transform) const;

	// If allowImperfect is true you may end up with a slightly cropped image. This cropping will happen if
	// CanDoPerfectLosslessTransform returned false. If allowImperfect is false, this function will return false and do
	// nothing unless it can guarantee no cropping.
	bool LosslessTransform(Transform, bool allowImperfect = true);

	// After this call you are the owner of the pixels and must eventually delete[] them. This tImageJPG object is
	// invalid afterwards.
	tPixel* StealPixels();
	tFrame* GetFrame(bool steal = true) override;

	tPixel* GetPixels() const																							{ return Pixels; }
	tPixelFormat PixelFormatSrc = tPixelFormat::Invalid;

	// A place to store EXIF and XMP metadata. JPeg files often contain this metadata. This field is not populated if
	// NoDecompress flag was used during load.
	tMetaData MetaData;

private:
	bool PopulateMetaData(const uint8* jpgFileInMemory, int numBytes);
	void ClearPixelData();

	int GetIndex(int x, int y) const																					{ tAssert((x >= 0) && (y >= 0) && (x < Width) && (y < Height)); return y * Width + x; }
	static int GetIndex(int x, int y, int w, int h)																		{ tAssert((x >= 0) && (y >= 0) && (x < w) && (y < h)); return y * w + x; }
	void Rotate90(bool antiClockWise);
	void Flip(bool horizontal);

	int Width			= 0;
	int Height			= 0;
	tPixel* Pixels		= nullptr;
	uint8* MemImage		= nullptr;
	int MemImageSize	= 0;
};


// Implementation below this line.


inline void tImageJPG::ClearPixelData()
{
	Width = 0;
	Height = 0;
	delete[] Pixels;
	Pixels = nullptr;
}


}
