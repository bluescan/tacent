// tContGamepad.cpp
//
// This file implements a gamepad controller. Controllers represent physical input devices.
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

#include <chrono>
#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <xinput.h>
#endif
#include "Foundation/tPlatform.h"
#include "Input/tContGamepad.h"
#include "System/tPrint.h"
namespace tInput
{


void tContGamepad::StartPolling(int pollingPeriod, tGamepadID gamepadID)
{
	tAssert((pollingPeriod >= 1) && (gamepadID != tGamepadID::Invalid) && (gamepadID != tGamepadID::MaxGamepads));

	// If it's already running do nothing. We also don't update the period if we're already running.
	if (IsPolling())
		return;

	// PollingPeriod and PollingGamepadID is only ever set before the thread starts and doesn't change. We don't need to Mutex protect it.
	PollingGamepadID = gamepadID;
	PollingPeriod = pollingPeriod;
	PollingThread = std::thread(&tContGamepad::Poll, this);

}


void tContGamepad::StopPolling()
{
	// If it's already stopped so do nothing.
	if (!IsPolling())
		return;

	// We don't want to wait around to exit the polling thread to finish its sleep cycle so we use a condition_variable
	// that uses PollingExitRequested as the predecate.
	{
		const std::lock_guard<std::mutex> lock(Mutex);
		PollingExitRequested = true;
	}

	PollingThread.join();
	PollingGamepadID = tGamepadID::Invalid;
}


void tContGamepad::Poll()
{
	while (true)
	{
		static int pollNum = 0;
		tPrintf("Poll: %d\n", pollNum++);

		#ifdef PLATFORM_WINDOWS
		WinXInputState state;
		tStd::tMemclr(&state, sizeof(WinXInputState));

		// Get the state of the controller from XInput. From what I can tell reading different gamepads on different
		// threads does not require a mutex to protect this call. Only this thread instance will read this particular
		// controller and so two calls to XInputGetState for the same controller will never happed at the same time.
		WinDWord result = XInputGetState(int(PollingGamepadID), &state);

		if (result == WinErrorSuccess)
		{
			// Controller connected. We can read its state and update components.
			if (state.dwPacketNumber != PollingPacketNumber)
			{
				// The set calls to the components below are all mutex protected internally. Main thread may read input
				// unit valuesmso we want the protection.
				float leftTriggerNorm = float(state.Gamepad.bLeftTrigger) / 255.0f;
				LTrigger.SetDisplacementRaw(leftTriggerNorm);

				float rightTriggerNorm = float(state.Gamepad.bRightTrigger) / 255.0f;
				RTrigger.SetDisplacementRaw(rightTriggerNorm);

				PollingPacketNumber = state.dwPacketNumber;
			}
		}
		else
		{
			// Controller is not connected. This can happen if the detection thread of the controller system has not
			// realized yet that the controller was disconnected. In this case we can just stop polling. Note that
			// exiting the loop does not stop the thread from being joinable. It is still considered connected until the
			// detection thread calls StopPolling.
			break;
		}
		#endif

		// This unique_lock is just a more powerful version of lock_guard. Supports subsequent unlocking/locking which
		// is presumably needed by wait_for. In any case, wait_for needs this type of lock.
		std::unique_lock<std::mutex> lock(Mutex);
		bool exitRequested = PollingExitCondition.wait_for(lock, std::chrono::milliseconds(PollingPeriod), [this]{ return PollingExitRequested; });
		if (exitRequested)
			break;
	}
}


void tContGamepad::Update()
{
	LTrigger.Update();
	RTrigger.Update();
}


}
