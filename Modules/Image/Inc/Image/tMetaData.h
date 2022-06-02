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
#include <Foundation/tString.h>
namespace TinyEXIF { class EXIFInfo; }


namespace tImage
{


// These are the common metadata tags, often extracted from either EXIF and/or XMP data.
enum class tMetaTag
{
//	Tag Name		Type		Description
	Invalid = -1,

	// Camera Hardware Tags
	Make,			//	string	Camera make. eg. "Canon".
	Model,			//	string	Camera model. eg "Nikon Coolpix 5000".
	SerialNumber,	//	string	Serial number of the camera.

	// Geo Location Tags
	LatitudeDD,		//	float	Decimal degrees.
	LatitudeDMS,	//	string	Degrees, Minutes, Seconds, Direction. Eg. 42°33'56"N
	LongitudeDD,	//	float	Decimal degrees.
	LongitudeDMS,	//	string	Degrees, Minutes, Seconds, Direction. Eg. 160°59'4"W
	Altitude,		//	float	Altitude in meters relative to sea-level.
	AltitudeRelGnd,	//	string	Relative altitude ground reference. Applies to AltitudeRel value.
					//			"No Reference"		: Reference data unavailable.
					//			"Above Sea Level"	: Ground is above sea level.
					//			"Below Sea Level"	: Ground is below sea level.
	AltitudeRel,	//	float	Relative altitude in meters. Likely how high above ground.
	Roll,			//	float	Flight roll in degrees.
	Pitch,			//	float	Flight pitch in degrees.
	Yaw,			//	float	Flight yaw in degrees.
					//			The velocity/speed below are from a DJI MakerNotes. I want a DJI mini 3. Looks amazing.
	VelX,			//	float	X-Component of velocity in m/s. May be negative. X appears to be forwards direction.
	VelY,			//	float	Y-Component of velocity in m/s. May be negative. @todo Is Y left or right? RH or LH coord system?
	VelZ,			//	float	Z-Component of velocity in m/s. May be negative. @todo pos Z = up?
	Speed,			//	float	Length of velocity vector. m/s. Speed is always >= 0.
	GPSSurvey,		//	string	Geodetic survey data.
	GPSTimeStamp,	//	string	UTC Date and time as string in format "YYYY-MM-DD hh:mm:ss".
					//			It's possible one of the YYYY-MM-DD or hh:mm:ss parts is missing. You will get nothing/invalid
					//			or "YYYY-MM-DD" or "hh:mm:ss" or both: "YYYY-MM-DD hh:mm:ss" depending on what is available.

	// Camera Settings Tags
	ShutterSpeed,	//	float	Units s^-1. Reciprocal of exposure time. If not set, computed.
	ExposureTime,	//	float	Exposure time in seconds. Reciprocal of ShutterSpeed. If not set, computed.
	ExposureBias,	//	float	Exposure bias. APEX units.
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
	Aperture,		//	float	APEX units.
	Brightness,		//	float	Average scene luminance of whole image. APEX units.
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
	XPixelsPerUnit,	//	float	Horizontal pixels per DistanceUnit. AKA: XResolution.
	YPixelsPerUnit,	//	float	Veritical pixels per Distanceunit.  AKA: YResolution.
	LengthUnit,		//	uint32	The length unit used for XPixelsPerUnit and YPixelsPerUnit. AKA: ResolutionUnit.
					//			1: Not Specified.
					//			2: Inch.
					//			3: cm.
	BitsPerSample,	//	uint32	Bits per colour component. Not bits per pixel.
	ImageWidth,		//	uint32	Width in pixels.
	ImageHeight,	//	uint32	Height in pixels.
	ImageWidthOrig,	//	uint32	Original image width in pixels.
	ImageHeightOrig,//	uint32	Original image height in pixels.
	DateTimeChange,	//	string	Date and time the image was changed. "YYYY-MM-DD hh:mm:ss".
	DateTimeOrig,	//	string	Date and time of original image.
	DateTimeDigit,	//	string	Date and time the image was digitized.

	// Authoring Note Tags
	Software,		//	string	Software used to edit image.
	Description,	//	string	Image description.
	Copyright,		//	string	Copyright notice.
	NumTags
};


// A single piece of image metadata. Could be more memory efficient, but hardly seems worth it.
struct tMetaDatum
{
	enum class DatumType { Invalid = -1, Uint32, Float, String };
	tMetaDatum()																										: Type(DatumType::Invalid) { }
	tMetaDatum(const tMetaDatum& src)																					{ Set(src); }

	void Clear()																										{ Type = DatumType::Invalid; String.Clear(); }
	void Set(const tMetaDatum& src);
	void Set(uint32 v)																									{ Type = DatumType::Uint32; Uint32 = v; }
	void Set(float v)																									{ Type = DatumType::Float; Float = v; }
	void Set(const tString& v)																							{ Type = DatumType::String; String = v; }
	bool IsValid() const																								{ return (Type != DatumType::Invalid); }
	tMetaDatum& operator=(const tMetaDatum& src)																		{ Set(src); return *this; }

	DatumType Type;
	union { uint32 Uint32; float Float; };
	tString String;
};


class tMetaData
{
public:
	tMetaData()																											: NumTagsValid(0), Data() { }
	tMetaData(const tMetaData& src)																						{ Set(src); }
	tMetaData(const uint8* rawJpgImageData, int numBytes)																{ Set(rawJpgImageData, numBytes); }
	virtual ~tMetaData()																								{ }

	void Clear();
	bool Set(const tMetaData& src);
	bool Set(const uint8* rawJpgImageData, int numBytes);
	bool IsValid() const																								{ return NumTagsValid > 0; }

	tMetaData& operator=(const tMetaData& src)																			{ Set(src); return *this; }
	tMetaDatum& operator[](tMetaTag tag)																				{ return Data[int(tag)]; }

private:
	int NumTagsValid;
	tMetaDatum Data[int(tMetaTag::NumTags)];

	void SetTags_CamHardware(const TinyEXIF::EXIFInfo&);
	void SetTags_GeoLocation(const TinyEXIF::EXIFInfo&);
	void SetTags_CamSettings(const TinyEXIF::EXIFInfo&);
	void SetTags_AuthorNotes(const TinyEXIF::EXIFInfo&);
};


}


// Implementation below this line.


inline void tImage::tMetaDatum::Set(const tMetaDatum& src)
{
	Type = src.Type;
	switch (Type)
	{
		case DatumType::Uint32:		Uint32 = src.Uint32;	return;
		case DatumType::Float:		Float = src.Float;		return;
		case DatumType::String:		String = src.String;	return;
	}
}


inline void tImage::tMetaData::Clear()
{
	NumTagsValid = 0;
	for (int d = 0; d < int(tMetaTag::NumTags); d++)
		Data[d].Clear();
}


inline bool tImage::tMetaData::Set(const tMetaData& src)
{
	NumTagsValid = src.NumTagsValid;
	for (int d = 0; d < int(tMetaTag::NumTags); d++)
		Data[d] = src.Data[d];

	return IsValid();
}
