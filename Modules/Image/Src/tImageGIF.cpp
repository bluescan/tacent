// tImageGIF.cpp
//
// This knows how to load gifs. It knows the details of the gif file format and loads the data into multiple tPixel
// arrays, one for each frame (gifs may be animated). These arrays may be 'stolen' by tPictures.
//
// Copyright (c) 2020, 2021, 2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tStandard.h>
#include <Foundation/tString.h>
#include <System/tFile.h>
#include <GifLoad/gif_load.h>
#include <GifSave/gif.h>
#include "Image/tImageGIF.h"
using namespace tSystem;
namespace tImage
{


// This callback is a essentially the example code from gif_load.
void tImageGIF::FrameCallback(struct GIF_WHDR* whdr)
{
    #define RGBA(i)											\
	(														\
		(whdr->bptr[i] == whdr->tran) ? 0x00000000 :		\
		(													\
			uint32_t(whdr->cpal[whdr->bptr[i]].B << 16) |	\
			uint32_t(whdr->cpal[whdr->bptr[i]].G << 8) |	\
			uint32_t(whdr->cpal[whdr->bptr[i]].R << 0) |	\
			0xFF000000										\
		)													\
	)

	// Is first frame?
	if (whdr->ifrm == 0)
	{
		Width = whdr->xdim;
		Height = whdr->ydim;
		FrmPict = new tPixel[Width * Height];
		FrmPrev = new tPixel[Width * Height];

		// tPixel constructor does not initialize its members for efficiency. Must explicitely clear.
		tStd::tMemset(FrmPict, 0, Width * Height * sizeof(tPixel));
		tStd::tMemset(FrmPrev, 0, Width * Height * sizeof(tPixel));
	}

	int numPixels = Width * Height;
	tPixel* pict = FrmPict;
	tPixel* prev = nullptr;

    uint32 ddst = uint32(whdr->xdim * whdr->fryo + whdr->frxo);

	// Interlacing support.
	uint32 iter = whdr->intr ? 0 : 4;
    uint32 ifin = !iter ? 4 : 5;

	int y = 0;
	for (uint32 dsrc = (uint32)-1; iter < ifin; iter++)
		for (int yoff = 16U >> ((iter > 1) ? iter : 1), y = (8 >> iter) & 7; y < whdr->fryd; y += yoff)
			for (int x = 0; x < whdr->frxd; x++)
				if (whdr->tran != (long)whdr->bptr[++dsrc])
					pict[whdr->xdim * y + x + ddst].BP = RGBA(dsrc);

	tFrame* frame = new tFrame();
	Frames.Append(frame);
	frame->Pixels = new tPixel[numPixels];
	frame->Duration = float(whdr->time) / 100.0f;

	// We store rows starting from the bottom (lower left is 0,0).
	for (int row = Height-1; row >= 0; row--)
		tStd::tMemcpy(frame->Pixels + (row*Width), pict + ((Height-row-1)*Width), Width*sizeof(tPixel));

	if ((whdr->mode == GIF_PREV) && !FrmLast)
	{
		whdr->frxd = whdr->xdim;
		whdr->fryd = whdr->ydim;
		whdr->mode = GIF_BKGD;
		ddst = 0;
	}
	else
	{
		FrmLast = (whdr->mode == GIF_PREV) ? FrmLast : (whdr->ifrm + 1);
		pict = (whdr->mode == GIF_PREV) ? FrmPict : FrmPrev;
		prev = (whdr->mode == GIF_PREV) ? FrmPrev : FrmPict;
		for (int x = whdr->xdim * whdr->ydim; --x;
			pict[x - 1].BP = prev[x - 1].BP);
	}

	// Cutting a hole for the next frame.
	if (whdr->mode == GIF_BKGD)
	{
		int y = 0;
		for
		(
			whdr->bptr[0] = ((whdr->tran >= 0) ? uint8(whdr->tran) : uint8(whdr->bkgd)), y = 0, pict = FrmPict;
			y < whdr->fryd;
			y++
		)
		{
            for (int x = 0; x < whdr->frxd; x++)
				pict[whdr->xdim * y + x + ddst].BP = RGBA(0);
		}
	}
}


bool tImageGIF::Load(const tString& gifFile)
{
	Clear();

	if (tSystem::tGetFileType(gifFile) != tSystem::tFileType::GIF)
		return false;

	if (!tFileExists(gifFile))
		return false;

	int numBytes = 0;
	uint8* gifFileInMemory = tLoadFile(gifFile, nullptr, &numBytes);

	// This call allocated scratchpad memory pointed to by FrmPict and FrmPrev.
	// They are set to null just in case GIF_Load fails to allocate.
	FrmPict = nullptr;
	FrmPrev = nullptr;
	int result = GIF_Load(gifFileInMemory, numBytes, FrameCallbackBridge, nullptr, (void*)this, 0);
	delete[] FrmPict;
	delete[] FrmPrev;
	delete[] gifFileInMemory;
	if (result <= 0)
		return false;

	SrcPixelFormat = tPixelFormat::PAL_8BIT;
	return true;
}


bool tImageGIF::Set(tList<tFrame>& srcFrames, bool stealFrames)
{
	Clear();
	if (srcFrames.GetNumItems() <= 0)
		return false;

	Width = srcFrames.Head()->Width;
	Height = srcFrames.Head()->Height;
	tPixelFormat SrcPixelFormat = tPixelFormat::R8G8B8A8;

	if (stealFrames)
	{
		while (tFrame* frame = srcFrames.Remove())
			Frames.Append(frame);
		return true;
	}

	for (tFrame* frame = srcFrames.Head(); frame; frame = frame->Next())
		Frames.Append(new tFrame(*frame));

	return true;
}


bool tImageGIF::Save(const tString& gifFile, int overrideFrameDuration)
{
	if (!IsValid())
		return false;

	GifWriter writer;

	// We assume here that GifBegin can handle the (const char*) being UTF-8 even though it doesn't use char8_t.
	GifBegin(&writer, gifFile.Chr(), Width, Height, 100);

	for (tFrame* frame = Frames.First(); frame; frame = frame->Next())
	{
		// There's some evidence on various websites that delays lower than 2 (2/100 second) do not
		// animate at the proper speed in many viewers. Currently we clamp at 2.
		int delay = tMath::tClampMin((overrideFrameDuration < 0) ? int(frame->Duration * 100.0f) : overrideFrameDuration, 2);

		// This is not great, but the rows are in the wrong order.
		tFrame frameReordered(*frame);
		frameReordered.ReverseRows();
		GifWriteFrame(&writer, (uint8*)frameReordered.Pixels, frame->Width, frame->Height, delay);
	}
	GifEnd(&writer);

	return true;
}


}
