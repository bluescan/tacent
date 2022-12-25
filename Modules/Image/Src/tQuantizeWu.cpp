// tQuantizeWu.cpp
//
// This module implements Wu quantization by Xiaolin Wu. The original header is included below. Modifications include:
// * Placing it in a namespace.
// * Consolidating the state parameters so that it is threadsafe (no global state).
// * Bridging to a standardized Tacent interface.
// * Convert to C++ syntax (original is pure C).
// * No exit or printf on error.
//
// The algrithm works well for larger numbers of colours (generally 128 to 256 or 255) but it can handle values as
// low as 2.
//
// Modifications Copyright (c) 2022 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.
//
// Here is the original code header from Xiaolin Wu:
//
// Having received many constructive comments and bug reports about my previous C implementation of my color quantizer
// (Graphics Gems vol. II, p. 126-133), I am posting the following second version of my program (hopefully 100% healthy)
// as a reply to all those who are interested in the problem.
//
// C Implementation of Wu's Color Quantizer (v. 2) (see Graphics Gems vol. II, pp. 126-133)
// Author: Xiaolin Wu, Dept. of Computer Science, Univ. of Western Ontario, London, Ontario N6A 5B7, wu@csd.uwo.ca
// Algorithm: Greedy orthogonal bipartition of RGB space for variance minimization aided by inclusion-exclusion tricks.
// For speed no nearest neighbor search is done. Slightly better performance can be expected by more sophisticated but
// more expensive versions. The author thanks Tom Lane at Tom_Lane@G.GP.CS.CMU.EDU for much of additional documentation
// and a cure to a previous bug.
//
// Free to distribute, comments and suggestions are appreciated.
#include <Math/tColour.h>
#include "Image/tQuantize.h"
namespace tImage {


namespace tQuantizeWu
{
	// Constants.
	const int MaxColour		= 256;		// For 256 colours, fixed arrays need 8kb, plus space for the image.
	const int Red			= 2;
	const int Green			= 1;
	const int Blue			= 0;

	// We're putting all state on the stack (heap would work too). No globals. This allows the quantizer to be thread-safe.
	struct State
	{
		/* Histogram is in elements 1..HISTSIZE along each axis,
		* element 0 is for base or marginal value
		* NB: these must start out 0!
		*/
		float		m2[33][33][33];
		int32	wt[33][33][33], mr[33][33][33],	mg[33][33][33],	mb[33][33][33];
		uint8   *Ir, *Ig, *Ib;
		int			size; /*image size*/
		int		K;	/*color look-up table size*/
		uint16 *Qadd;
	};

	struct Box
	{
		// X0 is min value, exclusive. X1 is max value, inclusive.
		int r0; int r1; int g0; int g1; int b0; int b1;
		int vol;
	};

	// Build 3-D color histogram of counts, r/g/b, c^2. At conclusion of the histogram step, we can interpret:
 	// wt[r][g][b] = sum over voxel of P(c)
 	// mr[r][g][b] = sum over voxel of r*P(c),  similarly for mg, mb
 	// m2[r][g][b] = sum over voxel of c^2*P(c)
	// Actually each of these should be divided by 'size' to give the usual interpretation of P() as ranging from 0 to 1
	// but we needn't do that here.
	void Hist3d(State&, int32* vwt, int32* vmr, int32* vmg, int32* vmb, float* m2);

	// Compute cumulative moments. We now convert histogram into moments so that we can rapidly calculate the sums of
	// the above quantities over any desired box.
	void M3d(int32* vwt, int32* vmr, int32* vmg, int32* vmb, float* m2);
	
	// Compute sum over a box of any given statistic.
	int32 Vol(Box* cube, int32 mmt[33][33][33]);

	// The next two routines allow a slightly more efficient calculation of Vol() for a proposed subbox of a given box.
	// The sum of Top() and Bottom() is the Vol() of a subbox split in the given direction and with the specified new
	// upper bound.
	//
	// Compute part of Vol(cube, mmt) that doesn't depend on r1, g1, or b1 (depending on dir).
	int32 Bottom(Box* cube, uint8 dir, int32 mmt[33][33][33]);

	// Compute remainder of Vol(cube, mmt), substituting pos for r1, g1, or b1 (depending on dir).
	int32 Top(Box* cube, uint8 dir, int pos, int32 mmt[33][33][33]);

	// Compute the weighted variance of a box NB: as with the raw statistics, this is really the variance * size.
	float Var(State&, Box* cube);

	// We want to minimize the sum of the variances of two subboxes. The sum(c^2) terms can be ignored since their sum
	// over both subboxes is the same (the sum for the whole box) no matter where we split. The remaining terms have a
	// minus sign in the variance formula, so we drop the minus sign and MAXIMIZE the sum of the two terms.
	float Maximize(State&, Box* cube, uint8 dir, int first, int last, int* cut, int32 whole_r, int32 whole_g, int32 whole_b, int32 whole_w);

	int Cut(State&, Box* set1, Box* set2);
	void Mark(Box* cube, int label, uint8* tag);

	void Quantize(int numColours, int width, int height);

}


void tQuantizeWu::Hist3d(State& state, int32* vwt, int32* vmr, int32* vmg, int32* vmb, float* m2)
{
	int ind, r, g, b;
	int inr, ing, inb, table[256];
	int32 i;

	for (i=0; i<256; ++i)
		table[i]=i*i;

	state.Qadd = (uint16 *)malloc(sizeof(short int)*state.size);
	for (i=0; i<state.size; ++i)
	{
		r = state.Ir[i]; g = state.Ig[i]; b = state.Ib[i];
		inr=(r>>3)+1; 
		ing=(g>>3)+1; 
		inb=(b>>3)+1; 
		state.Qadd[i]=ind=(inr<<10)+(inr<<6)+inr+(ing<<5)+ing+inb;
		/*[inr][ing][inb]*/
		++vwt[ind];
		vmr[ind] += r;
		vmg[ind] += g;
		vmb[ind] += b;
		m2[ind] += (float)(table[r]+table[g]+table[b]);
	}
}


void tQuantizeWu::M3d(int32* vwt, int32* vmr, int32* vmg, int32* vmb, float* m2)
{
	uint16 ind1, ind2;
	uint8 i, r, g, b;
	int32 line, line_r, line_g, line_b, area[33], area_r[33], area_g[33], area_b[33];
	float line2, area2[33];

	for(r=1; r<=32; ++r)
	{
		for(i=0; i<=32; ++i)
		{
			area[i]=area_r[i]=area_g[i]=area_b[i]=0;
			area2[i] = 0.0f;
		}
		for(g=1; g<=32; ++g)
		{
			line = line_r = line_g = line_b = 0;
			line2 = 0.0f;
			for(b=1; b<=32; ++b)
			{
				ind1 = (r<<10) + (r<<6) + r + (g<<5) + g + b; /* [r][g][b] */
				line += vwt[ind1];
				line_r += vmr[ind1]; 
				line_g += vmg[ind1]; 
				line_b += vmb[ind1];
				line2 += m2[ind1];
				area[b] += line;
				area_r[b] += line_r;
				area_g[b] += line_g;
				area_b[b] += line_b;
				area2[b] += line2;
				ind2 = ind1 - 1089; /* [r-1][g][b] */
				vwt[ind1] = vwt[ind2] + area[b];
				vmr[ind1] = vmr[ind2] + area_r[b];
				vmg[ind1] = vmg[ind2] + area_g[b];
				vmb[ind1] = vmb[ind2] + area_b[b];
				m2[ind1] = m2[ind2] + area2[b];
			}
		}
	}
}


int32 tQuantizeWu::Vol(Box* cube, int32 mmt[33][33][33])
{
	return( mmt[cube->r1][cube->g1][cube->b1] 
	   -mmt[cube->r1][cube->g1][cube->b0]
	   -mmt[cube->r1][cube->g0][cube->b1]
	   +mmt[cube->r1][cube->g0][cube->b0]
	   -mmt[cube->r0][cube->g1][cube->b1]
	   +mmt[cube->r0][cube->g1][cube->b0]
	   +mmt[cube->r0][cube->g0][cube->b1]
	   -mmt[cube->r0][cube->g0][cube->b0] );
}


int32 tQuantizeWu::Bottom(Box* cube, uint8 dir, int32 mmt[33][33][33])
{
	switch(dir)
	{
		case Red:
			return( -mmt[cube->r0][cube->g1][cube->b1]
				+mmt[cube->r0][cube->g1][cube->b0]
				+mmt[cube->r0][cube->g0][cube->b1]
				-mmt[cube->r0][cube->g0][cube->b0] );
			break;

		case Green:
			return( -mmt[cube->r1][cube->g0][cube->b1]
				+mmt[cube->r1][cube->g0][cube->b0]
				+mmt[cube->r0][cube->g0][cube->b1]
				-mmt[cube->r0][cube->g0][cube->b0] );
			break;

		case Blue:
			return( -mmt[cube->r1][cube->g1][cube->b0]
				+mmt[cube->r1][cube->g0][cube->b0]
				+mmt[cube->r0][cube->g1][cube->b0]
				-mmt[cube->r0][cube->g0][cube->b0] );
			break;
	}

	return 0;
}


int32 tQuantizeWu::Top(Box* cube, uint8 dir, int pos, int32 mmt[33][33][33])
{
	switch(dir)
	{
		case Red:
			return( mmt[pos][cube->g1][cube->b1] 
			-mmt[pos][cube->g1][cube->b0]
			-mmt[pos][cube->g0][cube->b1]
			+mmt[pos][cube->g0][cube->b0] );
			break;
		case Green:
			return( mmt[cube->r1][pos][cube->b1] 
			-mmt[cube->r1][pos][cube->b0]
			-mmt[cube->r0][pos][cube->b1]
			+mmt[cube->r0][pos][cube->b0] );
			break;
		case Blue:
			return( mmt[cube->r1][cube->g1][pos]
			-mmt[cube->r1][cube->g0][pos]
			-mmt[cube->r0][cube->g1][pos]
			+mmt[cube->r0][cube->g0][pos] );
			break;
	}
	return 0;
}


float tQuantizeWu::Var(State& state, Box* cube)
{
	float dr, dg, db, xx;

	dr = float( Vol(cube, state.mr) );
	dg = float( Vol(cube, state.mg) );
	db = float( Vol(cube, state.mb) );
	xx =  state.m2[cube->r1][cube->g1][cube->b1] 
		-state.m2[cube->r1][cube->g1][cube->b0]
		-state.m2[cube->r1][cube->g0][cube->b1]
		+state.m2[cube->r1][cube->g0][cube->b0]
		-state.m2[cube->r0][cube->g1][cube->b1]
		+state.m2[cube->r0][cube->g1][cube->b0]
		+state.m2[cube->r0][cube->g0][cube->b1]
		-state.m2[cube->r0][cube->g0][cube->b0];

	return( xx - (dr*dr+dg*dg+db*db)/(float)Vol(cube,state.wt) );
}


float tQuantizeWu::Maximize(State& state, Box* cube, uint8 dir, int first, int last, int* cut, int32 whole_r, int32 whole_g, int32 whole_b, int32 whole_w)
{
	int32 half_r, half_g, half_b, half_w;
	int32 base_r, base_g, base_b, base_w;
	int i;
	float temp, max;

	base_r = Bottom(cube, dir, state.mr);
	base_g = Bottom(cube, dir, state.mg);
	base_b = Bottom(cube, dir, state.mb);
	base_w = Bottom(cube, dir, state.wt);
	max = 0.0;
	*cut = -1;
	for(i=first; i<last; ++i)
	{
		half_r = base_r + Top(cube, dir, i, state.mr);
		half_g = base_g + Top(cube, dir, i, state.mg);
		half_b = base_b + Top(cube, dir, i, state.mb);
		half_w = base_w + Top(cube, dir, i, state.wt);
		/* now half_x is sum over lower half of box, if split at i */
		if (half_w == 0)
		{	  /* subbox could be empty of pixels! */
			continue;			 /* never split into an empty box */
		}
		else
			temp = ((float)half_r*half_r + (float)half_g*half_g + (float)half_b*half_b)/half_w;

		half_r = whole_r - half_r;
		half_g = whole_g - half_g;
		half_b = whole_b - half_b;
		half_w = whole_w - half_w;

		/* subbox could be empty of pixels! */
		if (half_w == 0)
		{
			continue;		/* never split into an empty box */
		}
		else
			temp += ((float)half_r*half_r + (float)half_g*half_g + (float)half_b*half_b)/half_w;

		if (temp > max) { max=temp; *cut=i; }
	}
	return(max);
}


int tQuantizeWu::Cut(State& state, Box* set1, Box* set2)
{
	uint8 dir;
	int32 cutr, cutg, cutb;
	float maxr, maxg, maxb;
	int32 whole_r, whole_g, whole_b, whole_w;

	whole_r = Vol(set1, state.mr);
	whole_g = Vol(set1, state.mg);
	whole_b = Vol(set1, state.mb);
	whole_w = Vol(set1, state.wt);

	maxr = Maximize(state, set1, Red, set1->r0+1, set1->r1, &cutr,
			whole_r, whole_g, whole_b, whole_w);
	maxg = Maximize(state, set1, Green, set1->g0+1, set1->g1, &cutg,
			whole_r, whole_g, whole_b, whole_w);
	maxb = Maximize(state, set1, Blue, set1->b0+1, set1->b1, &cutb,
			whole_r, whole_g, whole_b, whole_w);

	if( (maxr>=maxg)&&(maxr>=maxb) )
	{
		dir = Red;
		if (cutr < 0) return 0; /* can't split the box */
	}
	else if ((maxg>=maxr)&&(maxg>=maxb))
		dir = Green;
	else
		dir = Blue;

	set2->r1 = set1->r1;
	set2->g1 = set1->g1;
	set2->b1 = set1->b1;

	switch (dir)
	{
		case Red:
			set2->r0 = set1->r1 = cutr;
			set2->g0 = set1->g0;
			set2->b0 = set1->b0;
			break;
		case Green:
			set2->g0 = set1->g1 = cutg;
			set2->r0 = set1->r0;
			set2->b0 = set1->b0;
			break;
		case Blue:
			set2->b0 = set1->b1 = cutb;
			set2->r0 = set1->r0;
			set2->g0 = set1->g0;
			break;
	}
	set1->vol=(set1->r1-set1->r0)*(set1->g1-set1->g0)*(set1->b1-set1->b0);
	set2->vol=(set2->r1-set2->r0)*(set2->g1-set2->g0)*(set2->b1-set2->b0);
	return 1;
}


void tQuantizeWu::Mark(Box* cube, int label, uint8* tag)
{
	int r, g, b;

	for(r=cube->r0+1; r<=cube->r1; ++r)
		for(g=cube->g0+1; g<=cube->g1; ++g)
			for(b=cube->b0+1; b<=cube->b1; ++b)
				tag[(r<<10) + (r<<6) + r + (g<<5) + g + b] = label;
}


void tQuantizeWu::Quantize(int numColours, int width, int height)
{
	Box	cube[MaxColour];
	uint8	*tag;
	uint8	lut_r[MaxColour], lut_g[MaxColour], lut_b[MaxColour];
	int		next;
	int32	i, weight;
	int	k;
	float		vv[MaxColour], temp;

	//////////////
	State state;
	state.K = numColours;

	state.size = width*height;
	/* input R,G,B components into Ir, Ig, Ib; set size to width*height */

	/////////////
	//printf("no. of colors:\n");
	//scanf("%d", &K);

	///////////////Hist3d(wt, mr, mg, mb, m2); printf("Histogram done\n");
	Hist3d(state, (int32*)state.wt, (int32*)state.mr, (int32*)state.mg, (int32*)state.mb, (float*)state.m2);
	free(state.Ig); free(state.Ib); free(state.Ir);

	M3d((int32*)state.wt, (int32*)state.mr, (int32*)state.mg, (int32*)state.mb, (float*)state.m2);

	cube[0].r0 = cube[0].g0 = cube[0].b0 = 0;
	cube[0].r1 = cube[0].g1 = cube[0].b1 = 32;
	next = 0;
	for(i=1; i<state.K; ++i)
	{
		if (Cut(state, &cube[next], &cube[i]))
		{
			/* volume test ensures we won't try to cut one-cell box */
			vv[next] = (cube[next].vol>1) ? Var(state, &cube[next]) : 0.0;
			vv[i] = (cube[i].vol>1) ? Var(state, &cube[i]) : 0.0;
		}
		else
		{
			vv[next] = 0.0;		/* don't try to split this box again */
			i--;				/* didn't create box i */
		}
		next = 0; temp = vv[0];
		for(k=1; k<=i; ++k)
			if (vv[k] > temp)
			{
				temp = vv[k]; next = k;
			}
		if (temp <= 0.0)
		{
			state.K = i+1;
			/////////////////
			fprintf(stderr, "Only got %d boxes\n", state.K);
			break;
		}
	}

	////////////////////
	printf("Partition done\n");

	/* the space for array m2 can be freed now */
	//////////////////////////////////
	tag = (uint8 *)malloc(33*33*33);
	if (tag==NULL)
	{
		printf("Not enough space\n"); exit(1);
	}

	for(k=0; k<state.K; ++k)
	{
		Mark(&cube[k], k, tag);
		weight = Vol(&cube[k], state.wt);
		if (weight)
		{
			lut_r[k] = Vol(&cube[k], state.mr) / weight;
			lut_g[k] = Vol(&cube[k], state.mg) / weight;
			lut_b[k] = Vol(&cube[k], state.mb) / weight;
		}
		else
		{
			///////////////////////
			fprintf(stderr, "bogus box %d\n", k);
			lut_r[k] = lut_g[k] = lut_b[k] = 0;		
		}
	}

	for(i=0; i<state.size; ++i) state.Qadd[i] = tag[state.Qadd[i]];
	
	/* output lut_r, lut_g, lut_b as color look-up table contents, Qadd as the quantized image (array of table addresses). */
	// Qadd needs to be freed!  Or supplied in the first place.
}


//
// The functions below make up the external interface.
//


bool tQuantizeWu::QuantizeImage
(
	int numColours, int width, int height, const tPixel3* pixels, tColour3i* destPalette, uint8* destIndices,
	bool checkExact
)
{
	if ((numColours < 2) || (numColours > 256) || (width <= 0) || (height <= 0) || !pixels || !destPalette || !destIndices)
		return false;

	if (checkExact)
	{
		bool success = tQuantize::QuantizeImageExact(numColours, width, height, pixels, destPalette, destIndices);
		if (success)
			return true;
	}

#if 0
	State state;
	state.netsize = numColours;

	initnet(state, (uint8*)pixels, width*height*3, sampleFactor);
	learn(state);
	unbiasnet(state);
	int resultNumColours = getColourMap(state, destPalette);

	// Exhaustive redmean is better.
	// inxbuild(state);
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			const tPixel3& pixel = pixels[x + y*width];
			destIndices[x + y*width] = FindIndexOfClosestColour_Redmean(destPalette, numColours, pixel);

			// Exhaustive redmean is better.
			// destIndices[x + y*width] = inxsearch(state, pixel.R, pixel.G, pixel.B);
		}
	}
	#endif

	return true;
}


bool tQuantizeWu::QuantizeImage
(
	int numColours, int width, int height, const tPixel* pixels, tColour3i* destPalette, uint8* destIndices,
	bool checkExact
)
{
	if ((numColours < 2) || (numColours > 256) || (width <= 0) || (height <= 0) || !pixels || !destPalette || !destIndices)
		return false;

	tPixel3* pixels3 = new tPixel3[width*height];
	for (int p = 0; p < width*height; p++)
		pixels3[p].Set( pixels[p].R, pixels[p].G, pixels[p].B );

	bool success = QuantizeImage(numColours, width, height, pixels3, destPalette, destIndices, checkExact);
	delete[] pixels3;
	return success;
}


}
