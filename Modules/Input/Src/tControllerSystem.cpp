// tControllerSystem.cpp
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

#include <chrono>
#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <xinput.h>
#endif
#include "Foundation/tPlatform.h"
#include "System/tPrint.h"
#include "Input/tControllerSystem.h"
namespace tInput
{


tControllerSystem::tControllerSystem(int pollingPeriod, int pollingControllerDetectionPeriod) :
	Gamepads(int(tGamepadID::MaxGamepads), tContGamepad(Mutex))
{
	// If auto-detect is turned on there is no need to set the PollingPeriod.
	if (pollingPeriod > 0)
		PollingPeriod = pollingPeriod;
	else
		PollingPeriodAutoDetect = true;

	if (pollingControllerDetectionPeriod > 0)
		PollingControllerDetectionPeriod = pollingControllerDetectionPeriod;
	else
		PollingControllerDetectionPeriod = 1000;

	PollingThread = std::thread(&tControllerSystem::Poll, this);
}


tControllerSystem::~tControllerSystem()
{
	{
		const std::lock_guard<std::mutex> lock(Mutex);
		PollExitRequested = true;
	}
	// Notify that we want to cooperatively stop the polling thread. Notify one (thread) should be sufficient.
	// Notify_all would also work but is overkill since only one (polling) thread is waiting. By using a condition
	// variable we've made it so we don't have to wait for the current polling cycle sleep to complete.
	PollExitCondition.notify_one();

	// The join just blocks until the polling thread has finished -- and has responded to the notify above.
	PollingThread.join();
}


void tControllerSystem::Poll()
{
	while (1)
	{
		// Do the poll.
		#ifdef PLATFORM_WINDOWS

		// This will be true if either a new controller is connected or one is removed.
		bool gamepadConnectionsChanged = false;
		int numConnected = 0;
		for (int g = 0; g < int(tGamepadID::MaxGamepads); g++)
		{
			WinXInputState state;
			tStd::tMemclr(&state, sizeof(WinXInputState));

			// Get the state of the controller from XInput.
			WinDWord result = XInputGetState(g, &state);

			if (result == WinErrorSuccess)
			{
				// Controller connected.
				numConnected++;
				if (!Gamepads[g].IsConnected())
				{
					// @todo queue up a connected event for Update to deal with.
					gamepadConnectionsChanged = true;
					Gamepads[g].SetConnected(true);
				}
			}
			else
			{
				// Controller is not connected.
				if (Gamepads[g].IsConnected())
				{
					// @todo queue up a disconnected event for Update to deal with.
					gamepadConnectionsChanged = true;
					Gamepads[g].SetConnected(false);
				}
			}

			// Test 
			//	DWORD XInputGetCapabilities(
			//  [in]  DWORD               dwUserIndex,
			//  [in]  DWORD               dwFlags,
			//  [out] XINPUT_CAPABILITIES *pCapabilities
			//);
		}
		#endif

		static int pollNum = 0;
		tPrintf("Poll: %d\n", pollNum++);

		// This unique_lock is just a more powerful version of lock_guard. Supports subsequent unlocking/locking which
		// is presumably needed by wait_for. In any case, wait_for needs this type of lock.
		std::unique_lock<std::mutex> lock(Mutex);
		bool exitRequested = PollExitCondition.wait_for(lock, std::chrono::seconds(1), [this]{ return PollExitRequested; });
		if (exitRequested)
			break;
	}
}


void tControllerSystem::Update()
{
}


}
