// tInputSystem.cpp
//
// This file implements the main API for the input system.
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

#include "Input/tInputSystem.h"
#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <xinput.h>
#endif
namespace tInput
{


void TestFun()
{
#ifdef PLATFORM_WINDOWS
	DWORD dwResult;    
	for (DWORD i=0; i< XUSER_MAX_COUNT; i++ )
	{
		XINPUT_STATE state;
		ZeroMemory( &state, sizeof(XINPUT_STATE) );

		// Simply get the state of the controller from XInput.
		dwResult = XInputGetState( i, &state );

		if( dwResult == ERROR_SUCCESS )
		{
			// Controller is connected
		}
		else
		{
			// Controller is not connected
		}	
	}
#endif
}


}
