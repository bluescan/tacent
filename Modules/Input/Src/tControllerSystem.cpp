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
#include <Foundation/tPlatform.h>
#include <Foundation/tName.h>
#include <System/tPrint.h>
#include "Input/tControllerSystem.h"
#include "Input/tControllerDefinitions.h"


#ifdef PLATFORM_WINDOWS
// Missing from xinput.h.
_XInputGetCapabilitiesEx XInputGetCapabilitiesEx;
#endif


namespace tInput
{


tControllerSystem::tControllerSystem(int pollingPeriod_ns, int detectionPeriod_ms) :
	PollingPeriod_ns(pollingPeriod_ns)
{
	Gamepads.reserve(int(tGamepadID::NumGamepads));
	Gamepads.emplace_back("Gamepad1", tGamepadID::GP0);
	Gamepads.emplace_back("Gamepad2", tGamepadID::GP1);
	Gamepads.emplace_back("Gamepad3", tGamepadID::GP2);
	Gamepads.emplace_back("Gamepad4", tGamepadID::GP3);

	DetectPeriod_ms = (detectionPeriod_ms > 0) ? detectionPeriod_ms : 1000;

	#ifdef PLATFORM_WINDOWS
	// This is better than LoadLibrary(TEXT("XInput1_4.dll") in two ways:
	// 1) The module is already loaded using the import library.
	// 2) We use the xinput.h header define in case xinput is ever updated.
    HMODULE moduleHandle = GetModuleHandle(XINPUT_DLL);
    XInputGetCapabilitiesEx = (_XInputGetCapabilitiesEx)GetProcAddress(moduleHandle, (char*)108);
	#endif

	DetectThread = std::thread(&tControllerSystem::Detect, this);
}


tControllerSystem::~tControllerSystem()
{
	{
		std::lock_guard<std::mutex> lock(Mutex);
		DetectExitRequested = true;
	}
	// Notify that we want to cooperatively stop the polling thread. Notify one (thread) should be sufficient.
	// Notify_all would also work but is overkill since only one (polling) thread is waiting. By using a condition
	// variable we've made it so we don't have to wait for the current polling cycle sleep to complete.
	DetectExitCondition.notify_one();

	// The join just blocks until the polling thread has finished -- and has responded to the notify above.
	DetectThread.join();
}


void tControllerSystem::Detect()
{
	while (1)
	{
		// Detect controllers connected and disconnected.
		#ifdef PLATFORM_WINDOWS

		for (int g = 0; g < int(tGamepadID::NumGamepads); g++)
		{
			// XInputGetState is generally faster for detecting device connectedness.
			WinXInputState state;
			tStd::tMemclr(&state, sizeof(WinXInputState));
			WinDWord result = XInputGetState(g, &state);

			if (result == WinErrorSuccess)
			{
				// If we're already polling then move on.
				if (Gamepads[g].IsPolling())
					continue;

				// Controller connected. Now we need to start polling the controller and queue a message that a
				// controller has been connected for the main Update to pick up. If polling period is set to 0 the
				// gamepad will do a hrdware lookup to determine the polling period.
				Gamepads[g].StartPolling(PollingPeriod_ns);
			}
			else
			{
				// Either WinErrorDeviceNotConnected or some other error. Either way treat controller as disconnected.
				if (Gamepads[g].IsPolling())
				{
					// Now we need to stop polling and queue a disconnect message to for the main update to pick up.
					// Since the gamepad is now disconnected
					Gamepads[g].StopPolling();
				}
			}
		}
		#endif

		static int detectNum = 0;
		tPrintf("Detect: %d\n", detectNum++);

		// This unique_lock is just a more powerful version of lock_guard. Supports subsequent unlocking/locking which
		// is presumably needed by wait_for. In any case, wait_for needs this type of lock.
		std::unique_lock<std::mutex> lock(Mutex);
		bool exitRequested = DetectExitCondition.wait_for(lock, std::chrono::milliseconds(DetectPeriod_ms), [this]{ return DetectExitRequested; });
		if (exitRequested)
			break;
	}
}


void tControllerSystem::Update()
{
	for (int g = 0; g < int(tGamepadID::NumGamepads); g++)
	{
		Gamepads[g].Update();
	}
}


}
