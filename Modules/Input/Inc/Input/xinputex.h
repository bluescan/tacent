// xinputex.h
//
// Simply includes xinput with some extra missing structs and types.
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
#include <xinput.h>


// Missing from xinput.h.
struct XINPUT_CAPABILITIES_EX
{
	XINPUT_CAPABILITIES Capabilities;
	WORD vendorId;
	WORD productId;
	WORD revisionId;
	DWORD a4; //unknown
};


typedef DWORD(_stdcall* _XInputGetCapabilitiesEx)(DWORD a1, DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES_EX* pCapabilities);
extern _XInputGetCapabilitiesEx XInputGetCapabilitiesEx;


#ifdef XINPUTEX_IMPLEMENTATION
_XInputGetCapabilitiesEx XInputGetCapabilitiesEx = nullptr;
#endif
