// TestFoundation.h
//
// Foundation module tests.
//
// Copyright (c) 2017, 2019-2022, 2025 Tristan Grimmer.
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
	tTestUnit(Types);
	tTestUnit(Array);
	tTestUnit(List);
	tTestUnit(ListExtra);
	tTestUnit(ListSort);
	tTestUnit(Map);
	tTestUnit(Promise);
	tTestUnit(Sort);
	tTestUnit(BitArray);
	tTestUnit(BitField);
	tTestUnit(FixInt);
	tTestUnit(String);
	tTestUnit(UTF);
	tTestUnit(Name);
	tTestUnit(RingBuffer);
	tTestUnit(PriorityQueue);
	tTestUnit(MemoryPool);
	tTestUnit(Hash);
	tTestUnit(SmallFloat);
}
