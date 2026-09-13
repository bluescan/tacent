// tMetaData.cpp
//
// A class to store image meta-data. Some image formats allow comments and other metadata to be stored inside the
// image. For example, jpg files may contain EXIF or XMP meta-data. This class is basically a map of key/value strings
// that may be a member of some tImageXXX types, It currently knows how to parse EXIF and XMP meta-data.
//
// Copyright (c) 2022, 2023, 2026 Tristan Grimmer.
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


tMetaData::~tMetaData()
{
	Clear();
}


void tMetaData::Clear()
{
	NumTagsValid = 0;
	for (int d = 0; d < int(tMetaTag::NumTags); d++)
		Data[d].Clear();
}


bool tMetaData::Set(const tMetaData& src)
{
	Clear();
	NumTagsValid = src.NumTagsValid;
	for (int d = 0; d < int(tMetaTag::NumTags); d++)
		Data[d] = src.Data[d];
	return IsValid();
}


const char* tMetaTagNames[] =
{
	// Camera Hardware Tag Names
	"Make",
	"Model",
	"Serial Number",
	"Make Model Serial",

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
	"Flash Present",
	"Flash Used",
	"Flash Strobe",
	"Flash Mode",
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
	"Copyright",

	// Appended after Copyright to match the tMetaTag::LensModel enum position (see the note there).
	"Lens Model"
};
tStaticAssert(tNumElements(tMetaTagNames) == int(tMetaTag::NumTags));


const char* tMetaTagDescs[] =
{
	// Camera Hardware Tag Descriptions
	"Camera make/manufacturer.",
	"Camera model.",
	"Camera serial number.",
	"Camera unique identifier containing make, model, and serial number.\n"
		"Takes form \"Make | Model | Serial\" when all 3 present.",

	// Geo Location Tag Descriptions
	"Latitude in decimal degrees.",
	"Latitude in degrees, minutes, seconds followed by N (north) or S (south).",
	"Longitude in decimal degrees.",
	"Longitude in degrees, minutes, seconds followed by W (west) or E (east).",
	"Altitude in meters relative to sea-level.",
	"Relative altitude ground reference. Applies to Altitude Rel value.\n"
		"\"Above Ground\": Reference data unavailable. Assume above ground.\n"
		"\"Above Sea Level\": Ground is above sea level.\n"
		"\"Below Sea Level\": Ground is below sea level.",
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
	"Exposure program. Will be one of following values:\n"
		"\"Not Defined\"\n"
		"\"Manual\"\n"
		"\"Normal Program\"\n"
		"\"Aperture Priority\"\n"
		"\"Shutter Priority\"\n"
		"\"Creative Program\"\n"
		"\"Action Program\"\n"
		"\"Portrait Mode\"\n"
		"\"Landscape Mode\"",
	"Equivalent ISO film speed rating.",
	"Aperture in APEX units.",
	"Average scene luminance of whole image in APEX units.",
	"Metering mode. Will be one of following values:\n"
		"\"Unknown\"\n"
		"\"Average\"\n"
		"\"Center Weighted Average\"\n"
		"\"Spot\"\n"
		"\"Multi-spot\"\n"
		"\"Pattern\"\n"
		"\"Partial\"",
	"Flash hardware present. Possible values \"Yes\" or \"No\"\n",
	"Flash used. Possible values \"Yes\" or \"No\"\n",
	"Flash strobe detection. Possible values:\n"
		"\"No Detection\"\n"
		"\"Reserved\"\n"
		"\"Strobe Return Light Not Detected\"\n"
		"\"Strobe Return Light Detected\"",
	"Flash camera mode. Possible values:\n"
		"\"Unknown\"\n"
		"\"Compulsory Flash Firing\"\n"
		"\"Compulsory Flash Suppression\"\n"
		"\"Auto\"",
	"Flash red-eye reduction. Possible values:\n"
		"\"No Red-Eye Reduction or Unknown\"\n"
		"\"Red-Eye Reduction\"",
	"Focal length in mm. Always > 0.",
	"Information on camera orientation when photo taken. The following\n"
		"transformations may be present in the image data:\n"
		"\"Unspecified\": Not orientation info provided.\n"
		"\"No Transforms\": Image is not mirrored or rotated.\n"
		"\"Flip-Y\": Image is mirrored about vertical axis (right <-> left).\n"
		"\"Flip-XY\": Image flipped about both axes. Same as 180° rotation.\n"
		"\"Flip-X\": Image is mirrored about horizontal axis (top <-> bottom).\n"
		"\"Rot-CW90 Flip-Y\": Image is rotated 90° clockwise and then flipped horizontally.\n"
		"\"Rot-ACW90\": Image is rotated 90° anti-clockwise.\n"
		"\"Rot-ACW90 Flip-Y\": Image is rotated 90° clockwise and then flipped horizontally.\n"
		"\"Rot-CW90\": Image is rotated 90° anti-clockwise.",
	"The length unit used for the Pixels-per-unit values:\n"
		"\"Not Specified\"\n"
		"\"Inch\"\n"
		"\"cm\"",
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
	"Copyright notice.",

	// Appended after Copyright to match the tMetaTag::LensModel enum position (see the note there).
	"Lens model."
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


bool tMetaData::AddSegments(tList<tMetaSegment>& exifSegments, tList<tMetaSegment>& xmpSegments)
{
	bool found = false;
	
	// EXIF first: where the same tag appears in both, the EXIF value wins. XMP (applied below) only fills in tags
	// that are not already set, so it can introduce new tags but never replace EXIF ones.
	for (tMetaSegment* exif = exifSegments.First(); exif; exif = exif->Next())
		found |= AddEXIF(exif->SegData, exif->SegNumBytes);
	for (tMetaSegment* xmp = xmpSegments.First(); xmp; xmp = xmp->Next())
		found |= AddXMP(xmp->SegData, xmp->SegNumBytes);
	return found;
}


bool tMetaData::AddEXIF(const uint8* exifSegment, int numBytes)
{
	if ((!exifSegment) || (numBytes <= 0))
		return false;

	// The parsed EXIF info is a temporary object; only the Data array persists. parseFromEXIFSegment() does not
	// initialize the fields it doesn't find, so clear() first: with cleared defaults (0, DBL_MAX, "") the SetTags_*
	// guards correctly skip absent fields.
	TinyEXIF::EXIFInfo tinyInfo;
	tinyInfo.clear();
	if (tinyInfo.parseFromEXIFSegment(exifSegment, (unsigned)numBytes) != TinyEXIF::PARSE_SUCCESS)
		return false;

	// Merge the parsed tags into the Data array; SetTag() only fills tags that are not already set.
	ApplyParsedTinyInfo(tinyInfo);
	return true;
}


bool tMetaData::AddXMP(const uint8* xmpXML, int numBytes)
{
	if ((!xmpXML) || (numBytes <= 0))
		return false;

	// The parsed XMP info is a temporary object; only the Data array persists. parseFromXMPSegmentXML() does not
	// initialize the fields it doesn't find, so clear() first: with cleared defaults (0, DBL_MAX, "") the SetTags_*
	// guards correctly skip absent fields.
	TinyEXIF::EXIFInfo tinyInfo;
	tinyInfo.clear();
	if (tinyInfo.parseFromXMPSegmentXML(reinterpret_cast<const char*>(xmpXML), (unsigned)numBytes) != TinyEXIF::PARSE_SUCCESS)
		return false;

	// Set-if-not-set: tags already present (from EXIF, or an earlier XMP segment) are left alone; XMP only
	// fills the gaps.
	ApplyParsedTinyInfo(tinyInfo);
	return true;
}


void tMetaData::ApplyParsedTinyInfo(const TinyEXIF::EXIFInfo& tinyInfo)
{
	SetTags_CamHardware(tinyInfo);
	SetTags_GeoLocation(tinyInfo);
	SetTags_CamSettings(tinyInfo);
	SetTags_AuthorNotes(tinyInfo);
}


bool tMetaData::SetTag(tMetaTag tag, uint32 value)
{
	if (tag == tMetaTag::Invalid)		return false;
	tMetaDatum& datum = Data[int(tag)];
	if (datum.IsSet())					return false;

	datum.Set(value);					NumTagsValid++;
	return true;
}


bool tMetaData::SetTag(tMetaTag tag, float value)
{
	if (tag == tMetaTag::Invalid)		return false;
	tMetaDatum& datum = Data[int(tag)];
	if (datum.IsSet())					return false;

	datum.Set(value);					NumTagsValid++;
	return true;
}


bool tMetaData::SetTag(tMetaTag tag, tString& value)
{
	if (tag == tMetaTag::Invalid)		return false;
	tMetaDatum& datum = Data[int(tag)];
	if (datum.IsSet())					return false;

	datum.Set(value);					NumTagsValid++;
	return true;
}


void tMetaData::SetTags_CamHardware(const TinyEXIF::EXIFInfo& tinyInfo)
{
	// Make. tString can handle nullptr.
	tString make = tinyInfo.Make.c_str();
	if (make.IsValid())
		SetTag(tMetaTag::Make, make);

	// Model
	tString model = tinyInfo.Model.c_str();
	if (model.IsValid())
		SetTag(tMetaTag::Model, model);
	
	// SerialNumber
	tString serial = tinyInfo.SerialNumber.c_str();
	if (serial.IsValid())
		SetTag(tMetaTag::SerialNumber, serial);

	// MakeModelSerial
	// Handles any combination of valid and invalid make, model, and serial strings.
	tString makeModelSerial = make + " | ";
	if (model.IsValid())
		makeModelSerial += model + " | ";
	if (serial.IsValid())
		makeModelSerial += serial + " | ";
	makeModelSerial.ExtractRight(" | ");

	if (makeModelSerial.IsValid())
		SetTag(tMetaTag::MakeModelSerial, makeModelSerial);

	// LensModel. Set by EXIF's LensModel tag (0xA433) and/or the XMP exifEX:LensModel / aux:Lens properties.
	tString lens = tinyInfo.LensInfo.Model.c_str();
	if (lens.IsValid())
		SetTag(tMetaTag::LensModel, lens);
}


void tMetaData::SetTags_GeoLocation(const TinyEXIF::EXIFInfo& tinyInfo)
{
	// If we have LatLong we should have it in DD and DMS formats.
	if (tinyInfo.GeoLocation.hasLatLon())
	{
		// LatitudeDD
		double lat = tinyInfo.GeoLocation.Latitude;
		SetTag(tMetaTag::LatitudeDD, float(lat));

		// LatitudeDMS
		// The tinyInfo should not have fraction values for the degree and minutes if they did everythng right.
		int degLat = int ( tMath::tRound(tinyInfo.GeoLocation.LatComponents.degrees) );
		int minLat = int ( tMath::tRound(tinyInfo.GeoLocation.LatComponents.minutes) );
		int secLat = int ( tMath::tRound(tinyInfo.GeoLocation.LatComponents.seconds) );
		char dirLat = tinyInfo.GeoLocation.LatComponents.direction;
		tString dmsLat;
		tsPrintf(dmsLat, "%d°%d'%d\"%c", degLat, minLat, secLat, dirLat);
		SetTag(tMetaTag::LatitudeDMS, dmsLat);

		// LongitudeDD
		double lon = tinyInfo.GeoLocation.Longitude;
		SetTag(tMetaTag::LongitudeDD, float(lon));

		// LongitudeDMS
		int degLon = int ( tMath::tRound(tinyInfo.GeoLocation.LonComponents.degrees) );
		int minLon = int ( tMath::tRound(tinyInfo.GeoLocation.LonComponents.minutes) );
		int secLon = int ( tMath::tRound(tinyInfo.GeoLocation.LonComponents.seconds) );
		char dirLon = tinyInfo.GeoLocation.LonComponents.direction;
		tString dmsLon;
		tsPrintf(dmsLon, "%d°%d'%d\"%c", degLon, minLon, secLon, dirLon);
		SetTag(tMetaTag::LongitudeDMS, dmsLon);
	}

	if (tinyInfo.GeoLocation.hasAltitude())
	{
		double alt = tinyInfo.GeoLocation.Altitude;
		SetTag(tMetaTag::Altitude, float(alt));
	}

	if (tinyInfo.GeoLocation.hasRelativeAltitude())
	{
		int8 ref = tinyInfo.GeoLocation.AltitudeRef;
		tString refStr("Above Ground");
		switch (ref)
		{
			case 0:		refStr = "Above Sea Level";		break;
			case -1:	refStr = "Below Sea Level";		break;
		}
		SetTag(tMetaTag::AltitudeRelRef, refStr);

		double altRel = tinyInfo.GeoLocation.RelativeAltitude;
		SetTag(tMetaTag::AltitudeRel, float(altRel));
	}

	if (tinyInfo.GeoLocation.hasOrientation())
	{
		double roll = tinyInfo.GeoLocation.RollDegree;
		SetTag(tMetaTag::Roll, float(roll));

		double pitch = tinyInfo.GeoLocation.PitchDegree;
		SetTag(tMetaTag::Pitch, float(pitch));

		double yaw = tinyInfo.GeoLocation.YawDegree;
		SetTag(tMetaTag::Yaw, float(yaw));
	}

	if (tinyInfo.GeoLocation.hasSpeed())
	{
		tVector3 vel
		(
			float(tinyInfo.GeoLocation.SpeedX),
			float(tinyInfo.GeoLocation.SpeedY),
			float(tinyInfo.GeoLocation.SpeedZ)
		);
		SetTag(tMetaTag::VelX, vel.x);
		SetTag(tMetaTag::VelY, vel.y);
		SetTag(tMetaTag::VelZ, vel.z);
		// NumTagsValid is incremented by the SetTag() calls above.

		SetTag(tMetaTag::Speed,  vel.Length() );
	}

	std::string survey = tinyInfo.GeoLocation.GPSMapDatum;
	if (!survey.empty())
	{
		tString surveyStr(survey.c_str());
		SetTag(tMetaTag::GPSSurvey, surveyStr);
	}

	// tString can handle nullptr.
	tString utcDate(tinyInfo.GeoLocation.GPSDateStamp.c_str());
	tString utcTime(tinyInfo.GeoLocation.GPSTimeStamp.c_str());
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
		SetTag(tMetaTag::GPSTimeStamp, dateTime);
	}
}


void tMetaData::SetTags_CamSettings(const TinyEXIF::EXIFInfo& tinyInfo)
{
	// These are annoying because they are not independent.
	double shutterSpeed = tinyInfo.ShutterSpeedValue;
	double exposureTime = tinyInfo.ExposureTime;

	// Compute one from the other if necessary.
	if ((shutterSpeed <= 0.0) && (exposureTime > 0.0))
		shutterSpeed = 1.0 / exposureTime;
	else if ((exposureTime <= 0.0) && (shutterSpeed > 0.0))
		exposureTime = 1.0 / shutterSpeed;

	if (shutterSpeed > 0.0)
		SetTag(tMetaTag::ShutterSpeed, float(shutterSpeed));

	if (exposureTime > 0.0)
		SetTag(tMetaTag::ExposureTime, float(exposureTime));

	double exposureBias = tinyInfo.ExposureBiasValue;
	if (exposureBias > 0.0)
		SetTag(tMetaTag::ExposureBias, float(exposureBias));

	double fstop = tinyInfo.FNumber;
	if (fstop > 0.0)
		SetTag(tMetaTag::FStop, float(fstop));

	// Only set exposure program if it's defined.
	uint32 prog = tinyInfo.ExposureProgram;
	if (prog)
		SetTag(tMetaTag::ExposureProgram, prog);

	uint32 iso = tinyInfo.ISOSpeedRatings;
	if (iso > 0)
		SetTag(tMetaTag::ISO, iso);

	double aperture = tinyInfo.ApertureValue;
	if (aperture > 0.0)
		SetTag(tMetaTag::Aperture, float(aperture));

	double brightness = tinyInfo.BrightnessValue;
	if (brightness)
		SetTag(tMetaTag::Brightness, float(brightness));

	// Only set metering mode if it's known.
	uint32 meterMode = tinyInfo.MeteringMode;
	if (meterMode)
		SetTag(tMetaTag::MeteringMode, meterMode);

	uint32 flash = tinyInfo.Flash;
	uint32 flashHardware = 0;
	if (flash)
	{
		// Flash bit 5. This bit is true if flash NOT present.
		flashHardware = ((flash & 0x00000020) >> 5) ? 0 : 1;
		SetTag(tMetaTag::FlashHardware, flashHardware);
	}

	if (flashHardware)
	{
		// Flash bit 0.
		uint32 flashUsed = (flash & 0x00000001);
		if (flashUsed)
			SetTag(tMetaTag::FlashUsed, flashUsed);

		// Flash bits 1 and 2. Only set if dectector present.
		uint32 flashStrobe = (flash & 0x00000006) >> 1;
		if (flashStrobe)
			SetTag(tMetaTag::FlashStrobe, flashStrobe);

		// Flash bits 3 and 4. Only set if mode not unknown.
		uint32 flashMode = (flash & 0x00000018) >> 3;
		if (flashMode)
			SetTag(tMetaTag::FlashMode, flashMode);

		// Flash bit 6.
		uint32 flashRedEye = (flash & 0x00000040) >> 6;
		if (flashRedEye)
			SetTag(tMetaTag::FlashRedEye, flashRedEye);
	}

	double focalLength = tinyInfo.FocalLength;
	if (focalLength > 0.0)
		SetTag(tMetaTag::FocalLength, float(focalLength));

	// Only set orientation if it's specified.
	uint32 orientation = tinyInfo.Orientation;
	if (orientation)
		SetTag(tMetaTag::Orientation, orientation);

	// Only set length unit if it's specified.
	uint32 lengthUnit = tinyInfo.ResolutionUnit;
	if (lengthUnit)
		SetTag(tMetaTag::LengthUnit, lengthUnit);

	double pixelsPerUnitX = tinyInfo.XResolution;
	if (pixelsPerUnitX > 0.0)
		SetTag(tMetaTag::XPixelsPerUnit, float(pixelsPerUnitX));

	double pixelsPerUnitY = tinyInfo.YResolution;
	if (pixelsPerUnitY > 0.0)
		SetTag(tMetaTag::YPixelsPerUnit, float(pixelsPerUnitY));

	uint32 bitsPerComponent = tinyInfo.BitsPerSample;
	if (bitsPerComponent)
		SetTag(tMetaTag::BitsPerSample, bitsPerComponent);

	uint32 imageWidth = tinyInfo.ImageWidth;
	if (imageWidth > 0)
		SetTag(tMetaTag::ImageWidth, imageWidth);

	uint32 imageHeight = tinyInfo.ImageHeight;
	if (imageHeight > 0)
		SetTag(tMetaTag::ImageHeight, imageHeight);

	uint32 imageWidthOrig = tinyInfo.RelatedImageWidth;
	if (imageWidthOrig > 0)
		SetTag(tMetaTag::ImageWidthOrig, imageWidthOrig);

	uint32 imageHeightOrig = tinyInfo.RelatedImageHeight;
	if (imageHeightOrig > 0)
		SetTag(tMetaTag::ImageHeightOrig, imageHeightOrig);

	tString dateTimeChange(tinyInfo.DateTime.c_str());
	if (dateTimeChange.IsValid())
	{
		tString yyyymmdd = dateTimeChange.ExtractLeft(' ');
		yyyymmdd.Replace(':', '-');
		dateTimeChange = yyyymmdd + " " + dateTimeChange;

		SetTag(tMetaTag::DateTimeChange, dateTimeChange);
	}

	tString dateTimeOrig(tinyInfo.DateTimeOriginal.c_str());
	if (dateTimeOrig.IsValid())
	{
		tString yyyymmdd = dateTimeOrig.ExtractLeft(' ');
		yyyymmdd.Replace(':', '-');
		dateTimeOrig = yyyymmdd + " " + dateTimeOrig;

		SetTag(tMetaTag::DateTimeOrig, dateTimeOrig);
	}

	tString dateTimeDig(tinyInfo.DateTimeDigitized.c_str());
	if (dateTimeDig.IsValid())
	{
		tString yyyymmdd = dateTimeDig.ExtractLeft(' ');
		yyyymmdd.Replace(':', '-');
		dateTimeDig = yyyymmdd + " " + dateTimeDig;

		SetTag(tMetaTag::DateTimeDigit, dateTimeDig);
	}
}


void tMetaData::SetTags_AuthorNotes(const TinyEXIF::EXIFInfo& tinyInfo)
{
	// Software
	tString software = tinyInfo.Software.c_str();
	if (software.IsValid())
		SetTag(tMetaTag::Software, software);

	// Description
	tString description = tinyInfo.ImageDescription.c_str();
	if (description.IsValid())
		SetTag(tMetaTag::Description, description);

	// Copyright
	tString copyright = tinyInfo.Copyright.c_str();
	if (copyright.IsValid())
		SetTag(tMetaTag::Copyright, copyright);
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
		case tMetaTag::MakeModelSerial:
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

		case tMetaTag::FlashHardware:
			value = datum.Uint32 ? "Hardware Present" : "Hardware Not Present";
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
				case 2: value = "Compulsory Suppression";	break;
				case 3: value = "Auto";						break;
			}
			break;

		case tMetaTag::FlashRedEye:
			value = datum.Uint32 ? "Reduction" : "No Reduction";
			break;

		case tMetaTag::FocalLength:
			tsPrintf(value, "%d mm", tClampMin(int(datum.Float), 1));
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
				tsPrintf(value, "%d pixels/%s", int(datum.Float), unit.Chr());
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
		case tMetaTag::LensModel:
			value = datum.String;
			break;
	}
	return value;
}


void tMetaData::Save(tChunkWriter& chunk) const
{
	chunk.Begin(tChunkID::Image_MetaData);
	{
		chunk.Begin(tChunkID::Image_MetaDataVersion);
		{
			chunk.Write(ChunkVersion);
		}
		chunk.End();

		for (int d = 0; d < int(tMetaTag::NumTags); d++)
		{
			if (!Data[d].IsValid())
				continue;

			const tMetaDatum& datum = Data[d];
			chunk.Begin(tChunkID::Image_MetaDatum);
			{
				chunk.Write(d);
				chunk.Write(datum.Type);
				switch (datum.Type)
				{
					case tMetaDatum::DatumType::Uint32:
						chunk.Write(datum.Uint32);
						break;

					case tMetaDatum::DatumType::Float:
						chunk.Write(datum.Float);
						break;

					case tMetaDatum::DatumType::String:
						chunk.Write(datum.String);
						break;
				}
			}
			chunk.End();
		}
	}
	chunk.End();
}


void tMetaData::Load(const tChunk& chunk)
{
	Clear();
	if (chunk.ID() != tChunkID::Image_MetaData)
		return;

	for (tChunk ch = chunk.First(); ch.IsValid(); ch = ch.Next())
	{
		switch (ch.ID())
		{
			case tChunkID::Image_MetaDataVersion:
			{
				int version;
				ch.GetItem(version);
				break;
			}

			case tChunkID::Image_MetaDatum:
			{
				int id = 0;
				ch.GetItem(id);
				tMetaDatum& datum = Data[id];

				ch.GetItem(*((int*)&datum.Type));
				switch (datum.Type)
				{
					case tMetaDatum::DatumType::Uint32:
						ch.GetItem(datum.Uint32);
						break;

					case tMetaDatum::DatumType::Float:
						ch.GetItem(datum.Float);
						break;

					case tMetaDatum::DatumType::String:
						ch.GetItem(datum.String);
						break;
				}
				NumTagsValid++;
				break;
			}
		}
	}
}
