// tUtilHEIF.cpp
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

#include <Foundation/tString.h>
#include <Foundation/tPlatform.h>
#include <System/tFile.h>
#include "Image/tUtilHEIF.h"
using namespace tStd;
using namespace tImage;


namespace { namespace tHEIF
{
	// Box parsing. An ISOBMFF box is: uint32 size; char[4] type;
	struct BoxRef
	{
		int offset;
		uint32 size;
		char type[5];
	};

	BoxRef MakeBoxRef(int offset, uint32 size, const char* type);
	bool BoxTypeIs(const BoxRef& box, const char* type);
	bool FindBox(const tArray<BoxRef>& boxes, const char* type, BoxRef& out);

	// Orientation as a 2x2 signed-permutation matrix (x right, y down). It maps a stored-image displacement to the
	// displayed one: displayed = M * stored.
	struct Mat2
	{
		int8_t m00, m01, m10, m11;
	};

	bool MatEq(const Mat2& a, const Mat2& b);
	Mat2 MatMul(const Mat2& a, const Mat2& b);

	const Mat2 kIdentity     = {  1,  0,  0,  1 };
	const Mat2 kRot90CW      = {  0, -1,  1,  0 };
	const Mat2 kRot180       = { -1,  0,  0, -1 };
	const Mat2 kRot90ACW     = {  0,  1, -1,  0 };
	const Mat2 kFlipH        = { -1,  0,  0,  1 };
	const Mat2 kFlipV        = {  1,  0,  0, -1 };
	const Mat2 kFlipMainDiag = {  0,  1,  1,  0 };   // Reflect across the main diagonal (y = x).
	const Mat2 kFlipAntiDiag = {  0, -1, -1,  0 };   // Reflect across the anti-diagonal (y = -x).

	Mat2 TransformMatrix(HEIFTransform transform);

	// Reconstruct the orientation matrix from the 'irot' value and the 'imir' byte, in the SAME convention the HEIC
	// decoder uses (verified against the ImageHEICRotatePixelVerify pixel test). Empirically Tacent decodes the boxes
	// as: irot 0 -> kIdentity,  irot 1 -> kRot90ACW,  irot 2 -> kRot180,  irot 3 -> kRot90CW.
	//
	// This MUST be the exact inverse of EncodeOrientation (see the round-trip check below) so that
	// decode(encode(M)) == M for all eight D4 orientations. Note: A prior version had irot=1 and irot=3 swapped here;
	// combined with a kFlipV-conjugation in the composition it only cancelled for irot=0 sources and silently swapped
	// CW/ACW for any source already carrying irot=1 or irot=3.
	//
	// For flips (mirroring): imir byte : 1 => horizontal mirror (left/right), 0 => vertical mirror (top/bottom).
	// Libheif applies the mirror first and then the rotation, so: displayed = Rotation * Mirror * stored. The order
	// matters only when both a rotation and a mirror are present: with the rotation first, irot=3 + mirror maps to the
	// two diagonal flips. EncodeOrientation and this decoder must agree on the order so that decode(encode(M)) == M for
	// all eight D4 orientations -- see the round-trip check.)
	Mat2 OrientationFromBoxes(int irotValue, uint8 imirByte, bool hasImir);

	// Encode an orientation matrix into its canonical (irot, imir) pair. This is the EXACT INVERSE of
	// OrientationFromBoxes above (i.e. the inverse of TacentView's own box decode), so decode(encode(M)) == M for all
	// eight D4 orientations. 'outHasImir' is false for pure rotations / identity (no 'imir' box needed).
	bool EncodeOrientation(const Mat2& m, int& outIrot, uint8& outImir, bool& outHasImir);

	// ApplyHEIFTransform_Impl: patch the 'irot'/'imir' item-property boxes to apply a lossless (metadata-only)
	// orientation transform. It composes the requested transform on top of the current orientation and produces the
	// result buffer, creating the 'irot' and/or 'imir' boxes (with 'ipma' linkage) if missing. `buffer` is the input
	// HEIF / AVIF bytes; on success the transformed bytes are returned in `out` and true is returned. The verified
	// box-rewriting algorithm is unchanged; only the I/O boundary moved. The public TransformHEIFData /
	// TransformHEIFFile wrappers are defined below.)
	bool ApplyHEIFTransform_Impl(const tArray<uint8>& buffer, HEIFTransform transform, tArray<uint8>& out);
} }


tHEIF::BoxRef tHEIF::MakeBoxRef(int offset, uint32 size, const char* type)
{
	BoxRef box;
	box.offset = offset;
	box.size = size;
	tMemcpy(box.type, type, 4);
	box.type[4] = 0;
	return box;
}


bool tHEIF::BoxTypeIs(const BoxRef& box, const char* type)
{
	return tMemcmp(box.type, type, 4) == 0;
}


bool tHEIF::FindBox(const tArray<BoxRef>& boxes, const char* type, tHEIF::BoxRef& out)
{
	for (const auto& box : boxes)
	{
		if (BoxTypeIs(box, type))
		{
			out = box;
			return true;
		}
	}
	return false;
}


bool tHEIF::MatEq(const Mat2& a, const Mat2& b)
{
	return a.m00 == b.m00 && a.m01 == b.m01 && a.m10 == b.m10 && a.m11 == b.m11;
}


tHEIF::Mat2 tHEIF::MatMul(const Mat2& a, const Mat2& b)
{
	return Mat2
	{
		int8_t(a.m00 * b.m00 + a.m01 * b.m10),
		int8_t(a.m00 * b.m01 + a.m01 * b.m11),
		int8_t(a.m10 * b.m00 + a.m11 * b.m10),
		int8_t(a.m10 * b.m01 + a.m11 * b.m11)
	};
}


tHEIF::Mat2 tHEIF::TransformMatrix(HEIFTransform transform)
{
	switch (transform)
	{
		case HEIFTransform::Rotate90CW:   return kRot90CW;
		case HEIFTransform::Rotate90ACW:  return kRot90ACW;
		case HEIFTransform::Rotate180:    return kRot180;
		case HEIFTransform::FlipH:        return kFlipH;
		case HEIFTransform::FlipV:        return kFlipV;
		case HEIFTransform::FlipMainDiag: return kFlipMainDiag;
		case HEIFTransform::FlipAntiDiag: return kFlipAntiDiag;
		default:                          return kIdentity;
	}
}


tHEIF::Mat2 tHEIF::OrientationFromBoxes(int irotValue, uint8 imirByte, bool hasImir)
{
	const Mat2 rotations[] = { kIdentity, kRot90ACW, kRot180, kRot90CW };
	Mat2 rotation = (irotValue >= 0 && irotValue <= 3) ? rotations[irotValue] : kIdentity;
	Mat2 mirror = hasImir ? (imirByte == 1 ? kFlipH : kFlipV) : kIdentity;
	return MatMul(rotation, mirror);
}


bool tHEIF::EncodeOrientation(const Mat2& m, int& outIrot, uint8& outImir, bool& outHasImir)
{
	if (MatEq(m, kIdentity))     { outIrot = 0; outImir = 0; outHasImir = false; return true; }
	if (MatEq(m, kFlipH))        { outIrot = 0; outImir = 1; outHasImir = true;  return true; }
	if (MatEq(m, kRot180))       { outIrot = 2; outImir = 0; outHasImir = false; return true; }
	if (MatEq(m, kFlipV))        { outIrot = 0; outImir = 0; outHasImir = true;  return true; }
	if (MatEq(m, kFlipMainDiag)) { outIrot = 3; outImir = 0; outHasImir = true;  return true; }
	if (MatEq(m, kRot90CW))      { outIrot = 3; outImir = 0; outHasImir = false; return true; }
	if (MatEq(m, kFlipAntiDiag)) { outIrot = 3; outImir = 1; outHasImir = true;  return true; }
	if (MatEq(m, kRot90ACW))     { outIrot = 1; outImir = 0; outHasImir = false; return true; }
	return false;
}


bool tHEIF::ApplyHEIFTransform_Impl(const tArray<uint8>& buffer, HEIFTransform transform, tArray<uint8>& out)
{
	const int size = buffer.GetNumElements();
	const uint8* buf = buffer.GetElements();
	if (size < 8)
		return false;

	// Locate the top-level 'meta' box.
	BoxRef meta;
	{
		int offset = 0;
		while (offset + 8 <= size)
		{
			uint32 boxSize = tReadU32BE(buf + offset);
			if (boxSize >= 8 && offset + boxSize <= size && tMemcmp(buf + offset + 4, "meta", 4) == 0)
			{
				meta = MakeBoxRef(offset, boxSize, "meta");
				break;
			}

			if (boxSize < 8)
				break;
			offset += boxSize;
		}
	}
	if (meta.size < 12)
		return false;

	// Find 'iprp' inside 'meta'. 'meta' is a FullBox: 8 header + 4 version/flags.
	BoxRef iprp;
	{
		int offset = meta.offset + 12;
		while (offset + 8 <= meta.offset + meta.size)
		{
			uint32 boxSize = tReadU32BE(buf + offset);
			if (boxSize < 8 || offset + boxSize > meta.offset + meta.size) break;
			if (tMemcmp(buf + offset + 4, "iprp", 4) == 0)
			{
				iprp = MakeBoxRef(offset, boxSize, "iprp");
				break;
			}
			offset += boxSize;
		}
	}

	if (iprp.size < 8)
		return false;

	// Collect the 'iprp' children.
	tArray<BoxRef> children;
	{
		int offset = iprp.offset + 8;
		while (offset + 8 <= iprp.offset + iprp.size)
		{
			uint32 boxSize = tReadU32BE(buf + offset);
			if (boxSize < 8 || offset + boxSize > iprp.offset + iprp.size)
				break;

			children.Append(MakeBoxRef(offset, boxSize, reinterpret_cast<const char*>(buf + offset + 4)));
			offset += boxSize;
		}
	}

	BoxRef ipma;
	BoxRef ipco;
	bool hasIpma = FindBox(children, "ipma", ipma);
	bool hasIpco = FindBox(children, "ipco", ipco);
	if (!hasIpco)
		return false;

	// Walk the children of 'ipco' (an ItemPropertyContainer, a plain container: children start at offset+8). The 'irot'
	// and 'imir' item properties live INSIDE 'ipco', not as siblings of it.
	tArray<BoxRef> ipcoChildren;
	{
		const int childEnd = ipco.offset + ipco.size;
		int offset = ipco.offset + 8;
		while (offset + 8 <= childEnd)
		{
			uint32 entrySize = tReadU32BE(buf + offset);
			if (entrySize < 8 || offset + entrySize > childEnd)
				break;
			ipcoChildren.Append(MakeBoxRef(offset, entrySize, (const char*)(buf + offset + 4)));
			offset += entrySize;
		}
	}
	int ipcoCount = ipcoChildren.GetNumElements();

	BoxRef irotBox;
	BoxRef imirBox;
	BoxRef freeBox;
	bool hasIrot = FindBox(ipcoChildren, "irot", irotBox);
	bool hasImir = FindBox(ipcoChildren, "imir", imirBox);

	// A 9-byte "free" box in ipco is the remnant of a previously-removed imir (or irot).
	// Reusing it avoids unbounded file growth when flipping back and forth.
	bool hasFreeBox = false;
	for (const auto& ch : ipcoChildren)
	{
		if (BoxTypeIs(ch, "free") && ch.size == 9)
		{
			freeBox = ch;
			hasFreeBox = true;
			break;
		}
	}

	// Read the current orientation. The 'irot' box is 9 bytes ([size]["irot"][rot]) and the 'imir' box is 9 bytes
	// ([size]["imir"][axis]); in both cases the orientation byte is the single payload byte at offset+8 (libheif
	// box.cc: read8/write8).
	int currentIrot = 0;
	uint8 currentImir = 0;
	bool currentHasImir = false;
	if (hasIrot && irotBox.size >= 9)
		currentIrot = int(tReadU8(buf + irotBox.offset + 8));

	if (hasImir && imirBox.size >= 9)
	{
		currentImir = tReadU8(buf + imirBox.offset + 8);
		currentHasImir = true;
	}

	// The requested transform D is applied to the image as decoded by Tacent. We therefore work entirely in the same
	// matrix convention that Tacent's own decoder uses: the current displayed orientation is currentMatrix =
	// OrientationFromBoxes(...), and the target displayed orientation is currentMatrix * D. OrientationFromBoxes and
	// EncodeOrientation are exact inverses (decode == Tacent decode, encode == its inverse), so no extra conjugation is
	// needed. Note: A prior version inserted a kFlipV-conjugation here AND had the two 90-degree rotation entries
	// swapped in both tables; those three inversions only cancelled for sources with irot=0, silently swapping CW/ACW
	// for any source already carrying irot=1 or irot=3.
	const Mat2 currentMatrix = OrientationFromBoxes(currentIrot, currentImir, currentHasImir);
	const Mat2 targetMatrix = MatMul(currentMatrix, TransformMatrix(transform));
	int finalIrot = 0;
	uint8 finalImir = 0;
	bool finalHasImir = false;
	if (!EncodeOrientation(targetMatrix, finalIrot, finalImir, finalHasImir))
		return false;

	// When a pre-existing 'imir' box must be effectively removed (target needs no mirror), we overwrite its type bytes
	// with "free" (a valid ISOBMFF no-op box of the same size). This keeps all box sizes and ipma indices intact while
	// making the property inert: decoders and Tacent's own box walker skip unrecognized types. A subsequent transform
	// will see hasImir=false and create a fresh imir box if needed.

	// Which boxes must be created (only those missing and non-identity). If a 9-byte "free" box exists
	// (previously-removed imir), reuse it in place instead of appending a new box (which would grow the file
	// unboundedly under repeated flips).
	const bool reuseImir = !hasImir && finalHasImir && hasFreeBox;
	const bool createIrot = !hasIrot && (finalIrot != 0);
	const bool createImir = !hasImir && finalHasImir && !reuseImir;
	if ((createIrot || createImir) && !hasIpma)
	{
		// Cannot add a property without an 'ipma' to associate it with an item.
		return false;
	}

	// Assign 'ipco' property indices (1-based) to the new boxes. They are appended at the END of 'ipco', so their
	// indices are (existing child count + 1), (+2), ... in append order.
	int nextPropIndex = ipcoCount + 1;
	int irotPropIndex = 0;
	int imirPropIndex = 0;
	if (createIrot) irotPropIndex = nextPropIndex++;
	if (createImir) imirPropIndex = nextPropIndex++;

	// Build the new 'irot' / 'imir' boxes to append to 'ipco'. 'irot' box is 9 bytes: [size]["irot"][rot]. 'imir' box
	// is 9 bytes: [size]["imir"][axis].
	tArray<uint8> ipcoAppend;
	if (createIrot)
	{
		uint8 box[9];
		tWriteU32BE(box, 9);
		tMemcpy(box + 4, "irot", 4);
		box[8] = uint8(finalIrot);
		ipcoAppend.Append(box, 9);
	}

	if (createImir)
	{
		uint8 box[9];
		tWriteU32BE(box, 9);
		tMemcpy(box + 4, "imir", 4);
		box[8] = finalImir;
		ipcoAppend.Append(box, 9);
	}

	// Original-buffer offset (end of 'ipco').
	const int ipcoInsertAt = ipco.offset + ipco.size;
	const int ipcoDelta = uint32(ipcoAppend.GetNumElements());

	// Associate the new orientation property(ies) with the item(s) that actually need them. Libheif applies
	// 'irot'/'imir' PER ITEM from that item's own 'ipma' association. An image's visible surface is the primary item
	// plus any auxiliary item (e.g. the alpha plane, which carries an auxiliary-type association box such as 'auxC').
	// The remaining 'ipma' items are opaque coded data (e.g. the many HEVC auxiliary layers in a HEVC-in-HEIF file)
	// that must NOT receive the orientation, otherwise the decoder corrupts them. So append the new index only to
	// entries that are the primary item or that carry an auxiliary-type association. For simple alpha-less /
	// primary+alpha files this is exactly the item(s) libheif renders, matching the prior behaviour.
	tArray<int> addIndices;
	if (createIrot) addIndices.Append(irotPropIndex);
	if (createImir) addIndices.Append(imirPropIndex);

	// Primary item id ('pitm' is a direct child of 'meta').
	int primaryItemId = 0;
	bool hasPrimary = false;
	{
		const int metaEnd = meta.offset + meta.size;
		int mo = meta.offset + 12; // 'meta' FullBox: children start after version/flags
		while (mo + 8 <= metaEnd)
		{
			const uint32 sz = tReadU32BE(buf + mo);
			if (sz < 8 || mo + sz > metaEnd) break;
			if (tMemcmp(buf + mo + 4, "pitm", 4) == 0)
			{
				const uint8 pv = tReadU8(buf + mo + 8);
				primaryItemId = (pv >= 1) ? tReadU32BE(buf + mo + 12) : tReadU16BE(buf + mo + 12);
				hasPrimary = true;
				break;
			}
			mo += sz;
		}
	}

	// An item is a secondary (auxiliary) plane -- e.g. the alpha channel -- when its 'ipma' association includes an
	// auxiliary-type property box. The canonical one is 'auxC' (auxiliary configuration), used for both AV1 (AVIF) and
	// HEVC (HEIF) alpha planes. Such an item must receive the same orientation as the primary so it stays aligned.
	// Opaque coded items (e.g. the many HEVC auxiliary layers in a HEVC-in-HEIF file) carry no auxiliary-type box and
	// are correctly left untouched.
	const auto isAuxTypeBox = [](const BoxRef& pr) -> bool
	{
		return tMemcmp(pr.type, "auxC", 4) == 0;
	};

	int ipmaDelta = 0;					// Total bytes added to 'ipma'.
	tArray<int> ipmaInsertAts;			// Original-buffer offset where each entry's bytes go.
	tArray<int> ipmaAssocCntFields;		// Original-buffer offset of each entry's association_count.
	tArray<tArray<uint8>> ipmaInserts;	// Bytes to append to each entry.
	if (createIrot || createImir)
	{
		// 'ipma' is a FullBox. Entry field widths depend on version (item_id 2 or 4 bytes) and flags bit 0 (association
		// index 1 or 2 bytes).
		const uint8 ipmaVersion = tReadU8(buf + ipma.offset + 8);
		const uint32 ipmaFlags =
			(uint32(tReadU8(buf + ipma.offset + 9)) << 16) |
			(uint32(tReadU8(buf + ipma.offset + 10)) << 8) |
			(uint32(tReadU8(buf + ipma.offset + 11)) << 0);
		const int itemW	= (ipmaVersion >= 1) ? 4 : 2;
		const int idxW	= (ipmaFlags & 1) ? 2 : 1;

		// Walk every 'ipma' entry; for each item that is the primary or carries an auxiliary-type association, append
		// the new orientation index (bumping that entry's association_count) and leave the rest alone.
		{
			const int eCount = tReadU32BE(buf + ipma.offset + 12);
			const int ipmaBodyEnd = ipma.offset + ipma.size;
			int eOff = ipma.offset + 16;

			for (int i = 0; i < eCount; ++i)
			{
				if (eOff + itemW + 1 > ipmaBodyEnd)
					break;

				const int itemID = (itemW == 4) ? tReadU32BE(buf + eOff) : tReadU16BE(buf + eOff);
				int acnt = tReadU8(buf + eOff + itemW);
				int eEnd = eOff + itemW + 1 + acnt * idxW;
				if (eEnd > ipmaBodyEnd)
					break;

				// Does this item need the orientation? (the primary, or an auxiliary plane).
				bool target = hasPrimary && (itemID == primaryItemId);
				if (!target)
				{
					for (int k = 0; k < acnt; ++k)
					{
						const int ao = eOff + itemW + 1 + k * idxW;
						const int v = (idxW == 2) ? tReadU16BE(buf + ao) : tReadU8(buf + ao);
						const int prIdx = (idxW == 2) ? (v & 0x7FFF) : (v & 0x7F); // essential bit ignored
						if (prIdx < 1 || prIdx > ipcoCount) continue;
						if (isAuxTypeBox(ipcoChildren[prIdx - 1])) { target = true; break; }
					}
				}

				if (!target)
				{
					eOff = eEnd;									// Leave this item's association list untouched.
					continue;
				}

				tArray<uint8> eIns;
				for (int idx : addIndices)
				{
					if (idxW == 1)
					{
						eIns.Append(uint8(0x80 | (idx & 0x7F)));	// Essential bit | property index.
					}
					else
					{
						uint8 t[2];
						tWriteU16BE(t, uint16(0x8000 | (idx & 0x7FFF)));
						eIns.Append(t, 2);
					}
				}

				ipmaInsertAts.Append(eEnd);
				ipmaAssocCntFields.Append(eOff + itemW);
				ipmaDelta += uint32(eIns.GetNumElements());			// Accumulate BEFORE moving eIns (a moved-from array has 0 elements).
				ipmaInserts.Append(tStd::rVal(eIns));
				eOff = eEnd;
			}

			if (ipmaInsertAts.GetNumElements() == 0)
				return false;										// No primary/auxiliary item to associate the orientation with.
		}
	}

	const int totalDelta = ipcoDelta + ipmaDelta;
	if (totalDelta == 0 && !hasIrot && !hasImir && !reuseImir)
		return false;												// Nothing to change.

	// Rebuild the buffer, inserting the new bytes at each (original-buffer) offset.
	struct Ins { int at; tArray<uint8> bytes; };
	tArray<Ins> insList;

	for (int k = 0; k < ipmaInserts.GetNumElements(); k++)
		insList.Append(Ins{ ipmaInsertAts[k], ipmaInserts[k] });

	if (ipcoDelta != 0)
		insList.Append(Ins{ ipcoInsertAt, ipcoAppend });

	// Sort insList by insertion offset (insertion sort; the list is typically 1-3 elements)..
	for (int i = 1; i < insList.GetNumElements(); i++)
	{
		Ins val = insList[i];
		int j = i - 1;
		for (; j >= 0 && val.at < insList[j].at; j--)
			insList[j + 1] = insList[j];

		insList[j + 1] = val;
	}

	out.Clear();
	out.GrowTo(size + totalDelta);
	int insIdx = 0;
	for (int i = 0; i < size; i++)
	{
		while (insIdx < insList.GetNumElements() && insList[insIdx].at == i)
		{
			out.Append(insList[insIdx].bytes.GetElements(), insList[insIdx].bytes.GetNumElements());
			insIdx++;
		}
		out.Append(buf[i]);
	}

	while (insIdx < insList.GetNumElements())	// Insertion exactly at the end of the file.
	{
		out.Append(insList[insIdx].bytes.GetElements(), insList[insIdx].bytes.GetNumElements());
		insIdx++;
	}

	// Map an original offset to its offset in the rebuilt buffer.
	auto finalOf = [&](int o) -> int
	{
		int shift = 0;
		for (const Ins& ins : insList)
			if (ins.at <= o)
				shift += uint32(ins.bytes.GetNumElements());

		return o + shift;
	};

	// Update the sizes of the enclosing boxes. A box is [size:4][type:4][...], so the size field is at the box's own
	// offset (NOT offset+4, which would clobber the 4-char type).
	tWriteU32BE(out.GetElements() + finalOf(meta.offset), meta.size + totalDelta);
	tWriteU32BE(out.GetElements() + finalOf(iprp.offset), iprp.size + totalDelta);
	tWriteU32BE(out.GetElements() + finalOf(ipco.offset), ipco.size + ipcoDelta);
	if (hasIpma)
	{
		tWriteU32BE(out.GetElements() + finalOf(ipma.offset), ipma.size + ipmaDelta);

		// Bump each entry's association_count by the number of indexes we appended to it.
		if (ipmaDelta != 0)
		{
			for (int k = 0; k < ipmaAssocCntFields.GetNumElements(); k++)
			{
				tWriteU8
				(
					out.GetElements() + finalOf(ipmaAssocCntFields[k]),
					uint8(tReadU8(buf + ipmaAssocCntFields[k]) + uint8(addIndices.GetNumElements()))
				);
			}
		}
	}

	// Set the 'irot' / 'imir' values on the pre-existing boxes (new ones were written above). In both boxes the
	// orientation/axis byte is the single payload byte at offset+8.
	if (hasIrot)
		tWriteU8(out.GetElements() + finalOf(irotBox.offset) + 8, uint8(finalIrot));

	if (hasImir)
	{
		if (finalHasImir)
			tWriteU8(out.GetElements() + finalOf(imirBox.offset) + 8, finalImir);
		else
			tMemcpy(out.GetElements() + finalOf(imirBox.offset) + 4, "free", 4);	// Inert ISOBMFF no-op.
	}

	// Reuse a "free" box as a new 'imir': restore type + write the axis byte. No size changes needed.
	if (reuseImir)
	{
		const int fOff = finalOf(freeBox.offset);
		tMemcpy(out.GetElements() + fOff + 4, "imir", 4);
		tWriteU8(out.GetElements() + fOff + 8, finalImir);
	}

	// Update the 'iloc' box. It stores ABSOLUTE file offsets to each item's data (in 'mdat' here). Inserting the new
	// 'irot'/'imir' boxes (and any 'ipma' bytes) before that data shifts it forward, so every extent offset must move
	// by the same amount or the loader reads from the wrong position (this was the "mdat offset is stale" corruption).
	// An extent's data position is (base_offset + extent.offset); we apply the shift to whichever field carries the
	// absolute value.
	{
		int iLocOff = 0, iLocSize = 0; bool hasIloc = false;
		{
			int w = meta.offset + 12;					// 'meta' is a FullBox: children start after version/flags.
			while (w + 8 <= meta.offset + meta.size)
			{
				uint32 bSize = tReadU32BE(buf + w);
				if (bSize < 8 || w + bSize > meta.offset + meta.size) break;
				if (tMemcmp(buf + w + 4, "iloc", 4) == 0)
				{
					iLocOff = w;
					iLocSize = bSize;
					hasIloc = true;
					break;
				}
				w += bSize;
			}
		}

		if (hasIloc)
		{
			const uint8 iv = tReadU8(buf + iLocOff + 8);
			const uint16 w4 = tReadU16BE(buf + iLocOff + 12);
			const int os  = (w4 >> 12) & 0xF;                       // offset_size
			const int ls  = (w4 >> 8) & 0xF;                        // length_size
			const int bos = (w4 >> 4) & 0xF;                        // base_offset_size
			const int isz = (iv == 1 || iv == 2) ? (w4 & 0xF) : 0;  // index_size (only v1/v2)
			int q = iLocOff + 14;
			int nItems = (iv >= 2) ? tReadU32BE(buf + q) : tReadU16BE(buf + q);
			q += (iv >= 2) ? 4 : 2;
			const int qEnd = iLocOff + iLocSize;

			for (int mi = 0; mi < nItems && q < qEnd; mi++)
			{
				q += (iv >= 2) ? 4 : 2;			// item_ID
				if (iv >= 1) q += 2;			// construction_method (2-byte field)
				q += 2;							// data_reference_index
				uint64 bOff = 0; int bPos = q;
				if (bos == 4)
				{
					bOff = tReadU32BE(buf + q);
					q += 4;
				}
				else if (bos == 8)
				{
					bOff = (uint64(tReadU32BE(buf + q)) << 32) | uint64(tReadU32BE(buf + q + 4));
					q += 8;
				}

				uint16 eCnt = 0;
				if (q + 2 <= qEnd)
				{
					eCnt = tReadU16BE(buf + q);
					q += 2;
				}

				for (int ei = 0; ei < eCnt && q < qEnd; ei++)
				{
					if ((iv == 1 || iv == 2) && isz > 0)
						q += (isz == 8) ? 8 : 4;								// Extent index.

					uint64 oVal = 0;
					int oPos = q;
					if (os == 4)
					{
						oVal = tReadU32BE(buf + q);
						q += 4;
					}
					else if (os == 8)
					{
						oVal = (uint64(tReadU32BE(buf + q)) << 32) | uint64(tReadU32BE(buf + q + 4));
						q += 8;
					}

					if (ls == 8)
						q += 8;
					else if (ls == 4)
						q += 4;

					// The absolute data position this extent points to, and how far that data moved.
					const uint64 fPos = bOff + oVal;
					uint64 sh = 0;
					for (const Ins& ins : insList)
						if (uint64(ins.at) <= fPos)
							sh += uint64(ins.bytes.GetNumElements());

					if (sh == 0)
						continue;
					if (bos > 0)
					{
						const uint64 nb = bOff + sh;
						if (bos == 4)
						{
							tWriteU32BE(out.GetElements() + finalOf(bPos), uint32(nb));
						}
						else
						{
							tWriteU32BE(out.GetElements() + finalOf(bPos), uint32(nb >> 32));
							tWriteU32BE(out.GetElements() + finalOf(bPos) + 4, uint32(nb));
						}
					}
					else if (os > 0)
					{
						const uint64 no = oVal + sh;
						if (os == 4)
						{
							tWriteU32BE(out.GetElements() + finalOf(oPos), uint32(no));
						}
						else
						{
							tWriteU32BE(out.GetElements() + finalOf(oPos), uint32(no >> 32));
							tWriteU32BE(out.GetElements() + finalOf(oPos) + 4, uint32(no));
						}
					}
				}
			}
		}
	}

	// The result is now in `out`. The compressed image data is untouched; only the 'irot'/'imir' values, The 'iloc'
	// extent offsets, and the 'ipco'/'ipma' (and enclosing 'meta'/'iprp') box sizes changed.
	return true;
}


tArray<uint8> tImage::TransformHEIFData(const tArray<uint8>& in, HEIFTransform transform)
{
	if (transform == HEIFTransform::Identity)
		return in;									// No-op: return an unchanged copy.

	tArray<uint8> out;
	if (!tHEIF::ApplyHEIFTransform_Impl(in, transform, out))
		return tArray<uint8>();

	return out;
}


bool tImage::TransformHEIFFile(const tString& file, HEIFTransform transform)
{
	if (transform == HEIFTransform::Identity)
		return tSystem::tFileExists(file);			// No-op: nothing to do.

	if (!tSystem::tFileExists(file))
		return false;

	tArray<uint8> buffer;
	if (!tSystem::tLoadFile(file, buffer))
		return false;

	tArray<uint8> out;
	if (!tHEIF::ApplyHEIFTransform_Impl(buffer, transform, out))
		return false;

	return tSystem::tSaveFile(file, out);
}
