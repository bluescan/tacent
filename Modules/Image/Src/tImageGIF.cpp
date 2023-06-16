// tImageGIF.cpp
//
// This knows how to load and save gifs. It knows the details of the gif file format and loads the data into multiple
// tPixel arrays, one for each frame (gifs may be animated). These arrays may be 'stolen' by tPictures.
//
// Copyright (c) 2020-2023 Tristan Grimmer.
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
#include <gifenc/gifenc.h>
#include "Image/tImageGIF.h"
#include "Image/tPicture.h"
using namespace tSystem;
namespace tImage
{


// This callback is a essentially the example code from gif_load.
void tImageGIF::FrameLoadCallback(struct GIF_WHDR* whdr)
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

	tFrame* frame = new tFrame;
	frame->Width = Width;
	frame->Height = Height;
	frame->Pixels = new tPixel[Width*Height];
	frame->PixelFormatSrc = tPixelFormat::PAL8BIT;
	frame->Duration = float(whdr->time) / 100.0f;

	// We store rows starting from the bottom (lower left is 0,0).
	for (int row = Height-1; row >= 0; row--)
		tStd::tMemcpy(frame->Pixels + (row*Width), pict + ((Height-row-1)*Width), Width*sizeof(tPixel));

	// The frame is ready. Append it.
	Frames.Append(frame);

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
	bool success = Load(gifFileInMemory, numBytes);
	delete[] gifFileInMemory;

	return success;
}


bool tImageGIF::Load(const uint8* gifFileInMemory, int numBytes)
{
	Clear();
	if ((numBytes <= 0) || !gifFileInMemory)
		return false;

	// This call allocated scratchpad memory pointed to by FrmPict and FrmPrev.
	// They are set to null just in case GIF_Load fails to allocate.
	FrmPict = nullptr;
	FrmPrev = nullptr;
	int paletteSize = 0;
	int result = GIF_Load((void*)gifFileInMemory, numBytes, FrameLoadCallbackBridge, nullptr, (void*)this, 0, paletteSize);
	delete[] FrmPict;
	delete[] FrmPrev;
	if (result <= 0)
		return false;

	PixelFormatSrc = tPixelFormat::PAL8BIT;
	switch (paletteSize)
	{
		case 2:		PixelFormatSrc = tPixelFormat::PAL1BIT; break;
		case 4:		PixelFormatSrc = tPixelFormat::PAL2BIT; break;
		case 8:		PixelFormatSrc = tPixelFormat::PAL3BIT; break;
		case 16:	PixelFormatSrc = tPixelFormat::PAL4BIT; break;
		case 32:	PixelFormatSrc = tPixelFormat::PAL5BIT; break;
		case 64:	PixelFormatSrc = tPixelFormat::PAL6BIT; break;
		case 128:	PixelFormatSrc = tPixelFormat::PAL7BIT; break;
		case 256:	PixelFormatSrc = tPixelFormat::PAL8BIT; break;
	}

	return true;
}


bool tImageGIF::Set(tList<tFrame>& srcFrames, bool stealFrames)
{
	Clear();
	if (srcFrames.GetNumItems() <= 0)
		return false;

	Width = srcFrames.Head()->Width;
	Height = srcFrames.Head()->Height;
	PixelFormatSrc = tPixelFormat::R8G8B8A8;

	if (stealFrames)
	{
		while (tFrame* frame = srcFrames.Remove())
			Frames.Append(frame);
	}
	else
	{
		for (tFrame* frame = srcFrames.Head(); frame; frame = frame->Next())
			Frames.Append(new tFrame(*frame));
	}

	return true;
}


bool tImageGIF::Set(tPixel* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Width = width;
	Height = height;
	tFrame* frame = new tFrame();
	if (steal)
		frame->StealFrom(pixels, width, height);
	else
		frame->Set(pixels, width, height);
	Frames.Append(frame);
	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	return true;
}


bool tImageGIF::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	Width = frame->Width;
	Height = frame->Height;
	if (steal)
		Frames.Append(frame);
	else
		Frames.Append(new tFrame(*frame));

	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	return true;
}


bool tImageGIF::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


tFrame* tImageGIF::GetFrame(bool steal)
{
	if (!IsValid())
		return nullptr;

	return steal ? Frames.Remove() : new tFrame( *Frames.First() );
}


bool tImageGIF::Save(const tString& gifFile, const SaveParams& saveParams) const
{
	SaveParams params = saveParams;
	if (!IsValid() || !tIsPaletteFormat(params.Format) || (tGetFileType(gifFile) != tFileType::GIF))
		return false;

	int numFrames = GetNumFrames();
	
	// The Loop int in the params is slightly different to the loop int expected by the encoder. The encoder accepts -1
	// to mean no loop information is included in the gif file, while the params has Loop as 0 for infinite loops and
	// >0 for a specific number of times. We do it this way because it simplifies the interface and we can check the
	// number of frames to see if it can be set to -1. Only one frame -> set to -1.
	int loop = params.Loop;
	if (numFrames == 1)
		loop = -1;
	
	if (params.Format == tPixelFormat::PAL1BIT)
		params.AlphaThreshold = -1;

	// Before we create a gif with gifenc's ge_new_gif we need to have created a good palette for it to use. This is a
	// little tricky for multiframe gifs because gifenc does not support frame-local palettes. The same palette is used
	// for all frames so it can apply size optimization. However, quantize calls work on a single image/frame.
	//
	// Additionally, some quantize methods dither as they go... so palette indexes are created in the same step. To
	// solve all this we need to create an 'uber-image' with all frames in one image -- and call quantize on that to
	// create both the palette and the indices on one go.
	//
	// Lastly, before we do this we need to determine if we will be generating a gif with transparency. This affects the
	// quantization step because it has one less colour to work with. For example, an 8-bit palette would have 255
	// colour entries and 1 transparency entry instead of 256 colour entries.
	int gifBitDepth			= tGetBitsPerPixel(params.Format);
	int gifPaletteSize		= tMath::tPow2(gifBitDepth);
	bool gifTransparency	= (params.AlphaThreshold >= 0);
	int quantNumColours		= gifPaletteSize - (gifTransparency ? 1 : 0);

	tPixel* pixels			= nullptr;
	int width				= 0;
	int height				= 0;

	if (numFrames == 1)
	{
		width = Width;
		height = Height;
		pixels = Frames.First()->Pixels;
	}
	else
	{
		// Create the uber image since numFrames > 1.
		width = Width * numFrames;
		height = Height;
		pixels = new tPixel[width*height];
		tStd::tMemset(pixels, 0, width*height*sizeof(tPixel));
		int frameNum = 0;
		for (tFrame* frame = Frames.First(); frame; frame = frame->Next(), frameNum++)
		{
			if ((frame->Width != Width) || (frame->Height != Height))
				continue;
			for (int y = 0; y < Height; y++)
			{
				for (int x = 0; x < Width; x++)
				{
					int srcIndex = x + y*Width;
					int dstX = x + (frameNum*Width);
					int dstY = y;
					int dstIndex = dstX + dstY*width;
					pixels[dstIndex] = frame->Pixels[srcIndex];
				}
			}
		}
	}

	// Now that width, height, and pixels are correct we can quantize.
	tColour3i* gifPalette = new tColour3i[gifPaletteSize];
	gifPalette[gifPaletteSize-1].Set(0, 0, 0);
	uint8* gifIndices = new uint8[width*height];
	bool checkExact = true;
	switch (params.Method)
	{
		case tQuantize::Method::Fixed:
			tQuantizeFixed::QuantizeImage(quantNumColours, width, height, pixels, gifPalette, gifIndices, checkExact);
			break;

		case tQuantize::Method::Neu:
			tQuantizeNeu::QuantizeImage(quantNumColours, width, height, pixels, gifPalette, gifIndices, checkExact, params.SampleFactor);
			break;

		case tQuantize::Method::Wu:
			tQuantizeWu::QuantizeImage(quantNumColours, width, height, pixels, gifPalette, gifIndices, checkExact);
			break;

		case tQuantize::Method::Spatial:
			tQuantizeSpatial::QuantizeImage(quantNumColours, width, height, pixels, gifPalette, gifIndices, checkExact, params.DitherLevel, params.FilterSize);
			break;
	}
	int bgIndex = -1;

	// Now that the indices are worked out, we need to replace any indices that are supposed to be transparent with
	// the reserved transparent index.
	if (gifTransparency)
	{
		bgIndex = gifPaletteSize-1;
		for (int p = 0; p < width*height; p++)
			if (pixels[p].A <= params.AlphaThreshold)
				gifIndices[p] = bgIndex;
	}

    ge_GIF* gifHandle = ge_new_gif(gifFile.Chr(), Width, Height, (uint8*)gifPalette, gifBitDepth, bgIndex, loop);

	// Call ge_add_frame for each frame.
	int frameNum = 0;
	for (tFrame* frame = Frames.First(); frame; frame = frame->Next(), frameNum++)
	{
		if ((frame->Width != Width) || (frame->Height != Height))
			continue;

		for (int y = 0; y < Height; y++)
		{
			for (int x = 0; x < Width; x++)
			{
				int dstIndex = x + y*Width;
				int srcX = x + (frameNum*Width);

				// The frames need to be given to the encoder from the top row down.
				int srcY = Height - y - 1;
				int srcIndex = srcX + srcY*width;
				gifHandle->frame[dstIndex] = gifIndices[srcIndex];
			}
		}
		// There's some evidence on various websites that delays lower than 2 (2/100 second) do not
		// animate at the proper speed in many viewers. Currently we clamp at 2.
		int delay = tMath::tClampMin((params.OverrideFrameDuration < 0) ? int(frame->Duration * 100.0f) : params.OverrideFrameDuration, 2);
		if (numFrames == 1)
			delay = 0;
			
		ge_add_frame(gifHandle, delay);
	}

	ge_close_gif(gifHandle);
	delete[] gifPalette;
	delete[] gifIndices;
	if (numFrames != 1)
		delete[] pixels;

	return true;
}


}
