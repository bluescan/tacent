// tControllerSystem.h
//
// This file implements the main API for the input system. It manages all attached controllers.
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
#include <thread>
#include <condition_variable>
#include <Foundation/tStandard.h>
#include <Input/tContGamepad.h>
namespace tInput
{


class tControllerSystem
{
public:
	tControllerSystem();
	virtual ~tControllerSystem();

	// Call this periodically from the main thread loop. When this is called any callbacks are executed and all
	// controller state is updated.
	void Update();

	enum class tGamepadID
	{
		Invalid,
		GP1, GP2, GP3, GP4,
		MaxGamepads = GP4
	};
	tContGamepad& GetGetpad(tGamepadID);

private:
	void Poll();

	// To simplify the implementation we are going to support up to precisely 4 gamepads. This matches the maximum
	// supported by xinput on windows and restricts the number of gamepads on Linux to 4, which seems perfectly
	// reasonable. By simply having an array of gamepads that are always present it also makes reading controller values
	// a simple process. Just loop through the controllers and ignore any that are in the disconnected state.
	tContGamepad Gamepads[int(tGamepadID::MaxGamepads)];

	// The PollExitRequested predicate is required to avoid spurious wakeups.
	bool PollExitRequested = false;
	std::condition_variable PollExitCondition;
	std::thread PollingThread;
	mutable std::mutex Mutex;
};


}
