________________________________________________________________________________
ASTC Images

All .astc test files were generated using the official ARM astcenc-sse2.exe.
This is ARM's (nice and-simple) format after all.

The LDR test images were generated from TacentTestPattern32.tga
The HDR test images were generated from Desk.exr

________________________________________________________________________________
Licence: Desk.exr

The Desk.exr image is from
https://github.com/AcademySoftwareFoundation/openexr-images
and has the following licence:

Copyright (c) 2004, Industrial Light & Magic, a division of Lucasfilm
Entertainment Company Ltd.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution.

  * Neither the name of Industrial Light & Magic nor the names of any other
    contributors to this software may be used to endorse or promote products
    derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

________________________________________________________________________________
Licence: TacentTestPattern.tga

The bottom right quadrant of TacentTestPattern.tga (the 'splash') is CC0 and
from user UploadMeToday at purepng.com. The other quadrants are under the same
ISC licence as Tacent.

________________________________________________________________________________
Generation Examples

astcenc-sse2.exe -ch Desk.exr ASTC4x4_HDR.astc 4x4 -thorough

The -ch means compress (c) and HDR (h). A lower-case h means RGB are HDR (linear
AND may be outside 0 to 1 range) but alpha A is LDR (0 to 1 linear). A capital
H would mean all 4 channels (RGBA) are HDR.

astcenc-sse2.exe -cl TacentTestPattern32.tga ASTC4x4_LDR.astc 4x4 -thorough
The 'l' means LDR. However this one is not used for the LDR test images. The
test pattern is in sRGB space so we need:

astcenc-sse2.exe -cs TacentTestPattern32.tga ASTC4x4_LDR.astc 4x4 -thorough
The 's' means sRGB with a linear LDR alpha, which is what we have.
