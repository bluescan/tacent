// tPicture.h
//
// This class represents a simple one-frame image. It is a collection of raw uncompressed 32-bit tPixels. It can load
// various formats from disk such as jpg, tga, png, etc. It intentionally _cannot_ load a dds file. More on that later.
// Image manipulation (excluding compression) is supported in a tPicture, so there are crop, scale, rotate, etc
// functions in this class.
//
// Some image disk formats have more than one 'frame' or image inside them. For example, tiff files can have more than
// page, and gif/webp images may be animated and have more than one frame. A tPicture can only prepresent _one_ of 
// these frames.
//
// Copyright (c) 2006, 2016, 2017, 2020, 2021 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Foundation/tList.h>
#include <Foundation/tConstants.h>
#include <Math/tColour.h>
#include <System/tFile.h>
#include <System/tChunk.h>
#include "Image/tImageAPNG.h"
#include "Image/tImageBMP.h"
#include "Image/tImageEXR.h"
#include "Image/tImageGIF.h"
#include "Image/tImageHDR.h"
#include "Image/tImageICO.h"
#include "Image/tImageJPG.h"
#include "Image/tImagePNG.h"
#include "Image/tImageTGA.h"
#include "Image/tImageTIFF.h"
#include "Image/tImageWEBP.h"
#include "Image/tImageXPM.h"
#include "Image/tPixelFormat.h"
#include "Image/tResample.h"
#include "Image/tLayer.h"
namespace tImage
{


// Verion information for the image loaders. This is all in the tImage namespace.
extern const char* Version_LibJpegTurbo;
extern const char* Version_OpenEXR;
extern const char* Version_ZLIB;
extern const char* Version_LibPNG;
extern const char* Version_LibTIFF;
extern const char* Version_ApngDis;
extern const char* Version_ApngAsm;
extern int Version_WEBP_Major;
extern int Version_WEBP_Minor;


// A tPicture is a single 2D image. A rectangular collection of RGBA pixels (32bits per pixel). The origin is the lower
// left, and the rows are ordered from bottom to top in memory. This matches the expectation of OpenGL texture
// manipulation functions for the most part (there are cases when it is inconsistent with itself).
class tPicture : public tLink<tPicture>
{
public:
	// Constructs an empty picture that is invalid. You must call Load yourself later.
	tPicture()																											{ }

	// Constructs a picture that is width by height pixels. It will be all black pixels with an opaque alpha of 255.
	// That is, every pixel will be (0, 0, 0, 255).
	tPicture(int width, int height)																						{ Set(width, height); }

	// This constructor allows you to specify an external buffer of pixels to use. If copyPixels is true, it simply
	// copies the values from the buffer you supply. If copyPixels is false, it means you are giving the buffer to the
	// tPicture. In this case the tPicture will delete[] the buffer for you when appropriate.
	tPicture(int width, int height, tPixel* pixelBuffer, bool copyPixels = true)										{ Set(width, height, pixelBuffer, copyPixels); }

	struct LoadParams
	{
		LoadParams() { }
		float	GammaValue				= 2.2f;//::tMath::DefaultGamma;
		int		HDR_Exposure			= tImageHDR::DefaultExposure;
		float	EXR_Exposure			= tImageEXR::DefaultExposure;
		float	EXR_Defog				= tImageEXR::DefaultDefog;
		float	EXR_KneeLow				= tImageEXR::DefaultKneeLow;
		float	EXR_KneeHigh			= tImageEXR::DefaultKneeHigh;
	};

	// Loads the supplied image file. If the image couldn't be loaded, IsValid will return false afterwards. Uses the
	// filename extension to determine what file type it is loading. dds files may _not_ be loaded into a tPicture.
	// Use a tTexture if you want to load a dds. For images with more than one frame (animated gif, tiff, etc) the
	// frameNum specifies which one to load and will result in an invalid tPicture if you go too high.
	tPicture(const tString& imageFile, int frameNum = 0, LoadParams params = LoadParams())								{ Load(imageFile, frameNum, params); }

	// Copy constructor.
	tPicture(const tPicture& src)																						: tPicture() { Set(src); }

	virtual ~tPicture()																									{ Clear(); }
	bool IsValid() const																								{ return Pixels ? true : false; }

	// Invalidated the picture and frees memory associated with it. The tPicture will be invalid after this.
	void Clear();

	// Sets the image to the dimensions provided. Image will be opaque black after this call. Internally, if the
	// existing buffer is the right size, it is reused. In all cases, the entire image is cleared to black.
	void Set(int width, int height, const tPixel& colour = tPixel::black);

	// Sets the image to the dimensions provided. Allows you to specify an external buffer of pixels to use. If
	// copyPixels is true, it simply copies the values from the buffer you supply. In this case it will attempt to
	// reuse it's existing buffer if it can. If copyPixels is false, it means you are giving the buffer to the
	// tPicture. In this case the tPicture will delete[] the buffer for you when appropriate. In all cases, existing
	// pixel data is lost.
	void Set(int width, int height, tPixel* pixelBuffer, bool copyPixels = true);
	void Set(const tPicture& src);

	// Can this class save the the filetype supplied?
	static bool CanSave(const tString& imageFile);
	static bool CanSave(tSystem::tFileType);

	// Can this class load the the filetype supplied?
	static bool CanLoad(const tString& imageFile);
	static bool CanLoad(tSystem::tFileType);

	enum class tColourFormat
	{
		Invalid,	// Invalid must be 0.
		Auto,		// Save function will decide format. Colour if all image pixels are opaque and ColourAndAlpha otherwise.
		Colour,
		ColourAndAlpha
	};

	// Saves to the image file you specify and examines the extension to determine filetype. Supports tga, png, bmp, jpg.
	// If tColourFormat is set to auto, the opacity/alpha channel will be excluded if all pixels are opaque.
	// Alpha channels are not supported for gif and jpg files. Quality (used for jpg) is in [1, 100].
	bool Save(const tString& imageFile, tColourFormat = tColourFormat::Auto, int quality = 95);

	bool SaveBMP(const tString& bmpFile) const;
	bool SaveJPG(const tString& jpgFile, int quality = 95) const;
	bool SavePNG(const tString& pngFile) const;
	bool SaveTGA
	(
		const tString& tgaFile, tImageTGA::tFormat = tImageTGA::tFormat::Auto,
		tImageTGA::tCompression = tImageTGA::tCompression::RLE
	) const;
	bool SaveWEBP(const tString& webpFile) const;
	bool SaveAPNG(const tString& apngFile) const;
	bool SaveTIFF(const tString& tiffFile, bool compress = true) const;
	bool SaveGIF (const tString& gifFile ) const;

	// Always clears the current image before loading. If false returned, you will have an invalid tPicture.
	bool Load(const tString& imageFile, int frameNum = 0, LoadParams params = LoadParams());

	// Save and Load to tChunk format.
	void Save(tChunkWriter&) const;
	void Load(const tChunk&);

	// Returns true if all pixels are completely opaque (an alphas of 255). This function checks the entire pixel
	// buffer every time it is called.
	bool IsOpaque() const;

	// These functions allow reading and writing pixels.
	tPixel& Pixel(int x, int y)																							{ return Pixels[ GetIndex(x, y) ]; }
	tPixel* operator[](int i)						/* Syntax: image[y][x] = colour;  No bounds checking performed. */	{ return Pixels + GetIndex(0, i); }
	tPixel GetPixel(int x, int y) const																					{ return Pixels[ GetIndex(x, y) ]; }
	tPixel* GetPixelPointer(int x = 0, int y = 0)																		{ return &Pixels[ GetIndex(x, y) ]; }
	tPixel* GetPixels()																									{ return Pixels; }
	tPixel* StealPixels()																								{ tPixel* p = Pixels; Pixels = nullptr; return p; }

	void SetPixel(int x, int y, const tColouri& c)																		{ Pixels[ GetIndex(x, y) ] = c; }
	void SetPixel(int x, int y, uint8 r, uint8 g, uint8 b, uint8 a = 0xFF)												{ Pixels[ GetIndex(x, y) ] = tColouri(r, g, b, a); }
	void SetAll(const tColouri& = tColouri(0, 0, 0));

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }
	int GetNumPixels() const																							{ return Width*Height; }

	void Rotate90(bool antiClockWise);

	// Rotates image about center point. The resultant image size is always big enough to hold every source pixel. Call
	// one or more of the crop functions after if you need to change the canvas size or remove transparent sides. The
	// rotate algorithm first upscales the image x4, rotates, then downscales. That is what upFilter and downFilter are
	// for. If you want to rotate pixel-art (nearest neighbour, no up/dowuse upFilter = none.
	//
	// UpFilter		DownFilter		Description
	// None			NA				No up/down scaling. Preserves colours. Nearest Neighbour. Fast. Good for pixel art.
	// Valid		Valid			Up/down scaling. Smooth. Good results with up=bilinear, down=box.
	// Valid		None			Up/down scaling. Use alternate (sharper) downscaling scheme (pad + 2 X ScaleHalf).
	void RotateCenter
	(
		float angle,
		const tPixel& fill			= tPixel::transparent,
		tResampleFilter	upFilter	= tResampleFilter::None,
		tResampleFilter	downFilter	= tResampleFilter::None
	);

	void Flip(bool horizontal);

	// Cropping. Can also perform a canvas enlargement. If width or height are smaller than the current size the image
	// is cropped. If larger, the fill colour is used. Fill defaults to transparent-zero-alpha black pixels.
	enum class Anchor
	{
		LeftTop,		MiddleTop,		RightTop,
		LeftMiddle,		MiddleMiddle,	RightMiddle,
		LeftBottom,		MiddleBottom,	RightBottom
	};
	void Crop(int newWidth, int newHeight, Anchor = Anchor::MiddleMiddle, const tColouri& fill = tColouri::transparent);
	void Crop(int newWidth, int newHeight, int originX, int originY, const tColouri& fill = tColouri::transparent);

	// Crops sides that match the specified colour. Optionally select only some channels to be considered.
	// If this function wants to remove everything it returns false and leaves the image untouched.
	bool Crop(const tColouri& = tColouri::transparent, uint32 channels = tMath::ColourChannel_A);

	// This function scales the image by half using a box filter. Useful for generating mipmaps. This function returns
	// false if the rescale could not be performed. For this function to succeed:
	//  - The image needs to be valid AND
	//  - The width must be divisible by two if it is not equal to 1 AND
	//  - The height must be divisible by two if it is not equal to 1.
	// Dimensions of 1 are handled since it's handy for mipmap generation. If width=10 and height=1, we'd end up with a
	// 5x1 image. An 11x1 image would yield an error and return false. A 1x1 successfully yields the same 1x1 image.
	bool ScaleHalf();

	// Resizes the image using the specified filter. Returns success. If the resample fails the tPicture is unmodified.
	bool Resample
	(
		int width, int height,
		tResampleFilter = tResampleFilter::Bilinear, tResampleEdgeMode = tResampleEdgeMode::Clamp
	);
	bool Resize
	(
		int width, int height,
		tResampleFilter filter = tResampleFilter::Bilinear, tResampleEdgeMode edgeMode = tResampleEdgeMode::Clamp
	)																													{ return Resample(width, height, filter, edgeMode); }

	// A convenience. This is sort of light tTexture functionality -- generate layers that may be passed off to HW.
	// Unlike tTexture that compresses to a BC format, this function always uses R8G8B8A8 pixel format and does not
	// require power-of-2 dimensions. If generating mipmap layers, each layer is half (truncated) in width and height
	// until a 1x1 is reached. There is no restriction on starting dimensions (they may be odd for example). Populates
	// (appends) to the supplied tLayer list. If resampleFilter is None no mipmap layers are generated, only a single
	// layer will be appened. In this case edgeMode is ignored. If chainGeneration is true, the previous mip texture
	// is gused to generate the next -- this is faster but may not be as good quality. Returns number of appended
	// layers.
	int GenerateLayers
	(
		tList<tLayer>&, tResampleFilter filter = tResampleFilter::Lanczos_Narrow,
		tResampleEdgeMode edgeMode = tResampleEdgeMode::Clamp,
		bool chainGeneration = true
	);
	bool operator==(const tPicture&) const;
	bool operator!=(const tPicture&) const;

	tString Filename;
	tPixelFormat SrcPixelFormat = tPixelFormat::Invalid;
	uint TextureID = 0;
	float Duration = 0.5f;

private:
	int GetIndex(int x, int y) const																					{ tAssert((x >= 0) && (y >= 0) && (x < Width) && (y < Height)); return y * Width + x; }
	static int GetIndex(int x, int y, int w, int h)																		{ tAssert((x >= 0) && (y >= 0) && (x < w) && (y < h)); return y * w + x; }

	void RotateCenterNearest(const tMath::tMatrix2& rotMat, const tMath::tMatrix2& invRot, const tPixel& fill);
	void RotateCenterResampled
	(
		const tMath::tMatrix2& rotMat, const tMath::tMatrix2& invRot, const tPixel& fill,
		tResampleFilter upFilter, tResampleFilter downFilter
	);

	int Width = 0;
	int Height = 0;
	tPixel* Pixels = nullptr;
};


// Implementation below this line.


inline void tPicture::Clear()
{
	Filename.Clear();
	delete[] Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	SrcPixelFormat = tPixelFormat::Invalid;
}


inline bool tPicture::CanLoad(const tString& imageFile)
{
	return CanLoad( tSystem::tGetFileType(imageFile) );
}


inline bool tPicture::CanSave(const tString& imageFile)
{
	return CanSave( tSystem::tGetFileType(imageFile) );
}


inline bool tPicture::IsOpaque() const
{
	for (int pixel = 0; pixel < (Width*Height); pixel++)
	{
		if (Pixels[pixel].A < 255)
			return false;
	}

	return true;
}


inline void tPicture::SetAll(const tColouri& clearColour)
{
	if (!Pixels)
		return;

	int numPixels = Width*Height;
	for (int p = 0; p < numPixels; p++)
		Pixels[p] = clearColour;
}


inline void tPicture::Set(const tPicture& src)
{
	Clear();
	if (!src.IsValid())
		return;

	Set(src.Width, src.Height, src.Pixels);
	Filename = src.Filename;
	SrcPixelFormat = src.SrcPixelFormat;
	Duration = src.Duration;
}


inline bool tPicture::operator==(const tPicture& src) const
{
	if (!Pixels || !src.Pixels)
		return false;

	if ((Width != src.Width) || (Height != src.Height))
		return false;

	for (int pixel = 0; pixel < (Width * Height); pixel++)
		if (Pixels[pixel] != src.Pixels[pixel])
			return false;

	return true;
}


inline bool tPicture::operator!=(const tPicture& src) const
{
	return !(*this == src);
}


}
