// tImagePVR.cpp
//
// This class knows how to load PowerVR (.pvr) files. It knows the details of the pvr file format and loads
// the data into tLayers, optionally decompressing them. Saving is not implemented yet. The layers may be 'stolen' from
// a tImagePVR so that excessive memcpys are avoided. After they are stolen the tImagePVR is invalid.
//
// Copyright (c) 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tString.h>
#include "Image/tImagePVR.h"
#include "Image/tPixelUtil.h"
#include "Image/tPicture.h"
namespace tImage
{


tImagePVR::tImagePVR()
{
}


tImagePVR::tImagePVR(const tString& pvrFile, const LoadParams& loadParams) :
	Filename(pvrFile)
{
	Load(pvrFile, loadParams);
}


tImagePVR::tImagePVR(const uint8* pvrFileInMemory, int numBytes, const LoadParams& loadParams)
{
	Load(pvrFileInMemory, numBytes, loadParams);
}


void tImagePVR::Clear()
{
	Results							= 0;							// This means no results.
	PixelFormat						= tPixelFormat::Invalid;
	PixelFormatSrc					= tPixelFormat::Invalid;
	ColourProfile					= tColourProfile::Unspecified;
	ColourProfileSrc				= tColourProfile::Unspecified;
	AlphaMode						= tAlphaMode::Unspecified;
	RowReversalOperationPerformed	= false;
}


bool tImagePVR::Set(tPixel* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	PixelFormat						= tPixelFormat::R8G8B8A8;
	PixelFormatSrc					= tPixelFormat::R8G8B8A8;
	ColourProfile					= tColourProfile::LDRsRGB_LDRlA;
	ColourProfileSrc				= tColourProfile::LDRsRGB_LDRlA;
	AlphaMode						= tAlphaMode::Normal;
	RowReversalOperationPerformed	= false;

	return true;
}


bool tImagePVR::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	tPixel* pixels = frame->GetPixels(steal);
	Set(pixels, frame->Width, frame->Height, steal);
	if (steal)
		delete frame;

	return true;
}


bool tImagePVR::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


tFrame* tImagePVR::GetFrame(bool steal)
{
	return nullptr;
}


bool tImagePVR::Load(const tString& pvrFile, const LoadParams& loadParams)
{
	Clear();
	Filename = pvrFile;
	if (tSystem::tGetFileType(pvrFile) != tSystem::tFileType::PVR)
	{
		Results |= 1 << int(ResultCode::Fatal_IncorrectFileType);
		return false;
	}

	if (!tSystem::tFileExists(pvrFile))
	{
		Results |= 1 << int(ResultCode::Fatal_FileDoesNotExist);
		return false;
	}

	int pvrSizeBytes = 0;
	uint8* pvrData = (uint8*)tSystem::tLoadFile(pvrFile, 0, &pvrSizeBytes);
	bool success = Load(pvrData, pvrSizeBytes, loadParams);
	delete[] pvrData;

	return success;
}


bool tImagePVR::Load(const uint8* pvrData, int pvrDataSize, const LoadParams& paramsIn)
{
	Clear();
	return false;
}


}
