// tUnit.h
//
// This file implements the base class for all input units. Units read single values from hardware. One or more units
// make a component.
//
// @todo It is at the unit level where remappings (input configurations) will be implemented. This allows any input unit
// to map to any other compatable input unit.
//
// Copyright (c) 2025 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <mutex>
#include <Foundation/tStandard.h>
#include <Foundation/tName.h>
#include <System/tPrint.h>
#define InitUnit(n) n(name+"|" #n, mutex)
namespace tInput
{


class tUnit
{
public:
	// All units have a name and a mutex. The mutex is a ref because it is shared with other components of a particular
	// controller.
	tUnit(const tName& name, std::mutex& mutex)																			: Name(name), Mutex(mutex) { }
	virtual ~tUnit()																									{ }

	tName Name;
	std::mutex& Mutex;
};


}
