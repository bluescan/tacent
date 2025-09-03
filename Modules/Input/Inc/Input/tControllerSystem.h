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
#include <vector>
#include <Foundation/tStandard.h>
#include <Input/tContGamepad.h>
namespace tInput
{


class tControllerSystem
{
public:
	// PollingPeriod is in microseconds. A value of 0 means auto-detect the polling period by inspecting the vendor and
	// prodict ID of any controller that is plugged in. For example the polling rate of an 8BitDo Ultimate 2 Wireless is
	// 1000Hz so it will set the pollingPeriod to 1000us (1ms -> 1000Hz, 2000Hz -> 500us). Note that there is a separate
	// polling thread for each controller. In auto-mode the polling rate may be different for each controller. If
	// controller details can't be determined or pollingPeriod is 0, 8000us (125Hz) is used. This is the XBoxOne
	// controller (gamepad) polling rate. PollingControllerDetectionPeriod is in milliseconds. A value of 0 means use
	// the default 1000ms period.
	tControllerSystem(int pollingPeriod_us = 0, int pollingControllerDetectionPeriod_ms = 0);
	virtual ~tControllerSystem();

	// Call this periodically from the main thread loop. When this is called any callbacks are executed and all
	// controller state is updated.
	void Update();

	tContGamepad& GetGetpad(tGamepadID gid)																				{ return Gamepads[int(gid)]; }

private:
	// This function runs on a different thread.
	void Detect();

	// This mutex protects PollExitRequested, the gamepad Connected state variable, and all tUnit members in the
	// components of the gamepads -- ditto for other controller types when we get around to implementing them.
	std::mutex Mutex;

	int PollingPeriod_us = 0;				// In microseconds.
	int DetectPeriod_ms = 0;				// In milliseconds.

	// To simplify the implementation we are going to support up to precisely 4 gamepads. This matches the maximum
	// supported by xinput on windows and restricts the number of gamepads on Linux to 4, which seems perfectly
	// reasonable. By simply having an array of gamepads that are always present it also makes reading controller values
	// a simple process. Just loop through the controllers and ignore any that are in the disconnected state.
	// Parts of tContGamepad mutex protected: the connected bool and tUnit values in the components.
	std::vector<tContGamepad> Gamepads;

	// The DetectExitRequested predicate is required to avoid spurious wakeups. Mutex protected.
	bool DetectExitRequested = false;
	std::condition_variable DetectExitCondition;
	std::thread DetectThread;
};


}
