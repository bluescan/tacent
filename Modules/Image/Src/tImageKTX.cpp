// tImageKTX.cpp
//
// This knows how to load/save KTX files. It knows the details of the ktx and ktx2 file format and loads the data into
// multiple tPixel arrays, one for each frame (KTKs may be animated). These arrays may be 'stolen' by tPictures.
//
// Copyright (c) 2022 Tristan Grimmer.
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
#include "Image/tImageKTX.h"

#define KHRONOS_STATIC
#include "LibKTX/include/ktx.h"

using namespace tSystem;
namespace tImage
{


bool tImageKTX::Load(const tString& ktxFile)
{
	Clear();
	tFileType fileType = tSystem::tGetFileType(ktxFile);
	if ((fileType != tSystem::tFileType::KTX) && (fileType != tSystem::tFileType::KTX2))
		return false;

	if (!tFileExists(ktxFile))
		return false;

	int numBytes = 0;
	uint8* ktxFileInMemory = tLoadFile(ktxFile, nullptr, &numBytes, true);
	bool success = Set(ktxFileInMemory, numBytes);
	delete[] ktxFileInMemory;

	return success;
}


bool tImageKTX::Set(uint8* ktxFileInMemory, int numBytes)
{
	ktx_size_t offset;

	ktxTexture* texture = nullptr;
	ktx_error_code_e result = ktxTexture_CreateFromMemory(ktxFileInMemory, numBytes, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
	if (!texture)
		return false;

	ktx_uint32_t numLevels = texture->numLevels;
	ktx_uint32_t baseWidth = texture->baseWidth;
	bool isArray = texture->isArray;
	bool isCompressed = texture->isCompressed;

	// We need to determine the pixel-format of the data. To do this we first need to know if we are dealing with
	// a ktx1 (OpenGL-style) or ktx2 (Vulkan-style) file.
	ktxTexture1* ktx1 = (texture->classId == ktxTexture1_c) ? (ktxTexture1*)texture : nullptr;
	ktxTexture2* ktx2 = (texture->classId == ktxTexture2_c) ? (ktxTexture2*)texture : nullptr;
	if ((ktx1 && ktx2) || (!ktx1 && !ktx2))
		return false;

	tPixelFormat pixelFormat = tPixelFormat::Invalid;
	if (ktx1)
		pixelFormat = GetPixelFormatFromGLFormat(ktx1->glInternalformat);
	else if (ktx2)
		pixelFormat = GetPixelFormatFromVKFormat(ktx2->vkFormat);
	if (pixelFormat == tPixelFormat::Invalid)
		return false;
	
	// Retrieve a data pointer to the image for a specific miplevel, arraylayer, and slice (face or depth.
	ktx_uint32_t miplevel = 1;
	ktx_uint32_t arraylayer = 0;
	ktx_uint32_t slice = 3;
	result = ktxTexture_GetImageOffset(texture, miplevel, arraylayer, slice, &offset);
	uint8* pixelData = ktxTexture_GetData(texture) + offset;

	// WIP Convert / decode the pixelData based on the pixelFormat into R8G8B8A8.

	ktxTexture_Destroy(texture);
	return true;
}


tPixelFormat GetPixelFormatFromGLFormat(uint32 glFormat)
{
	// WIP
	return tPixelFormat::Invalid;
}


tPixelFormat GetPixelFormatFromVKFormat(uint32 vkFormat)
{
	// WIP
	return tPixelFormat::Invalid;
}


}
