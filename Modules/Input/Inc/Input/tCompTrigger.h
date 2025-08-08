// tCompTrigger.h
//
// This file implements a trigger input component. Components are input classes that are grouped together
// in a device.
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
#include "Input/tComp.h"
#include "Input/tUnitContinuousDisp.h"
namespace tInput
{


class tCompTrigger : public tComponent
{
public:
	tCompTrigger(const tName& name, std::mutex& mutex)																	: tComponent(name), Disp(mutex) { }
	virtual ~tCompTrigger()																								{ }

	float GetDisplacement() const																						{ return Disp.GetDisplacement(); }
	void Reset()																										{ Disp.Reset(); }
	void Update();

private:
	// The tContGamepad is allowed to set the raw displacement directly. We don't want any writers to be in the public
	// interface that clients on the main thread use. It is the update call that processes antijitter and dead zones
	// because the result would be the same if it were done in the polling thread, but in Update it's more efficient
	// since there are fewer updates than polls.
	friend class tContGamepad;
	void SetDisplacementRaw(float displacement)																			{ Disp.SetDisplacementRaw(displacement); }
	void SetDisplacement(float displacement)																			{ Disp.SetDisplacement(displacement); }

	// The tUnit has been cnstructed with the mutex ref. Calls made to it are mutex protected.
	tUnitContinuousDisp Disp;
};


}
