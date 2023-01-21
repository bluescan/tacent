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
// Copyright (c) 2006, 2016, 2017, 2020-2023  Tristan Grimmer.
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
#include "Image/tBaseImage.h"
#include "Image/tPixelFormat.h"
#include "Image/tResample.h"
#include "Image/tLayer.h"
namespace tImage
{


// Verion information for the image loaders and libraries they may use. This is all in the tImage namespace.
extern const char* Version_LibJpegTurbo;
extern const char* Version_ASTCEncoder;
extern const char* Version_OpenEXR;
extern const char* Version_ZLIB;
extern const char* Version_LibPNG;
extern const char* Version_LibTIFF;
extern const char* Version_LibKTX;
extern const char* Version_ApngDis;
extern const char* Version_ApngAsm;
extern int Version_WEBP_Major;
extern int Version_WEBP_Minor;
extern int Version_BCDec_Major;
extern int Version_BCDec_Minor;
extern int Version_TinyXML2_Major;
extern int Version_TinyXML2_Minor;
extern int Version_TinyXML2_Patch;
extern int Version_TinyEXIF_Major;
extern int Version_TinyEXIF_Minor;
extern int Version_TinyEXIF_Patch;


// A tPicture is a single 2D image. A rectangular collection of R8G8B8A8 pixels (32bits per pixel). The origin is the
// lower left, and the rows are ordered from bottom to top in memory. @todo At some point we need to support HDR images and
// should represent the pixels as R32G32B32A32f.
//
// The main purpose of a tPicture is to allow manipulation of a single image. Things like setting pixel colours,
// rotation, flips, resampling and resizing are found here.
//
// There is no saving and loading directly from image files because some types may have multiple frames. For example
// a gif or webp may be animated. We could just choose a particular frame, but that would mean loading all frames only
// to keep a single one. There is the same complexity with saving. Different image formats have drastically different
// parameters that need to be specified for saving -- jpgs need a quality setting, astc files have a multitude of
// compression parameters in addition to the block size, targas can be RLE encoded, ktx files can be supercompressed...
// or not, etc. The purpose of the tImageNNN files is to deal with that complexity for each specific image type. From
// these loaders you can construct one or more tPictures by passing in the pixels, width, and height.
//
// There is some save/load functionality directly for a tPicture. It has it's own file format based of tChunks. It can
// save/load itself to/from a .tac file.
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

	// Construct from a tFrame. If steal is true the tPicture will take ownership of the tFrame. If steal is false it
	// will copy the pixels out. The frame duration is also taken from the frame.
	tPicture(tFrame* frame, bool steal)																					{ Set(frame, steal); }

	// Constructs from any type derived from tImageBase (eg. tImageASTC). If steal is true the tImageBase MAY be
	// modified. In particular it may be invalid afterwards because the pixels may have been stolen from it. For
	// multiframe images it meay still be valid after but down a frame. On the other hand with steal false you are
	// guaranteed that image remains unmodified, but at the cost of duplicating memory for the pixels.
	tPicture(tBaseImage& image, bool steal = true)																		{ Set(image, steal); }

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
	// pixel data is lost. Other members of the tPicture are unmodified.
	void Set(int width, int height, tPixel* pixelBuffer, bool copyPixels = true);

	// Same as above except the tPicture does NOT own the pixels. You are responsibe for keeping them valid for the
	// lifetime of this tPicture. This object will not delete them.
	void SetExt(int width, int height, tPixel* pixelBuffer);

	// Sets from a tFrame. If steal is true the tPicture will take ownership of the tFrame. If steal is false it will
	// copy the pixels out. The frame duration is also taken from the frame.
	void Set(tFrame* frame, bool steal);

	// Sets from any type derived from tImageBase (eg. tImageASTC). If steal is true the tImageBase MAY be
	// modified. In particular it may be invalid afterwards because the pixels may have been stolen from it. For
	// multiframe images it meay still be valid after but down a frame. On the other hand with steal false you are
	// guaranteed that image remains unmodified, but at the cost of duplicating memory for the pixels.
	void Set(tBaseImage& image, bool steal = true);

	void Set(const tPicture& src);

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
	void SetAll(const tColouri& = tColouri(0, 0, 0), tcomps channels = tComp_RGBA);

	// Blends in supplied colour according to the current alpha. If pixel alpha is 255, then none of the blend colour is
	// used for that pixel. If alpha is 0, all of it is used. If alpha is 64, then 1/4 of the current pixel colour and
	// 3/4 of the supplied, If resetAlpha is true, the operation essentially creates a premultiplied opaque image by
	// setting the alphas to 255 after the blend. Note that the alpha of the supplied colour is ignored.
	void AlphaBlendColour(const tColouri& colour, bool resetAlpha = true);

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }
	int GetArea() const																									{ return Width*Height; }
	int GetNumPixels() const																							{ return GetArea(); }

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
	bool Crop(const tColouri& = tColouri::transparent, uint32 channels = tComp_A);

	// Ideally adjustments (brightness, contrast etc) would be done in a fragment shader and then 'committed' to the
	// tPicture with a simple adjust call. However currently the clients of tPicture don't have that ability so we're
	// going with a begin/adjust/end setup where a new 'adjusted' pixel buffer is allocated on begin, and an adjustment
	// writes to the new buffer. End then either copies the adjustment buffer to the original, or cancels and does not
	// update original pixels. In either case End deletes the adjustment buffer. After Begin, only one AdjustmentNNN
	// will have an effect (the last one called) until you call End with commit. This is because an adjustment is
	// always based on the source pixels. It stops the issue, for example, of setting the brightness to full and losing
	// all the colour data when you move back down.
	//
	// AdjustmentBegin returns the adjustment pixel buffer with the adjusted pixels. You don't own it. Returns nullptr
	// on failure (invalid image). This function also precomputes the internal min/max colour values and histograms.
	tPixel* AdjustmentBegin();

	// Adjust brightness based on the tPicture pixels and write them into the adjustment pixel buffer. Brightness is in
	// [0.0,1.0]. When brightness at 0.0 adjustment buffer will be completely black. When brightness at 1.0, pure white,
	// Note that the range of the brigtness is computed so that all values between [0,1] have an effect on the image.
	// This is possible because the min and max colour values were computed by inspecting every pixel when begin was
	// called. In other words the values the colours move up or down for a particular brightness are image dependent,
	// Returns success.
	//
	// The DefaultNNN functions get the parameters needed to have zero affect on the image. For brightness in particular
	// it is dependent on the image contents and may not be exactly 0.5. If the min/max colour values did not reach 0
	// and full, the default brightness may be offset from 0.5.
	bool AdjustBrightness(float brightness);
	bool GetDefaultBrightness(float& brightness);

	// Adjust contrast based on the tPicture pixels and write them into the adjustment pixel buffer. Contrast is in
	// [0.0, 1.0]. When contrast is at 0.0, adjustment buffer will be lowest contrast. When contrast at 1.0, highest,
	// Returns success.
	bool AdjustContrast(float contrast);
	bool GetDefaultContrast(float& contrast);

	// Adjust levels. All values are E [0, 1]. Ensure blackPoint <= midPoint <= whitePoint and blackOut <= whiteOut.
	// If these conditions are not met they are silently enforced starting at black (unmodified). The powerMidGamma
	// option lets you decide between 2 algorithms to determine the curve on the gamma. If false it uses some code that
	// tries to mimic Photoshop. See https://stackoverflow.com/questions/39510072/algorithm-for-adjustment-of-image-levels
	// The curve for the above is C1 discontinuous at gamma 1, so I didn't like it. powerMidGamma, the default, uses
	// a continuous base-10 power curve that smoothly goes from gamma 0.1 to gamma 10.
	// For the power curve the gamma range is [0.1,  10.0] where 1.0 is linear. This approximates GIMP.
	// For the photo curve the gamma range is [0.01, 9.99] where 1.0 is linear. This approximates PS.
	// The defaults to result in no change are the same for both algorithms.
	bool AdjustLevels(float blackPoint, float midPoint, float whitePoint, float blackOut, float whiteOut, bool powerMidGamma = true);
	bool GetDefaultLevels(float& blackPoint, float& midPoint, float& whitePoint, float& blackOut, float& whiteOut);

	// When commit is false, cancels the adjustment. When true applies the adjustment buffer to the tPicture pixels.
	// Returns success -- not whether committed or not, but rather was operation successful.
	bool AdjustmentEnd(bool commit);
	
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
	tPixelFormat PixelFormatSrc = tPixelFormat::Invalid;
	uint TextureID = 0;
	float Duration = 0.5f;

	// Transient parameters. Only access between AdjustmentBegin/End.
	int BrightnessRGBMin		= 0;				// Used for brightness adjustments.
	int BrightnessRGBMax		= 0;				// Used for brightness adjustments.
	static const int NumGroups	= 256;				// Sure makes it easy choosing 256 groups.
	int HistogramR[NumGroups];	int MaxRCount;		// Frequency of Red.   Max R count in all groups.
	int HistogramG[NumGroups];	int MaxGCount;		// Frequency of Green. Max G count in all groups.
	int HistogramB[NumGroups];	int MaxBCount;		// Frequency of Blue.  Max B count in all groups.
	int HistogramA[NumGroups];	int MaxACount;		// Frequency of Alpha. Max A count in all groups.
	int HistogramI[NumGroups];	int MaxICount;		// Frequency of Intensity (avg of RGB). Max I count in all groups.

private:
	int GetIndex(int x, int y) const																					{ tAssert((x >= 0) && (y >= 0) && (x < Width) && (y < Height)); return y * Width + x; }
	static int GetIndex(int x, int y, int w, int h)																		{ tAssert((x >= 0) && (y >= 0) && (x < w) && (y < h)); return y * w + x; }

	void RotateCenterNearest(const tMath::tMatrix2& rotMat, const tMath::tMatrix2& invRot, const tPixel& fill);
	void RotateCenterResampled
	(
		const tMath::tMatrix2& rotMat, const tMath::tMatrix2& invRot, const tPixel& fill,
		tResampleFilter upFilter, tResampleFilter downFilter
	);

	int Width				= 0;
	int Height				= 0;
	bool ExternalPixels		= false;
	tPixel* Pixels			= nullptr;
	tPixel* AdjustedPixels	= nullptr;
};


// Implementation below this line.


inline void tPicture::Clear()
{
	Filename.Clear();
	if (!ExternalPixels)
		delete[] Pixels;
	Pixels = nullptr;
	ExternalPixels = false;
	delete[] AdjustedPixels;
	AdjustedPixels = nullptr;
	Width = 0;
	Height = 0;
	PixelFormatSrc = tPixelFormat::Invalid;
}


inline void tPicture::Set(int width, int height, const tPixel& colour)
{
	tAssert((width > 0) && (height > 0));
	if (ExternalPixels)
		Clear();

	// Reuse the existing buffer if possible.
	if (width*height != Width*Height)
	{
		delete[] Pixels;
		Pixels = new tPixel[width*height];
	}
	Width = width;
	Height = height;
	for (int pixel = 0; pixel < (Width*Height); pixel++)
		Pixels[pixel] = colour;

	PixelFormatSrc = tPixelFormat::R8G8B8A8;
}


inline void tPicture::Set(int width, int height, tPixel* pixelBuffer, bool copyPixels)
{
	tAssert((width > 0) && (height > 0) && pixelBuffer);
	if (ExternalPixels)
		Clear();

	// If we're copying the pixels we may be able to reuse the existing buffer if it's the right size. If we're not
	// copying and the buffer is being handed to us, we just need to free our current buffer.
	if (copyPixels)
	{
		if ((width*height != Width*Height) || !Pixels)
		{
			delete[] Pixels;
			Pixels = new tPixel[width*height];
		}
	}
	else
	{
		delete[] Pixels;
		Pixels = pixelBuffer;
	}
	Width = width;
	Height = height;

	if (copyPixels)
		tStd::tMemcpy(Pixels, pixelBuffer, Width*Height*sizeof(tPixel));

	PixelFormatSrc = tPixelFormat::R8G8B8A8;
}


inline void tPicture::SetExt(int width, int height, tPixel* pixelBuffer)
{
	tAssert((width > 0) && (height > 0) && pixelBuffer);
	Clear();

	ExternalPixels = true;
	Pixels = pixelBuffer;
	Width = width;
	Height = height;
	PixelFormatSrc = tPixelFormat::R8G8B8A8;
}


inline void tPicture::Set(tFrame* frame, bool steal)
{
	if (!frame || !frame->IsValid())
		return;

	Set(frame->Width, frame->Height, frame->GetPixels(steal), !steal);
	Duration = frame->Duration;
	if (steal)
		delete frame;
}


inline void tPicture::Set(tBaseImage& image, bool steal)
{
	if (!image.IsValid())
		return;

	tFrame* frame = image.GetFrame(steal);

	// The true here is correct. Whether steal was true or not, we now have a frame that is under our
	// management and must be eventually deleted.
	Set(frame, true);
}


inline void tPicture::Set(const tPicture& src)
{
	Clear();
	if (!src.IsValid())
		return;

	Set(src.Width, src.Height, src.Pixels);
	Filename = src.Filename;
	PixelFormatSrc = src.PixelFormatSrc;
	Duration = src.Duration;
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


inline void tPicture::SetAll(const tColouri& clearColour, tcomps channels)
{
	if (!Pixels)
		return;

	int numPixels = Width*Height;
	if (channels == tComp_RGBA)
	{
		for (int p = 0; p < numPixels; p++)
			Pixels[p] = clearColour;
	}
	else
	{
		for (int p = 0; p < numPixels; p++)
		{
			if (channels & tComp_R) Pixels[p].R = clearColour.R;
			if (channels & tComp_G) Pixels[p].G = clearColour.G;
			if (channels & tComp_B) Pixels[p].B = clearColour.B;
			if (channels & tComp_A) Pixels[p].A = clearColour.A;
		}
	}
}


inline void tPicture::AlphaBlendColour(const tColouri& blend, bool resetAlpha)
{
	if (!Pixels)
		return;

	int numPixels = Width*Height;
	for (int p = 0; p < numPixels; p++)
	{
		tColourf pixelCol(Pixels[p]);
		tColourf blendCol(blend);
		tColourf pixel;
		float alpha = pixelCol.A;
		float oneMinusAlpha = 1.0f - alpha; 
		pixel.R = pixelCol.R*alpha + blendCol.R*oneMinusAlpha;
		pixel.G = pixelCol.G*alpha + blendCol.G*oneMinusAlpha;
		pixel.B = pixelCol.B*alpha + blendCol.B*oneMinusAlpha;
		if (resetAlpha)
			pixel.A = 1.0f;

		Pixels[p].Set(pixel);
	}
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
