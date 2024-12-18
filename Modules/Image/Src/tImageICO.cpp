// tImageICO.cpp
//
// This class knows how to load windows icon (ico) files. It loads the data into multiple tPixel arrays, one for each
// part (ico files may be multiple images at different resolutions). These arrays may be 'stolen' by tPictures. The
// loading code is a modificaton of code from Victor Laskin. In particular the code now:
// a) Loads all parts of an ico, not just the biggest one.
// b) Supports embedded png images.
// c) Supports widths and heights of 256.
//
// Copyright (c) 2020-2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.
//
// Includes modified version of code from Victor Laskin. Here is Victor Laskin's header/licence in the original ico.cpp:
//
// Code by Victor Laskin (victor.laskin@gmail.com)
// Rev 2 - 1bit color was added, fixes for bit mask.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <Foundation/tStandard.h>
#include <Foundation/tString.h>
#include <System/tFile.h>
#include "Image/tImageICO.h"
#include "Image/tImagePNG.h"			// ICO files may have embedded PNGs.
#include "Image/tPicture.h"
using namespace tSystem;
namespace tImage
{

	
// These structs represent how the icon information is stored in an ICO file.
struct IconDirEntry
{
	uint8				Width;			// Width of the image.
	uint8				Height;			// Height of the image (times 2).
	uint8				ColorCount;		// Number of colors in image (0 if >=8bpp).
	uint8				Reserved;
	uint16				Planes;			// Colour planes.
	uint16				BitCount;		// Bits per pixel.
	uint32				BytesInRes;		// How many bytes in this resource?
	uint32				ImageOffset;	// Where in the file is this image.
};


struct IconDir
{
	uint16				Reserved;
	uint16				Type;			// Resource type (1 for icons).
	uint16				Count;			// How many images?
	//IconDirEntrys follow. One for each image.
};


// size - 40 bytes
struct BitmapInfoHeader
{
	uint32				Size;
	uint32				Width;
	uint32				Height;			// Icon Height (added height of XOR-Bitmap and AND-Bitmap).
	uint16				Planes;
	uint16				BitCount;
	uint32				Compression;
	int32				SizeImage;
	uint32				XPelsPerMeter;
	uint32				YPelsPerMeter;
	uint32				ClrUsed;
	uint32				ClrImportant;
};


struct IconImage
{
	BitmapInfoHeader	Header;			// DIB header.
	uint32				Colours[1];		// Color table (short 4 bytes) //RGBQUAD.
	uint8				XOR[1];			// DIB bits for XOR mask.
	uint8				AND[1];			// DIB bits for AND mask.
};
	

bool tImageICO::Load(const tString& icoFile)
{
	Clear();

	if (tGetFileType(icoFile) != tFileType::ICO)
		return false;

	if (!tFileExists(icoFile))
		return false;

	int numBytes = 0;
	uint8* icoFileInMemory = tLoadFile(icoFile, nullptr, &numBytes);
	bool success = Load(icoFileInMemory, numBytes);	
	delete[] icoFileInMemory;

	return success;
}


bool tImageICO::Load(const uint8* icoFileInMemory, int numBytes)
{
	Clear();
	IconDir* icoDir = (IconDir*)icoFileInMemory;
	int iconsCount = icoDir->Count;

	if (icoDir->Reserved != 0)
		return false;
		
	if (icoDir->Type != 1)
		return false;

	if (iconsCount == 0)
		return false;
		
	if (iconsCount > 20)
		return false;

	const uint8* cursor = icoFileInMemory;
	cursor += 6;
	IconDirEntry* dirEntry = (IconDirEntry*)cursor;

	for (int i = 0; i < iconsCount; i++)
	{
		int w = dirEntry->Width;
		if (w == 0)
			w = 256;
			
		int h = dirEntry->Height;
		if (h == 0)
			h = 256;
			
		int offset = dirEntry->ImageOffset;
		if (!offset || (offset >= numBytes))
			continue;
			
		tFrame* newFrame = CreateFrame(icoFileInMemory+offset, w, h, numBytes);
		if (!newFrame)
			continue;

		Frames.Append(newFrame);
		dirEntry++;
	}
	if (Frames.IsEmpty())
		return false;

	PixelFormatSrc = GetBestSrcPixelFormat();
	PixelFormat = tPixelFormat::R8G8B8A8;

	// From a file image we always assume it was sRGB.
	ColourProfileSrc = tColourProfile::sRGB;
	ColourProfile = tColourProfile::sRGB;
	return true;
}


bool tImage::tImageICO::Set(tList<tFrame>& srcFrames, bool stealFrames)
{
	Clear();
	if (srcFrames.GetNumItems() <= 0)
		return false;

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

	PixelFormatSrc		= GetBestSrcPixelFormat();
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume srcFrames must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

	return true;
}


bool tImageICO::Set(tPixel4b* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	tFrame* frame = new tFrame();
	if (steal)
		frame->StealFrom(pixels, width, height);
	else
		frame->Set(pixels, width, height);
	Frames.Append(frame);

	PixelFormatSrc		= tPixelFormat::R8G8B8A8;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume pixels must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

	return true;
}


bool tImageICO::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	PixelFormatSrc = frame->PixelFormatSrc;
	PixelFormat = tPixelFormat::R8G8B8A8;
	if (steal)
		Frames.Append(frame);
	else
		Frames.Append(new tFrame(*frame));

	PixelFormatSrc		= frame->PixelFormatSrc;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	ColourProfileSrc	= tColourProfile::sRGB;		// We assume frame must be sRGB.
	ColourProfile		= tColourProfile::sRGB;

	return true;
}


bool tImageICO::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	PixelFormatSrc		= picture.PixelFormatSrc;
	PixelFormat			= tPixelFormat::R8G8B8A8;
	// We don't know colour profile of tPicture.

	// This is worth some explanation. If steal is true the picture becomes invalid and the
	// 'set' call will steal the stolen pixels. If steal is false GetPixels is called and the
	// 'set' call will memcpy them out... which makes sure the picture is still valid after and
	// no-one is sharing the pixel buffer. We don't check the success of 'set' because it must
	// succeed if picture was valid.
	tPixel4b* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	bool success = Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
	tAssert(success);
	return true;
}


tFrame* tImageICO::GetFrame(bool steal)
{
	if (!IsValid())
		return nullptr;

	return steal ? Frames.Remove() : new tFrame( *Frames.First() );
}


tFrame* tImageICO::CreateFrame(const uint8* cursor, int width, int height, int numBytes)
{
	IconImage* icon = (IconImage*)cursor;
	
	// ICO files may have embedded pngs.
	if (icon->Header.Size == 0x474e5089)
	{
		tImagePNG::LoadParams params;
		tAssert(params.Flags & tImagePNG::LoadFlag_ForceToBpc8);
		tImagePNG pngImage(cursor, numBytes, params);
		if (!pngImage.IsValid())
			return nullptr;

		width = pngImage.GetWidth();
		height = pngImage.GetHeight();
		tAssert((width > 0) && (height > 0));

		tPixel4b* pixels = pngImage.StealPixels8();
		bool isOpaque = pngImage.IsOpaque();
		
		tFrame* newFrame = new tFrame;
		newFrame->PixelFormatSrc = isOpaque ? tPixelFormat::R8G8B8 : tPixelFormat::R8G8B8A8;
		newFrame->Width = width;
		newFrame->Height = height;
		newFrame->Pixels = pixels;
		return newFrame;
	}
	
	int realBitsCount = int(icon->Header.BitCount);
	bool hasAndMask = (realBitsCount < 32) && (height != icon->Header.Height);

	cursor += 40;
	int numPixels = width * height;
	
	tPixel4b* pixels = new tPixel4b[numPixels];
	uint8* image = (uint8*)pixels;
	tPixelFormat srcPixelFormat = tPixelFormat::Invalid;
	
	// rgba + vertical swap
	if (realBitsCount == 32)
	{
		srcPixelFormat = tPixelFormat::R8G8B8A8;
		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
			{
				int shift = 4 * (x + y * width);
				
				// Rows from bottom to top.
				// int shift2 = 4 * (x + (height - y - 1) * width);
				int shift2 = 4 * (x + y * width);
				
				image[shift] = cursor[shift2 +2];
				image[shift+1] = cursor[shift2 +1];
				image[shift+2] = cursor[shift2 ];
				image[shift+3] = cursor[shift2 +3];
			}
		}
	}

	if (realBitsCount == 24)
	{
		srcPixelFormat = tPixelFormat::R8G8B8;
		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
			{
				int shift = 4 * (x + y * width);
				
				// Rows from bottom to top.
				// int shift2 = 3 * (x + (height - y - 1) * width);
				int shift2 = 3 * (x + y * width);
				
				image[shift] = cursor[shift2 +2];
				image[shift+1] = cursor[shift2 +1];
				image[shift+2] = cursor[shift2 ];
				image[shift+3] = 255;
			}
		}
	}

	if (realBitsCount == 8)
	{
		// 256 colour palette.
		srcPixelFormat = tPixelFormat::PAL8BIT;
		
		uint8* colors = (uint8*)cursor;
		cursor += 256 * 4;
		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
			{
				int shift = 4 * (x + y * width);
				
				// Rows from bottom to top.
				// int shift2 = (x + (height - y - 1) * width);
				int shift2 = (x + y * width);
				
				int index = 4 * cursor[shift2];
				image[shift] = colors[index + 2];
				image[shift+1] = colors[index + 1];
				image[shift+2] = colors[index ];
				image[shift+3] = 255;
			}
		}
	}

	if (realBitsCount == 4)
	{
		// 16 colour palette.
		srcPixelFormat = tPixelFormat::PAL4BIT;

		uint8* colors = (uint8*)cursor;
		cursor += 16 * 4;
		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
			{
				int shift = 4 * (x + y * width);

				// Rows from bottom to top.
				// int shift2 = (x + (height - y - 1) * width);
				int shift2 = (x + y * width);

				uint8 index = cursor[shift2 / 2];
				if (shift2 % 2 == 0)
					index = (index >> 4) & 0xF;
				else
					index = index & 0xF;
				index *= 4;

				image[shift] = colors[index + 2];
				image[shift+1] = colors[index + 1];
				image[shift+2] = colors[index ];
				image[shift+3] = 255;
			}
		}
	}

	if (realBitsCount == 1)
	{
		// 2 colour palette.
		srcPixelFormat = tPixelFormat::PAL1BIT;

		uint8* colors = (uint8*)cursor;
		cursor += 2 * 4;

		int boundary = width; //!!! 32 bit boundary (http://www.daubnet.com/en/file-format-ico)
		while (boundary % 32 != 0)
			boundary++;

		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
			{
				int shift = 4 * (x + y * width);

				// Rows from bottom to top.
				// int shift2 = (x + (height - y - 1) * boundary);
				int shift2 = (x + y * boundary);
				
				uint8 index = cursor[shift2 / 8];

				// Select 1 bit only.
				uint8 bit = 7 - (x % 8);
				index = (index >> bit) & 0x01;
				index *= 4;
				
				image[shift] = colors[index + 2];
				image[shift+1] = colors[index + 1];
				image[shift+2] = colors[index ];
				image[shift+3] = 255;
			}
		}
	}

	// Read AND mask after base color data - 1 BIT MASK
	if (hasAndMask)
	{
		int boundary = width * realBitsCount; //!!! 32 bit boundary (http://www.daubnet.com/en/file-format-ico)
		while (boundary % 32 != 0)
			boundary++;
		cursor += boundary * height / 8;

		boundary = width;
		while (boundary % 32 != 0)
			boundary++;

		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				int shift = 4 * (x + y * width) + 3;
				uint8 bit = 7 - (x % 8);

				// Rows from bottom to top.
				// int shift2 = (x + (height - y - 1) * boundary) / 8;
				int shift2 = (x + y * boundary) / 8;

				int mask = (0x01 & ((unsigned char)cursor[shift2] >> bit));
				image[shift] *= 1 - mask;
			}
		}
	}
	
	tFrame* newFrame = new tFrame;
	newFrame->PixelFormatSrc = srcPixelFormat;
	newFrame->Width = width;
	newFrame->Height = height;
	newFrame->Pixels = pixels;

	return newFrame;
}


}
