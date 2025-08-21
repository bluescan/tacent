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


// Missing from xinput.h.
#ifdef PLATFORM_WINDOWS
struct XINPUT_CAPABILITIES_EX
{
    XINPUT_CAPABILITIES Capabilities;
    WORD vendorId;
    WORD productId;
    WORD revisionId;
    DWORD a4; //unknown
};
typedef DWORD(_stdcall* _XInputGetCapabilitiesEx)(DWORD a1, DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES_EX* pCapabilities);
_XInputGetCapabilitiesEx XInputGetCapabilitiesEx;
#endif


namespace tInput
{


tControllerSystem::tControllerSystem(int pollingPeriod, int detectionPeriod)
{
	Gamepads.reserve(int(tGamepadID::NumGamepads));
	Gamepads.emplace_back("Gamepad1", tGamepadID::GP0);
	Gamepads.emplace_back("Gamepad2", tGamepadID::GP1);
	Gamepads.emplace_back("Gamepad3", tGamepadID::GP2);
	Gamepads.emplace_back("Gamepad4", tGamepadID::GP3);

	// If auto-detect is turned on there is no need to set the PollingPeriod.
	if (pollingPeriod > 0)
		PollingPeriod = pollingPeriod;
	else
		PollingPeriodAutoDetect = true;

	if (detectionPeriod > 0)
		DetectPeriod = detectionPeriod;

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

				// Controller connected. Can go ahead and try to figure out polling rate to use.
				int pollingPeriod = PollingPeriod;
				if (PollingPeriodAutoDetect)
				{
					// In auto detect mode we try to determin the polling rate from the hardware.
					// Do not modify pollingPeriod if detection fails.
					int optimalPollingPeriod = DetermineGamepadPollingPeriodFromHardwareInfo(tGamepadID(g));
					if (optimalPollingPeriod)
						pollingPeriod = optimalPollingPeriod;
				}
				tAssert(pollingPeriod > 0);

				// Now we need to start polling the controller and queue a message that a controller has
				// been connected for the main Update to pick up.
				Gamepads[g].StartPolling(pollingPeriod, tGamepadID(g));
			}
			else
			{
				// Either WinErrorDeviceNotConnected or some other error. Either way treat controller as disconnected.
				if (Gamepads[g].IsPolling())
				{
					// Now we need to stop polling and queue a disconnect message to for the main update to pick up.
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
		bool exitRequested = DetectExitCondition.wait_for(lock, std::chrono::milliseconds(DetectPeriod), [this]{ return DetectExitRequested; });
		if (exitRequested)
			break;
	}
}


int tControllerSystem::DetermineGamepadPollingPeriodFromHardwareInfo(tGamepadID g)
{
	#ifdef PLATFORM_WINDOWS

    int i = int(g);
	tPrintf("Gamepad %d ", i);

	WinXInputCapabilitiesEx capsEx;
	tStd::tMemclr(&capsEx, sizeof(WinXInputCapabilitiesEx));
	if (XInputGetCapabilitiesEx(1, i, 0, &capsEx) == WinErrorSuccess)
	{
		tVidPid vidpid(uint16(capsEx.vendorId), uint16(capsEx.productId));
		const tContDefn* defn = tLookupContDefn(vidpid);
		const char* vendor = defn ? defn->Vendor : "unknown";
		const char* product = defn ? defn->Product : "unknown";
		tPrintf
		(
			"connected. vid = 0x%04X pid = 0x%04X Vendor:%s Product:%s\n",
			int(capsEx.vendorId), int(capsEx.productId), vendor, product
		);
		if (defn && defn->MaxPollingFreq)
			return int(1000000.0f/defn->MaxPollingFreq);
	}
	else
	{
		tPrintf("not connected\n");
	}

	#else
	#endif
	return 0;
}


void tControllerSystem::Update()
{
	for (int g = 0; g < int(tGamepadID::NumGamepads); g++)
	{
		Gamepads[g].Update();
	}
}


}
