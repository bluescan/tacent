// TestImage.cpp
//
// Image module tests.
//
// Copyright (c) 2017, 2019-2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Image/tTexture.h>
#include <Image/tImageDDS.h>
#include <Image/tImageKTX.h>
#include <Image/tImageASTC.h>
#include <Image/tImageEXR.h>
#include <Image/tImageGIF.h>
#include <Image/tImageHDR.h>
#include <Image/tImageICO.h>
#include <Image/tImageJPG.h>
#include <Image/tImagePNG.h>
#include <Image/tImageQOI.h>
#include <Image/tImageAPNG.h>
#include <Image/tImageTGA.h>
#include <Image/tImageWEBP.h>
#include <Image/tImageXPM.h>
#include <Image/tImageBMP.h>
#include <Image/tImageTIFF.h>
#include <Image/tPaletteImage.h>
#include <System/tFile.h>
#include "UnitTests.h"
using namespace tImage;
namespace tUnitTest
{


tTestUnit(ImageLoad)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageLoad)

	// Test direct loading classes.
	tImageAPNG imgAPNG("TestData/Images/Flame.apng");
	tRequire(imgAPNG.IsValid());

	tImageASTC imgASTC("TestData/Images/ASTC/ASTC10x10_LDR.astc");
	tRequire(imgASTC.IsValid());

	tImageBMP imgBMP("TestData/Images/UpperB.bmp");
	tRequire(imgBMP.IsValid());

	tImageDDS imgDDS("TestData/Images/DDS/BC1DXT1_RGB_Legacy.dds");
	tRequire(imgDDS.IsValid());

	tImageEXR imgEXR("TestData/Images/Desk.exr");
	tRequire(imgEXR.IsValid());

	tImageGIF imgGIF("TestData/Images/8-cell-simple.gif");
	tRequire(imgGIF.IsValid());

	tImageHDR imgHDR("TestData/Images/mpi_atrium_3.hdr");
	tRequire(imgHDR.IsValid());

	tImageICO imgICO("TestData/Images/UpperBounds.ico");
	tRequire(imgICO.IsValid());

	tImageJPG imgJPG("TestData/Images/WiredDrives.jpg");
	tRequire(imgJPG.IsValid());

	tImageKTX imgKTX("TestData/Images/KTX2/BC7_RGBA.ktx2");
	tRequire(imgKTX.IsValid());

	tImagePNG imgPNG("TestData/Images/TacentTestPattern.png");
	tRequire(imgPNG.IsValid());

	tImageQOI imgQOI24("TestData/Images/TacentTestPattern24.qoi");
	tRequire(imgQOI24.IsValid());

	tImageQOI imgQOI32("TestData/Images/TacentTestPattern32.qoi");
	tRequire(imgQOI32.IsValid());

	tImageTGA imgTGA("TestData/Images/WhiteBorderRLE.tga");
	tRequire(imgTGA.IsValid());

	tImageTIFF imgTIFF("TestData/Images/Tiff_NoComp.tif");
	tRequire(imgTIFF.IsValid());

	tImageWEBP imgWEBP("TestData/Images/RockyBeach.webp");
	tRequire(imgWEBP.IsValid());
}


tTestUnit(ImageSave)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageSave)

	tImageTGA tga("TestData/Images/TacentTestPattern32.tga");
	int tgaW = tga.GetWidth();
	int tgaH = tga.GetHeight();
	tPixel* tgaPixels = tga.StealPixels();
	tImageQOI qoi(tgaPixels, tgaW, tgaH, true);
	tImageQOI::tFormat result32 = qoi.Save("TestData/Images/WrittenTacentTestPattern32.qoi", tImageQOI::tFormat::Bit32);
	tRequire(result32 == tImageQOI::tFormat::Bit32);
	tImageQOI::tFormat result24 = qoi.Save("TestData/Images/WrittenTacentTestPattern24.qoi", tImageQOI::tFormat::Bit24);
	tRequire(result24 == tImageQOI::tFormat::Bit24);

	tImagePNG pngA("TestData/Images/Xeyes.png");
	pngA.Save("TestData/Images/WrittenNewA.png");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenNewA.png"));

	tImagePNG pngB("TestData/Images/TextCursor.png");
	pngB.Save("TestData/Images/WrittenNewB.png");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenNewB.png"));

	tList<tFrame> frames;

	// Test writing webp images. The basic pattern to save as a different type is to steal from one and give to the other.
	tImageAPNG apng("TestData/Images/Flame.apng");
	apng.StealFrames(frames);
	tImageWEBP webp;
	webp.Set(frames, true);
	webp.Save("TestData/Images/WrittenFlameOneFrame.webp");
	tRequire(frames.IsEmpty());
	tRequire(tSystem::tFileExists("TestData/Images/WrittenFlameOneFrame.webp"));

	tImageEXR exr("TestData/Images/Desk.exr");
	exr.StealFrames(frames);
	webp.Set(frames, true);
	webp.Save("TestData/Images/WrittenDesk.webp");
	tRequire(frames.IsEmpty());
	tRequire(tSystem::tFileExists("TestData/Images/WrittenDesk.webp"));
}


tTestUnit(ImageTexture)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageTexture)

	// Test dxt1 texture.
	tTexture dxt1Tex("TestData/Images/DDS/BC1DXT1_RGB_Legacy.dds");
	tRequire(dxt1Tex.IsValid());

	tChunkWriter writer("TestData/Images/Written_BC1DXT1_RGB_Legacy.tac");
	dxt1Tex.Save(writer);
	tRequire( tSystem::tFileExists("TestData/Images/Written_BC1DXT1_RGB_Legacy.tac") );

	tChunkReader reader("TestData/Images/Written_BC1DXT1_RGB_Legacy.tac");
	dxt1Tex.Load( reader.Chunk() );
	tRequire(dxt1Tex.IsValid());

	// Test cubemap.
	tTexture cubemap("TestData/Images/DDS/CubemapLayoutGuide.dds");
	tRequire(cubemap.IsValid());

	// Test jpg to texture. This will do conversion to BC1.
	tImageJPG jpg("TestData/Images/WiredDrives.jpg");
	int w = jpg.GetWidth(); int h = jpg.GetHeight();
	tPicture pic(w, h, jpg.StealPixels(), false); 
	tTexture bc1Tex(pic, true);

	tRequire(bc1Tex.IsValid());
	tChunkWriter chunkWriterBC1("TestData/Images/Written_WiredDrives_BC1.tac");
	bc1Tex.Save(chunkWriterBC1);
	tRequire( tSystem::tFileExists("TestData/Images/Written_WiredDrives_BC1.tac") );

	// Test ico with alpha to texture. This will do conversion to BC3.
	tImageICO ico("TestData/Images/UpperBounds.ico");
	tFrame* frame = ico.StealFrame(0);
	w = frame->Width; h = frame->Height;
	pic.Set(w, h, frame->GetPixels(true), false);
	delete frame;
	tTexture bc3Tex(pic, true);

	tRequire(bc3Tex.IsValid());
	tChunkWriter chunkWriterBC3("TestData/Images/Written_UpperBounds_BC3.tac");
	bc3Tex.Save(chunkWriterBC3);
	tRequire( tSystem::tFileExists("TestData/Images/Written_UpperBounds_BC3.tac"));
}


tTestUnit(ImagePicture)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImagePicture)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/");

	// Test generate layers.
	tImageBMP bmpL("UpperB.bmp");
	tRequire(bmpL.IsValid());

	// Test pixel constructor and mipmap gen.
	int w = bmpL.GetWidth(); int h = bmpL.GetHeight();
	tPicture srcPic(w, h, bmpL.StealPixels(), false);
	tRequire(srcPic.IsValid());
	tPrintf("GenLayers Orig W=%d H=%d\n", srcPic.GetWidth(), srcPic.GetHeight());
	tList<tLayer> layers;
	srcPic.GenerateLayers(layers);
	int lev = 0;
	for (tLayer* lay = layers.First(); lay; lay = lay->Next(), lev++)
		tPrintf("GenLayers Mip:%02d W=%d H=%d\n", lev, lay->Width, lay->Height);
	tRequire(layers.GetNumItems() == 10);
	tPicture pic;
	tImageTGA tga;

	//
	// tPicture loading/saving tests. These all save as tga and to the corresponding format if save is supported.
	//
	tImageAPNG apng;
	apng.Load("Flame.apng");
	apng.Save("WrittenFlame.apng");
	pic.Set(apng); tga.Set(pic);
	tga.Save("WrittenFlame.tga");
	tRequire( tSystem::tFileExists("WrittenFlame.apng"));

	tImageASTC astc;
	astc.Load("ASTC/ASTC10x10_LDR.astc");
	pic.Set(astc); tga.Set(pic);
	tga.Save("WrittenASTC10x10_LDR.tga");
	tRequire(tSystem::tFileExists("WrittenASTC10x10_LDR.tga"));

	tImageBMP bmp;
	bmp.Load("UpperB.bmp");
	bmp.Save("WrittenUpperB.bmp");
	pic.Set(bmp); tga.Set(pic);
	tga.Save("WrittenUpperB.tga");
	tRequire( tSystem::tFileExists("WrittenUpperB.bmp"));

	bmp.Load("Bmp_Alpha.bmp");
	bmp.Save("WrittenBmp_Alpha.bmp");
	pic.Set(bmp); tga.Set(pic);
	tga.Save("WrittenBmp_Alpha.tga");
	tRequire( tSystem::tFileExists("WrittenBmp_Alpha.bmp"));

	bmp.Load("Bmp_Lambda.bmp");
	bmp.Save("WrittenBmp_Lambda.bmp");
	pic.Set(bmp); tga.Set(pic);
	tga.Save("WrittenBmp_Lambda.tga");
	tRequire( tSystem::tFileExists("WrittenBmp_Lambda.bmp"));

	bmp.Load("Bmp_RefLena.bmp");
	bmp.Save("WrittenBmp_RefLena.bmp");
	pic.Set(bmp); tga.Set(pic);
	tga.Save("WrittenBmp_RefLena.tga");
	tRequire( tSystem::tFileExists("WrittenBmp_RefLena.bmp"));

	bmp.Load("Bmp_RefLena101.bmp");
	bmp.Save("WrittenBmp_RefLena101.bmp");
	pic.Set(bmp); tga.Set(pic);
	tga.Save("WrittenBmp_RefLena101.tga");
	tRequire( tSystem::tFileExists("WrittenBmp_RefLena101.bmp"));

	bmp.Load("Bmp_RefLenaFlip.bmp");
	bmp.Save("WrittenBmp_RefLenaFlip.bmp");
	pic.Set(bmp); tga.Set(pic);
	tga.Save("WrittenBmp_RefLenaFlip.tga");
	tRequire( tSystem::tFileExists("WrittenBmp_RefLenaFlip.bmp"));

	tImageDDS dds;
	dds.Load("DDS/BC1DXT1_RGB_Modern.dds");
	pic.Set(dds); tga.Set(pic);
	tga.Save("WrittenBC1DXT1_RGB_Modern.tga");
	tRequire( tSystem::tFileExists("WrittenBC1DXT1_RGB_Modern.tga"));

	tImageEXR exr;
	exr.Load("Desk.exr");
	pic.Set(exr); tga.Set(pic);
	tga.Save("WrittenDesk.tga");
	tRequire( tSystem::tFileExists("WrittenDesk.tga"));

	tImageGIF gif;
	gif.Load("8-cell-simple.gif");
	gif.Save("Written8-cell-simple.gif");
	pic.Set(gif); tga.Set(pic);
	tga.Save("Written8-cell-simple.tga");
	tRequire( tSystem::tFileExists("Written8-cell-simple.gif"));

	tImageHDR hdr;
	hdr.Load("mpi_atrium_3.hdr");
	pic.Set(hdr); tga.Set(pic);
	tga.Save("Writtenmpi_atrium_3.tga");
	tRequire( tSystem::tFileExists("Writtenmpi_atrium_3.tga"));

	tImageICO ico;
	ico.Load("UpperBounds.ico");
	pic.Set(ico); tga.Set(pic);
	tga.Save("WrittenUpperBounds.tga");
	tRequire( tSystem::tFileExists("WrittenUpperBounds.tga"));

	tImageJPG jpg;
	jpg.Load("WiredDrives.jpg");
	jpg.Save("WrittenWiredDrives.jpg");
	pic.Set(jpg); tga.Set(pic);
	tga.Save("WrittenWiredDrives.tga");
	tRequire( tSystem::tFileExists("WrittenWiredDrives.jpg"));

	tImageKTX ktx;
	ktx.Load("KTX1/BC7_RGBA.ktx");
	pic.Set(ktx); tga.Set(pic);
	tga.Save("WrittenBC7_RGBA.tga");
	tRequire( tSystem::tFileExists("WrittenBC7_RGBA.tga"));

	ktx.Load("KTX2/R32G32B32A32f_RGBA.ktx2");
	pic.Set(ktx); tga.Set(pic);
	tga.Save("WrittenR32G32B32A32f_RGBA.tga");
	tRequire( tSystem::tFileExists("WrittenR32G32B32A32f_RGBA.tga"));

	tImagePNG png;
	png.Load("Icos4D.png");
	png.Save("WrittenIcos4D.png");
	pic.Set(png); tga.Set(pic);
	tga.Save("WrittenIcos4D.tga");
	tRequire( tSystem::tFileExists("WrittenIcos4D.png"));

	png.Load("Xeyes.png");
	png.Save("WrittenXeyes.png");
	pic.Set(png); tga.Set(pic);
	tga.Save("WrittenXeyes.tga");
	tRequire( tSystem::tFileExists("WrittenXeyes.png"));

	tImageQOI qoi;
	qoi.Load("TacentTestPattern32.qoi");
	qoi.Save("WrittenTacentTestPattern32.qoi");
	pic.Set(qoi); tga.Set(pic);
	tga.Save("WrittenTacentTestPattern32.tga");
	tRequire( tSystem::tFileExists("WrittenTacentTestPattern32.qoi"));

	tga.Load("TacentTestPattern32RLE.tga");
	tga.Save("WrittenTacentTestPattern32RLE.tga");
	tRequire( tSystem::tFileExists("WrittenTacentTestPattern32RLE.tga"));

	tImageTIFF tif;
	tif.Load("Tiff_NoComp.tif");
	tif.Save("WrittenTiff_NoComp.tif");
	pic.Set(tif); tga.Set(pic);
	tga.Save("WrittenTiff_NoComp.tga");
	tRequire( tSystem::tFileExists("WrittenTiff_NoComp.tif"));

	tif.Load("Tiff_Pack.tif");
	tif.Save("WrittenTiff_Pack.tif");
	pic.Set(tif); tga.Set(pic);
	tga.Save("WrittenTiff_Pack.tga");
	tRequire( tSystem::tFileExists("WrittenTiff_Pack.tif"));

	tif.Load("Tiff_LZW.tif");
	tif.Save("WrittenTiff_LZW.tif");
	pic.Set(tif); tga.Set(pic);
	tga.Save("WrittenTiff_LZW.tga");
	tRequire( tSystem::tFileExists("WrittenTiff_LZW.tif"));

	tif.Load("Tiff_ZIP.tif");
	tif.Save("WrittenTiff_ZIP.tif");
	pic.Set(tif); tga.Set(pic);
	tga.Save("WrittenTiff_ZIP.tga");
	tRequire( tSystem::tFileExists("WrittenTiff_ZIP.tif"));

	tImageWEBP webp;
	webp.Load("RockyBeach.webp");
	webp.Save("WrittenRockyBeach.webp");
	pic.Set(webp); tga.Set(pic);
	tga.Save("WrittenRockyBeach.tga");
	tRequire( tSystem::tFileExists("WrittenRockyBeach.webp"));

	// tImageXPM xpm;
	// xpm.Load("Crane.xmp"); pic.Set(xpm); tga.Set(pic);
	// tga.Save("WrittenCrane.tga");
	// tRequire( tSystem::tFileExists("WrittenCrane.tga"));

	tSystem::tSetCurrentDir(origDir);
}


void QuantizeImage(int w, int h, tPixel* pixels, tPixelFormat fmt, tImage::tQuantizeMethod method)
{
	tPaletteImage pal;
	pal.Set(fmt, w, h, pixels, method);	// Create a palettized image with a specific-sized palette.

	//pal.Set(tPixelFormat::PAL1BIT, w, h, tgapix, tImage::tQuantizeMethod::Spatial);	
	tPixel* palpix = new tPixel[w*h];
	pal.Get(palpix);																// Depalettize into a pixel buffer.

	tImageTGA dsttga;
	dsttga.Set(palpix, w, h, true);													// Give the pixels to the tga.

	tString saveName;
	tsPrintf(saveName, "Written_%s_%s.tga", tGetPixelFormatName(fmt), tGetQuantizeMethodName(method));
	dsttga.Save(saveName);										// And save it out.
	tRequire(tSystem::tFileExists(saveName));
}


tTestUnit(ImagePalette)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImagePalette)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/");

	// We'll start by loading a test image.
	tImageTGA srctga;
	srctga.Load("Dock640.tga");
	int w = srctga.GetWidth();
	int h = srctga.GetHeight();
	tPixel* tgapix = srctga.GetPixels();

	//
	// Spatial quantization (scolorq).
	//
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL1BIT, tImage::tQuantizeMethod::Spatial);
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL2BIT, tImage::tQuantizeMethod::Spatial);
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL3BIT, tImage::tQuantizeMethod::Spatial);
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL4BIT, tImage::tQuantizeMethod::Spatial);
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL5BIT, tImage::tQuantizeMethod::Spatial);

	//
	// NeuQuant quantization.
	//
	/*
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL1BIT, tImage::tQuantizeMethod::Neu);
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL2BIT, tImage::tQuantizeMethod::Neu);
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL3BIT, tImage::tQuantizeMethod::Neu);
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL4BIT, tImage::tQuantizeMethod::Neu);
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL5BIT, tImage::tQuantizeMethod::Neu);
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL6BIT, tImage::tQuantizeMethod::Neu);
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL7BIT, tImage::tQuantizeMethod::Neu);
	*/
	QuantizeImage(w, h, tgapix, tPixelFormat::PAL8BIT, tImage::tQuantizeMethod::Neu);
	
	tSystem::tSetCurrentDir(origDir);
}


static void PrintMetaDataTag(const tMetaData& metaData, tMetaTag tag)
{
	tString tagName = tGetMetaTagName(tag);
	tString tagDesc = tGetMetaTagDesc(tag);
	tPrintf("TagName [%s]\n", tagName.Chr());

	// Just want to print all on one line for now.
	tagDesc.Replace('\n', '_');
	tPrintf("TagDesc [%s]\n", tagDesc.Chr());

	const tMetaDatum& datum = metaData[tag];
	switch (datum.Type)
	{
		case tMetaDatum::DatumType::Invalid:
			tPrintf("TagNotSet\n");
			break;

		case tMetaDatum::DatumType::Uint32:
			tPrintf("RawValue(Uint32) [%08x]\n", datum.Uint32);
			break;

		case tMetaDatum::DatumType::Float:
			tPrintf("RawValue(Float)  [%f]\n", datum.Float);
			break;

		case tMetaDatum::DatumType::String:
			tPrintf("RawValue(String) [%s]\n", datum.String.Chr());
			break;
	}
	tString value = metaData.GetPrettyValue(tag);
	if (value.IsValid())
		tPrintf("PrettyValue      [%s]\n", value.Chr());

	tPrintf("\n");
}


tTestUnit(ImageMetaData)
{
	if (!tSystem::tDirExists("TestData/Images/EXIF_XMP"))
		tSkipUnit(ImageMetaData)

	#if 0
	tList<tStringItem> images;
	tSystem::tFindFiles(images, "TestData/Images/EXIF_XMP/", "jpg");
	for (tStringItem* file = images.First(); file; file = file->Next())
	{
		tImageJPG tmpImg;
		tPrintf("OpeningFile:%s\n", file->Chr());
		tmpImg.Load(*file);
	}
	return;
	#endif

	tImageJPG jpgWithMeta("TestData/Images/EXIF_XMP/HasLatLong.jpg");
	tRequire(jpgWithMeta.MetaData.IsValid());
	tMetaData& metaData = jpgWithMeta.MetaData;

	PrintMetaDataTag(metaData, tMetaTag::Make);
	PrintMetaDataTag(metaData, tMetaTag::Model);
	PrintMetaDataTag(metaData, tMetaTag::SerialNumber);
	PrintMetaDataTag(metaData, tMetaTag::MakeModelSerial);
	PrintMetaDataTag(metaData, tMetaTag::LatitudeDD);
	PrintMetaDataTag(metaData, tMetaTag::LatitudeDMS);
	PrintMetaDataTag(metaData, tMetaTag::LongitudeDD);
	PrintMetaDataTag(metaData, tMetaTag::LongitudeDMS);
	PrintMetaDataTag(metaData, tMetaTag::Altitude);
	PrintMetaDataTag(metaData, tMetaTag::AltitudeRelRef);
	PrintMetaDataTag(metaData, tMetaTag::AltitudeRel);
	PrintMetaDataTag(metaData, tMetaTag::Roll);
	PrintMetaDataTag(metaData, tMetaTag::Pitch);
	PrintMetaDataTag(metaData, tMetaTag::Yaw);
	PrintMetaDataTag(metaData, tMetaTag::VelX);
	PrintMetaDataTag(metaData, tMetaTag::VelY);
	PrintMetaDataTag(metaData, tMetaTag::VelZ);
	PrintMetaDataTag(metaData, tMetaTag::Speed);

	jpgWithMeta.Load("TestData/Images/EXIF_XMP/HasUTCDateTime.jpg");

	PrintMetaDataTag(metaData, tMetaTag::GPSSurvey);
	PrintMetaDataTag(metaData, tMetaTag::GPSTimeStamp);

	// Go back to original file.
	jpgWithMeta.Load("TestData/Images/EXIF_XMP/HasLatLong.jpg");

	PrintMetaDataTag(metaData, tMetaTag::ShutterSpeed);
	PrintMetaDataTag(metaData, tMetaTag::ExposureTime);
	PrintMetaDataTag(metaData, tMetaTag::ExposureBias);
	PrintMetaDataTag(metaData, tMetaTag::FStop);
	PrintMetaDataTag(metaData, tMetaTag::ExposureProgram);
	PrintMetaDataTag(metaData, tMetaTag::ISO);
	PrintMetaDataTag(metaData, tMetaTag::Aperture);
	PrintMetaDataTag(metaData, tMetaTag::Brightness);
	PrintMetaDataTag(metaData, tMetaTag::MeteringMode);

	jpgWithMeta.Load("TestData/Images/EXIF_XMP/NoFlashComp.jpg");
	PrintMetaDataTag(metaData, tMetaTag::FlashHardware);
	PrintMetaDataTag(metaData, tMetaTag::FlashUsed);
	PrintMetaDataTag(metaData, tMetaTag::FlashStrobe);
	PrintMetaDataTag(metaData, tMetaTag::FlashMode);
	PrintMetaDataTag(metaData, tMetaTag::FlashRedEye);
	jpgWithMeta.Load("TestData/Images/EXIF_XMP/HasLatLong.jpg");

	PrintMetaDataTag(metaData, tMetaTag::FocalLength);
	PrintMetaDataTag(metaData, tMetaTag::Orientation);
	PrintMetaDataTag(metaData, tMetaTag::LengthUnit);
	PrintMetaDataTag(metaData, tMetaTag::XPixelsPerUnit);
	PrintMetaDataTag(metaData, tMetaTag::YPixelsPerUnit);
	PrintMetaDataTag(metaData, tMetaTag::BitsPerSample);
	PrintMetaDataTag(metaData, tMetaTag::ImageWidth);
	PrintMetaDataTag(metaData, tMetaTag::ImageHeight);
	PrintMetaDataTag(metaData, tMetaTag::ImageWidthOrig);
	PrintMetaDataTag(metaData, tMetaTag::ImageHeightOrig);
	PrintMetaDataTag(metaData, tMetaTag::DateTimeChange);
	PrintMetaDataTag(metaData, tMetaTag::DateTimeOrig);
	PrintMetaDataTag(metaData, tMetaTag::DateTimeDigit);

	jpgWithMeta.Load("TestData/Images/EXIF_XMP/HasAuthorNotes.jpg");

	PrintMetaDataTag(metaData, tMetaTag::Software);
	PrintMetaDataTag(metaData, tMetaTag::Description);
	PrintMetaDataTag(metaData, tMetaTag::Copyright);

	// Test loading/saving with compensation for exif orientation tags.
	for (int i = 0; i < 9; i++)
	{
		tString file;
		tsPrintf(file, "Landscape_%d.jpg", i);
		tImageJPG jpg(tString("TestData/Images/ExifOrientation/") + file);
		jpg.Save(tString("TestData/Images/ExifOrientation/Written") + file);
	}

	for (int i = 0; i < 9; i++)
	{
		tString file;
		tsPrintf(file, "Portrait_%d.jpg", i);
		tImageJPG jpg(tString("TestData/Images/ExifOrientation/") + file);
		jpg.Save(tString("TestData/Images/ExifOrientation/Written") + file);
	}
}


tTestUnit(ImageRotation)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageRotation)

	// Test writing rotated images.
	tImagePNG aropng("TestData/Images/RightArrow.png");
	int w = aropng.GetWidth(); int h = aropng.GetHeight();
	tPicture aroPic(w, h, aropng.StealPixels(), false);
	tRequire(aroPic.IsValid());

	tPrintf("Image dimensions before rotate: W:%d H:%d\n", aroPic.GetWidth(), aroPic.GetHeight());
	float angleDelta = tMath::tDegToRad(30.0f);
	int numRotations = 12;
	for (int rotNum = 0; rotNum < numRotations; rotNum++)
	{
		tPicture rotPic(aroPic);
		float angle = float(rotNum) * tMath::TwoPi / numRotations;
		rotPic.RotateCenter(angle, tColouri::transparent);

		tPrintf("Rotated %05.1f Dimensions: W:%d H:%d\n", tMath::tRadToDeg(angle), rotPic.GetWidth(), rotPic.GetHeight());
		tString writeFile;
		tsPrintf(writeFile, "TestData/Images/WrittenRightArrow_NoResampRot%03d.tga", int(tMath::tRadToDeg(angle)));

		int w = rotPic.GetWidth(); int h = rotPic.GetHeight();
		tImageTGA rottga(rotPic.StealPixels(), w, h, true);
		rottga.Save(writeFile);
	}

	// Test resampled (high quality) rotations.
	for (int rotNum = 0; rotNum < numRotations; rotNum++)
	{
		tPicture rotPic(aroPic);
		float angle = float(rotNum) * tMath::TwoPi / numRotations;
		rotPic.RotateCenter(angle, tColouri::transparent, tImage::tResampleFilter::Bilinear, tImage::tResampleFilter::None);

		tPrintf("Rotated %05.1f Dimensions: W:%d H:%d\n", tMath::tRadToDeg(angle), rotPic.GetWidth(), rotPic.GetHeight());
		tString writeFile;
		tsPrintf(writeFile, "TestData/Images/WrittenRightArrow_BilinearResampleRot%03d.tga", int(tMath::tRadToDeg(angle)));
		int w = rotPic.GetWidth(); int h = rotPic.GetHeight();
		tImageTGA rottga(rotPic.StealPixels(), w, h, true);
		rottga.Save(writeFile);
	}

	tPrintf("Test 'plane' rotation.\n");
	tImagePNG planepng("TestData/Images/plane.png");
	w = planepng.GetWidth(); h = planepng.GetHeight();
	tPicture planePic(w, h, planepng.StealPixels(), false);
	w = planePic.GetWidth();
	h = planePic.GetHeight();
	planePic.RotateCenter(-tMath::PiOver4, tColouri::transparent);
}


tTestUnit(ImageCrop)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageCrop)

	// Crop black pixels ignoring alpha (RGB channels only).
	tImagePNG png("TestData/Images/plane.png");
	tPicture planePic(png);
	int w = planePic.GetWidth();
	int h = planePic.GetHeight();
	planePic.Crop(tColouri::black, tComp_RGB);
	planePic.Crop(w, h, tPicture::Anchor::MiddleMiddle, tColouri::transparent);
	png.Set(planePic);
	bool ok = png.Save("TestData/Images/WrittenPlane.png");
	tRequire(ok);
}


tTestUnit(ImageDetection)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageDetection)

	// Test APNG detection.
	bool isAnimA = tImageAPNG::IsAnimatedPNG("TestData/Images/TextCursor.png");
	tRequire(!isAnimA);

	bool isAnimB = tImageAPNG::IsAnimatedPNG("TestData/Images/Icos4D.apng");
	tRequire(isAnimB);

	bool isAnimC = tImageAPNG::IsAnimatedPNG("TestData/Images/Icos4D.png");
	tRequire(isAnimC);

	int rgbaBPP		= tImage::tGetBitsPerPixel(tPixelFormat::R8G8B8A8);
	float rgbaBPPf	= tImage::tGetBitsPerPixelFloat(tPixelFormat::R8G8B8A8);
	tRequire(rgbaBPP == 32);
	tRequire(rgbaBPPf == 32.0f);

	int rgbBPP		= tImage::tGetBitsPerPixel(tPixelFormat::R8G8B8);
	float rgbBPPf	= tImage::tGetBitsPerPixelFloat(tPixelFormat::R8G8B8);
	tRequire(rgbBPP == 24);
	tRequire(rgbBPPf == 24.0f);

	int exrBPP		= tImage::tGetBitsPerPixel(tPixelFormat::OPENEXR);
	float exrBPPf	= tImage::tGetBitsPerPixelFloat(tPixelFormat::OPENEXR);
	tPrintf("EXR BPP:%d BPPf:%f\n", exrBPP, exrBPPf);
}


tTestUnit(ImageFilter)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageFilter)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/");

	for (int filt = 0; filt < int(tResampleFilter::NumFilters); filt++)
		tPrintf("Filter Name %d: %s\n", filt, tResampleFilterNames[filt]);

	// Resample tests of 512x256 image.
	tImagePNG png("TextCursor.png");
	tImageTGA tga;
	tPicture pic;

	pic.Set(png, false);
	pic.Resample(800, 300, tResampleFilter::Nearest);
	tga.Set(pic); tga.Save("WrittenResampledNearest.tga");

	pic.Set(png, false);
	pic.Resample(800, 300, tResampleFilter::Box);
	tga.Set(pic); tga.Save("WrittenResampledBox.tga");

	pic.Set(png, false);
	pic.Resample(800, 300, tResampleFilter::Bilinear);
	tga.Set(pic); tga.Save("WrittenResampledBilinear.tga");

	pic.Set(png, false);
	pic.Resample(800, 300, tResampleFilter::Bicubic_Standard);
	tga.Set(pic); tga.Save("WrittenResampledBicubicStandard.tga");

	pic.Set(png, false);
	pic.Resample(800, 300, tResampleFilter::Bicubic_CatmullRom);
	tga.Set(pic); tga.Save("WrittenResampledBicubicCatmullRom.tga");

	pic.Set(png, false);
	pic.Resample(800, 300, tResampleFilter::Bicubic_Mitchell);
	tga.Set(pic); tga.Save("WrittenResampledBicubicMitchell.tga");

	pic.Set(png, false);
	pic.Resample(800, 300, tResampleFilter::Bicubic_Cardinal);
	tga.Set(pic); tga.Save("WrittenResampledBicubicCardinal.tga");

	pic.Set(png, false);
	pic.Resample(800, 300, tResampleFilter::Bicubic_BSpline);
	tga.Set(pic); tga.Save("WrittenResampledBicubicBSpline.tga");

	pic.Set(png, false);
	pic.Resample(800, 300, tResampleFilter::Lanczos_Narrow);
	tga.Set(pic); tga.Save("WrittenResampledLanczosNarrow.tga");

	pic.Set(png, false);
	pic.Resample(800, 300, tResampleFilter::Lanczos_Normal);
	tga.Set(pic); tga.Save("WrittenResampledLanczosNormal.tga");

	pic.Set(png, false);
	pic.Resample(800, 300, tResampleFilter::Lanczos_Wide);
	tga.Set(pic); tga.Save("WrittenResampledLanczosWide.tga");

	tSystem::tSetCurrentDir(origDir);
}


tTestUnit(ImageMultiFrame)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageMultiFrame)

	tImageTIFF tif;
	tImageTGA tga;
	tPicture pic;

	// A multipage tiff.
	tif.Load("TestData/Images/Tiff_Multipage_ZIP.tif");
	tRequire(tif.IsValid());

	tFrame* frame0 = tif.GetFrame(0);
	pic.Set(frame0, false); tga.Set(pic);
	tga.Save("TestData/Images/WrittenTiff_Multipage_ZIP_P1.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenTiff_Multipage_ZIP_P1.tga"));

	tFrame* frame1 = tif.GetFrame(1);
	pic.Set(frame1, false); tga.Set(pic);
	tga.Save("TestData/Images/WrittenTiff_Multipage_ZIP_P2.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenTiff_Multipage_ZIP_P2.tga"));

	tFrame* frame2 = tif.GetFrame(2);
	pic.Set(frame2, false); tga.Set(pic);
	tga.Save("TestData/Images/WrittenTiff_Multipage_ZIP_P3.tga");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenTiff_Multipage_ZIP_P3.tga"));

	// tImageWEBP also supports saving multi-frame webp files.
	tImageAPNG apngSrc("TestData/Images/Flame.apng");
	tImageWEBP webpDst( apngSrc.Frames, true);
	webpDst.Save("TestData/Images/WrittenFlameManyFrames.webp");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenFlameManyFrames.webp"));

	tImageAPNG apngSrc2("TestData/Images/Icos4D.apng");
	tImageWEBP webpDst2( apngSrc2.Frames, true);
	webpDst2.Save("TestData/Images/WrittenIcos4DManyFrames.webp");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenIcos4DManyFrames.webp"));

	// tImageGIF supports saving multi-frame gif files.
	tImageAPNG apngSrc3("TestData/Images/Icos4D.apng");
	tImageGIF gifDst(apngSrc3.Frames, true);
	gifDst.Save("TestData/Images/WrittenIcos4DManyFrames.gif");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenIcos4DManyFrames.gif"));

	// tImageAPNG supports saving multi-frame apng files.
	tImageAPNG apngSrc4("TestData/Images/Icos4D.apng");
	tImageAPNG apngDst(apngSrc4.Frames, true);
	apngDst.Save("TestData/Images/WrittenIcos4DManyFrames.apng");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenIcos4DManyFrames.apng"));

	// Load a multipage tiff with no page duration info.
	tPrintf("Test multipage TIFF load.\n");
	tImageTIFF tiffMultipage("TestData/Images/Tiff_Multipage_ZIP.tif");
	tRequire(tiffMultipage.IsValid());

	// Create a multipage tiff with page duration info.
	tImageAPNG apngSrc5("TestData/Images/Icos4D.apng");
	tImageTIFF tiffDst(apngSrc5.Frames, true);
	tiffDst.Save("TestData/Images/WrittenIcos4DManyFrames.tiff");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenIcos4DManyFrames.tiff"));

	// Load a multipage tiff with page duration info since it was saved from Tacent.
	tImageTIFF tiffWithDur("TestData/Images/WrittenIcos4DManyFrames.tiff");
	tiffWithDur.Save("TestData/Images/WrittenIcos4DManyFrames2.tiff");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenIcos4DManyFrames2.tiff"));
}


tTestUnit(ImageGradient)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageGradient)

	const int width = 640;
	const int height = 90;
	tPixel* pixels = nullptr;

	// Gradient black to white.
	pixels = new tPixel[width*height];
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			pixels[y*width + x] = tColour(256*x / width, 256*x / width, 256*x / width, 255);
	tImageTGA blackToWhite(pixels, width, height, true);
	tRequire(blackToWhite.IsValid());
	blackToWhite.Save("TestData/Images/Written_Gradient_BlackToWhite.tga", tImageTGA::tFormat::Bit24, tImageTGA::tCompression::RLE);

	// Gradient black to transparent.
	pixels = new tPixel[width*height];
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			pixels[y*width + x] = tColour(0, 0, 0, 255 - 256*x / width);
	tImageTGA blackToTrans(pixels, width, height, true);
	tRequire(blackToTrans.IsValid());
	blackToTrans.Save("TestData/Images/Written_Gradient_BlackToTrans.tga", tImageTGA::tFormat::Bit32, tImageTGA::tCompression::RLE);

	// Gradient transparent to white.
	pixels = new tPixel[width*height];
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			pixels[y*width + x] = tColour(255, 255, 255, 256*x / width);
	tImageTGA transToWhite(pixels, width, height, true);
	tRequire(transToWhite.IsValid());
	transToWhite.Save("TestData/Images/Written_Gradient_TransToWhite.tga", tImageTGA::tFormat::Bit32, tImageTGA::tCompression::RLE);

	// Gradient red to yellow to green to cyan to blue to magenta to red.
	const int section = width / 6;
	pixels = new tPixel[width*height];
	for (int y = 0; y < height; y++)
	{
		// Red to yellow.
		for (int x = 0; x < section; x++)
			pixels[y*width + section*0+x+0] = tColour(255, 256*x/section, 0, 255);

		// Yellow to Green.
		for (int x = 0; x < section+1; x++)
			pixels[y*width + section*1+x+0] = tColour(255-256*x/section, 255, 0, 255);

		// Green to Cyan.
		for (int x = 0; x < section+1; x++)
			pixels[y*width + section*2+x+1] = tColour(0, 255, 256*x/section, 255);

		// Cyan to Blue.
		for (int x = 0; x < section+1; x++)
			pixels[y*width + section*3+x+2] = tColour(0, 255-256*x/section, 255, 255);

		// Blue to Magenta.
		for (int x = 0; x < section+1; x++)
			pixels[y*width + section*4+x+3] = tColour(256*x/section, 0, 255, 255);

		// Magenta to Red.
		for (int x = 0; x < section; x++)
			pixels[y*width + section*5+x+4] = tColour(255, 0, 255-256*x/section, 255);
	}
	tImageTGA redToRed(pixels, width, height, true);
	tRequire(redToRed.IsValid());
	redToRed.Save("TestData/Images/Written_Gradient_RedToRed.tga", tImageTGA::tFormat::Bit24, tImageTGA::tCompression::RLE);	
}


// Helper for tImageDDS unit tests.
void DDSLoadDecodeSave(const tString& ddsfile, uint32 loadFlags = 0, bool saveAllMips = false)
{
	// We're just going to turn on auto-gamma-compression for all files.
	loadFlags |= tImageKTX::LoadFlag_AutoGamma;

	tString basename = tSystem::tGetFileBaseName(ddsfile);
	tString savename = basename + "_";
	savename += (loadFlags & tImageDDS::LoadFlag_Decode)			? "D" : "x";
	if ((loadFlags & tImageKTX::LoadFlag_GammaCompression) || (loadFlags & tImageKTX::LoadFlag_SRGBCompression))
		savename += "G";
	else if (loadFlags & tImageKTX::LoadFlag_AutoGamma)
		savename += "g";
	else
		savename += "x";
	savename += (loadFlags & tImageDDS::LoadFlag_ReverseRowOrder)	? "R" : "x";
	savename += (loadFlags & tImageDDS::LoadFlag_SpreadLuminance)	? "S" : "x";
	tPrintf("DDS Load %s\n", savename.Chr());
	tString formatname = basename.Left('_');

	tImageDDS::LoadParams params;
	params.Flags = loadFlags;
	tImageDDS dds(ddsfile, params);
	tRequire(dds.IsValid());
	tPixelFormat fileformat = tGetPixelFormat(formatname.Chr());
	tPixelFormat ddsformat = dds.GetPixelFormat();
	tPixelFormat ddsformatsrc = dds.GetPixelFormatSrc();
	tRequire(fileformat == ddsformatsrc);
	if (loadFlags & tImageDDS::LoadFlag_Decode)
		tRequire(ddsformat == tPixelFormat::R8G8B8A8);
	else
		tRequire(ddsformat == fileformat);

	// If we asked to flip rows but it couldn't, print a message. The conditional is
	// only set if we requested and it couldn't be done.
	if (dds.IsResultSet(tImageDDS::ResultCode::Conditional_CouldNotFlipRows))
		tPrintf("Could not flip rows for %s\n", savename.Chr());

	tList<tImage::tLayer> layers;
	dds.StealLayers(layers);

	if (ddsformat != tPixelFormat::R8G8B8A8)
	{
		tPrintf("No tga save. Pixel format not R8G8B8A8\n");
	}
	else
	{
		if (saveAllMips)
		{
			int mipNum = 0;
			for (tLayer* layer = layers.First(); layer; layer = layer->Next(), mipNum++)
			{
				tImageTGA tga((tPixel*)layer->Data, layer->Width, layer->Height);
				tString mipName;
				tsPrintf(mipName, "Written_%s_Mip%02d.tga", savename.Chr(), mipNum);
				tga.Save(mipName);
			}
		}
		else
		{
			if (tLayer* layer = layers.First())
			{
				tImageTGA tga((tPixel*)layer->Data, layer->Width, layer->Height);
				tga.Save("Written_" + savename + ".tga");
			}
		}
	}
	tPrintf("\n");
}


tTestUnit(ImageDDS)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageDDS)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/DDS/");

	uint32 decode = tImageDDS::LoadFlag_Decode;
	uint32 revrow = tImageDDS::LoadFlag_ReverseRowOrder;
	uint32 spread = tImageDDS::LoadFlag_SpreadLuminance;

	tPrintf("Testing DDS Loading/Decoding. Legacy = No DX10 Header.\n\n");
	tPrintf("D = Decode\n");
	tPrintf("G = Explicit Gamma or sRGB Compression. g = auto\n");
	tPrintf("R = Reverse Row Order\n");
	tPrintf("S = Spread Luminance\n");

	//
	// Block Compressed Formats.
	//
	// BC1
	DDSLoadDecodeSave("BC1DXT1_RGB_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("BC1DXT1_RGB_Modern.dds", decode | revrow);

	// BC1a
	DDSLoadDecodeSave("BC1DXT1a_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("BC1DXT1a_RGBA_Modern.dds", decode | revrow);

	// BC2
	DDSLoadDecodeSave("BC2DXT2DXT3_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("BC2DXT2DXT3_RGBA_Modern.dds", decode | revrow);

	// BC3
	DDSLoadDecodeSave("BC3DXT4DXT5_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("BC3DXT4DXT5_RGBA_Modern.dds", decode | revrow);

	// BC4
	DDSLoadDecodeSave("BC4ATI1_R_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("BC4ATI1_R_Modern.dds", decode | revrow | spread);

	// BC5
	DDSLoadDecodeSave("BC5ATI2_RG_Modern.dds", decode | revrow);

	// BC6
	DDSLoadDecodeSave("BC6s_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("BC6u_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("BC6s_HDRRGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("BC6u_HDRRGB_Modern.dds", decode | revrow);

	// BC7
	DDSLoadDecodeSave("BC7_RGBA_Modern.dds", decode | revrow, true);

	//
	// ASTC
	//
	DDSLoadDecodeSave("ASTC4x4_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC5x4_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC5x5_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC6x5_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC6x6_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC8x5_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC8x6_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC8x8_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC10x5_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC10x6_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC10x8_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC10x10_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC12x10_RGB_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("ASTC12x12_RGB_Modern.dds", decode | revrow);

	//
	// Uncompressed Integer Formats.
	//
	// A8
	DDSLoadDecodeSave("A8_A_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("A8_A_Modern.dds", decode | revrow);

	// L8
	DDSLoadDecodeSave("L8_L_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("L8_L_Legacy.dds", decode | revrow | spread);
	DDSLoadDecodeSave("R8_L_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("R8_L_Modern.dds", decode | revrow | spread);

	// B8G8R8
	DDSLoadDecodeSave("B8G8R8_RGB_Legacy.dds", decode | revrow);

	// B8G8R8A8
	DDSLoadDecodeSave("B8G8R8A8_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("B8G8R8A8_RGBA_Modern.dds", decode | revrow);

	// B5G6R5
	DDSLoadDecodeSave("B5G6R5_RGB_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("B5G6R5_RGB_Modern.dds", decode | revrow);

	// B4G4R4A4
	DDSLoadDecodeSave("B4G4R4A4_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("B4G4R4A4_RGBA_Modern.dds", decode | revrow);

	// B5G5R5A1
	DDSLoadDecodeSave("B5G5R5A1_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("B5G5R5A1_RGBA_Modern.dds", decode | revrow);

	//
	// Uncompressed Floating-Point (HDR) Formats.
	//
	// R16F
	DDSLoadDecodeSave("R16f_R_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("R16f_R_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("R16f_R_Legacy.dds", decode | revrow | spread);
	DDSLoadDecodeSave("R16f_R_Modern.dds", decode | revrow | spread);

	// R16G16F
	DDSLoadDecodeSave("R16G16f_RG_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("R16G16f_RG_Modern.dds", decode | revrow);

	// R16G16B16A16F
	DDSLoadDecodeSave("R16G16B16A16f_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("R16G16B16A16f_RGBA_Modern.dds", decode | revrow);

	// R32F
	DDSLoadDecodeSave("R32f_R_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("R32f_R_Modern.dds", decode | revrow);
	DDSLoadDecodeSave("R32f_R_Legacy.dds", decode | revrow | spread);
	DDSLoadDecodeSave("R32f_R_Modern.dds", decode | revrow | spread);

	// R32G32F
	DDSLoadDecodeSave("R32G32f_RG_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("R32G32f_RG_Modern.dds", decode | revrow);

	// R32G32B32A32F
	DDSLoadDecodeSave("R32G32B32A32f_RGBA_Legacy.dds", decode | revrow);
	DDSLoadDecodeSave("R32G32B32A32f_RGBA_Modern.dds", decode | revrow);

	// Do this all over again, but without decoding and tRequire the pixel-format to be as expected.
	// This time, since not decoding, it may be impossible to reverse the rows, so we can also expect
	// to get conditional valids if it couldn't be done (for some of the BC formats). We're only going
	// to bother with the modern-style dds files (for the most part) this time through.
	tPrintf("Testing DDS Loading/No-decoding.\n\n");

	DDSLoadDecodeSave("BC1DXT1_RGB_Modern.dds", revrow);		// Revrow should work for BC1.
	DDSLoadDecodeSave("BC1DXT1a_RGBA_Modern.dds");
	DDSLoadDecodeSave("BC2DXT2DXT3_RGBA_Modern.dds", revrow);
	DDSLoadDecodeSave("BC3DXT4DXT5_RGBA_Modern.dds", revrow);
	DDSLoadDecodeSave("BC4ATI1_R_Modern.dds", revrow);			// Should print warning and be unable to flip rows. May be able to implement.
	DDSLoadDecodeSave("BC5ATI2_RG_Modern.dds", revrow);			// No reverse.
	DDSLoadDecodeSave("BC6s_RGB_Modern.dds", revrow);			// No reverse.
	DDSLoadDecodeSave("BC6u_RGB_Modern.dds");
	DDSLoadDecodeSave("BC6s_HDRRGB_Modern.dds");
	DDSLoadDecodeSave("BC6u_HDRRGB_Modern.dds", revrow);		// No reverse.
	DDSLoadDecodeSave("BC7_RGBA_Modern.dds", revrow);			// No reverse.

	DDSLoadDecodeSave("ASTC4x4_RGB_Modern.dds", revrow);		// No reverse.
	DDSLoadDecodeSave("ASTC5x4_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC5x5_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC6x5_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC6x6_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC8x5_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC8x6_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC8x8_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC10x5_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC10x6_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC10x8_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC10x10_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC12x10_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC12x12_RGB_Modern.dds");

	DDSLoadDecodeSave("A8_A_Modern.dds");
	DDSLoadDecodeSave("R8_L_Modern.dds", revrow);
	DDSLoadDecodeSave("L8_L_Legacy.dds", revrow);
	DDSLoadDecodeSave("B8G8R8_RGB_Legacy.dds");					// Only legacy supports this format.
	DDSLoadDecodeSave("B8G8R8A8_RGBA_Modern.dds");
	DDSLoadDecodeSave("B5G6R5_RGB_Modern.dds", revrow);
	DDSLoadDecodeSave("B4G4R4A4_RGBA_Modern.dds", revrow);
	DDSLoadDecodeSave("B5G5R5A1_RGBA_Modern.dds");

	DDSLoadDecodeSave("R16f_R_Modern.dds", revrow);
	DDSLoadDecodeSave("R16f_R_Modern.dds");
	DDSLoadDecodeSave("R16G16f_RG_Modern.dds", revrow);
	DDSLoadDecodeSave("R16G16B16A16f_RGBA_Modern.dds");

	DDSLoadDecodeSave("R32f_R_Modern.dds", revrow);
	DDSLoadDecodeSave("R32f_R_Modern.dds");
	DDSLoadDecodeSave("R32G32f_RG_Modern.dds");
	DDSLoadDecodeSave("R32G32B32A32f_RGBA_Modern.dds", revrow);

	tSystem::tSetCurrentDir(origDir.Chr());
}


// Helper for tImageKTX (V1 and V2) unit tests.
void KTXLoadDecodeSave(const tString& ktxfile, uint32 loadFlags = 0, bool saveAllMips = false)
{
	// We're just going to turn on auto-gamma-compression for all files.
	loadFlags |= tImageKTX::LoadFlag_AutoGamma;

	tString basename = tSystem::tGetFileBaseName(ktxfile);
	tString savename = basename + "_";
	savename += (loadFlags & tImageKTX::LoadFlag_Decode)			? "D" : "x";
	if ((loadFlags & tImageKTX::LoadFlag_GammaCompression) || (loadFlags & tImageKTX::LoadFlag_SRGBCompression))
		savename += "G";
	else if (loadFlags & tImageKTX::LoadFlag_AutoGamma)
		savename += "g";
	else
		savename += "x";
	savename += (loadFlags & tImageKTX::LoadFlag_ReverseRowOrder)	? "R" : "x";
	savename += (loadFlags & tImageKTX::LoadFlag_SpreadLuminance)	? "S" : "x";
	tPrintf("KTX Load %s\n", savename.Chr());
	tString formatname = basename.Left('_');

	tImageKTX::LoadParams params;
	params.Flags = loadFlags;
	tImageKTX ktx(ktxfile, params);
	tRequire(ktx.IsValid());
	tPixelFormat fileformat = tGetPixelFormat(formatname.Chr());
	tPixelFormat ktxformat = ktx.GetPixelFormat();
	tPixelFormat ktxformatsrc = ktx.GetPixelFormatSrc();
	tRequire(fileformat == ktxformatsrc);
	if (loadFlags & tImageKTX::LoadFlag_Decode)
		tRequire(ktxformat == tPixelFormat::R8G8B8A8);
	else
		tRequire(ktxformat == fileformat);

	// If we asked to flip rows but it couldn't, print a message. The conditional is
	// only set if we requested and it couldn't be done.
	if (ktx.IsResultSet(tImageKTX::ResultCode::Conditional_CouldNotFlipRows))
		tPrintf("Could not flip rows for %s\n", savename.Chr());

	tList<tImage::tLayer> layers;
	ktx.StealLayers(layers);

	if (ktxformat != tPixelFormat::R8G8B8A8)
	{
		tPrintf("No tga save. Pixel format not R8G8B8A8\n");
	}
	else
	{
		if (saveAllMips)
		{
			int mipNum = 0;
			for (tLayer* layer = layers.First(); layer; layer = layer->Next(), mipNum++)
			{
				tImageTGA tga((tPixel*)layer->Data, layer->Width, layer->Height);
				tString mipName;
				tsPrintf(mipName, "Written_%s_Mip%02d.tga", savename.Chr(), mipNum);
				tga.Save(mipName);
			}
		}
		else
		{
			if (tLayer* layer = layers.First())
			{
				tImageTGA tga((tPixel*)layer->Data, layer->Width, layer->Height);
				tga.Save("Written_" + savename + ".tga");
			}
		}
	}
	tPrintf("\n");
}


tTestUnit(ImageKTX1)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageKTX1)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/KTX1/");

	uint32 decode = tImageKTX::LoadFlag_Decode;
	uint32 revrow = tImageKTX::LoadFlag_ReverseRowOrder;
	uint32 spread = tImageKTX::LoadFlag_SpreadLuminance;

	tPrintf("Testing KTX V1 Loading/Decoding Using LibKTX %s\n\n", tImage::Version_LibKTX);
	tPrintf("D = Decode\n");
	tPrintf("G = Explicit Gamma or sRGB Compression. g = auto\n");
	tPrintf("R = Reverse Row Order\n");
	tPrintf("S = Spread Luminance\n");

	//
	// Block Compressed Formats.
	//
	// BC1
	KTXLoadDecodeSave("BC1DXT1_RGB.ktx", decode | revrow);

	// BC1a
	KTXLoadDecodeSave("BC1DXT1a_RGBA.ktx", decode | revrow);

	// BC2
	KTXLoadDecodeSave("BC2DXT2DXT3_RGBA.ktx", decode | revrow);

	// BC3
	KTXLoadDecodeSave("BC3DXT4DXT5_RGBA.ktx", decode | revrow);

	// BC4
	KTXLoadDecodeSave("BC4ATI1_R.ktx", decode | revrow);

	// BC5
	KTXLoadDecodeSave("BC5ATI2_RG.ktx", decode | revrow);

	// BC6
	KTXLoadDecodeSave("BC6u_RGB.ktx", decode | revrow);
	KTXLoadDecodeSave("BC6s_RGB.ktx", decode | revrow);

	// BC7
	KTXLoadDecodeSave("BC7_RGBA.ktx", decode | revrow);

	//
	// ASTC
	//
	KTXLoadDecodeSave("ASTC4x4_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC5x4_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC5x5_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC6x5_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC6x6_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC8x5_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC8x6_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC8x8_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC10x5_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC10x6_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC10x8_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC10x10_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC12x10_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC12x12_HDRRGBA.ktx", decode | revrow);

	//
	// Uncompressed Formats.
	//
	KTXLoadDecodeSave("R8G8B8A8_RGBA.ktx", decode | revrow );
	KTXLoadDecodeSave("R16G16B16A16f_RGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("R32G32B32A32f_RGBA.ktx", decode | revrow);

	// Do this all over again, but without decoding and tRequire the pixel-format to be as expected.
	// This time, since not decoding, it may be impossible to reverse the rows, so we can also expect
	// to get conditional valids if it couldn't be done (for some of the BC formats).
	// Note that without decoding KTXLoadDecodeSave will NOT write a tga file unless the pixel-format
	// is already R8G8B8A8.
	tPrintf("Testing KTX V1 Loading/No-decoding.\n\n");

	KTXLoadDecodeSave("BC1DXT1_RGB.ktx", revrow);				// Revrow should work for BC1.
	KTXLoadDecodeSave("BC1DXT1a_RGBA.ktx");
	KTXLoadDecodeSave("BC2DXT2DXT3_RGBA.ktx", revrow);
	KTXLoadDecodeSave("BC3DXT4DXT5_RGBA.ktx", revrow);
	KTXLoadDecodeSave("BC4ATI1_R.ktx", revrow);					// Should print warning and be unable to flip rows. May be able to implement.
	KTXLoadDecodeSave("BC5ATI2_RG.ktx", revrow);				// No reverse.
	KTXLoadDecodeSave("BC6u_RGB.ktx", revrow);					// No reverse.
	KTXLoadDecodeSave("BC6s_RGB.ktx", revrow);					// No reverse.
	KTXLoadDecodeSave("BC7_RGBA.ktx", revrow);					// No reverse.
	KTXLoadDecodeSave("ASTC4x4_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC5x4_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC5x5_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC6x5_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC6x6_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC8x5_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC8x6_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC8x8_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10x5_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10x6_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10x8_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10x10_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC12x10_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC12x12_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("R8G8B8A8_RGBA.ktx", revrow);				// Will write a tga even without decode since it's already in correct format.
	KTXLoadDecodeSave("R16G16B16A16f_RGBA.ktx", revrow);
	KTXLoadDecodeSave("R32G32B32A32f_RGBA.ktx", revrow);

	tSystem::tSetCurrentDir(origDir.Chr());
}


tTestUnit(ImageKTX2)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageKTX2)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/KTX2/");

	uint32 decode = tImageKTX::LoadFlag_Decode;
	uint32 revrow = tImageKTX::LoadFlag_ReverseRowOrder;
	uint32 spread = tImageKTX::LoadFlag_SpreadLuminance;

	tPrintf("Testing KTX2 Loading/Decoding Using LibKTX %s\n\n", tImage::Version_LibKTX);
	tPrintf("D = Decode\n");
	tPrintf("G = Explicit Gamma or sRGB Compression. g = auto\n");
	tPrintf("R = Reverse Row Order\n");
	tPrintf("S = Spread Luminance\n");

	//
	// Block Compressed Formats.
	//
	// BC1
	KTXLoadDecodeSave("BC1DXT1_RGB.ktx2", decode | revrow);

	// BC1a
	KTXLoadDecodeSave("BC1DXT1a_RGBA.ktx2", decode | revrow);

	// BC2
	KTXLoadDecodeSave("BC2DXT2DXT3_RGBA.ktx2", decode | revrow);

	// BC3
	KTXLoadDecodeSave("BC3DXT4DXT5_RGBA.ktx2", decode | revrow);

	// BC4
	KTXLoadDecodeSave("BC4ATI1_R.ktx2", decode | revrow);

	// BC5
	KTXLoadDecodeSave("BC5ATI2_RG.ktx2", decode | revrow);

	// BC6
	KTXLoadDecodeSave("BC6s_RGB.ktx2", decode | revrow);

	// BC7
	KTXLoadDecodeSave("BC7_RGBA.ktx2", decode | revrow, true);
	KTXLoadDecodeSave("BC7_RGBANoSuper.ktx2", decode | revrow, true);

	//
	// ASTC
	//
	KTXLoadDecodeSave("ASTC4x4_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC5x4_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC5x5_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC6x5_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC6x6_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC8x5_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC8x6_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC8x8_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10x5_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10x5_RGBA_Mipmaps.ktx2", decode | revrow, true);
	KTXLoadDecodeSave("ASTC10x6_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10x8_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10x10_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC12x10_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC12x12_RGBA.ktx2", decode | revrow);

	KTXLoadDecodeSave("ASTC4x4_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC5x4_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC5x5_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC6x5_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC6x6_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC8x5_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC8x6_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC8x8_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10x5_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10x6_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10x8_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10x10_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC12x10_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC12x12_HDRRGBA.ktx2", decode | revrow);

	//
	// Uncompressed Integer Formats.
	//
	// R8
	KTXLoadDecodeSave("R8_A.ktx2", decode | revrow);

	// L8
	KTXLoadDecodeSave("R8_L.ktx2", decode | revrow);
	KTXLoadDecodeSave("R8_L.ktx2", decode | revrow | spread);

	// B8G8R8
	KTXLoadDecodeSave("B8G8R8_RGB.ktx2", decode | revrow);

	// B8G8R8A8
	KTXLoadDecodeSave("B8G8R8A8_RGBA.ktx2", decode | revrow);

	//
	// Uncompressed Floating-Point (HDR) Formats.
	//
	// R16F
	KTXLoadDecodeSave("R16f_R.ktx2", decode | revrow);
	KTXLoadDecodeSave("R16f_R.ktx2", decode | revrow | spread);

	// R16G16F
	KTXLoadDecodeSave("R16G16f_RG.ktx2", decode | revrow);

	// R16G16B16A16F
	KTXLoadDecodeSave("R16G16B16A16f_RGBA.ktx2", decode | revrow);

	// R32F
	KTXLoadDecodeSave("R32f_R.ktx2", decode | revrow);
	KTXLoadDecodeSave("R32f_R.ktx2", decode | revrow | spread);

	// R32G32F
	KTXLoadDecodeSave("R32G32f_RG.ktx2", decode | revrow);

	// R32G32B32A32F
	KTXLoadDecodeSave("R32G32B32A32f_RGBA.ktx2", decode | revrow);

	// Do this all over again, but without decoding and tRequire the pixel-format to be as expected.
	// This time, since not decoding, it may be impossible to reverse the rows, so we can also expect
	// to get conditional valids if it couldn't be done (for some of the BC formats).
	tPrintf("Testing KTX2 Loading/No-decoding.\n\n");

	KTXLoadDecodeSave("BC1DXT1_RGB.ktx2", revrow);				// Revrow should work for BC1.
	KTXLoadDecodeSave("BC1DXT1a_RGBA.ktx2");
	KTXLoadDecodeSave("BC2DXT2DXT3_RGBA.ktx2", revrow);
	KTXLoadDecodeSave("BC3DXT4DXT5_RGBA.ktx2", revrow);
	KTXLoadDecodeSave("BC4ATI1_R.ktx2", revrow);				// Should print warning and be unable to reverse rows. May be able to implement.
	KTXLoadDecodeSave("BC5ATI2_RG.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("BC6s_RGB.ktx2", revrow);					// No reverse.
	KTXLoadDecodeSave("BC7_RGBA.ktx2", revrow);					// No reverse.
	KTXLoadDecodeSave("BC7_RGBANoSuper.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC4x4_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC5x4_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC5x5_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC6x5_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC6x6_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC8x5_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC8x6_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC8x8_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC10x5_RGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10x6_RGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10x8_RGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10x10_RGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC12x10_RGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC12x12_RGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC4x4_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC5x4_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC5x5_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC6x5_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC6x6_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC8x5_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC8x6_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC8x8_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10x5_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10x6_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10x8_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10x10_HDRRGBA.ktx2", revrow);		// No reverse.
	KTXLoadDecodeSave("ASTC12x10_HDRRGBA.ktx2", revrow);		// No reverse.
	KTXLoadDecodeSave("ASTC12x12_HDRRGBA.ktx2", revrow);		// No reverse.
	KTXLoadDecodeSave("R8_A.ktx2");
	KTXLoadDecodeSave("R8_L.ktx2", revrow);
	KTXLoadDecodeSave("B8G8R8_RGB.ktx2");
	KTXLoadDecodeSave("B8G8R8A8_RGBA.ktx2");

	KTXLoadDecodeSave("R16f_R.ktx2", revrow);
	KTXLoadDecodeSave("R16f_R.ktx2");
	KTXLoadDecodeSave("R16G16f_RG.ktx2", revrow);
	KTXLoadDecodeSave("R16G16B16A16f_RGBA.ktx2");

	KTXLoadDecodeSave("R32f_R.ktx2", revrow);
	KTXLoadDecodeSave("R32f_R.ktx2");
	KTXLoadDecodeSave("R32G32f_RG.ktx2");
	KTXLoadDecodeSave("R32G32B32A32f_RGBA.ktx2", revrow);

	tSystem::tSetCurrentDir(origDir.Chr());
}


// Helper for tImageASTC unit tests.
void ASTCLoadDecodeSave(const tString& astcfile, const tImageASTC::LoadParams& params)
{
	uint32 loadFlags = params.Flags;
	tString basename = tSystem::tGetFileBaseName(astcfile);
	tString savename = basename + "_";
	if (loadFlags & tImageASTC::LoadFlag_Decode)
		savename += "D";
	if ((loadFlags & tImageASTC::LoadFlag_GammaCompression) || (loadFlags & tImageKTX::LoadFlag_SRGBCompression))
		savename += "G";
	if (loadFlags & tImageASTC::LoadFlag_ReverseRowOrder)
		savename += "R";

	switch (params.Profile)
	{
		case tImageASTC::ColourProfile::LDR:		savename += "l";	break;	// RGB in sRGB space. Linear alpha.
		case tImageASTC::ColourProfile::LDR_FULL:	savename += "L";	break;	// RGBA all linear.
		case tImageASTC::ColourProfile::HDR:		savename += "h";	break;	// RGB in linear HDR space. Linear LDR alpha.
		case tImageASTC::ColourProfile::HDR_FULL:	savename += "H";	break;	// RGBA all in linear HDR.
	}

	tPrintf("ASTC Load %s\n", savename.Chr());
	tString formatname = basename.Left('_');

	tImageASTC astc(astcfile, params);
	tRequire(astc.IsValid());
	tPixelFormat fileformat = tGetPixelFormat(formatname.Chr());
	tPixelFormat astcformat = astc.GetPixelFormat();
	tPixelFormat astcformatsrc = astc.GetPixelFormatSrc();
	tRequire(fileformat == astcformatsrc);
	if (loadFlags & tImageASTC::LoadFlag_Decode)
		tRequire(astcformat == tPixelFormat::R8G8B8A8);
	else
		tRequire(astcformat == fileformat);

	tLayer* layer = astc.StealLayer();
	tAssert(layer->OwnsData);
	if (astcformat == tPixelFormat::R8G8B8A8)
	{
		tImageTGA tga((tPixel*)layer->Data, layer->Width, layer->Height);
		tga.Save("Written_" + savename + ".tga");
	}
	else
	{
		tPrintf("No decode, no tga save. Pixel format not R8G8B8A8\n");
	}
	delete layer;
	tPrintf("\n");
}


tTestUnit(ImageASTC)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageASTC)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/ASTC/");

	tPrintf("Testing ASTC Loading/Decoding using astcenc V %s\n\n", tImage::Version_ASTCEncoder);
	tPrintf("D = Decode\n");
	tPrintf("G = Explicit Gamma or sRGB Compression.\n");
	tPrintf("l = LDR Profile.      RGB in sRGB space. Linear alpha. All in [0,1]\n");
	tPrintf("L = LDR FULL Profile. RGBA all linear. All in [0, 1]\n");
	tPrintf("h = HDR Profile.      RGB linear space in [0, inf]. LDR [0, 1] A in linear space.\n");
	tPrintf("H = HDR FULL Profile. RGBA linear space in [0, inf].\n");

	//
	// LDR.
	//
	tImageASTC::LoadParams ldrParams;
	ldrParams.Profile = tImageASTC::ColourProfile::LDR;
	ldrParams.Flags = tImageASTC::LoadFlag_Decode | tImageASTC::LoadFlag_ReverseRowOrder;
	ASTCLoadDecodeSave("ASTC4x4_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC5x4_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC5x5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC6x5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC6x6_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC8x5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC8x6_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC8x8_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10x5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10x6_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10x8_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10x10_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC12x10_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC12x12_LDR.astc", ldrParams);

	//
	// LDR.
	//
	tImageASTC::LoadParams hdrParams;
	hdrParams.Profile = tImageASTC::ColourProfile::HDR;
	hdrParams.Flags = tImageASTC::LoadFlag_Decode | tImageASTC::LoadFlag_SRGBCompression | tImageASTC::LoadFlag_ReverseRowOrder;
	ASTCLoadDecodeSave("ASTC4x4_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC5x4_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC5x5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC6x5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC6x6_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC8x5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC8x6_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC8x8_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10x5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10x6_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10x8_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10x10_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC12x10_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC12x12_HDR.astc", hdrParams);

	// Do this all over again, but without decoding and tRequire the pixel-format to be as expected.
	tPrintf("Testing ASTC Loading/No-decoding.\n\n");
	ldrParams.Flags = 0;
	ASTCLoadDecodeSave("ASTC4x4_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC5x4_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC5x5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC6x5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC6x6_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC8x5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC8x6_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC8x8_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10x5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10x6_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10x8_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10x10_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC12x10_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC12x12_LDR.astc", ldrParams);

	hdrParams.Flags = 0;
	ASTCLoadDecodeSave("ASTC4x4_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC5x4_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC5x5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC6x5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC6x6_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC8x5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC8x6_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC8x8_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10x5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10x6_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10x8_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10x10_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC12x10_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC12x12_HDR.astc", hdrParams);

	tSystem::tSetCurrentDir(origDir.Chr());
}


}
