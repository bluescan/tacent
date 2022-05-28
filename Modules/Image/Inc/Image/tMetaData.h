// tMetaData.h
//
// A class to store image meta-data. Some image formats allow comments and other metadata to be stored inside the
// image. For example, jpg files may contain EXIF or XMP meta-data. This class is basically a map of key/value strings
// that may be a member of some tImageXXX types, It currently knows how to parse EXIF and XMP meta-data.
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

#pragma once
#include <Foundation/tMap.h>
#include <Foundation/tString.h>
namespace tImage
{


// These are the common metadata tags, often extracted from either EXIF and/or XMP data.
enum class tMetaTag
{
//	Tag Name		Type		Description
	Invalid = -1,
	LatitudeDD,		//	float	Decimal degrees.
	LatitudeDMS,	//	string	Degrees, Minutes, Seconds, Direction. Eg. 42°33'56"N
	LongitudeDD,	//	float	Decimal degrees.
	LongitudeDMS,	//	string	Degrees, Minutes, Seconds, Direction. Eg. 160°59'4"W
	Make,			//	string	Camera make. eg. "Canon".
	Model,			//	string	Camera model. eg "Nikon Coolpix 5000".
	ExposureTime,	//	float	Exposure time in seconds.
	FStop,			//	float	F/Stop. Unitless. Ratio of the lens focal length to the diameter of the entrance pupil.
	ExposureProgram,//	uint32	Exposure Program.
					//			0: Not Defined.
					//			1: Manual.
					//			2: Normal Program.
					//			3: Aperture Priority.
					//			4: Shutter Priority.
					//			5: Creative Program.
					//			6: Action Program.
					//			7: Portrait Mode.
					//			8: Landscape Mode.
	ISO,			//	uint32	Equivalent ISO film speed rating.
	ShutterSpeed,	//	float	Units s^-1. Reciprocal of exposure time.
	Aperture,		//	float	APEX units.
	ExposureBias,	//	float	Exposure bias. APEX units.
	MeteringMode,	//	uint32	Metering Mode.
					//			0: Unknown.
					//			1: Average.
					//			2: Center Weighted Average.
					//			3: Spot.
					//			4: Multi-spot.
					//			5: Pattern.
					//			6: Partial.
	Flash,			//	uint32	Flash Info.
					//			(Flash & 1)			:	Used.
					//									0: No flash.
					//									1: Flash Used.
					//			((Flash & 6) >> 1)	:	Light Status.
					//									0: No Strobe Return Detection Function.
					//									1: Reserved.
					//									2: Strobe Return Light Not Detected.
					//									3: Strobe Return Light Detected.
					//			((Flash & 24) >> 3)	:	Mode.
					//									0: Unknown.
					//									1: Compulsory Flash Firing.
					//									2: Compulsory Flash Suppression.
					//									3: Auto Mode.
					//			((Flash & 32) >> 5)	:	Function.
					//									0: Flash Present,
					//									1: No Flash Present.
					//			((Flash & 64) >> 6)	:	Red-Eye Reduction.
					//									0: No Red-Eye Reduction Mode Or Unknown.
					//									1: Red-Eye Reduction Supported.
	FocalLength,	//	float	Focal length in pixels.
	Orientation,	//	uint32	Orientation.
					//			Note the descriptions below describe the transformations that are present in the data in the current file.
					//			Reverse ops in reverse order to obtain an untransformed image.
					//			0: Unspecified.
					//			1: NoTransforms.		Image data is not mirrored or rotated.
					//			2: Flip-Y.				Image is mirrored about vertical axis (right <-> left).
					//			3: Flip-XY.				Same as 180 Degrees Rotation.
					//			4: Flip-X.				The image is mirrored about horizontal axis (top <-> bottom).
					//			5: Rot-CW90  Flip-Y.	The image is rotated 90 degrees clockwise and then flipped about verical axis.
					//			6: Rot-ACW90.			The image is rotated 90 degrees anti-clockwise.
					//			7: Rot-ACW90 Flip-Y.	The image is rotated 90 degrees clockwise and then flipped about verical axis.
					//			8: Rot-CW90.			The image is rotated 90 degrees anti-clockwise.
	XResolution,
	YResolution,
	ResolutionUnit,
	ImageWidth,
	ImageHeight,
	ModifyDate,
	DateTimeOriginal,
	Copyright
};



class tMetaData
{
public:
	tMetaData() { }

	tMetaData(const tMetaData&);
	virtual ~tMetaData() { }

	bool IsValid() const { return (KeyValueMap.GetNumItems() > 0); }

private:
	tMap<tString, tString> KeyValueMap;
};


// Implementation below this line.


}
