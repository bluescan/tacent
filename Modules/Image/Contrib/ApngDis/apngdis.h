/* APNG Disassembler 2.9
 *
 * Deconstructs APNG files into individual frames.
 *
 * http://apngdis.sourceforge.net
 *
 * Copyright (c) 2010-2017 Max Stepin
 * maxst at users.sourceforge.net
 *
 * zlib license
 * ------------
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */
//////////////////////////////////////////////////////////////////////////////////////
// This is a modified version of apngdis											//
//																					//
// The modifications were made by Tristan Grimmer and are primarily to remove		//
// main so the functionality can be called directly from other source files.		//
// A header file has been created to allow external access.							//
//																					//
// All modifications should be considered to be covered by the zlib license above.	//
//////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

// @tacent Added version string. Value taken from printf in main().
#define APNGDIS_VERSION_STRING "2.9"

// @tacent Put it all in a namespace.
namespace APngDis
{


struct Image
{
	typedef unsigned char * ROW;
	unsigned int w, h, bpp, delay_num, delay_den;
	unsigned char * p;
	ROW * rows;
	Image() : w(0), h(0), bpp(0), delay_num(1), delay_den(10), p(0), rows(0) { }
	~Image() { }
	void init(unsigned int w1, unsigned int h1, unsigned int bpp1)
	{
		w = w1; h = h1; bpp = bpp1;
		int rowbytes = w * bpp;
		rows = new ROW[h];
		rows[0] = p = new unsigned char[h * rowbytes];
		for (unsigned int j=1; j<h; j++)
		rows[j] = rows[j-1] + rowbytes;
	}
	void free() { delete[] rows; delete[] p; }
};

// Returns -1 on error.
int load_apng(const char * szIn, std::vector<Image>& img);


}
