// tImageBMP.cpp
//
// This class knows how to load and save windows bitmap (.bmp) files into tPixel arrays. These tPixels may be 'stolen' by the
// tPicture's constructor if a targa file is specified. After the array is stolen the tImageBMP is invalid. This is
// purely for performance.
//
// The code in this module is a modification of code from https://github.com/phm97/bmp under the BSD 2-Clause License:
//
// Copyright (c) 2019, phm97
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
// following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
//    disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//    following disclaimer in the documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The modifications to use Tacent datatypes and conversion to C++ are under the ISC licence:
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

#include <System/tFile.h>
#include <Foundation/tArray.h>
#include "Image/tImageBMP.h"
#include "Image/tPicture.h"
using namespace tSystem;
namespace tImage
{


bool tImageBMP::Load(const tString& bmpFile)
{
	Clear();

	if ((tSystem::tGetFileType(bmpFile) != tSystem::tFileType::BMP) || !tFileExists(bmpFile))
		return false;

	tFileHandle file = tOpenFile(bmpFile.Chars(), "rb");
	if (!file)
		return false;
	
	Header bmpHeader;
	tStaticAssert(sizeof(bmpHeader) == 14);
	tReadFile(file, &bmpHeader, 14);
	if (bmpHeader.FourCC != FourCC)
	{
		tCloseFile(file);
		return false;
	}

	InfoHeader infoHeader;
	tStaticAssert(sizeof(infoHeader) == 40);
	tReadFile(file, &infoHeader, 40);

	// Some sanity-checking.
	// @todo Support jpeg and png compression.
	// @todo Support more than one plane.
	if
	(
		((infoHeader.HeaderSize != 40) && (infoHeader.HeaderSize != 108) && (infoHeader.HeaderSize != 124))	||
		(infoHeader.NumPlanes != 1)																			||
		((infoHeader.Compression == 4) || (infoHeader.Compression == 5))
	)
	{
		tCloseFile(file);
		return false;
	}

	Width = infoHeader.Width;
	Height = infoHeader.Height;

	bool flipped = false;
	if (Height < 0)
	{
		flipped = true;
		Height = -Height;
	}

	// Is this bmp indexed (using a palette)?
	PaletteColour* palette = nullptr;
	if (infoHeader.BPP <= 8)
	{
		tFileSeek(file, 14 + infoHeader.HeaderSize, tSeekOrigin::Set);
		if (infoHeader.ColoursUsed == 0)
		{
			// Only 1, 4, and 8 bit indexes allowed.
			if ((infoHeader.BPP != 1) && (infoHeader.BPP != 4) && (infoHeader.BPP != 8))
			{
				tCloseFile(file);
				Clear();			// Clears Width and Height.
				return false;
			}
			infoHeader.ColoursUsed = 1 << infoHeader.BPP;
		}
		palette = new PaletteColour[infoHeader.ColoursUsed];
		tReadFile(file, palette, sizeof(PaletteColour)*infoHeader.ColoursUsed);
	}

	uint8* buf = new uint8[Width * Height * 4];
	tStd::tMemset(buf, 0x00, Width*Height * 4);
	tFileSeek(file, bmpHeader.Offset, tSeekOrigin::Set);
	PixelFormatSrc = tPixelFormat::R8G8B8A8;

	switch (infoHeader.BPP)
	{
		case 32:
			ReadRow_Pixels32(file, buf);
			PixelFormatSrc = tPixelFormat::R8G8B8A8;
			break;

		case 24:
			ReadRow_Pixels24(file, buf);
			PixelFormatSrc = tPixelFormat::R8G8B8;
			break;

		case 16: 
			ReadRow_Pixels16(file, buf);
			PixelFormatSrc = tPixelFormat::B5G5R5A1;
			break;

		case 8:
			tAssert(palette);
			if (infoHeader.Compression == 1)
				ReadRow_IndexedRLE8(file, buf, palette);
			else
				ReadRow_Indexed8(file, buf, palette);
			PixelFormatSrc = tPixelFormat::PAL8BIT;
			break;

		case 4:
			tAssert(palette);
			if (infoHeader.Compression == 2)
				ReadRow_IndexedRLE4(file, buf, palette);
			else
				ReadRow_Indexed4(file, buf, palette);
			PixelFormatSrc = tPixelFormat::PAL4BIT;
			break;

		case 1:
			tAssert(palette);
			ReadRow_Indexed1(file, buf, palette);
			PixelFormatSrc = tPixelFormat::PAL1BIT;
			break;
	}

	Pixels = (tPixel*)buf;
	delete[] palette;
	tCloseFile(file);

	if (flipped)
	{
		tPixel* newPixels = new tPixel[Width * Height];
		for (int y = 0; y < Height; y++)
			for (int x = 0; x < Width; x++)
				newPixels[ y*Width + x ] = Pixels[ (Height-1-y)*Width + x ];
		delete[] Pixels;
		Pixels = newPixels;
	}
	
	return true;
}


void tImageBMP::ReadRow_Pixels32(tFileHandle file, uint8* dest)
{
	for (int p = 0; p < Width*Height; p++)
	{
		uint8 pixel[4];
		tReadFile(file, pixel, 4);
		*(dest++) = pixel[2];
		*(dest++) = pixel[1];
		*(dest++) = pixel[0];
		*(dest++) = pixel[3];
	}
}


void tImageBMP::ReadRow_Pixels24(tFileHandle file, uint8* dest)
{
	int rowPad = (Width*3 % 4) ? 4 - (Width*3 % 4) : 0;
	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			uint8 pixel[3];
			tReadFile(file, pixel, 3);
			*(dest++) = pixel[2];
			*(dest++) = pixel[1];
			*(dest++) = pixel[0];
			*(dest++) = 0xFF;
		}

		if (rowPad)
			tFileSeek(file, rowPad, tSeekOrigin::Current);
	}
}


void tImageBMP::ReadRow_Pixels16(tFileHandle file, uint8* dest)
{
	int rowPad = (Width*2 % 4) ? 4 - (Width*2 % 4) : 0;
	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			uint16 pixel;
			tReadFile(file, &pixel, 2);
			uint16 b = (pixel >> 10)	& 0x1F;
			uint16 g = (pixel >> 5)		& 0x1F;
			uint16 r = (pixel >> 0)		& 0x1F;
			*(dest++) = r*8;
			*(dest++) = g*8;
			*(dest++) = b*8;
			*(dest++) = 0xFF;		// @todo Correct? Should we not read the 1-bit alpha?
		}

		if (rowPad)
			tFileSeek(file, rowPad, tSeekOrigin::Current);
	}
}


void tImageBMP::ReadRow_Indexed8(tFileHandle file, uint8* dest, PaletteColour* palette)
{
	int rowPad = (Width % 4) ? 4 - (Width % 4) : 0;
	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			uint8 index;
			tReadFile(file, &index, 1);
			*(dest++) = palette[index].R;
			*(dest++) = palette[index].G;
			*(dest++) = palette[index].B;
			*(dest++) = 0xFF;
		}
		if (rowPad)
			tFileSeek(file, rowPad, tSeekOrigin::Current);
	}
}


void tImageBMP::ReadRow_Indexed4(tFileHandle file, uint8* dest, PaletteColour* palette)
{
	int size = (Width+1) / 2;
	int rowPad = (size % 4) ? 4 - (size % 4) : 0;
	tArray<uint8> rowStride(size);

	for (int y = 0; y < Height; y++)
	{
		tReadFile(file, (uint8*)rowStride, size);
		for (int x = 0; x < Width; x++)
		{
			uint8 byte = rowStride[x/2];
			uint8 index = (x % 2) ? 0x0F & byte : byte >> 4;
			*(dest++) = palette[index].R;
			*(dest++) = palette[index].G;
			*(dest++) = palette[index].B;
			*(dest++) = 0xFF;
		}
		if (rowPad)
			tFileSeek(file, rowPad, tSeekOrigin::Current);
	}
}


void tImageBMP::ReadRow_Indexed1(tFileHandle file, uint8* dest, PaletteColour* palette)
{
	int size = (Width + 7) / 8;
	int rowPad = (size % 4) ? 4 - (size % 4) : 0;
	tArray<uint8> rowStride(size);
	
	for (int y = 0; y < Height; y++)
	{
		tReadFile(file, (uint8*)rowStride, size);
		for (int x = 0; x < Width; x++)
		{
			int bit = (x % 8) + 1;
			uint8 byte = rowStride[x/8];
			uint8 index = byte >> (8-bit);
			index &= 0x01;
			*(dest++) = palette[index].R;
			*(dest++) = palette[index].G;
			*(dest++) = palette[index].B;
			*(dest++) = 0xFF;
		}
		if (rowPad)
			tFileSeek(file, rowPad, tSeekOrigin::Current);
	}
}


void tImageBMP::ReadRow_IndexedRLE8(tFileHandle file, uint8* dest, PaletteColour* palette)
{
	uint8 byte = 0;
	int currentLine = 0;
	uint8* d = dest;

	bool keepReading = true;
	while (keepReading)
	{
		byte = tGetc(file);
		if (byte)
		{
			int index = tGetc(file);
			for (int i = 0; i < byte; i++)
			{
				*d++ = palette[index].R;
				*d++ = palette[index].G;
				*d++ = palette[index].B;
				*d++ = 0xFF;
			}
		}
		else
		{
			byte = tGetc(file);
			switch (byte) 
			{
				case 0:
					currentLine++;
					d = dest + currentLine*Width*4;
					break;

				case 1:
					keepReading = false;
					break;

				case 2:
				{
					int xoffset = tGetc(file);
					int yoffset = tGetc(file);
					currentLine += yoffset;
					d += yoffset*Width*4 + xoffset*4;
					break;
				}

				default:
				{
					for (int i = 0; i < byte; i++)
					{
						int index = tGetc(file);
						*d++ = palette[index].R;
						*d++ = palette[index].G;
						*d++ = palette[index].B;
						*d++ = 0xFF;
					}
					if (byte % 2)
						tFileSeek(file, 1, tSeekOrigin::Current);
					break;
				}
			}
		}
		if (d >= dest + Width*Height*4)
			keepReading = false;
	}
}


void tImageBMP::ReadRow_IndexedRLE4(tFileHandle file, uint8* dest, PaletteColour* palette)
{
	int currentLine = 0;
	uint8* d = dest;
	bool keepReading = true;
	while (keepReading)
	{
		uint8 byte1 = tGetc(file);
		if (byte1)
		{
			uint8 byte2 = tGetc(file);
			uint8 index1 = byte2 >> 4;
			uint8 index2 = byte2 & 0x0F;
			for (int i = 0; i < (byte1 / 2); i++)
			{
				*(d++) = palette[index1].R;
				*(d++) = palette[index1].G;
				*(d++) = palette[index1].B;
				*(d++) = 0x0F;

				*(d++) = palette[index2].R;
				*(d++) = palette[index2].G;
				*(d++) = palette[index2].B;
				*(d++) = 0x0F;
			}
			if (byte1 % 2)
			{
				*(d++) = palette[index1].R;
				*(d++) = palette[index1].G;
				*(d++) = palette[index1].B;
				*(d++) = 0x0F;
			}
		}
		else // Absolute mode.
		{	
			uint8 byte2 = tGetc(file);
			switch (byte2)
			{
				case 0:
					currentLine++;
					d = dest + currentLine*Width*4;
					break;

				case 1:
					keepReading = false;
					break;

				case 2:
				{
					int xoffset = tGetc(file);
					int yoffset = tGetc(file);
					currentLine += yoffset;
					d += yoffset*Width*4 + xoffset*4;
					break;
				}

				default:
					for (int i = 0; i < (byte2/2); i++)
					{
						byte1 = tGetc(file);
						uint8 index1 = byte1 >> 4;
						*(d++) = palette[index1].R;
						*(d++) = palette[index1].G;
						*(d++) = palette[index1].B;
						*(d++) = 0x0F;

						uint8 index2 = byte1 & 0x0F;
						*(d++) = palette[index2].R;
						*(d++) = palette[index2].G;
						*(d++) = palette[index2].B;
						*(d++) = 0x0F;
					}
					if (byte2 % 2)
					{
						byte1 = tGetc(file);
						uint8 index = byte1 >> 4;
						*(d++) = palette[index].R;
						*(d++) = palette[index].G;
						*(d++) = palette[index].B;
						*(d++) = 0x0F;
					}
					if (((byte2 + 1)/2) % 2)
						tFileSeek(file, 1, tSeekOrigin::Current);
					break;
			}
		}
		if (d >= dest + Width*Height*4)
			keepReading = false;
	}
}


bool tImageBMP::Set(tPixel* pixels, int width, int height, bool steal)
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
		Pixels = new tPixel[Width*Height];
		tStd::tMemcpy(Pixels, pixels, Width*Height*sizeof(tPixel));
	}

	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	return true;
}


bool tImageBMP::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	Set(frame->GetPixels(steal), frame->Width, frame->Height, steal);
	if (steal)
		delete frame;

	return true;
}


bool tImageBMP::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


tFrame* tImageBMP::GetFrame(bool steal)
{
	if (!IsValid())
		return nullptr;

	tFrame* frame = new tFrame();
	frame->PixelFormatSrc = PixelFormatSrc;

	if (steal)
	{
		frame->StealFrom(Pixels, Width, Height);
		Pixels = nullptr;
	}
	else
	{
		frame->Set(Pixels, Width, Height);
	}
	return frame;
}


tImageBMP::tFormat tImageBMP::Save(const tString& bmpFile, tFormat format) const
{
	SaveParams params;
	params.Format = format;
	return Save(bmpFile, params);
}


tImageBMP::tFormat tImageBMP::Save(const tString& bmpFile, const SaveParams& params) const
{
	tFormat format = params.Format;
	if (!IsValid() || (format == tFormat::Invalid))
		return tFormat::Invalid;

	if (tSystem::tGetFileType(bmpFile) != tSystem::tFileType::BMP)
		return tFormat::Invalid;

	if (format == tFormat::Auto)
		format = IsOpaque() ? tFormat::BPP24 : tFormat::BPP32;

	tFileHandle file = tOpenFile(bmpFile.Chars(), "wb");
	if (!file)
		return tFormat::Invalid;

	int bytesPerPixel = (format == tFormat::BPP24) ? 3 : 4;

	Header bmpHeader;
	bmpHeader.FourCC				= FourCC;
	bmpHeader.Size					= Width*Height*bytesPerPixel + 54;
	bmpHeader.AppId					= 0;
	bmpHeader.Offset				= 54;
	tWriteFile(file, &bmpHeader, sizeof(Header));
	
	InfoHeader infoHeader;
	infoHeader.HeaderSize			= 40;
	infoHeader.Width				= Width;
	infoHeader.Height				= Height;
	infoHeader.NumPlanes			= 1;
	infoHeader.BPP					= bytesPerPixel*8;
	infoHeader.Compression			= 0;
	infoHeader.ImageSize			= Width * Height * bytesPerPixel;
	infoHeader.HorizontalResolution	= 0;
	infoHeader.VerticalResolution	= 0;
	infoHeader.ColoursUsed			= 0;
	infoHeader.ColoursImportant		= 0;
	tWriteFile(file, &infoHeader, sizeof(InfoHeader));
	
	uint8* p = (uint8*)Pixels;
	int rowPad = (Width*bytesPerPixel % 4) ? 4 - (Width*bytesPerPixel % 4) : 0;
	uint8 zeroPad[4];
	tStd::tMemset(zeroPad, 0, 4);
	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			uint8 sample[4];
			sample[2] = *p++;
			sample[1] = *p++;
			sample[0] = *p++;
			sample[3] = *p++;
			tWriteFile(file, sample, bytesPerPixel);
		}
		if (rowPad)
			tWriteFile(file, zeroPad, rowPad);
	}
	
	tCloseFile(file);
	return format;
}


bool tImageBMP::IsOpaque() const
{
	for (int p = 0; p < (Width*Height); p++)
	{
		if (Pixels[p].A < 255)
			return false;
	}

	return true;
}


tPixel* tImageBMP::StealPixels()
{
	tPixel* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	return pixels;
}


}
