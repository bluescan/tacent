// TestImage.cpp
//
// Image module tests.
//
// Copyright (c) 2017, 2019-2023 Tristan Grimmer.
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
#include <Image/tImagePVR.h>
#include <Image/tImageASTC.h>
#include <Image/tImageEXR.h>
#include <Image/tImageGIF.h>
#include <Image/tImageHDR.h>
#include <Image/tImageICO.h>
#include <Image/tImageJPG.h>
#include <Image/tImagePKM.h>
#include <Image/tImagePNG.h>
#include <Image/tImageQOI.h>
#include <Image/tImageAPNG.h>
#include <Image/tImageTGA.h>
#include <Image/tImageWEBP.h>
#include <Image/tImageXPM.h>
#include <Image/tImageBMP.h>
#include <Image/tImageTIFF.h>
#include <Image/tImagePVR.h>
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

	tImageASTC imgASTC("TestData/Images/ASTC/ASTC10X10_LDR.astc");
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

	tImagePVR imgPVR("TestData/Images/PVR_V3/PVRBPP4_UNORM_SRGB_RGBA_T.png");
	tRequire(!imgPVR.IsValid());

	tImageQOI imgQOI24("TestData/Images/TacentTestPattern24.qoi");
	tRequire(imgQOI24.IsValid());

	tImageQOI imgQOI32("TestData/Images/TacentTestPattern32.qoi");
	tRequire(imgQOI32.IsValid());

	// Test loading a corrupt tga.
	tImageTGA imgTGACorrupt("TestData/Images/Corrupt.tga");
	tRequire(!imgTGACorrupt.IsValid());

	tImageTGA imgTGA("TestData/Images/WhiteBorderRLE.tga");
	tRequire(imgTGA.IsValid());

	tImageTIFF imgTIFF("TestData/Images/Tiff_NoComp.tif");
	tRequire(imgTIFF.IsValid());

	tImageWEBP imgWEBP("TestData/Images/RockyBeach.webp");
	tRequire(imgWEBP.IsValid());
}


void TestSaveGif(const tString& pngFile, tPixelFormat format, tQuantize::Method method, bool transparency, float dither = 0.0f)
{
	tImageAPNG apng(pngFile);
	tList<tFrame> frames;
	apng.StealFrames(frames);
	int numFrames = frames.Count();
	tImageGIF gif;
	gif.Set(frames, true);
	tRequire(frames.IsEmpty());

	tString dithStr;
	if (method == tQuantize::Method::Spatial)
	{
		dithStr = "_Dith_Auto";
		if (dither > 0.0f)
			tsPrintf(dithStr, "_Dith_%3.2f", dither);
	}

	tString gifFile;
	tsPrintf
	(
		gifFile,
		"WrittenGIF_%s_%s_%s_%s%s.gif",
		(numFrames > 1) ? "Animat" : "Single",
		transparency ? "Transp" : "Opaque",
		tGetPixelFormatName(format),
		tQuantize::GetMethodName(method),
		dithStr.Chr()
	);
	tImageGIF::SaveParams params;
	params.Format = format;
	params.Method = method;
	params.DitherLevel = double(dither);
	params.AlphaThreshold = transparency ? 127 : 255;		// 255 means force opaque.

	gif.Save(gifFile, params);
	tRequire(tSystem::tFileExists(gifFile));
}


tTestUnit(ImageSave)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageSave)

	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/");

	tList<tFrame> frames;

	// Test dither from 0.0f (auto) to 1.5f.
	tPrintf("Testing GIF save spatial quantization dither with 2-colour palette.\n"); 
	for (int d = 0; d < 16; d++)
		TestSaveGif("TacentTestPattern.png",tPixelFormat::PAL1BIT, tQuantize::Method::Spatial,	false, float(d)*0.1f);

	// Test writing a non-animated gif without transparency at different bit-depths.
	tPrintf("Testing GIF save single frame opaque.\n");
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL1BIT, tQuantize::Method::Fixed,	false);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL2BIT, tQuantize::Method::Spatial,	false);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL3BIT, tQuantize::Method::Spatial,	false);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL4BIT, tQuantize::Method::Spatial,	false);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL5BIT, tQuantize::Method::Wu,		false);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL6BIT, tQuantize::Method::Wu,		false);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL7BIT, tQuantize::Method::Wu,		false);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL8BIT, tQuantize::Method::Wu,		false);

	// Test writing a non-animated gif with transparency at different bit-depths.
	tPrintf("Testing GIF save single frame transparent.\n");
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL1BIT, tQuantize::Method::Fixed,	true);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL2BIT, tQuantize::Method::Wu,		true);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL3BIT, tQuantize::Method::Wu,		true);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL4BIT, tQuantize::Method::Wu,		true);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL5BIT, tQuantize::Method::Wu,		true);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL6BIT, tQuantize::Method::Neu,		true);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL7BIT, tQuantize::Method::Neu,		true);
	TestSaveGif("TacentTestPattern.png",	tPixelFormat::PAL8BIT, tQuantize::Method::Neu,		true);

	// Test writing animated gif with transparency at different bit-depths.
	tPrintf("Testing GIF save animated transparent.\n");
	TestSaveGif("Icos4D.apng",				tPixelFormat::PAL1BIT, tQuantize::Method::Fixed,	true);
	TestSaveGif("Icos4D.apng",				tPixelFormat::PAL2BIT, tQuantize::Method::Neu,		true);
	TestSaveGif("Icos4D.apng",				tPixelFormat::PAL3BIT, tQuantize::Method::Neu,		true);
	TestSaveGif("Icos4D.apng",				tPixelFormat::PAL4BIT, tQuantize::Method::Neu,		true);
	TestSaveGif("Icos4D.apng",				tPixelFormat::PAL5BIT, tQuantize::Method::Neu,		true);
	TestSaveGif("Icos4D.apng",				tPixelFormat::PAL6BIT, tQuantize::Method::Wu,		true);
	TestSaveGif("Icos4D.apng",				tPixelFormat::PAL7BIT, tQuantize::Method::Wu,		true);
	TestSaveGif("Icos4D.apng",				tPixelFormat::PAL8BIT, tQuantize::Method::Wu,		true);

	tImageTGA tga("TacentTestPattern32.tga");
	int tgaW = tga.GetWidth();
	int tgaH = tga.GetHeight();
	tPixel* tgaPixels = tga.StealPixels();
	tImageQOI qoi(tgaPixels, tgaW, tgaH, true);
	tImageQOI::tFormat result32 = qoi.Save("WrittenTacentTestPattern32.qoi", tImageQOI::tFormat::BPP32);
	tRequire(result32 == tImageQOI::tFormat::BPP32);
	tImageQOI::tFormat result24 = qoi.Save("WrittenTacentTestPattern24.qoi", tImageQOI::tFormat::BPP24);
	tRequire(result24 == tImageQOI::tFormat::BPP24);

	tImagePNG pngA("Xeyes.png");
	pngA.Save("WrittenNewA.png");
	tRequire( tSystem::tFileExists("WrittenNewA.png"));

	tImagePNG pngB("TextCursor.png");
	pngB.Save("WrittenNewB.png");
	tRequire( tSystem::tFileExists("WrittenNewB.png"));

	// Test writing webp images. The basic pattern to save as a different type is to steal from one and give to the other.
	tImageAPNG apng("Flame.apng");
	apng.StealFrames(frames);
	tImageWEBP webp;
	webp.Set(frames, true);
	webp.Save("WrittenFlameOneFrame.webp");
	tRequire(frames.IsEmpty());
	tRequire(tSystem::tFileExists("WrittenFlameOneFrame.webp"));

	tImageEXR exr("Desk.exr");
	exr.StealFrames(frames);
	webp.Set(frames, true);
	webp.Save("WrittenDesk.webp");
	tRequire(frames.IsEmpty());
	tRequire(tSystem::tFileExists("WrittenDesk.webp"));

	tSystem::tSetCurrentDir(origDir);
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

	//
	// tPicture loading/saving tests. These all save as tga and to the corresponding format if save is supported.
	//
	tPicture pic;
	tImageTGA tga;

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

	tImageAPNG apng;
	apng.Load("Flame.apng");
	apng.Save("WrittenFlame.apng");
	pic.Set(apng); tga.Set(pic);
	tga.Save("WrittenFlame.tga");
	tRequire( tSystem::tFileExists("WrittenFlame.apng"));

	tImageASTC astc;
	astc.Load("ASTC/ASTC10X10_LDR.astc");
	pic.Set(astc); tga.Set(pic);
	tga.Save("WrittenASTC10X10_LDR.tga");
	tRequire(tSystem::tFileExists("WrittenASTC10X10_LDR.tga"));

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


tTestUnit(ImageQuantize)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageQuantize)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/");

	tImageTGA srctga; srctga.Load("Dock512.tga");
	int w = srctga.GetWidth(); int h = srctga.GetHeight(); tPixel* srcpixels = srctga.GetPixels();

	tColour3i* palette = new tColour3i[256];
	uint8* indices = new uint8[w*h];

	// The full range of palette sizes is [2, 256]. This takes a _long_ time to compute, especially for spatialized
	// quantization. For testing purposes we only do 3 sizes: 15, 16, and 17.
	// int minPalSize = 2;
	// int maxPalSize = 256;
	int minPalSize = 15;
	int maxPalSize = 17;

	// Fixed Quantization
	for (int palSize = minPalSize; palSize <= maxPalSize; palSize++)
	{
		// Quantize. Do not look for exact match.
		tQuantizeFixed::QuantizeImage(palSize, w, h, srcpixels, palette, indices, false);

		// Get the quantization back into a pixel array.
		tPixel* dstpixels = new tPixel[w*h];
		tQuantize::ConvertToPixels(dstpixels, w, h, palette, indices);

		// Give the pixels to the dsttga object. The true does this.
		tImageTGA dsttga; dsttga.Set(dstpixels, w, h, true);

		// Save the quantized tga out.
		tString filename; tsPrintf(filename, "Written_QuantizedFixed_%03dColours.tga", palSize);
		dsttga.Save(filename); tRequire(tSystem::tFileExists(filename));
	}

	// Spatial Quantization
	for (int palSize = minPalSize; palSize <= maxPalSize; palSize++)
	{
		tQuantizeSpatial::QuantizeImage(palSize, w, h, srcpixels, palette, indices, false);
		tPixel* dstpixels = new tPixel[w*h];
		tQuantize::ConvertToPixels(dstpixels, w, h, palette, indices);
		tImageTGA dsttga; dsttga.Set(dstpixels, w, h, true);
		tString filename; tsPrintf(filename, "Written_QuantizedSpatial_%03dColours.tga", palSize);
		dsttga.Save(filename); tRequire(tSystem::tFileExists(filename));
	}

	// Neu Quantization
	for (int palSize = minPalSize; palSize <= maxPalSize; palSize++)
	{
		tQuantizeNeu::QuantizeImage(palSize, w, h, srcpixels, palette, indices, false);
		tPixel* dstpixels = new tPixel[w*h];
		tQuantize::ConvertToPixels(dstpixels, w, h, palette, indices);
		tImageTGA dsttga; dsttga.Set(dstpixels, w, h, true);
		tString filename; tsPrintf(filename, "Written_QuantizedNeu_%03dColours.tga", palSize);
		dsttga.Save(filename); tRequire(tSystem::tFileExists(filename));
	}

	// Wu Quantization
	for (int palSize = minPalSize; palSize <= maxPalSize; palSize++)
	{
		tQuantizeWu::QuantizeImage(palSize, w, h, srcpixels, palette, indices, false);
		tPixel* dstpixels = new tPixel[w*h];
		tQuantize::ConvertToPixels(dstpixels, w, h, palette, indices);
		tImageTGA dsttga; dsttga.Set(dstpixels, w, h, true);
		tString filename; tsPrintf(filename, "Written_QuantizedWu_%03dColours.tga", palSize);
		dsttga.Save(filename); tRequire(tSystem::tFileExists(filename));
	}

	delete[] indices;
	delete[] palette;
	tSystem::tSetCurrentDir(origDir);
}


void PalettizeImage(int w, int h, tPixel* pixels, tPixelFormat fmt, tImage::tQuantize::Method method)
{
	bool ok;
	tPaletteImage pal;
	ok = pal.Set(fmt, w, h, pixels, method);			// Create a palettized image with a specific-sized palette.
	tRequire(ok); if (!ok) return;

	tPixel* palpix = new tPixel[w*h];
	ok = pal.Get(palpix);								// Depalettize into a pixel buffer.
	tRequire(ok); if (!ok) return;

	tImageTGA dstimg;
	dstimg.Set(palpix, w, h, true);						// Give the pixels to the tga.

	tString saveName;
	tsPrintf(saveName, "Written_%s_%s.tga", tGetPixelFormatName(fmt), tQuantize::GetMethodName(method));
	dstimg.Save(saveName);								// And save it out.
	tRequire(tSystem::tFileExists(saveName));
}


tTestUnit(ImagePalette)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImagePalette)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/");

	tImageTGA tga; int w = 0; int h = 0; tPixel* pixels = nullptr;
	
	// We'll start by loading a test image with only 4 colours and text quantization.
	// It should always find an exact match for 2-bit palette formats and higher.
	tga.Load("Dock640_4ColoursOnly.tga");
	w = tga.GetWidth(); h = tga.GetHeight(); pixels = tga.GetPixels();
	PalettizeImage(w, h, pixels, tPixelFormat::PAL8BIT, tImage::tQuantize::Method::Fixed);

	// And now load the image with full colours for the remainder of the tests.
	tga.Load("Dock512.tga");
	w = tga.GetWidth(); h = tga.GetHeight(); pixels = tga.GetPixels();

	//
	// Fixed quantization.
	//
	PalettizeImage(w, h, pixels, tPixelFormat::PAL8BIT, tImage::tQuantize::Method::Fixed);

	//
	// Spatial quantization (scolorq).
	//
	PalettizeImage(w, h, pixels, tPixelFormat::PAL1BIT, tImage::tQuantize::Method::Spatial);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL2BIT, tImage::tQuantize::Method::Spatial);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL3BIT, tImage::tQuantize::Method::Spatial);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL4BIT, tImage::tQuantize::Method::Spatial);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL5BIT, tImage::tQuantize::Method::Spatial);

	//
	// NeuQuant quantization.
	//
	PalettizeImage(w, h, pixels, tPixelFormat::PAL1BIT, tImage::tQuantize::Method::Neu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL2BIT, tImage::tQuantize::Method::Neu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL3BIT, tImage::tQuantize::Method::Neu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL4BIT, tImage::tQuantize::Method::Neu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL5BIT, tImage::tQuantize::Method::Neu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL6BIT, tImage::tQuantize::Method::Neu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL7BIT, tImage::tQuantize::Method::Neu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL8BIT, tImage::tQuantize::Method::Neu);

	//
	// Wu quantization.
	//
	PalettizeImage(w, h, pixels, tPixelFormat::PAL1BIT, tImage::tQuantize::Method::Wu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL2BIT, tImage::tQuantize::Method::Wu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL3BIT, tImage::tQuantize::Method::Wu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL4BIT, tImage::tQuantize::Method::Wu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL5BIT, tImage::tQuantize::Method::Wu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL6BIT, tImage::tQuantize::Method::Wu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL7BIT, tImage::tQuantize::Method::Wu);
	PalettizeImage(w, h, pixels, tPixelFormat::PAL8BIT, tImage::tQuantize::Method::Wu);

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

	// Test saving meta-data to a file and then reloading.
	tChunkWriter writer("TestData/Images/EXIF_XMP/WrittenMetaData.bin");
	metaData.Save(writer);

	tMetaData metaDataLoaded;
	tChunkReader reader("TestData/Images/EXIF_XMP/WrittenMetaData.bin");
	metaDataLoaded.Load(reader.Chunk());
	tRequire(metaDataLoaded == metaData);

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


tTestUnit(ImageLosslessTransform)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageRotation)

	tImageJPG::Transform trans;
	tImageJPG::LoadParams params;
	params.Flags = tImageJPG::LoadFlag_NoDecompress;
	bool ok = false;

	trans = tImageJPG::Transform::Rotate90ACW;
	tImageJPG imgJPG_ACW("TestData/Images/TacentTestPattern.jpg", params);
	tRequire(imgJPG_ACW.IsValid());
	tRequire(imgJPG_ACW.CanDoPerfectLosslessTransform(trans));
	imgJPG_ACW.LosslessTransform(trans);
	imgJPG_ACW.Save("TestData/Images/WrittenLosslessACW90.jpg");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenLosslessACW90.jpg"));

	trans = tImageJPG::Transform::Rotate90CW;
	tImageJPG imgJPG_CW("TestData/Images/TacentTestPattern.jpg", params);
	tRequire(imgJPG_CW.IsValid());
	tRequire(imgJPG_CW.CanDoPerfectLosslessTransform(trans));
	imgJPG_CW.LosslessTransform(trans);
	imgJPG_CW.Save("TestData/Images/WrittenLosslessCW90.jpg");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenLosslessCW90.jpg"));

	trans = tImageJPG::Transform::FlipH;
	tImageJPG imgJPG_FH("TestData/Images/TacentTestPattern.jpg", params);
	tRequire(imgJPG_FH.IsValid());
	tRequire(imgJPG_FH.CanDoPerfectLosslessTransform(trans));
	imgJPG_FH.LosslessTransform(trans);
	imgJPG_FH.Save("TestData/Images/WrittenLosslessFlipH.jpg");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenLosslessFlipH.jpg"));

	trans = tImageJPG::Transform::FlipH;
	tImageJPG imgJPG_FV("TestData/Images/TacentTestPattern.jpg", params);
	tRequire(imgJPG_FV.IsValid());
	tRequire(imgJPG_FV.CanDoPerfectLosslessTransform(trans));
	imgJPG_FV.LosslessTransform(trans);
	imgJPG_FV.Save("TestData/Images/WrittenLosslessFlipV.jpg");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenLosslessFlipV.jpg"));

	// Test an image that can be perfectly ACW rotated or horizontally flipped, but cannot be
	// CW rotated or vertically flipped perfectly.
	trans = tImageJPG::Transform::Rotate90ACW;
	tImageJPG imgJPG_OK_ACW("TestData/Images/EXIF_XMP/Bebop_2.jpg", params);
	tRequire(imgJPG_OK_ACW.IsValid());
	tRequire(imgJPG_OK_ACW.CanDoPerfectLosslessTransform(trans));
	ok = imgJPG_OK_ACW.LosslessTransform(trans);
	tRequire(ok);
	imgJPG_OK_ACW.Save("TestData/Images/WrittenLosslessOK_ACW.jpg");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenLosslessOK_ACW.jpg"));

	trans = tImageJPG::Transform::Rotate90CW;
	tImageJPG imgJPG_NO_CW("TestData/Images/EXIF_XMP/Bebop_2.jpg", params);
	tRequire(imgJPG_NO_CW.IsValid());
	tRequire(!imgJPG_NO_CW.CanDoPerfectLosslessTransform(trans));
	ok = imgJPG_NO_CW.LosslessTransform(trans);
	tRequire(ok);
	imgJPG_NO_CW.Save("TestData/Images/WrittenLosslessNO_CW.jpg");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenLosslessNO_CW.jpg"));

	trans = tImageJPG::Transform::FlipH;
	tImageJPG imgJPG_OK_FH("TestData/Images/EXIF_XMP/Bebop_2.jpg", params);
	tRequire(imgJPG_OK_FH.IsValid());
	tRequire(imgJPG_OK_FH.CanDoPerfectLosslessTransform(trans));
	ok = imgJPG_OK_FH.LosslessTransform(trans);
	tRequire(ok);
	imgJPG_OK_FH.Save("TestData/Images/WrittenLosslessOK_FH.jpg");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenLosslessOK_FH.jpg"));

	trans = tImageJPG::Transform::FlipV;
	tImageJPG imgJPG_NO_FV("TestData/Images/EXIF_XMP/Bebop_2.jpg", params);
	tRequire(imgJPG_NO_FV.IsValid());
	tRequire(!imgJPG_NO_FV.CanDoPerfectLosslessTransform(trans));
	ok = imgJPG_NO_FV.LosslessTransform(trans);
	tRequire(ok);
	imgJPG_NO_FV.Save("TestData/Images/WrittenLosslessNO_FV.jpg");
	tRequire( tSystem::tFileExists("TestData/Images/WrittenLosslessNO_FV.jpg"));
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
	planePic.Deborder(tColouri::black, tCompBit_RGB);
	planePic.Crop(w, h, tPicture::Anchor::MiddleMiddle, tColouri::transparent);
	png.Set(planePic);
	tImagePNG::tFormat fmt = png.Save("TestData/Images/WrittenPlane.png");
	tRequire(fmt != tImagePNG::tFormat::Invalid);
}


tTestUnit(ImageAdjustment)
{
	if (!tSystem::tDirExists("TestData/Images/"))
		tSkipUnit(ImageAdjustment)

	// Test brightness.
	for (int level = 0; level <= 10; level++)
	{
		tImagePNG png("TestData/Images/TacentTestPattern.png");
		tPicture pic(png);

		float brightness = float(level)/10.0f;
		pic.AdjustmentBegin();
		pic.AdjustBrightness(brightness);
		pic.AdjustmentEnd();

		// Save it out.
		tString file;
		tsPrintf(file, "TestData/Images/WrittenBright%3.1f.png", brightness);
		png.Set(pic, false);
		tImagePNG::tFormat fmt = png.Save(file.Chr());
		tRequire(fmt != tImagePNG::tFormat::Invalid);
	}

	// Test contrast.
	for (int level = 0; level <= 10; level++)
	{
		tImagePNG png("TestData/Images/TacentTestPattern.png");
		tPicture pic(png);

		float contrast = float(level)/10.0f;
		pic.AdjustmentBegin();
		pic.AdjustContrast(contrast);
		pic.AdjustmentEnd();

		// Save it out.
		tString file;
		tsPrintf(file, "TestData/Images/WrittenContrast%3.1f.png", contrast);
		png.Set(pic, false);
		tImagePNG::tFormat fmt = png.Save(file.Chr());
		tRequire(fmt != tImagePNG::tFormat::Invalid);
	}

	// Test levels.
	for (int level = 0; level <= 10; level++)
	{
		tImagePNG png("TestData/Images/TacentTestPattern.png");
		tPicture pic(png);
		float blackPoint = 0.0f;
		float midPoint = float(level)/10.0f;
		float whitePoint = 1.0f;
		pic.AdjustmentBegin();
		pic.AdjustLevels(blackPoint, midPoint, whitePoint, 0.0f, 1.0f, true);
		pic.AdjustmentEnd();

		// Save it out.
		tString file;
		tsPrintf(file, "TestData/Images/WrittenLevels_MidAdjPower_BP%3.1f_MP%3.1f_WP%3.1f.png", blackPoint, midPoint, whitePoint);
		png.Set(pic, false);
		tImagePNG::tFormat fmt = png.Save(file.Chr());
		tRequire(fmt != tImagePNG::tFormat::Invalid);
	}

	for (int level = 0; level <= 10; level++)
	{
		tImagePNG png("TestData/Images/TacentTestPattern.png");
		tPicture pic(png);
		float blackPoint = 0.0f;
		float midPoint = float(level)/10.0f;
		float whitePoint = 1.0f;
		pic.AdjustmentBegin();
		pic.AdjustLevels(blackPoint, midPoint, whitePoint, 0.0f, 1.0f, false);
		pic.AdjustmentEnd();

		// Save it out.
		tString file;
		tsPrintf(file, "TestData/Images/WrittenLevels_MidAdjPhoto_BP%3.1f_MP%3.1f_WP%3.1f.png", blackPoint, midPoint, whitePoint);
		png.Set(pic, false);
		tImagePNG::tFormat fmt = png.Save(file.Chr());
		tRequire(fmt != tImagePNG::tFormat::Invalid);
	}

	for (int level = 0; level <= 10; level++)
	{
		tImagePNG png("TestData/Images/TacentTestPattern.png");
		tPicture pic(png);
		float blackPoint = float(level)/10.0f;
		float whitePoint = 1.0f;
		float midPoint = (blackPoint+whitePoint)/2.0f;
		pic.AdjustmentBegin();
		pic.AdjustLevels(blackPoint, midPoint, whitePoint, 0.0f, 1.0f, true);
		pic.AdjustmentEnd();

		// Save it out.
		tString file;
		tsPrintf(file, "TestData/Images/WrittenLevels_BlkAdjPower_BP%3.1f_MP%3.1f_WP%3.1f.png", blackPoint, midPoint, whitePoint);
		png.Set(pic, false);
		tImagePNG::tFormat fmt = png.Save(file.Chr());
		tRequire(fmt != tImagePNG::tFormat::Invalid);
	}
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

	#if 0
	tImageWEBP webpSrc0("TestData/Images/Demux_Shy.webp");
	tImageTIFF tiffDst0(webpSrc0.Frames, true);
	tiffDst0.Save("TestData/Images/Demux_Shy.tiff");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenDemux_Shy.tiff"));

	tImageWEBP webpSrc1("TestData/Images/Demux_Confused.webp");
	tImageTIFF tiffDst1(webpSrc1.Frames, true);
	tiffDst1.Save("TestData/Images/Demux_Confused.tiff");
	tRequire(tSystem::tFileExists("TestData/Images/WrittenDemux_Confused.tiff"));
	return;
	#endif

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
	blackToWhite.Save("TestData/Images/Written_Gradient_BlackToWhite.tga", tImageTGA::tFormat::BPP24, tImageTGA::tCompression::RLE);

	// Gradient black to transparent.
	pixels = new tPixel[width*height];
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			pixels[y*width + x] = tColour(0, 0, 0, 255 - 256*x / width);
	tImageTGA blackToTrans(pixels, width, height, true);
	tRequire(blackToTrans.IsValid());
	blackToTrans.Save("TestData/Images/Written_Gradient_BlackToTrans.tga", tImageTGA::tFormat::BPP32, tImageTGA::tCompression::RLE);

	// Gradient transparent to white.
	pixels = new tPixel[width*height];
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			pixels[y*width + x] = tColour(255, 255, 255, 256*x / width);
	tImageTGA transToWhite(pixels, width, height, true);
	tRequire(transToWhite.IsValid());
	transToWhite.Save("TestData/Images/Written_Gradient_TransToWhite.tga", tImageTGA::tFormat::BPP32, tImageTGA::tCompression::RLE);

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
	redToRed.Save("TestData/Images/Written_Gradient_RedToRed.tga", tImageTGA::tFormat::BPP24, tImageTGA::tCompression::RLE);	
}


// Helper for tImageDDS unit tests.
void DDSLoadDecodeSave(const tString& ddsfile, uint32 loadFlags = 0, bool saveAllMips = false)
{
	tString basename = tSystem::tGetFileBaseName(ddsfile);
	tString savename = basename + "_";
	savename += (loadFlags & tImageDDS::LoadFlag_Decode)			? "D" : "x";
	if ((loadFlags & tImageDDS::LoadFlag_GammaCompression) || (loadFlags & tImageDDS::LoadFlag_SRGBCompression))
		savename += "G";
	else if (loadFlags & tImageDDS::LoadFlag_AutoGamma)
		savename += "g";
	else
		savename += "x";
	savename += (loadFlags & tImageDDS::LoadFlag_ReverseRowOrder)	? "R" : "x";
	savename += (loadFlags & tImageDDS::LoadFlag_SpreadLuminance)	? "S" : "x";
	tPrintf("DDS Load %s\n", savename.Chr());
	tString formatname = basename.Left('_');
	tString ddsTypeString = basename.Right('_');
	bool fileSpecifiedAsModern = ddsTypeString.IsEqualCI("Modern");

	tImageDDS::LoadParams params;
	params.Flags = loadFlags;

	// The ETC files exported from compressonator need their RGB components swizzled.
	tPixelFormat fileformat = tGetPixelFormat(formatname.Chr());
	tImageDDS dds(ddsfile, params);
	tRequire(dds.IsValid());
	tRequire(fileSpecifiedAsModern == dds.IsModern());
	tPixelFormat ddsformat = dds.GetPixelFormat();
	tPixelFormat ddsformatsrc = dds.GetPixelFormatSrc();
	tRequire(fileformat == ddsformatsrc);
	if (loadFlags & tImageDDS::LoadFlag_Decode)
		tRequire(ddsformat == tPixelFormat::R8G8B8A8);
	else
		tRequire(ddsformat == fileformat);

	// If we asked to flip rows but it couldn't, print a message. The conditional is
	// only set if we requested and it couldn't be done.
	if (dds.IsStateSet(tImageDDS::StateBit::Conditional_CouldNotFlipRows))
		tPrintf("Could not flip rows for %s\n", savename.Chr());

	tList<tImage::tLayer> layers;
	dds.StealLayers(layers);

	tRequire(!(loadFlags & tImageDDS::LoadFlag_Decode) || (ddsformat == tPixelFormat::R8G8B8A8));
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
	uint32 autoga = tImageDDS::LoadFlag_AutoGamma;

	tPrintf("Testing DDS Loading/Decoding. Legacy = No DX10 Header.\n\n");
	tPrintf("D = Decode\n");
	tPrintf("G = Explicit Gamma or sRGB Compression. g = auto\n");
	tPrintf("R = Reverse Row Order\n");
	tPrintf("S = Spread Luminance\n");

	//
	// Block Compressed Formats.
	//
	// BC1
	DDSLoadDecodeSave("BC1DXT1_RGB_Legacy.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("BC1DXT1_RGB_Modern.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("BC1DXT1_RGB_Modern_317x177.dds",	decode | revrow | autoga);
	DDSLoadDecodeSave("BC1DXT1_RGB_Modern_318x178.dds",	decode | revrow | autoga);
	DDSLoadDecodeSave("BC1DXT1_RGB_Modern_319x179.dds",	decode | revrow | autoga);
	DDSLoadDecodeSave("BC1DXT1_RGB_Modern_320x180.dds",	decode | revrow | autoga);

	// BC1A
	DDSLoadDecodeSave("BC1DXT1A_RGBA_Legacy.dds",		decode | revrow | autoga);
	DDSLoadDecodeSave("BC1DXT1A_RGBA_Modern.dds",		decode | revrow | autoga);

	// BC2
	DDSLoadDecodeSave("BC2DXT2DXT3_RGBA_Legacy.dds",	decode | revrow | autoga);
	DDSLoadDecodeSave("BC2DXT2DXT3_RGBA_Modern.dds",	decode | revrow | autoga);

	// BC3
	DDSLoadDecodeSave("BC3DXT4DXT5_RGBA_Legacy.dds",	decode | revrow | autoga);
	DDSLoadDecodeSave("BC3DXT4DXT5_RGBA_Modern.dds",	decode | revrow | autoga);

	// BC4
	DDSLoadDecodeSave("BC4ATI1_R_Modern.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("BC4ATI1_R_Modern.dds",			decode | revrow | autoga | spread);

	// BC5
	DDSLoadDecodeSave("BC5ATI2_RG_Modern.dds",			decode | revrow | autoga);

	// BC6
	DDSLoadDecodeSave("BC6U_RGB_Modern.dds",			decode | revrow);
	DDSLoadDecodeSave("BC6S_RGB_Modern.dds",			decode | revrow);
	DDSLoadDecodeSave("BC6U_HDRRGB_Modern.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("BC6S_HDRRGB_Modern.dds",			decode | revrow | autoga);

	// BC7
	DDSLoadDecodeSave("BC7_RGBA_Modern.dds",			decode | revrow | autoga, true);

	//
	// ETC
	//
	DDSLoadDecodeSave("ETC1_RGB_Legacy.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("ETC2RGB_RGB_Legacy.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("ETC2RGBA_RGBA_Legacy.dds",		decode | revrow | autoga);
	DDSLoadDecodeSave("ETC2RGBA1_RGBA_Legacy.dds",		decode | revrow | autoga);

	//
	// ASTC
	//
	DDSLoadDecodeSave("ASTC4X4_RGB_Modern.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC5X4_RGB_Modern.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC5X5_RGB_Modern.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC6X5_RGB_Modern.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC6X6_RGB_Modern.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC8X5_RGB_Modern.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC8X6_RGB_Modern.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC8X8_RGB_Modern.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC10X5_RGB_Modern.dds",		decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC10X6_RGB_Modern.dds",		decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC10X8_RGB_Modern.dds",		decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC10X10_RGB_Modern.dds",		decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC12X10_RGB_Modern.dds",		decode | revrow | autoga);
	DDSLoadDecodeSave("ASTC12X12_RGB_Modern.dds",		decode | revrow | autoga);

	//
	// Uncompressed Integer Formats.
	//
	// A8
	DDSLoadDecodeSave("A8_A_Legacy.dds",				decode | revrow | autoga);
	DDSLoadDecodeSave("A8_A_Modern.dds",				decode | revrow);

	// L8
	DDSLoadDecodeSave("L8_L_Legacy.dds",				decode | revrow);
	DDSLoadDecodeSave("L8_L_Legacy.dds",				decode | revrow | spread);
	DDSLoadDecodeSave("R8_L_Modern.dds",				decode | revrow);
	DDSLoadDecodeSave("R8_L_Modern.dds",				decode | revrow | spread);

	// B8G8R8
	DDSLoadDecodeSave("B8G8R8_RGB_Legacy.dds",			decode | revrow | autoga);

	// B8G8R8A8
	DDSLoadDecodeSave("B8G8R8A8_RGBA_Legacy.dds",		decode | revrow | autoga);
	DDSLoadDecodeSave("B8G8R8A8_RGBA_Modern.dds",		decode | revrow | autoga);

	// G3B5R5G3
	DDSLoadDecodeSave("G3B5R5G3_RGB_Legacy.dds",		decode | revrow | autoga);
	DDSLoadDecodeSave("G3B5R5G3_RGB_Modern.dds",		decode | revrow | autoga);

	// G4B4A4R4
	DDSLoadDecodeSave("G4B4A4R4_RGBA_Legacy.dds",		decode | revrow | autoga);
	DDSLoadDecodeSave("G4B4A4R4_RGBA_Modern.dds",		decode | revrow | autoga);

	// G3B5A1R5G2
	DDSLoadDecodeSave("G3B5A1R5G2_RGBA_Legacy.dds",		decode | revrow | autoga);
	DDSLoadDecodeSave("G3B5A1R5G2_RGBA_Modern.dds",		decode | revrow | autoga);

	//
	// Uncompressed Floating-Point (HDR) Formats.
	//
	// R16F
	DDSLoadDecodeSave("R16f_R_Legacy.dds",				decode | revrow | autoga);
	DDSLoadDecodeSave("R16f_R_Modern.dds",				decode | revrow | autoga);
	DDSLoadDecodeSave("R16f_R_Legacy.dds",				decode | revrow | autoga | spread);
	DDSLoadDecodeSave("R16f_R_Modern.dds",				decode | revrow | autoga | spread);

	// R16G16F
	DDSLoadDecodeSave("R16G16f_RG_Legacy.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("R16G16f_RG_Modern.dds",			decode | revrow | autoga);

	// R16G16B16A16F
	DDSLoadDecodeSave("R16G16B16A16f_RGBA_Legacy.dds",	decode | revrow | autoga);
	DDSLoadDecodeSave("R16G16B16A16f_RGBA_Modern.dds",	decode | revrow | autoga);

	// R32F
	DDSLoadDecodeSave("R32f_R_Legacy.dds",				decode | revrow | autoga);
	DDSLoadDecodeSave("R32f_R_Modern.dds",				decode | revrow | autoga);
	DDSLoadDecodeSave("R32f_R_Legacy.dds",				decode | revrow | autoga | spread);
	DDSLoadDecodeSave("R32f_R_Modern.dds",				decode | revrow | autoga | spread);

	// R32G32F
	DDSLoadDecodeSave("R32G32f_RG_Legacy.dds",			decode | revrow | autoga);
	DDSLoadDecodeSave("R32G32f_RG_Modern.dds",			decode | revrow | autoga);

	// R32G32B32A32F
	DDSLoadDecodeSave("R32G32B32A32f_RGBA_Legacy.dds",	decode | revrow | autoga);
	DDSLoadDecodeSave("R32G32B32A32f_RGBA_Modern.dds",	decode | revrow | autoga);

	// Do this all over again, but without decoding and tRequire the pixel-format to be as expected.
	// This time, since not decoding, it may be impossible to reverse the rows, so we can also expect
	// to get conditional valids if it couldn't be done (for some of the BC formats). We're only going
	// to bother with the modern-style dds files (for the most part) this time through.
	tPrintf("Testing DDS Loading/No-decoding.\n\n");

	DDSLoadDecodeSave("BC1DXT1_RGB_Modern.dds",			revrow);	// Revrow should work for BC1.
	DDSLoadDecodeSave("BC1DXT1A_RGBA_Modern.dds");
	DDSLoadDecodeSave("BC2DXT2DXT3_RGBA_Modern.dds",	revrow);
	DDSLoadDecodeSave("BC3DXT4DXT5_RGBA_Modern.dds",	revrow);
	DDSLoadDecodeSave("BC4ATI1_R_Modern.dds",			revrow);	// Should print warning and be unable to flip rows. May be able to implement.
	DDSLoadDecodeSave("BC5ATI2_RG_Modern.dds",			revrow);	// No reverse.
	DDSLoadDecodeSave("BC6U_RGB_Modern.dds");
	DDSLoadDecodeSave("BC6S_RGB_Modern.dds",			revrow);	// No reverse.
	DDSLoadDecodeSave("BC6U_HDRRGB_Modern.dds",			revrow);	// No reverse.
	DDSLoadDecodeSave("BC6S_HDRRGB_Modern.dds");
	DDSLoadDecodeSave("BC7_RGBA_Modern.dds",			revrow);	// No reverse.

	DDSLoadDecodeSave("ASTC4X4_RGB_Modern.dds",			revrow);	// No reverse.
	DDSLoadDecodeSave("ASTC5X4_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC5X5_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC6X5_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC6X6_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC8X5_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC8X6_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC8X8_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC10X5_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC10X6_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC10X8_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC10X10_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC12X10_RGB_Modern.dds");
	DDSLoadDecodeSave("ASTC12X12_RGB_Modern.dds");

	DDSLoadDecodeSave("A8_A_Modern.dds");
	DDSLoadDecodeSave("R8_L_Modern.dds",				revrow);
	DDSLoadDecodeSave("L8_L_Legacy.dds",				revrow);
	DDSLoadDecodeSave("B8G8R8_RGB_Legacy.dds");						// Only legacy supports this format.
	DDSLoadDecodeSave("B8G8R8A8_RGBA_Modern.dds");
	DDSLoadDecodeSave("G3B5R5G3_RGB_Modern.dds",		revrow);
	DDSLoadDecodeSave("G4B4A4R4_RGBA_Modern.dds",		revrow);
	DDSLoadDecodeSave("G3B5A1R5G2_RGBA_Modern.dds");

	DDSLoadDecodeSave("R16f_R_Modern.dds",				revrow);
	DDSLoadDecodeSave("R16f_R_Modern.dds");
	DDSLoadDecodeSave("R16G16f_RG_Modern.dds",			revrow);
	DDSLoadDecodeSave("R16G16B16A16f_RGBA_Modern.dds");

	DDSLoadDecodeSave("R32f_R_Modern.dds",				revrow);
	DDSLoadDecodeSave("R32f_R_Modern.dds");
	DDSLoadDecodeSave("R32G32f_RG_Modern.dds");
	DDSLoadDecodeSave("R32G32B32A32f_RGBA_Modern.dds",	revrow);

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
	tString ext = tSystem::tGetFileExtension(ktxfile); ext.ToUpper();
	tPrintf("%s Load %s\n", ext.Chr(), savename.Chr());
	tString formatname = basename.Left('_');

	tImageKTX::LoadParams params;
	params.Flags = loadFlags;
	tImageKTX ktx(ktxfile, params);
	tRequire(ktx.IsValid());

	if (ktx.IsStateSet(tImageKTX::StateBit::Conditional_ExtVersionMismatch))
		tPrintf("%s Extension %s.\n", tImageKTX::GetStateDesc(tImageKTX::StateBit::Conditional_ExtVersionMismatch), ext.Chr());
	tRequire(!ktx.IsStateSet(tImageKTX::StateBit::Conditional_ExtVersionMismatch));

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
	if (ktx.IsStateSet(tImageKTX::StateBit::Conditional_CouldNotFlipRows))
		tPrintf("Could not flip rows for %s\n", savename.Chr());

	tList<tImage::tLayer> layers;
	ktx.StealLayers(layers);

	tRequire(!(loadFlags & tImageKTX::LoadFlag_Decode) || (ktxformat == tPixelFormat::R8G8B8A8));
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
	KTXLoadDecodeSave("BC1DXT1_RGB.ktx",	decode | revrow);

	// BC1a
	KTXLoadDecodeSave("BC1DXT1A_RGBA.ktx",	decode | revrow);

	// BC2
	KTXLoadDecodeSave("BC2DXT2DXT3_RGBA.ktx",	decode | revrow);

	// BC3
	KTXLoadDecodeSave("BC3DXT4DXT5_RGBA.ktx",	decode | revrow);

	// BC4
	KTXLoadDecodeSave("BC4ATI1_R.ktx",			decode | revrow);

	// BC5
	KTXLoadDecodeSave("BC5ATI2_RG.ktx",			decode | revrow);

	// BC6
	KTXLoadDecodeSave("BC6U_RGB.ktx",			decode | revrow);
	KTXLoadDecodeSave("BC6S_RGB.ktx",			decode | revrow);

	// BC7
	KTXLoadDecodeSave("BC7_RGBA.ktx",			decode | revrow);

	//
	// ETC, ETC2, and EAC.
	//
	KTXLoadDecodeSave("ETC1_RGB.ktx",				decode | revrow);
	KTXLoadDecodeSave("ETC1_RGB_1281x721.ktx",		decode | revrow);
	KTXLoadDecodeSave("ETC1_RGB_1282x722.ktx",		decode | revrow);
	KTXLoadDecodeSave("ETC1_RGB_1283x723.ktx",		decode | revrow);

	KTXLoadDecodeSave("ETC2RGB_RGB.ktx",			decode | revrow);
	KTXLoadDecodeSave("ETC2RGB_RGB_1281x721.ktx",	decode | revrow);
	KTXLoadDecodeSave("ETC2RGB_RGB_1282x722.ktx",	decode | revrow);
	KTXLoadDecodeSave("ETC2RGB_RGB_1283x723.ktx",	decode | revrow);
	KTXLoadDecodeSave("ETC2RGB_sRGB.ktx",			decode | revrow);

	KTXLoadDecodeSave("ETC2RGBA_RGBA.ktx",			decode | revrow);
	KTXLoadDecodeSave("ETC2RGBA_sRGBA.ktx",			decode | revrow);

	KTXLoadDecodeSave("ETC2RGBA1_RGBA.ktx",			decode | revrow);
	KTXLoadDecodeSave("ETC2RGBA1_sRGBA.ktx",		decode | revrow);

	KTXLoadDecodeSave("EACR11U_R.ktx",				decode | revrow);
	KTXLoadDecodeSave("EACR11S_R.ktx",				decode | revrow);
	KTXLoadDecodeSave("EACRG11U_RG.ktx",				decode | revrow);
	KTXLoadDecodeSave("EACRG11S_RG.ktx",			decode | revrow);

	//
	// ASTC
	//
	KTXLoadDecodeSave("ASTC4X4_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC5X4_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC5X5_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC6X5_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC6X6_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC8X5_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC8X6_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC8X8_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC10X5_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC10X6_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC10X8_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC10X10_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC12X10_HDRRGBA.ktx", decode | revrow);
	KTXLoadDecodeSave("ASTC12X12_HDRRGBA.ktx", decode | revrow);

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
	KTXLoadDecodeSave("BC1DXT1A_RGBA.ktx");
	KTXLoadDecodeSave("BC2DXT2DXT3_RGBA.ktx", revrow);
	KTXLoadDecodeSave("BC3DXT4DXT5_RGBA.ktx", revrow);
	KTXLoadDecodeSave("BC4ATI1_R.ktx", revrow);					// Should print warning and be unable to flip rows. May be able to implement.
	KTXLoadDecodeSave("BC5ATI2_RG.ktx", revrow);				// No reverse.
	KTXLoadDecodeSave("BC6U_RGB.ktx", revrow);					// No reverse.
	KTXLoadDecodeSave("BC6S_RGB.ktx", revrow);					// No reverse.
	KTXLoadDecodeSave("BC7_RGBA.ktx", revrow);					// No reverse.
	KTXLoadDecodeSave("ASTC4X4_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC5X4_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC5X5_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC6X5_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC6X6_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC8X5_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC8X6_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC8X8_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10X5_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10X6_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10X8_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10X10_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC12X10_HDRRGBA.ktx", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC12X12_HDRRGBA.ktx", revrow);			// No reverse.
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
	KTXLoadDecodeSave("BC1DXT1_RGB.ktx2",			decode | revrow);
	KTXLoadDecodeSave("BC1DXT1_RGB_317x177.ktx2",	decode | revrow);
	KTXLoadDecodeSave("BC1DXT1_RGB_318x178.ktx2",	decode | revrow);
	KTXLoadDecodeSave("BC1DXT1_RGB_319x179.ktx2",	decode | revrow);
	KTXLoadDecodeSave("BC1DXT1_RGB_320x180.ktx2",	decode | revrow);

	// BC1a
	KTXLoadDecodeSave("BC1DXT1A_RGBA.ktx2", decode | revrow);

	// BC2
	KTXLoadDecodeSave("BC2DXT2DXT3_RGBA.ktx2", decode | revrow);

	// BC3
	KTXLoadDecodeSave("BC3DXT4DXT5_RGBA.ktx2", decode | revrow);

	// BC4
	KTXLoadDecodeSave("BC4ATI1_R.ktx2", decode | revrow);

	// BC5
	KTXLoadDecodeSave("BC5ATI2_RG.ktx2", decode | revrow);

	// BC6
	KTXLoadDecodeSave("BC6S_RGB.ktx2", decode | revrow);

	// BC7
	KTXLoadDecodeSave("BC7_RGBA.ktx2", decode | revrow, true);
	KTXLoadDecodeSave("BC7_RGBANoSuper.ktx2", decode | revrow, true);

	//
	// ETC2
	//
	KTXLoadDecodeSave("ETC2RGB_RGB.ktx2",			decode | revrow);
	KTXLoadDecodeSave("ETC2RGBA_RGBA.ktx2",			decode | revrow);
	KTXLoadDecodeSave("ETC2RGBA1_RGBA.ktx2",		decode | revrow);

	//
	// ASTC
	//
	KTXLoadDecodeSave("ASTC4X4_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC5X4_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC5X5_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC6X5_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC6X6_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC8X5_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC8X6_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC8X8_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10X5_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10X5_RGBA_Mipmaps.ktx2", decode | revrow, true);
	KTXLoadDecodeSave("ASTC10X6_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10X8_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10X10_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC12X10_RGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC12X12_RGBA.ktx2", decode | revrow);

	KTXLoadDecodeSave("ASTC4X4_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC5X4_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC5X5_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC6X5_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC6X6_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC8X5_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC8X6_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC8X8_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10X5_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10X6_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10X8_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC10X10_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC12X10_HDRRGBA.ktx2", decode | revrow);
	KTXLoadDecodeSave("ASTC12X12_HDRRGBA.ktx2", decode | revrow);

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
	KTXLoadDecodeSave("BC1DXT1A_RGBA.ktx2");
	KTXLoadDecodeSave("BC2DXT2DXT3_RGBA.ktx2", revrow);
	KTXLoadDecodeSave("BC3DXT4DXT5_RGBA.ktx2", revrow);
	KTXLoadDecodeSave("BC4ATI1_R.ktx2", revrow);				// Should print warning and be unable to reverse rows. May be able to implement.
	KTXLoadDecodeSave("BC5ATI2_RG.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("BC6S_RGB.ktx2", revrow);					// No reverse.
	KTXLoadDecodeSave("BC7_RGBA.ktx2", revrow);					// No reverse.
	KTXLoadDecodeSave("BC7_RGBANoSuper.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC4X4_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC5X4_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC5X5_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC6X5_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC6X6_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC8X5_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC8X6_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC8X8_RGBA.ktx2", revrow);				// No reverse.
	KTXLoadDecodeSave("ASTC10X5_RGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10X6_RGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10X8_RGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10X10_RGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC12X10_RGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC12X12_RGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC4X4_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC5X4_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC5X5_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC6X5_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC6X6_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC8X5_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC8X6_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC8X8_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10X5_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10X6_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10X8_HDRRGBA.ktx2", revrow);			// No reverse.
	KTXLoadDecodeSave("ASTC10X10_HDRRGBA.ktx2", revrow);		// No reverse.
	KTXLoadDecodeSave("ASTC12X10_HDRRGBA.ktx2", revrow);		// No reverse.
	KTXLoadDecodeSave("ASTC12X12_HDRRGBA.ktx2", revrow);		// No reverse.
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
	if ((loadFlags & tImageASTC::LoadFlag_GammaCompression) || (loadFlags & tImageASTC::LoadFlag_SRGBCompression))
		savename += "G";
	if (loadFlags & tImageASTC::LoadFlag_ReverseRowOrder)
		savename += "R";

	switch (params.Profile)
	{
		case tColourProfile::sRGB:	savename += "l";	break;	// RGB in sRGB or gRGB space. Linear alpha.
		case tColourProfile::gRGB:	savename += "l";	break;	// RGB in sRGB or gRGB space. Linear alpha.
		case tColourProfile::lRGB:	savename += "L";	break;	// RGBA all linear.
		case tColourProfile::HDRa:	savename += "h";	break;	// RGB in linear HDR space. Linear LDR alpha.
		case tColourProfile::HDRA:	savename += "H";	break;	// RGBA all in linear HDR.
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
	tRequire(!(loadFlags & tImageASTC::LoadFlag_Decode) || (astcformat == tPixelFormat::R8G8B8A8));
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
	tPrintf("l = LDR Profile.      RGB in sRGB/gRGB space. Linear alpha. All in [0,1]\n");
	tPrintf("L = LDR FULL Profile. RGBA all linear. All in [0, 1]\n");
	tPrintf("h = HDR Profile.      RGB linear space in [0, inf]. LDR [0, 1] A in linear space.\n");
	tPrintf("H = HDR FULL Profile. RGBA linear space in [0, inf].\n");

	//
	// LDR.
	//
	tImageASTC::LoadParams ldrParams;
	ldrParams.Profile = tColourProfile::sRGB;
	ldrParams.Flags = tImageASTC::LoadFlag_Decode | tImageASTC::LoadFlag_ReverseRowOrder;
	ASTCLoadDecodeSave("ASTC4X4_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC5X4_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC5X5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC6X5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC6X6_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC8X5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC8X6_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC8X8_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10X5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10X6_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10X8_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10X10_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC12X10_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC12X12_LDR.astc", ldrParams);

	//
	// LDR.
	//
	tImageASTC::LoadParams hdrParams;
	hdrParams.Profile = tColourProfile::HDRa;
	hdrParams.Flags = tImageASTC::LoadFlag_Decode | tImageASTC::LoadFlag_SRGBCompression | tImageASTC::LoadFlag_ReverseRowOrder;
	ASTCLoadDecodeSave("ASTC4X4_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC5X4_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC5X5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC6X5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC6X6_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC8X5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC8X6_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC8X8_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10X5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10X6_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10X8_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10X10_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC12X10_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC12X12_HDR.astc", hdrParams);

	// Do this all over again, but without decoding and tRequire the pixel-format to be as expected.
	tPrintf("Testing ASTC Loading/No-decoding.\n\n");
	ldrParams.Flags = 0;
	ASTCLoadDecodeSave("ASTC4X4_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC5X4_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC5X5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC6X5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC6X6_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC8X5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC8X6_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC8X8_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10X5_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10X6_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10X8_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC10X10_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC12X10_LDR.astc", ldrParams);
	ASTCLoadDecodeSave("ASTC12X12_LDR.astc", ldrParams);

	hdrParams.Flags = 0;
	ASTCLoadDecodeSave("ASTC4X4_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC5X4_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC5X5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC6X5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC6X6_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC8X5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC8X6_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC8X8_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10X5_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10X6_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10X8_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC10X10_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC12X10_HDR.astc", hdrParams);
	ASTCLoadDecodeSave("ASTC12X12_HDR.astc", hdrParams);

	tSystem::tSetCurrentDir(origDir.Chr());
}


// Helper for tImagePKM unit tests.
void PKMLoadDecodeSave(const tString& pkmfile, uint32 loadFlags = 0)
{
	// We're just going to turn on auto-gamma-compression for all files.
	loadFlags |= tImagePKM::LoadFlag_AutoGamma;

	tString basename = tSystem::tGetFileBaseName(pkmfile);
	tString savename = basename + "_";
	savename += (loadFlags & tImagePKM::LoadFlag_Decode)			? "D" : "x";
	if ((loadFlags & tImagePKM::LoadFlag_GammaCompression) || (loadFlags & tImagePKM::LoadFlag_SRGBCompression))
		savename += "G";
	else if (loadFlags & tImagePKM::LoadFlag_AutoGamma)
		savename += "g";
	else
		savename += "x";
	savename += (loadFlags & tImagePKM::LoadFlag_ReverseRowOrder)	? "R" : "x";
	savename += (loadFlags & tImagePKM::LoadFlag_SpreadLuminance)	? "S" : "x";
	tPrintf("PKM Load %s\n", savename.Chr());
	tString formatname = basename.Left('_');

	tImagePKM::LoadParams params;
	params.Flags = loadFlags;

	tImagePKM pkm(pkmfile, params);
	tRequire(pkm.IsValid());
	tPixelFormat fileformat = tGetPixelFormat(formatname.Chr());
	tPixelFormat pkmformat = pkm.GetPixelFormat();
	tPixelFormat pkmformatsrc = pkm.GetPixelFormatSrc();
	tRequire(fileformat == pkmformatsrc);
	if (loadFlags & tImagePKM::LoadFlag_Decode)
		tRequire(pkmformat == tPixelFormat::R8G8B8A8);
	else
		tRequire(pkmformat == fileformat);

	const char* profileName = tGetColourProfileName(pkm.GetColourProfile());
	if (profileName)
		tPrintf("ColourProfile: %s\n", profileName);

	tLayer* layer = pkm.StealLayer();
	tRequire(layer && layer->OwnsData);
	tRequire(!(loadFlags & tImagePKM::LoadFlag_Decode) || (pkmformat == tPixelFormat::R8G8B8A8));

	if (pkmformat == tPixelFormat::R8G8B8A8)
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


tTestUnit(ImagePKM)
{
	if (!tSystem::tDirExists("TestData/Images/PKM/"))
		tSkipUnit(ImagePicture)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/PKM/");

	uint32 decode = tImagePKM::LoadFlag_Decode;
	uint32 revrow = tImagePKM::LoadFlag_ReverseRowOrder;
	uint32 spread = tImagePKM::LoadFlag_SpreadLuminance;

	tPrintf("Testing PKM Loading/Decoding\n\n");
	tPrintf("D = Decode\n");
	tPrintf("G = Explicit Gamma or sRGB Compression. g = auto\n");
	tPrintf("R = Reverse Row Order\n");
	tPrintf("S = Spread Luminance\n");

	// EAC
	PKMLoadDecodeSave("EACR11U_R.pkm",				decode | revrow);
	PKMLoadDecodeSave("EACR11U_R.pkm",				decode | revrow | spread);
	PKMLoadDecodeSave("EACR11S_R.pkm",				decode | revrow);
	PKMLoadDecodeSave("EACR11S_R.pkm",				decode | revrow | spread);
	PKMLoadDecodeSave("EACRG11U_RG.pkm",			decode | revrow);
	PKMLoadDecodeSave("EACRG11U_RG.pkm",			decode | revrow | spread);
	PKMLoadDecodeSave("EACRG11S_RG.pkm",			decode | revrow);
	PKMLoadDecodeSave("EACRG11S_RG.pkm",			decode | revrow | spread);

	// ETC1
	PKMLoadDecodeSave("ETC1_RGB.pkm",				decode | revrow);
	PKMLoadDecodeSave("ETC1_RGB_1281x721.pkm",		decode | revrow);
	PKMLoadDecodeSave("ETC1_RGB_1282x722.pkm",		decode | revrow);
	PKMLoadDecodeSave("ETC1_RGB_1283x723.pkm",		decode | revrow);

	// ETC2
	PKMLoadDecodeSave("ETC2RGB_RGB.pkm",			decode | revrow);
	PKMLoadDecodeSave("ETC2RGB_RGB_1281x721.pkm",	decode | revrow);
	PKMLoadDecodeSave("ETC2RGB_RGB_1282x722.pkm",	decode | revrow);
	PKMLoadDecodeSave("ETC2RGB_RGB_1283x723.pkm",	decode | revrow);
	PKMLoadDecodeSave("ETC2RGB_sRGB.pkm",			decode | revrow);
	PKMLoadDecodeSave("ETC2RGBA_RGBA.pkm",			decode | revrow);
	PKMLoadDecodeSave("ETC2RGBA_sRGBA.pkm",			decode | revrow);
	PKMLoadDecodeSave("ETC2RGBA1_RGBA.pkm",			decode | revrow);
	PKMLoadDecodeSave("ETC2RGBA1_sRGBA.pkm",		decode | revrow);

	tSystem::tSetCurrentDir(origDir);
}


// Helper for tImagePKM unit tests.
void PVRLoadDecodeSave(const tString& pvrFile, uint32 loadFlags = 0, bool saveAllMips = false)
{
	// We're just going to turn on auto-gamma-compression for all files.
	loadFlags |= tImagePKM::LoadFlag_AutoGamma;

	tString basename = tSystem::tGetFileBaseName(pvrFile);

	tString savename = basename + "_";
	savename += (loadFlags & tImagePVR::LoadFlag_Decode)			? "D" : "x";
	if ((loadFlags & tImagePVR::LoadFlag_GammaCompression) || (loadFlags & tImagePVR::LoadFlag_SRGBCompression))
		savename += "G";
	else if (loadFlags & tImagePVR::LoadFlag_AutoGamma)
		savename += "g";
	else
		savename += "x";
	savename += (loadFlags & tImagePVR::LoadFlag_ReverseRowOrder)	? "R" : "x";
	savename += (loadFlags & tImagePVR::LoadFlag_SpreadLuminance)	? "S" : "x";
	tPrintf("PVR savename %s\n", savename.Chr());

	// Determine pixel format from filename.
	tString fileFormatName = basename.Left('_');
	fileFormatName.ExtractLeft('+');
	tPrintf("Format name: %s\n", fileFormatName.Chr());
	tPixelFormat fileFormat = tGetPixelFormat(fileFormatName.Chr());

	// If necessary the underscores can be counted to determine PVR1/2 vs PVR3.
	tImagePVR::LoadParams params;
	params.Flags = loadFlags;

	tImagePVR pvr(pvrFile, params);
	tRequire(pvr.IsValid());
	tPixelFormat pvrFormat = pvr.GetPixelFormat();
	tPixelFormat pvrFormatSrc = pvr.GetPixelFormatSrc();
	tRequire(fileFormat == pvrFormatSrc);

	if (loadFlags & tImagePVR::LoadFlag_Decode)
		tRequire(pvrFormat == tPixelFormat::R8G8B8A8);
	else
		tRequire(pvrFormat == fileFormat);

	const char* profileName = tGetColourProfileName(pvr.GetColourProfile());
	if (profileName)
		tPrintf("ColourProfile: %s\n", profileName);

	tList<tImage::tLayer> layers;
	pvr.StealLayers(layers);

	tRequire(!(loadFlags & tImagePVR::LoadFlag_Decode) || (pvrFormat == tPixelFormat::R8G8B8A8));
	if (pvrFormat != tPixelFormat::R8G8B8A8)
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


tTestUnit(ImagePVR2)
{
	if (!tSystem::tDirExists("TestData/Images/PVR_V2/"))
		tSkipUnit(ImagePicture)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/PVR_V2/");

	uint32 decode = tImagePVR::LoadFlag_Decode;
	uint32 revrow = tImagePVR::LoadFlag_ReverseRowOrder;
	uint32 spread = tImagePVR::LoadFlag_SpreadLuminance;

	tPrintf("Testing PVR V2 Loading/Decoding\n\n");

	// PVRLoadDecodeSave("PVRBPP4_UNORM_SRGB_RGBA_TM.pvr",		decode | revrow);
	// PVRLoadDecodeSave("B8G8R8A8_UNORM_SRGB_RGBA_TM.pvr",	decode | revrow,	true);

	PVRLoadDecodeSave("B8G8R8A8_RGBA_T.pvr",		decode | revrow,	true);
	PVRLoadDecodeSave("R8G8B8A8_RGBA_TM.pvr",		decode | revrow,	true);
	PVRLoadDecodeSave("ETC1_RGB_TM.pvr",			decode | revrow,	true);

	// And again without decoding.
	PVRLoadDecodeSave("B8G8R8A8_RGBA_T.pvr",		revrow);
	PVRLoadDecodeSave("R8G8B8A8_RGBA_TM.pvr",		revrow);
	PVRLoadDecodeSave("ETC1_RGB_TM.pvr",			revrow);

	tSystem::tSetCurrentDir(origDir);
}


tTestUnit(ImagePVR3)
{
	if (!tSystem::tDirExists("TestData/Images/PVR_V3/"))
		tSkipUnit(ImagePicture)
	tString origDir = tSystem::tGetCurrentDir();
	tSystem::tSetCurrentDir(origDir + "TestData/Images/PVR_V3/");

	uint32 decode = tImagePVR::LoadFlag_Decode;
	uint32 revrow = tImagePVR::LoadFlag_ReverseRowOrder;
	uint32 spread = tImagePVR::LoadFlag_SpreadLuminance;

	tPrintf("Testing PVR V3 Loading/Decoding\n\n");

	// Flags:
	// T = Regular 2D texture.
	// V = Volume 3D texture.
	// C = Cubemap. Has up to 6 2D textures.
	//
	// A = Texture array. Has an arbitrary number of textures.
	// M = Has Mipmaps.
	// P = Premultiplied Alpha
	PVRLoadDecodeSave("A1R5G5B5+G3B5A1R5G2_UNORM_LIN_RGBA_T.pvr",	decode | revrow,	true);
	PVRLoadDecodeSave("A4R4G4B4+G4B4A4R4_UNORM_LIN_RGBA_T.pvr",		decode | revrow,	true);
	PVRLoadDecodeSave("A4R4G4B4+G4B4A4R4_UNORM_LIN_RGBA_TM.pvr",	decode | revrow,	true);
	PVRLoadDecodeSave("A4R4G4B4+G4B4A4R4_UNORM_LIN_RGBA_TP.pvr",	decode | revrow,	true);
	PVRLoadDecodeSave("B8G8R8A8_UNORM_SRGB_RGBA_T.pvr",				decode | revrow,	true);
	PVRLoadDecodeSave("R4G4B4A4+B4A4R4G4_UNORM_LIN_RGBA_T.pvr",		decode | revrow,	true);
	PVRLoadDecodeSave("R4G4B4A4+B4A4R4G4_UNORM_LIN_RGBA_TP.pvr",	decode | revrow,	true);

	PVRLoadDecodeSave("R5G5B5A1+G2B5A1R5G3_UNORM_LIN_RGBA_T.pvr",	decode | revrow,	true);
	PVRLoadDecodeSave("R5G6B5+G3B5R5G3_UNORM_LIN_RGB_T.pvr",		decode | revrow,	true);
	PVRLoadDecodeSave("R8G8B8A8_UNORM_SRGB_RGBA_TM.pvr",			decode | revrow,	true);

	PVRLoadDecodeSave("R32G32B32A32f_FLOAT_SRGB_RGBA_C.pvr",		decode | revrow,	true);

	PVRLoadDecodeSave("R32G32B32A32f_FLOAT_LIN_RGBA_TM.pvr",		decode | revrow,	true);
	PVRLoadDecodeSave("B10G11R11uf_UFLOAT_LIN_RGB_TM.pvr",			decode | revrow,	true);

	tSystem::tSetCurrentDir(origDir);
}


}
