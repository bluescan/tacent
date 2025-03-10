// TestImage.h
//
// Image module tests.
//
// Copyright (c) 2017, 2019, 2021-2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include "UnitTests.h"


namespace tUnitTest
{
	tTestUnit(ImageLoad);
	tTestUnit(ImageSave);
	tTestUnit(ImageTexture);
	tTestUnit(ImagePicture);
	tTestUnit(ImageQuantize);
	tTestUnit(ImagePalette);
	tTestUnit(ImageMetaData);
	tTestUnit(ImageLosslessTransform);
	tTestUnit(ImageRotation);
	tTestUnit(ImageCrop);
	tTestUnit(ImageCopyRegion);
	tTestUnit(ImageAdjustment);
	tTestUnit(ImageDetection);
	tTestUnit(ImageMipmap);
	tTestUnit(ImageFilter);
	tTestUnit(ImageMultiFrame);
	tTestUnit(ImageGradient);
	tTestUnit(ImagePNG);
	tTestUnit(ImageDDS);
	tTestUnit(ImageKTX2);
	tTestUnit(ImageKTX1);
	tTestUnit(ImageASTC);
	tTestUnit(ImagePKM);
	tTestUnit(ImagePVR2);
	tTestUnit(ImagePVR3);
}
