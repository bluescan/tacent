// tFrame.h
//
// A tFrame is a container for an array of tPixels (in RGBA format) along with some minimal satellite information
// including width, height, and duration. The tFrame class is primarily used by tImage formats that support more
// than one frame in a single image file (like gif, tiff, apng, and webp). A tFrame differs from a tLayer in that
// they are much simpler and do not support multiple pixel formats.
//
// Copyright (c) 2021, 2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Foundation/tStandard.h>
#include <Foundation/tList.h>
#include <Image/tPixelFormat.h>
namespace tImage
{


struct tFrame : public tLink<tFrame>
{
	tFrame()																											{ }

	// These mem copy the pixels from src.
	tFrame(const tFrame& src)																							{ Set(src); }
	tFrame(const tPixel4b* src, int width, int height, float duration)													{ Set(src, width, height, duration); }

	virtual ~tFrame()																									{ Clear(); }

	// These mem copy the pixels from src.
	bool Set(const tFrame& src);
	bool Set(const tPixel4b* src, int width, int height, float duration = 0.0f);

	// Steals the pixels from the src frame.
	bool StealFrom(tFrame& src);

	// Takes ownership of the src pixel array.
	bool StealFrom(tPixel4b* src, int width, int height, float duration = 0.0f);

	// If steal is true the frame will be invalid after and you must delete[] the returned pixels. They are yours.
	// If steal is false the pixels remain owned by this tFrame. You can look or modify them, but they're not yours.
	tPixel4b* GetPixels(bool steal = false)																				{ if (steal) { tPixel4b* p = Pixels; Pixels = nullptr; return p; } else return Pixels; }

	void SetPixel(int x, int y, const tPixel4b& c)																		{ Pixels[ GetIndex(x, y) ] = c; }

	void Clear();
	bool IsValid() const																								{ return (Width > 0) && (Height > 0) && Pixels; }
	void ReverseRows();
	bool IsOpaque() const;

	int Width																	= 0;
	int Height																	= 0;
	float Duration					/* Frame duration in seconds. */			= 0.0f;
	tPixelFormat PixelFormatSrc		/* Use of PixelFormatSrc is optional. */	= tPixelFormat::Invalid;
	tPixel4b* Pixels															= nullptr;

private:
	int GetIndex(int x, int y) const																					{ tAssert((x >= 0) && (y >= 0) && (x < Width) && (y < Height)); return y * Width + x; }
};


// Implementation below this line.


inline bool tFrame::Set(const tFrame& frame)
{
	Clear();

	// If frame is not valid this one gets returned with defaults (also invalid).
	if ((&frame == this) || !frame.IsValid())
		return false;

	Width			= frame.Width;
	Height			= frame.Height;
	Duration		= frame.Duration;
	PixelFormatSrc	= frame.PixelFormatSrc;

	tAssert((frame.Width > 0) && (frame.Height > 0) && frame.Pixels);
	Pixels = new tPixel4b[Width*Height];
	tStd::tMemcpy(Pixels, frame.Pixels, Width*Height*sizeof(tPixel4b));

	return true;
}


inline bool tFrame::Set(const tPixel4b* srcPixels, int width, int height, float duration)
{
	Clear();
	if (!srcPixels || (width <= 0) || (height <= 0))
		return false;

	Width = width;
	Height = height;
	Duration = duration;
	PixelFormatSrc = tPixelFormat::R8G8B8A8;

	Pixels = new tPixel4b[Width*Height];
	tStd::tMemcpy(Pixels, srcPixels, Width*Height*sizeof(tPixel4b));
	return true;
}


inline bool tFrame::StealFrom(tFrame& frame)
{
	// If frame is not valid this one gets returned with defaults (also invalid).
	if ((&frame == this) || !frame.IsValid())
		return false;

	Width			= frame.Width;
	Height			= frame.Height;
	Duration		= frame.Duration;
	PixelFormatSrc	= frame.PixelFormatSrc;
	Pixels			= frame.Pixels;
	frame.Pixels	= nullptr;		// Frame is left invalid.
	return true;
}


inline bool tFrame::StealFrom(tPixel4b* src, int width, int height, float duration)
{
	if (!src || (width <= 0) || (height <= 0))
		return false;

	Width			= width;
	Height			= height;
	Duration		= duration;
	PixelFormatSrc	= tPixelFormat::R8G8B8A8;
	Pixels			= src;
	return true;
}


inline void tFrame::Clear()
{
	Width = 0;
	Height = 0;
	delete[] Pixels;
	Pixels = nullptr;
	Duration = 0.0f;
	PixelFormatSrc = tPixelFormat::Invalid;
}


inline void tFrame::ReverseRows()
{
	int numPixels = Width * Height;
	tPixel4b* origPixels = Pixels;
	Pixels = new tPixel4b[numPixels];

	int bytesPerRow = Width*sizeof(tPixel4b);
	for (int y = Height-1; y >= 0; y--)
		tStd::tMemcpy((uint8*)Pixels + ((Height-1)-y)*bytesPerRow, (uint8*)origPixels + y*bytesPerRow, bytesPerRow);

	delete[] origPixels;
}


inline bool tFrame::IsOpaque() const
{
	for (int p = 0; p < (Width * Height); p++)
		if (Pixels[p].A < 255)
			return false;
	return true;
}


}
