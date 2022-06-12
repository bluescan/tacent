// tMetaData.cpp
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

#include "Image/tMetaData.h"
#include "System/tPrint.h"
#include "Math/tVector3.h"
#include "TinyEXIF/TinyEXIF.h"
using namespace tImage;
using namespace tMath;


const char* tMetaTagNames[] =
{
	// Camera Hardware Tag Names
	"Make",
	"Model",
	"Serial Number",

	// Geo Location Tag Names
	"Latitude DD",
	"Latitude",
	"Longitude DD",
	"Longitude",
	"Altitude",
	"Altitude Ref",
	"Altitude Rel",
	"Roll",
	"Pitch",
	"Yaw",
	"VelX",
	"VelY",
	"VelZ",
	"Speed",
	"GPS Survey",
	"GPS Time Stamp",

	// Camera Settings Tag Names
	"Shutter Speed",
	"Exposure Time",
	"Exposure Bias",
	"F-Stop",
	"Exposure Program",
	"ISO",
	"Aperture",
	"Brightness",
	"Metering Mode",
	"Flash Used",
	"Flash Strobe",
	"Flash Mode",
	"Flash Present",
	"Flash Red-Eye",
	"Focal Length",
	"Orientation",
	"Length Unit",
	"X-Pixels Per Unit",
	"Y-Pixels Per Unit",
	"Bits Per Sample",
	"Image Width",
	"Image Height",
	"Image Width Orig",
	"Image Height Orig",
	"Date/Time Change",
	"Date/Time Orig",
	"Date/Time Digitized",

	// Authoring Note Tag Names
	"Software",
	"Description",
	"Copyright"
};
tStaticAssert(tNumElements(tMetaTagNames) == int(tMetaTag::NumTags));


const char* tMetaTagDescs[] =
{
	// Camera Hardware Tag Descriptions
	"Camera make/manufacturer.",
	"Camera model.",
	"Camera serial number.",

	// Geo Location Tag Descriptions
	"Latitude in decimal degrees.",
	"Latitude in degrees, minutes, seconds followed by N (north) or S (south).",
	"Longitude in decimal degrees.",
	"Longitude in degrees, minutes, seconds followed by W (west) or E (east).",
	"Altitude in meters relative to sea-level.",
	"Relative altitude ground reference. Applies to Altitude Rel value. Values:\n"
		"\"Above Ground\"    : Reference data unavailable. Assume above ground.\n"
		"\"Above Sea Level\" : Ground is above sea level.\n"
		"\"Below Sea Level\" : Ground is below sea level.",
	"Relative altitude in meters. Often how high above ground.",
	"Flight roll in degrees.",
	"Flight pitch in degrees.",
	"Flight yaw in degrees.",
	"X-Component (forwards/backwards) of velocity in m/s. May be negative. DJI maker-note.",
	"Y-Component (left/right) of velocity in m/s. May be negative. DJI maker-note.",
	"Z-Component (up/down) of velocity in m/s. May be negative. DJI maker-note.",
	"Length of velocity vector in m/s. Speed is always >= 0. DJI maker-note.",
	"Geodetic survey data.",
	"UTC Date and time of GPS data in format YYYY-MM-DD hh:mm:ss\n"
		"It's possible one of YYYY-MM-DD or hh:mm:ss is not available.",

	// Camera Settings Tag Descriptions
	"Shutter speed in units 1/s. Reciprocal of exposure time. If not set, computed.",
	"Exposure time in seconds. Reciprocal of Shutter Speed. If not set, computed.",
	"Exposure bias in APEX units.",
	"Ratio of the lens focal length to the diameter of the entrance pupil. Unitless.",
	"Exposure program. Values:\n"
		"0: Not Defined.\n"
		"1: Manual.\n"
		"2: Normal Program.\n"
		"3: Aperture Priority.\n"
		"4: Shutter Priority.\n"
		"5: Creative Program.\n"
		"6: Action Program.\n"
		"7: Portrait Mode.\n"
		"8: Landscape Mode.",
	"Equivalent ISO film speed rating.",
	"Aperture in APEX units.",
	"Average scene luminance of whole image in APEX units.",
	"Metering mode,	Values:\n"
		"0: Unknown.\n"
		"1: Average.\n"
		"2: Center Weighted Average.\n"
		"3: Spot.\n"
		"4: Multi-spot.\n"
		"5: Pattern.\n"
		"6: Partial.",
	"Flash used. Values:\n"
		"0: No.\n"
		"1: Yes.",
	"Flash strobe detection. Values:\n"
		"0: No Detection.\n"
		"1: Reserved.\n"
		"2: Strobe Return Light Not Detected.\n"
		"3: Strobe Return Light Detected.",
	"Flash camera mode. Values:\n"
		"0: Unknown.\n"
		"1: Compulsory Flash Firing.\n"
		"2: Compulsory Flash Suppression.\n"
		"3: Auto.",
	"Flash hardware present. Values:\n"
		"0: Flash Present.\n"
		"1: No Flash Present.",
	"Flash red-eye reduction.\n"
		"0: No Red-Eye Reduction or Unknown.\n"
		"1: Red-Eye Reduction.",
	"Focal length in pixels.",
	"Information on camera orientation when photo taken. The following\n"
		"transformations may be present in the image data:\n"
		"0: Unspecified.\n"
		"1: No Transforms.    Image is not mirrored or rotated.\n"
		"2: Flip-Y.           Image is mirrored about vertical axis (right <-> left).\n"
		"3: Flip-XY.          Image flipped about both axes. Same as 180 degrees rotation.\n"
		"4: Flip-X.           Image is mirrored about horizontal axis (top <-> bottom).\n"
		"5: Rot-CW90  Flip-Y. Image is rotated 90 degrees clockwise and then flipped about verical axis.\n"
		"6: Rot-ACW90.        Image is rotated 90 degrees anti-clockwise.\n"
		"7: Rot-ACW90 Flip-Y. Image is rotated 90 degrees clockwise and then flipped about verical axis.\n"
		"8: Rot-CW90.         Image is rotated 90 degrees anti-clockwise.",
	"The length unit used for the Pixels-per-unit values:\n"
		"1: Not Specified.\n"
		"2: Inch.\n"
		"3: cm.",
	"Horizontal pixels per length unit.",
	"Veritical pixels per length unit.",
	"Bits per colour component. Not bits per pixel.",
	"Image width in pixels.",
	"Image height in pixels.",
	"Original image width (before edits) in pixels.",
	"Original image height(before edits) in pixels.",
	"Date and time the image was changed in format YYYY-MM-DD hh:mm:ss",
	"Date and time of original image in format YYYY-MM-DD hh:mm:ss.",
	"Date and time the image was digitized in format YYYY-MM-DD hh:mm:ss.",

	// Authoring Note Tag Descriptions
	"Software used to edit image.",
	"Image description.",
	"Copyright notice."
};
tStaticAssert(tNumElements(tMetaTagDescs) == int(tMetaTag::NumTags));


const char* tImage::tGetMetaTagName(tMetaTag tag)
{
	return tMetaTagNames[int(tag)];
}


const char* tImage::tGetMetaTagDesc(tMetaTag tag)
{
	return tMetaTagDescs[int(tag)];
}


bool tMetaData::Set(const uint8* rawJpgImageData, int numBytes)
{
	Clear();
	TinyEXIF::EXIFInfo exifInfo;
	int errorCode = exifInfo.parseFrom(rawJpgImageData, numBytes);
	if (errorCode)
		return false;

	SetTags_CamHardware(exifInfo);
	SetTags_GeoLocation(exifInfo);
	SetTags_CamSettings(exifInfo);
	SetTags_AuthorNotes(exifInfo);

	// @todo This function will get quite large.
	return IsValid();
}


void tMetaData::SetTags_CamHardware(const TinyEXIF::EXIFInfo& exifInfo)
{
	// Make
	std::string make = exifInfo.Make;
	if (!make.empty())
	{
		Data[ int(tMetaTag::Make) ].Set(make.c_str());
		NumTagsValid++;
	}

	// Model
	std::string model = exifInfo.Model;
	if (!model.empty())
	{
		Data[ int(tMetaTag::Model) ].Set(model.c_str());
		NumTagsValid++;
	}
	
	// SerialNumber
	std::string serial = exifInfo.SerialNumber;
	if (!serial.empty())
	{
		Data[ int(tMetaTag::SerialNumber) ].Set(serial.c_str());
		NumTagsValid++;
	}
}


void tMetaData::SetTags_GeoLocation(const TinyEXIF::EXIFInfo& exifInfo)
{
	// If we have LatLong we should have it in DD and DMS formats.
	if (exifInfo.GeoLocation.hasLatLon())
	{
		// LatitudeDD
		double lat = exifInfo.GeoLocation.Latitude;
		Data[ int(tMetaTag::LatitudeDD) 	].Set(float(lat));
		NumTagsValid++;

		// LatitudeDMS
		// The exifInfo should not have fraction values for the degreed and minutes if they did everythng right.
		int degLat = int ( tMath::tRound(exifInfo.GeoLocation.LatComponents.degrees) );
		int minLat = int ( tMath::tRound(exifInfo.GeoLocation.LatComponents.minutes) );
		int secLat = int ( tMath::tRound(exifInfo.GeoLocation.LatComponents.seconds) );
		char dirLat = exifInfo.GeoLocation.LatComponents.direction;
		tString dmsLat;
		tsPrintf(dmsLat, u8"%d°%d'%d\"%c", degLat, minLat, secLat, dirLat);
		Data[ int(tMetaTag::LatitudeDMS) 	].Set(dmsLat);
		NumTagsValid++;

		// LongitudeDD
		double lon = exifInfo.GeoLocation.Longitude;
		Data[ int(tMetaTag::LongitudeDD) 	].Set(float(lon));
		NumTagsValid++;

		// LongitudeDMS
		int degLon = int ( tMath::tRound(exifInfo.GeoLocation.LonComponents.degrees) );
		int minLon = int ( tMath::tRound(exifInfo.GeoLocation.LonComponents.minutes) );
		int secLon = int ( tMath::tRound(exifInfo.GeoLocation.LonComponents.seconds) );
		char dirLon = exifInfo.GeoLocation.LonComponents.direction;
		tString dmsLon;
		tsPrintf(dmsLon, u8"%d°%d'%d\"%c", degLon, minLon, secLon, dirLon);
		Data[ int(tMetaTag::LongitudeDMS) 	].Set(dmsLon);
		NumTagsValid++;
	}

	if (exifInfo.GeoLocation.hasAltitude())
	{
		double alt = exifInfo.GeoLocation.Altitude;
		Data[ int(tMetaTag::Altitude) 	].Set(float(alt));
		NumTagsValid++;
	}

	if (exifInfo.GeoLocation.hasRelativeAltitude())
	{
		int8 ref = exifInfo.GeoLocation.AltitudeRef;
		tString refStr("Above Ground");
		switch (ref)
		{
			case 0:		refStr = "Above Sea Level";		break;
			case -1:	refStr = "Below Sea Level";		break;
		}
		Data[ int(tMetaTag::AltitudeRelRef) ].Set(refStr);
		NumTagsValid++;

		double altRel = exifInfo.GeoLocation.RelativeAltitude;
		Data[ int(tMetaTag::AltitudeRel) ].Set(float(altRel));
		NumTagsValid++;
	}

	if (exifInfo.GeoLocation.hasOrientation())
	{
		double roll = exifInfo.GeoLocation.RollDegree;
		Data[ int(tMetaTag::Roll) ].Set(float(roll));
		NumTagsValid++;

		double pitch = exifInfo.GeoLocation.PitchDegree;
		Data[ int(tMetaTag::Pitch) ].Set(float(pitch));
		NumTagsValid++;

		double yaw = exifInfo.GeoLocation.YawDegree;
		Data[ int(tMetaTag::Yaw) ].Set(float(yaw));
		NumTagsValid++;
	}

	if (exifInfo.GeoLocation.hasSpeed())
	{
		tVector3 vel
		(
			float(exifInfo.GeoLocation.SpeedX),
			float(exifInfo.GeoLocation.SpeedY),
			float(exifInfo.GeoLocation.SpeedZ)
		);
		Data[ int(tMetaTag::VelX) ].Set(vel.x);
		Data[ int(tMetaTag::VelY) ].Set(vel.y);
		Data[ int(tMetaTag::VelZ) ].Set(vel.z);
		NumTagsValid += 3;

		Data[ int(tMetaTag::Speed) ].Set( vel.Length() );
		NumTagsValid++;
	}

	std::string survey = exifInfo.GeoLocation.GPSMapDatum;
	if (!survey.empty())
	{
		Data[ int(tMetaTag::GPSSurvey) ].Set(survey.c_str());
		NumTagsValid++;
	}

	// tString can handle nullptr.
	tString utcDate(exifInfo.GeoLocation.GPSDateStamp.c_str());
	tString utcTime(exifInfo.GeoLocation.GPSTimeStamp.c_str());
	if (utcDate.IsValid())
		utcDate.Replace(':', '-');
	if (utcTime.IsValid())
	{
		utcTime.ExtractRight('.');
		utcTime.Replace(' ', ':');
	}
	tString dateTime;
	if (utcDate.IsValid() && utcTime.IsValid())
		dateTime = utcDate + " " + utcTime;
	else if (utcDate.IsValid())
		dateTime = utcDate;
	else if (utcTime.IsValid())
		dateTime = utcTime;
	if (dateTime.IsValid())
	{
		Data[ int(tMetaTag::GPSTimeStamp) ].Set(dateTime);
		NumTagsValid++;
	}
}


void tMetaData::SetTags_CamSettings(const TinyEXIF::EXIFInfo& exifInfo)
{
	// These are annoying because they are not independent.
	double shutterSpeed = exifInfo.ShutterSpeedValue;
	double exposureTime = exifInfo.ExposureTime;

	// Compute one from the other if necessary.
	if ((shutterSpeed <= 0.0) && (exposureTime > 0.0))
		shutterSpeed = 1.0 / exposureTime;
	else if ((exposureTime <= 0.0) && (shutterSpeed > 0.0))
		exposureTime = 1.0 / shutterSpeed;

	if (shutterSpeed > 0.0)
	{
		Data[ int(tMetaTag::ShutterSpeed) ].Set(float(shutterSpeed));
		NumTagsValid++;
	}

	if (exposureTime > 0.0)
	{
		Data[ int(tMetaTag::ExposureTime) ].Set(float(exposureTime));
		NumTagsValid++;
	}

	double exposureBias = exifInfo.ExposureBiasValue;
	Data[ int(tMetaTag::ExposureBias) ].Set(float(exposureBias));
	NumTagsValid++;

	double fstop = exifInfo.FNumber;
	Data[ int(tMetaTag::FStop) ].Set(float(fstop));
	NumTagsValid++;

	// Only set exposure program if it's defined.
	uint32 prog = exifInfo.ExposureProgram;
	if (prog)
	{
		Data[ int(tMetaTag::ExposureProgram) ].Set(prog);
		NumTagsValid++;
	}

	uint32 iso = exifInfo.ISOSpeedRatings;
	Data[ int(tMetaTag::ISO) ].Set(iso);
	NumTagsValid++;

	double aperture = exifInfo.ApertureValue;
	Data[ int(tMetaTag::Aperture) ].Set(float(aperture));
	NumTagsValid++;

	double brightness = exifInfo.BrightnessValue;
	Data[ int(tMetaTag::Brightness) ].Set(float(brightness));
	NumTagsValid++;

	// Only set metering mode if it's known.
	uint32 meterMode = exifInfo.MeteringMode;
	if (meterMode)
	{
		Data[ int(tMetaTag::MeteringMode) ].Set(meterMode);
		NumTagsValid++;
	}

	uint32 flash = exifInfo.Flash;

	// Flash bit 0.
	uint32 flashUsed = (flash & 0x00000001);
	Data[ int(tMetaTag::FlashUsed) ].Set(flashUsed);
	NumTagsValid++;

	// Flash bits 1 and 2. Only set if dectector present.
	uint32 flashStrobe = (flash & 0x00000006) >> 1;
	if (flashStrobe)
	{
		Data[ int(tMetaTag::FlashStrobe) ].Set(flashStrobe);
		NumTagsValid++;
	}

	// Flash bits 3 and 4. Only set if mode not unknown.
	uint32 flashMode = (flash & 0x00000018) >> 3;
	if (flashMode)
	{
		Data[ int(tMetaTag::FlashMode) ].Set(flashMode);
		NumTagsValid++;
	}

	// Flash bit 5.
	uint32 flashPresent = (flash & 0x00000020) >> 5;
	Data[ int(tMetaTag::FlashPresent) ].Set(flashPresent);
	NumTagsValid++;

	// Flash bit 6.
	uint32 flashRedEye = (flash & 0x00000040) >> 6;
	Data[ int(tMetaTag::FlashRedEye) ].Set(flashRedEye);
	NumTagsValid++;

	double focalLength = exifInfo.FocalLength;
	Data[ int(tMetaTag::FocalLength) ].Set(float(focalLength));
	NumTagsValid++;

	// Only set orientation if it's specified.
	uint32 orientation = exifInfo.Orientation;
	if (orientation)
	{
		Data[ int(tMetaTag::Orientation) ].Set(orientation);
		NumTagsValid++;
	}

	// Only set length unit if it's specified.
	uint32 lengthUnit = exifInfo.ResolutionUnit;
	if (lengthUnit)
	{
		Data[ int(tMetaTag::LengthUnit) ].Set(lengthUnit);
		NumTagsValid++;
	}

	double pixelsPerUnitX = exifInfo.XResolution;
	if (pixelsPerUnitX > 0.0)
	{
		Data[ int(tMetaTag::XPixelsPerUnit) ].Set(float(pixelsPerUnitX));
		NumTagsValid++;
	}

	double pixelsPerUnitY = exifInfo.YResolution;
	if (pixelsPerUnitY > 0.0)
	{
		Data[ int(tMetaTag::YPixelsPerUnit) ].Set(float(pixelsPerUnitY));
		NumTagsValid++;
	}

	uint32 bitsPerComponent = exifInfo.BitsPerSample;
	if (bitsPerComponent)
	{
		Data[ int(tMetaTag::BitsPerSample) ].Set(bitsPerComponent);
		NumTagsValid++;
	}

	uint32 imageWidth = exifInfo.ImageWidth;
	if (imageWidth > 0)
	{
		Data[ int(tMetaTag::ImageWidth) ].Set(imageWidth);
		NumTagsValid++;
	}

	uint32 imageHeight = exifInfo.ImageHeight;
	if (imageHeight > 0)
	{
		Data[ int(tMetaTag::ImageHeight) ].Set(imageHeight);
		NumTagsValid++;
	}

	uint32 imageWidthOrig = exifInfo.RelatedImageWidth;
	if (imageWidthOrig > 0)
	{
		Data[ int(tMetaTag::ImageWidthOrig) ].Set(imageWidthOrig);
		NumTagsValid++;
	}

	uint32 imageHeightOrig = exifInfo.RelatedImageHeight;
	if (imageHeightOrig > 0)
	{
		Data[ int(tMetaTag::ImageHeightOrig) ].Set(imageHeightOrig);
		NumTagsValid++;
	}

	tString dateTimeChange(exifInfo.DateTime.c_str());
	if (dateTimeChange.IsValid())
	{
		tString yyyymmdd = dateTimeChange.ExtractLeft(' ');
		yyyymmdd.Replace(':', '-');
		dateTimeChange = yyyymmdd + " " + dateTimeChange;

		Data[ int(tMetaTag::DateTimeChange) ].Set(dateTimeChange);
		NumTagsValid++;
	}

	tString dateTimeOrig(exifInfo.DateTimeOriginal.c_str());
	if (dateTimeOrig.IsValid())
	{
		tString yyyymmdd = dateTimeOrig.ExtractLeft(' ');
		yyyymmdd.Replace(':', '-');
		dateTimeOrig = yyyymmdd + " " + dateTimeOrig;

		Data[ int(tMetaTag::DateTimeOrig) ].Set(dateTimeOrig);
		NumTagsValid++;
	}

	tString dateTimeDig(exifInfo.DateTimeDigitized.c_str());
	if (dateTimeDig.IsValid())
	{
		tString yyyymmdd = dateTimeDig.ExtractLeft(' ');
		yyyymmdd.Replace(':', '-');
		dateTimeDig = yyyymmdd + " " + dateTimeDig;

		Data[ int(tMetaTag::DateTimeDigit) ].Set(dateTimeDig);
		NumTagsValid++;
	}
}


void tMetaData::SetTags_AuthorNotes(const TinyEXIF::EXIFInfo& exifInfo)
{
	// Software
	tString software = exifInfo.Software.c_str();
	if (software.IsValid())
	{
		Data[ int(tMetaTag::Software) ].Set(software);
		NumTagsValid++;
	}

	// Description
	tString description = exifInfo.ImageDescription.c_str();
	if (description.IsValid())
	{
		Data[ int(tMetaTag::Description) ].Set(description);
		NumTagsValid++;
	}

	// Copyright
	tString copyright = exifInfo.Copyright.c_str();
	if (copyright.IsValid())
	{
		Data[ int(tMetaTag::Copyright) ].Set(copyright);
		NumTagsValid++;
	}
}


tString tMetaData::GetPrettyValue(tMetaTag tag) const
{
	tString value;
	if (!IsValid())
		return value;

	const tMetaDatum& datum = Data[int(tag)];
	if (!datum.IsSet())
		return value;

	switch (tag)
	{
		case tMetaTag::Make:
		case tMetaTag::Model:
		case tMetaTag::SerialNumber:
			value = datum.String;
			break;

		case tMetaTag::LatitudeDD:
			tsPrintf(value, "%f°", datum.Float);
			break;

		case tMetaTag::LatitudeDMS:
			value = datum.String;
			break;
		
		case tMetaTag::LongitudeDD:
			tsPrintf(value, "%f°", datum.Float);
			break;

		case tMetaTag::LongitudeDMS:
			value = datum.String;
			break;

		case tMetaTag::Altitude:
			tsPrintf(value, "%f m", datum.Float);
			break;

		case tMetaTag::AltitudeRelRef:
			value = datum.String;
			break;

		case tMetaTag::AltitudeRel:
		{
			tsPrintf(value, "%f m", datum.Float);
			const tMetaDatum& refDatum = Data[int(tMetaTag::AltitudeRelRef)];
			if (refDatum.IsSet())
				value = value + " " + refDatum.String.Lower();
			break;
		}

		case tMetaTag::Roll:
		case tMetaTag::Pitch:
		case tMetaTag::Yaw:
			tsPrintf(value, "%f°", datum.Float);
			break;

		case tMetaTag::VelX:
		case tMetaTag::VelY:
		case tMetaTag::VelZ:
		case tMetaTag::Speed:
			tsPrintf(value, "%f m/s", datum.Float);
			break;

		case tMetaTag::GPSSurvey:
		case tMetaTag::GPSTimeStamp:
			value = datum.String;
			break;

		case tMetaTag::ShutterSpeed:
			tsPrintf(value, "%f 1/s", datum.Float);
			break;

		case tMetaTag::ExposureTime:
			tsPrintf(value, "%f s", datum.Float);
			break;

		case tMetaTag::ExposureBias:
			tsPrintf(value, "%f APEX", datum.Float);
			break;

		case tMetaTag::FStop:
			tsPrintf(value, "%.1f", datum.Float);
			break;

		case tMetaTag::ExposureProgram:
			value = "Not Defined";
			switch (datum.Uint32)
			{
				case 1: value = "Manual";				break;
				case 2: value = "Normal Program";		break;
				case 3: value = "Aperture Priority";	break;
				case 4: value = "Shutter Priority";		break;
				case 5: value = "Creative Program";		break;
				case 6: value = "Action Program";		break;
				case 7: value = "Portrait Mode";		break;
				case 8: value = "Landscape Mode";		break;
			}
			break;

		case tMetaTag::ISO:
			tsPrintf(value, "%u", datum.Uint32);
			break;

		case tMetaTag::Aperture:
		case tMetaTag::Brightness:
			tsPrintf(value, "%f APEX", datum.Float);
			break;

		case tMetaTag::MeteringMode:
			value = "Unknown";
			switch (datum.Uint32)
			{
				case 1: value = "Average";					break;
				case 2: value = "Center Weighted Average";	break;
				case 3: value = "Spot";						break;
				case 4: value = "Multi-spot";				break;
				case 5: value = "Pattern";					break;
				case 6: value = "Partial";					break;
			}
			break;

		case tMetaTag::FlashUsed:
			value = datum.Uint32 ? "Yes" : "No";
			break;

		case tMetaTag::FlashStrobe:
			value = "No Detector";
			switch (datum.Uint32)
			{
				case 1: value = "Reserved";					break;
				case 2: value = "Not Detected";				break;
				case 3: value = "Detected";					break;
			}
			break;

		case tMetaTag::FlashMode:
			value = "Unknown";
			switch (datum.Uint32)
			{
				case 1: value = "Compulsory Firing";		break;
				case 2: value = "Compulsory Suppession";	break;
				case 3: value = "Auto";						break;
			}
			break;

		case tMetaTag::FlashPresent:
			value = datum.Uint32 ? "Not Present" : "Present";
			break;

		case tMetaTag::FlashRedEye:
			value = datum.Uint32 ? "Reduction" : "No Reduction";
			break;

		case tMetaTag::FocalLength:
			tsPrintf(value, "%d pixels", int(datum.Float));
			break;

		case tMetaTag::Orientation:
		{
			value = "Unspecified";
			switch (datum.Uint32)
			{
				case 1: value = "Normal";			break;
				case 2: value = "Flip-Y";			break;
				case 3: value = "Flip-XY";			break;
				case 4: value = "Flip-X";			break;
				case 5: value = "Rot-CW90 Flip-Y";	break;
				case 6: value = "Rot-ACW90";		break;
				case 7: value = "Rot-ACW90 Flip-Y";	break;
				case 8: value = "Rot-CW90";			break;
			}
			break;
		}

		case tMetaTag::LengthUnit:
		{
			value = "units";
			switch (datum.Uint32)
			{
				case 2: value = "inch";	break;
				case 3: value = "cm";	break;
			}
			break;
		}

		case tMetaTag::XPixelsPerUnit:
		case tMetaTag::YPixelsPerUnit:
		{
			tString unit = GetPrettyValue(tMetaTag::LengthUnit);
			if (unit.IsValid())
				tsPrintf(value, "%d pixels/%s", int(datum.Float), unit.Chars());
			else
				tsPrintf(value, "%d pixels", int(datum.Float));
			break;
		}

		case tMetaTag::BitsPerSample:
			tsPrintf(value, "%d bits/component", datum.Uint32);
			break;

		case tMetaTag::ImageWidth:
		case tMetaTag::ImageHeight:
		case tMetaTag::ImageWidthOrig:
		case tMetaTag::ImageHeightOrig:
			tsPrintf(value, "%d pixels", datum.Uint32);
			break;

		case tMetaTag::DateTimeChange:
		case tMetaTag::DateTimeOrig:
		case tMetaTag::DateTimeDigit:
		case tMetaTag::Software:
		case tMetaTag::Description:
		case tMetaTag::Copyright:
			value = datum.String;
			break;
	}
	return value;
}
