// tBaseImage.h
//
// Abstract base class for all tImageTYPE classes that load and save to a specific format.
//
// Copyright (c) 2022, 2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Foundation/tString.h>
#include <Math/tColour.h>
#include <Image/tPixelFormat.h>
#include "Image/tFrame.h"
namespace tImage
{
class tPicture;


// Abstract base class for all tImage types. At a minumum all tImageEXTs need to be able to be set from a single tFrame
// and return a single tFrame.
class tBaseImage
{
public:
	virtual ~tBaseImage()																								{ }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	virtual bool Set(tPixel4* pixels, int width, int height, bool steal = false)	= 0;

	// For multi-frame image types (say an animated webp), the first frame is chosen. The 'steal' bool tells the object
	// whether it is allowed to take ownership of the supplied frame, or whether it must copy the data out of it.
	// Returns true on success. Image will be invalid if false returned. If steal true, entire frame object is stolen.
	virtual bool Set(tFrame*, bool steal = true)									= 0;

	// Similar to above but sets from a tPicture. If steal is true, it takes the pixels from the picture and leaves it
	// in an invalid state.
	virtual bool Set(tPicture& picture, bool steal = true)							= 0;

	// For some formats (eg. .astc, .dds, .ktx) the internal representation may not be R8G8B8A8 unless a decode was
	// performed (which is optional). In these cases a new frame will be generated if the decode was performed, and
	// nullptr otherwise.
	//
	// Stealing a frame (the default) may or may-not invalidate an image. For multiframe image types, if there is more
	// than one frame, stealing just takes one away. Only if it was the last one, will it invalidate the object. In all
	// cases if steal is false, you are guaranteed the tImage is not modified. A new frame is created for you if
	// possible (again it won't force a decode for, say, ktx2 files).
	virtual tFrame* GetFrame(bool steal = true)										= 0;

	// After this call no memory will be consumed by the object and it will be invalid.
	virtual void Clear()															= 0;
	virtual bool IsValid() const													= 0;

	// Returns the original (src) pixel format of the image.
	virtual tPixelFormat GetPixelFormatSrc() const									= 0;

	// Returns the current pixel format of the image. Load paramters may have modified it from the original.
	virtual tPixelFormat GetPixelFormat() const										= 0;

	// Returns the original (src) colour profile of the image.
	virtual tColourProfile GetColourProfileSrc() const								{ return tColourProfile::Unspecified; }

	// Returns the current colour profile of the image. Load paramters may have modified it from the original.
	virtual tColourProfile GetColourProfile() const									{ return tColourProfile::Unspecified; }

	virtual tAlphaMode GetAlphaMode() const											{ return tAlphaMode::Unspecified; }
	virtual tChannelType GetChannelType() const										{ return tChannelType::Unspecified; }
};


// These are hany for all image types that may contain cubemaps.
enum tFaceIndex				: uint32
{
	tFaceIndex_Default,
	tFaceIndex_PosX			= tFaceIndex_Default,
	tFaceIndex_NegX,
	tFaceIndex_PosY,
	tFaceIndex_NegY,
	tFaceIndex_PosZ,
	tFaceIndex_NegZ,
	tFaceIndex_NumFaces
};

// Faces are always specified using a left-handed coord system even when using the OpenGL functions.
enum tFaceFlag				: uint32
{
	tFaceFlag_PosX			= 1 << tFaceIndex_PosX,
	tFaceFlag_NegX			= 1 << tFaceIndex_NegX,
	tFaceFlag_PosY			= 1 << tFaceIndex_PosY,
	tFaceFlag_NegY			= 1 << tFaceIndex_NegY,
	tFaceFlag_PosZ			= 1 << tFaceIndex_PosZ,
	tFaceFlag_NegZ			= 1 << tFaceIndex_NegZ,
	tFaceFlag_All			= 0xFFFFFFFF
};


}
