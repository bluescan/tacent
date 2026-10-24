// tUtilHEIF.h
//
// Lossless (metadata-only) orientation transforms for HEIF/ISOBMFF based image formats (HEIC, AVIF). Instead of
// decoding and re-encoding the pixel data, this patches the item-property boxes that describe orientation:
//	'irot' (RotationBox)	: A multiple of 90 degrees.
//	'imir' (ImirBox)		: Horizontal and/or vertical mirroring.
// The combination of 'irot' and 'imir' describes exactly one of the 8 dihedral (orthogonal) orientations of the image.
// Other angles (e.g. 45 degrees) cannot be represented in these boxes and would require re-encoding, so only these 8
// lossless transforms are supported.
//
// The compressed coded data is never touched, so the transform is byte-perfect and fast. The orientation math uses 2x2
// signed-permutation matrices (the group D4), which makes composing and encoding orientations exact.
//
// Copyright (c) 2026 Tristan Grimmer.
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
#include <cstddef>
#include <cstdint>
#include <vector>


namespace tImage
{
	// The 8 dihedral (orthogonal) orientations of a HEIF / AVIF image.
	enum class HEIFTransform
	{
		Identity,
		Rotate90CW,
		Rotate90ACW,
		Rotate180,
		FlipH,				// About the vertical midline axis.
		FlipV,				// About the horizontal midline axis.
		FlipMainDiag,		// About the line y = x
		FlipAntiDiag		// About the line y = -x
	};

	// Rewrites the 'irot' / 'imir' item-property boxes (and the 'iloc' extent offsets) of an in-memory
	// HEIF / AVIF buffer `in` to apply `transform`. The compressed coded data is untouched, so the
	// result is byte-perfect and fast. Returns the transformed buffer, or an empty vector if `in` is
	// not a supported HEIF / AVIF container, or `transform` could not be applied (including Identity,
	// which is a no-op).
	std::vector<uint8_t> TransformHEIFData(const std::vector<uint8_t>& in, HEIFTransform transform);

	// File convenience: read `file`, apply `transform` via TransformHEIFData, and write the result back
	// over `file`. Returns true on success. (Provided for tests and for callers that work with files
	// on disk rather than in-memory buffers.)
	bool TransformHEIFFile(const tString& file, HEIFTransform transform);
}
