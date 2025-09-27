// tCont.h
//
// This file implements the base class for a controller. Controllers represent physical devices like gamepads.
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
#include "Input/tControllerDefinitions.h"
#define InitCompMove(n) n(src.Name+"|" #n, Mutex)
#define InitCompCopy(n) n(name+"|" #n, Mutex)
namespace tInput
{


class tController
{
public:
	tController(const tName& name)																						: Name(name) { Definition.Clear(); }
	virtual ~tController()																								{ }

	// All controllers have a name.
	tName Name;

	// All connected controllers have a definition which will indicate what polling rate should be used.
	tControllerDefinition Definition;

	// Protects updates to all the components since they may be read by the main thread at any time.
	// Protects PollExitRequested.
	std::mutex Mutex;
};


}
