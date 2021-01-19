// tFrame.h
//
// A tFrame is a container for an array of tPixels (in RGBA format) along with some minimal satellite information
// including width, height, and duration. The tFrame class is primarily used by tImage formats that support more
// than one frame in a single image file (like gif, tiff, apng, and webp). A tFrame differs from a tLayer in that
// they are much simpler and do not support multiple pixel formats.
//
// Copyright (c) 2021 Tristan Grimmer.
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
	tFrame()																											: Width(0), Height(0), Pixels(nullptr), Duration(0.0f), SrcPixelFormat(tPixelFormat::Invalid) { }
	tFrame(const tFrame& src)																							{ Set(src); }
	tFrame(const tPixel* srcPixels, int width, int height, float duration = 0.0f)										{ Set(srcPixels, width, height, duration); }
	virtual ~tFrame()																									{ Clear(); }

	void Set(const tFrame&);
	void Set(const tPixel*, int width, int height, float duration = 0.0f);
	void StealFrom(tFrame&);
	tPixel* StealPixels()																								{ tPixel* p = Pixels; Pixels = nullptr; return p; }
	void Clear();
	bool IsValid() const																								{ return (Width > 0) && (Height > 0) && Pixels; }
	void ReverseRows();
	bool IsOpaque() const;

	int Width																	= 0;
	int Height																	= 0;
	tPixel* Pixels																= nullptr;
	float Duration					/* Frame duration in seconds. */			= 0.0f;
	tPixelFormat SrcPixelFormat		/* Use of SrcPixelFormat is optional. */	= tPixelFormat::Invalid;
};


// Implementation below this line.


inline void tFrame::Set(const tFrame& frame)
{
	Clear();

	// If frame is not valid this one gets returned with defaults (also invalid).
	if ((&frame == this) || !frame.IsValid())
		return;

	Width			= frame.Width;
	Height			= frame.Height;
	Duration		= frame.Duration;
	SrcPixelFormat	= frame.SrcPixelFormat;

	tAssert((frame.Width > 0) && (frame.Height > 0) && frame.Pixels);
	Pixels = new tPixel[Width*Height];
	tStd::tMemcpy(Pixels, frame.Pixels, Width*Height*sizeof(tPixel));
}


inline void tFrame::Set(const tPixel* srcPixels, int width, int height, float duration)
{
	Clear();
	if (!srcPixels || (width <= 0) || (height <= 0))
		return;

	Width = width;
	Height = height;
	Duration = duration;
	SrcPixelFormat = tPixelFormat::R8G8B8A8;

	Pixels = new tPixel[Width*Height];
	tStd::tMemcpy(Pixels, srcPixels, Width*Height*sizeof(tPixel));
}


inline void tFrame::StealFrom(tFrame& frame)
{
	// If frame is not valid this one gets returned with defaults (also invalid).
	if ((&frame == this) || !frame.IsValid())
		return;

	Width			= frame.Width;
	Height			= frame.Height;
	Duration		= frame.Duration;
	SrcPixelFormat	= frame.SrcPixelFormat;
	Pixels			= frame.Pixels;
	frame.Pixels	= nullptr;		// Frame is left invalid.
}


inline void tFrame::Clear()
{
	Width = 0;
	Height = 0;
	delete[] Pixels;
	Pixels = nullptr;
	Duration = 0.0f;
	SrcPixelFormat = tPixelFormat::Invalid;
}


inline void tFrame::ReverseRows()
{
	int numPixels = Width * Height;
	tPixel* origPixels = Pixels;
	Pixels = new tPixel[numPixels];

	int bytesPerRow = Width*sizeof(tPixel);
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
