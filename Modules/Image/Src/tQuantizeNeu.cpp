// tQuantizeNeu.cpp
//
// This module implements the NeuQuant (Neural-Net) Quantization Algorithm written by Anthony Dekker. Only the parts
// modified by me are under the ISC licence. The majority is under what looks like the MIT licence, The original
// author's licence can be found below. Modifications include:
// * Placing it in a namespace.
// * Consolidating the state parameters so that it is threadsafe (no global state).
// * Bridging to a standardized Tacent interface.
// * Replacing the inxsearch and inxbuild with red-mean perceptual colour distance metric to choose best colours.
// * Support for an arbitrary number of colours between 2 and 256.
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
// Here is the original copyright:
//
// Copyright (c) 1994 Anthony Dekker
//
// NEUQUANT Neural-Net quantization algorithm by Anthony Dekker, 1994. See "Kohonen neural networks for optimal colour
// quantization" in "Network: Computation in Neural Systems" Vol. 5 (1994) pp351-367. for a discussion of the algorithm.
// See also http://members.ozemail.com.au/~dekker/NEUQUANT.HTML.
//
// Any party obtaining a copy of these files from the author, directly or indirectly, is granted, free of charge, a full
// and unrestricted irrevocable, world-wide, paid up, royalty-free, nonexclusive right and license to deal in this
// software and documentation files (the "Software"), including without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons who receive copies
// from any such party to do so, with the only requirement being that this copyright notice remain intact.

#include <cstdint>
#include <Math/tColour.h>
#include "Image/tQuantize.h"
namespace tImage {


namespace tQuantizeNeu
{
	// For 256 colours, fixed arrays need 8kb, plus space for the image.
	const int maxnetsize		= 256;					// Maximum network size (number of colours).

	// Four primes near 500 - assume no image has a length so large that it is divisible by all four primes.
	const int prime1			= 499;
	const int prime2			= 491;
	const int prime3			= 487;
	const int prime4			= 503;

	const int minpicturebytes	= (3 * prime4);			// Minimum size for input image.

	// Network Definitions.
	const int maxnetpos			= maxnetsize - 1;
	const int netbiasshift		= 4;					// Bias for colour values.
	const int ncycles			= 100;					// Number of learning cycles.

	// Defs for freq and bias.
	const int intbiasshift		= 16;					// Bias for fractions.
	const int intbias			= 65536;				// 1 << intbiasshift.
	const int gammashift		= 10;					// Gamma = 1024.
	const int gamma				= 1024;					// 1 << gammashift.
	const int betashift			= 10;
	const int beta				= 64;					// intbias >> betashift beta = 1/1024.
	const int betagamma			= 65536;				// intbias << (gammashift - betashift).

	// Defs for decreasing radius factor.
	const int initrad			= 32;					// netsize >> 3 for 256 cols, radius starts
	const int radiusbiasshift	= 6;					// at 32.0 biased by 6 bits.
	const int radiusbias		= 64;					// 1 << radiusbiasshift
	const int initradius		= 2048;					// initrad * radiusbias and decreases by a
	const int radiusdec			= 30;					// factor of 1/30 each cycle.

	// Defs for decreasing alpha factor.
	const int alphabiasshift	= 10;					// Alpha starts at 1.0
	const int initalpha			= 1024;					// 1 << alphabiasshift

	// Radbias and alpharadbias used for radpower calculation.
	const int radbiasshift		= 8;
	const int radbias			= 256;					// 1 << radbiasshift
	const int alpharadbshift	= 18;					// alphabiasshift + radbiasshift
	const int alpharadbias		= 262144;

	struct State
	{
		int netsize				= maxnetsize;			// This defaults to maxnetsize but can be reduced to as low as 2 colours.
		int alphadec;							 		// Biased by 10 bits.
		unsigned char *thepicture;						// The input image itself.
		int lengthcount;						  		// lengthcount = H*W*3.
		int samplefac;									// Sampling factor 1..30.
		typedef int pixel_bgr[4];						// BGRc
		pixel_bgr network[maxnetsize];					// The network itself.
		int netindex[maxnetsize];						// For network lookup - really 256.
		int bias[maxnetsize];							// Bias and freq arrays for learning.
		int freq[maxnetsize];							// Frequency array for learning.
		int radpower[initrad];							// radpower for precomputation.
	};

	// Initialise network in range (0,0,0) to (255,255,255) and set parameters.
	int getNetwork(State&, int i, int j);
	void initnet(State&, unsigned char *thepic, int len, int sample);

	// Unbias network to give byte values 0..255 and record position i to prepare for sort.
	void unbiasnet(State&);

	// This gets the palette in the out variable.
	int getColourMap(State&, tColour3i* out);

	// Insertion sort of network and building of netindex[0..255] (to do after unbias).
	// We don't call this function since we do an exhaustive red-mean distance check.
	void inxbuild(State&);

	// Search for BGR values 0..255 (after net is unbiased) and return colour index.
	// We don't call this function since we do an exhaustive red-mean distance check.
	int inxsearch(State&, int b, int g, int r);

	// Main Learning Loop.
	void learn(State&);

	int contest(State&, int b, int g, int r);
	void altersingle(State&, int alpha, int i, int b, int g, int r);
	void alterneigh(State&, int rad, int i, int b, int g, int r);

	int FindIndexOfClosestColour_Redmean(const tColour3i* searchSpace, int searchSize, const tColour3i& searchColour);
}


int tQuantizeNeu::getNetwork(State& state, int i, int j)
{
	return state.network[i][j];
}


void tQuantizeNeu::initnet(State& state, unsigned char *thepic, int len, int sample)
{
	// Initialise network in range (0,0,0) to (255,255,255) and set parameters.
	int i;
	int *p;

	state.thepicture = thepic;
	state.lengthcount = len;
	state.samplefac = sample;

	for (i = 0; i < state.netsize; i++)
	{
		p = state.network[i];
		p[0] = p[1] = p[2] = (i << (netbiasshift + 8)) / state.netsize;
		state.freq[i] = intbias / state.netsize;	/* 1/netsize */
		state.bias[i] = 0;
	}
}


void tQuantizeNeu::unbiasnet(State& state)
{
	// Unbias network to give byte values 0..255 and record position i to prepare for sort.
	int i, j, temp;

	for (i = 0; i < state.netsize; i++)
	{
		for (j = 0; j < 3; j++)
		{
			// OLD CODE: network[i][j] >>= netbiasshift;
			// Fix based on bug report by Juergen Weigert jw@suse.de.
			temp = (state.network[i][j] + (1 << (netbiasshift - 1))) >> netbiasshift;
			if (temp > 255) temp = 255;
			state.network[i][j] = temp;
		}

		// Record colour no.
		state.network[i][3] = i;
	}
}


int tQuantizeNeu::getColourMap(State& state, tColour3i* out)
{
	// Output colour map. The palette.
	int index[maxnetsize];
	for (int i = 0; i < state.netsize; i++)
		index[state.network[i][3]] = i;

	for (int j = 0; j < state.netsize; j++)
	{
		out[j].R = uint8(state.network[j][0]);
		out[j].G = uint8(state.network[j][1]);
		out[j].B = uint8(state.network[j][2]);
	}
	return state.netsize;
}


void tQuantizeNeu::inxbuild(State& state)
{
	// Insertion sort of network and building of netindex[0..255] (to do after unbias).
	int i, j, smallpos, smallval;
	int *p, *q;
	int previouscol, startpos;

	previouscol = 0;
	startpos = 0;
	for (i = 0; i < state.netsize; i++)
	{
		p = state.network[i];
		smallpos = i;
		smallval = p[1];							// Index on g.

		// Find smallest in i..netsize-1.
		for (j = i + 1; j < state.netsize; j++)
		{
			q = state.network[j];
			if (q[1] < smallval)					// Index on g.
			{
				smallpos = j;
				smallval = q[1];					// Index on g.
			}
		}
		q = state.network[smallpos];

		// Swap p (i) and q (smallpos) entries.
		if (i != smallpos)
		{
			j = q[0];
			q[0] = p[0];
			p[0] = j;
			j = q[1];
			q[1] = p[1];
			p[1] = j;
			j = q[2];
			q[2] = p[2];
			p[2] = j;
			j = q[3];
			q[3] = p[3];
			p[3] = j;
		}

		// Smallval entry is now in position i.
		if (smallval != previouscol)
		{
			state.netindex[previouscol] = (startpos + i) >> 1;
			for (j = previouscol + 1; j < smallval; j++) state.netindex[j] = i;
			previouscol = smallval;
			startpos = i;
		}
	}
	state.netindex[previouscol] = (startpos + maxnetpos) >> 1;
	for (j = previouscol + 1; j < 256; j++)
		state.netindex[j] = maxnetpos;
}


int tQuantizeNeu::inxsearch(State& state, int b, int g, int r)
{
	// Search for BGR values 0..255 (after net is unbiased) and return colour index.
	int i, j, dist, a, bestd;
	int *p;
	int best;

	bestd = 1000;							// Biggest possible dist is 256*3.
	best = -1;
	i = state.netindex[g];					// Index on g.
	j = i - 1;								// Start at netindex[g] and work outwards.

	while ((i < state.netsize) || (j >= 0))
	{
		if (i < state.netsize)
		{
			p = state.network[i];
			dist = p[1] - g;				// Inx key.
			if (dist >= bestd)
				i = state.netsize;			// Stop iter.
			else
			{
				i++;
				if (dist < 0) dist = -dist;
				a = p[0] - b;
				if (a < 0) a = -a;
				dist += a;
				if (dist < bestd)
				{
					a = p[2] - r;
					if (a < 0) a = -a;
					dist += a;
					if (dist < bestd)
					{
						bestd = dist;
						best = p[3];
					}
				}
			}
		}
		if (j >= 0)
		{
			p = state.network[j];
			dist = g - p[1];				// Inx key - reverse dif.
			if (dist >= bestd)
				j = -1;						// Stop iter.
			else
			{
				j--;
				if (dist < 0) dist = -dist;
				a = p[0] - b;
				if (a < 0) a = -a;
				dist += a;
				if (dist < bestd)
				{
					a = p[2] - r;
					if (a < 0) a = -a;
					dist += a;
					if (dist < bestd)
					{
						bestd = dist;
						best = p[3];
					}
				}
			}
		}
	}
	return best;
}


int tQuantizeNeu::contest(State& state, int b, int g, int r)
{
	// Search for biased BGR values. Finds closest neuron (min dist) and updates freq finds best neuron (min dist-bias)
	// and returns position for frequently chosen neurons, freq[i] is high and bias[i] is negative.
	// bias[i] = gamma*((1/netsize)-freq[i])
	int i, dist, a, biasdist, betafreq;
	int bestpos, bestbiaspos, bestd, bestbiasd;
	int *p, *f, *n;

	bestd = ~(((int) 1) << 31);
	bestbiasd = bestd;
	bestpos = -1;
	bestbiaspos = bestpos;
	p = state.bias;
	f = state.freq;

	for (i = 0; i < state.netsize; i++)
	{
		n = state.network[i];
		dist = n[0] - b;
		if (dist < 0)
			dist = -dist;

		a = n[1] - g;
		if (a < 0)
			a = -a;

		dist += a;
		a = n[2] - r;
		if (a < 0)
			a = -a;

		dist += a;
		if (dist < bestd)
		{
			bestd = dist;
			bestpos = i;
		}
		biasdist = dist - ((*p) >> (intbiasshift - netbiasshift));
		if (biasdist < bestbiasd)
		{
			bestbiasd = biasdist;
			bestbiaspos = i;
		}
		betafreq = (*f >> betashift);
		*f++ -= betafreq;
		*p++ += (betafreq << gammashift);
	}
	state.freq[bestpos] += beta;
	state.bias[bestpos] -= betagamma;
	return (bestbiaspos);
}


void tQuantizeNeu::altersingle(State& state, int alpha, int i, int b, int g, int r)
{
	// Move neuron i towards biased (b,g,r) by factor alpha.
	int *n;

	n = state.network[i];					// Alter hit neuron.
	*n -= (alpha * (*n - b)) / initalpha;
	n++;
	*n -= (alpha * (*n - g)) / initalpha;
	n++;
	*n -= (alpha * (*n - r)) / initalpha;
}


void tQuantizeNeu::alterneigh(State& state, int rad, int i, int b, int g, int r)
{
	// Move adjacent neurons by precomputed alpha*(1-((i-j)^2/[r]^2)) in radpower[|i-j|].
	int j, k, lo, hi, a;
	int *p, *q;

	lo = i - rad;
	if (lo < -1)
		lo = -1;

	hi = i + rad;
	if (hi > state.netsize)
		hi = state.netsize;

	j = i + 1;
	k = i - 1;
	q = state.radpower;
	while ((j < hi) || (k > lo))
	{
		a = (*(++q));
		if (j < hi)
		{
			p = state.network[j];
			*p -= (a * (*p - b)) / alpharadbias;
			p++;
			*p -= (a * (*p - g)) / alpharadbias;
			p++;
			*p -= (a * (*p - r)) / alpharadbias;
			j++;
		}
		if (k > lo)
		{
			p = state.network[k];
			*p -= (a * (*p - b)) / alpharadbias;
			p++;
			*p -= (a * (*p - g)) / alpharadbias;
			p++;
			*p -= (a * (*p - r)) / alpharadbias;
			k--;
		}
	}
}


void tQuantizeNeu::learn(State& state)
{
	// Main Learning Loop.
	int i, j, b, g, r;
	int radius, rad, alpha, step, delta, samplepixels;
	unsigned char *p;
	unsigned char *lim;

	state.alphadec = 30 + ((state.samplefac - 1) / 3);
	p = state.thepicture;
	lim = state.thepicture + state.lengthcount;
	samplepixels = state.lengthcount / (3 * state.samplefac);
	delta = samplepixels / ncycles;
	alpha = initalpha;
	radius = initradius;

	rad = radius >> radiusbiasshift;
	if (rad <= 1)
		rad = 0;

	for (i = 0; i < rad; i++)
		state.radpower[i] = alpha * (((rad * rad - i * i) * radbias) / (rad * rad));

	if ((state.lengthcount % prime1) != 0)
	{
		step = 3 * prime1;
	}
	else
	{
		if ((state.lengthcount % prime2) != 0)
		{
			step = 3 * prime2;
		}
		else
		{
			if ((state.lengthcount % prime3) != 0)
				step = 3 * prime3;
			else
				step = 3 * prime4;
		}
	}

	i = 0;
	while (i < samplepixels)
	{
		b = p[0] << netbiasshift;
		g = p[1] << netbiasshift;
		r = p[2] << netbiasshift;
		j = contest(state, b, g, r);

		altersingle(state, alpha, j, b, g, r);
		if (rad)
			alterneigh(state, rad, j, b, g, r);			// Alter neighbours.

		p += step;
		if (p >= lim)
			p -= state.lengthcount;

		i++;
		if (i % delta == 0)
		{
			alpha -= alpha / state.alphadec;
			radius -= radius / radiusdec;
			rad = radius >> radiusbiasshift;
			if (rad <= 1)
				rad = 0;

			for (j = 0; j < rad; j++)
				state.radpower[j] = alpha * (((rad * rad - j * j) * radbias) / (rad * rad));
		}
	}
}


int tQuantizeNeu::FindIndexOfClosestColour_Redmean(const tColour3i* searchSpace, int searchSize, const tColour3i& colour)
{
	float closest = 1000.0f;
	int closestIndex = -1;

	for (int i = 0; i < searchSize; i++)
	{
		float diff = tMath::tColourDiffRedmean(colour, searchSpace[i]);
		if (diff < closest)
		{
			closest = diff;
			closestIndex = i;
		}
	}
	return closestIndex;
}


//
// The functions below make up the external interface.
//


bool tQuantizeNeu::QuantizeImage
(
	int numColours, int width, int height, const tPixel3* pixels,
	tColour3i* destPalette, uint8* destIndices,
	bool checkExact, int sampleFactor
)
{
	if ((numColours < 2) || (numColours > 256) || (width <= 0) || (height <= 0) || !pixels || !destPalette || !destIndices)
		return false;

	if ((sampleFactor < 1) || (sampleFactor > 30))
		return false;

	if (checkExact)
	{
		bool success = tQuantizeFixed::QuantizeImageExact(numColours, width, height, pixels, destPalette, destIndices);
		if (success)
			return true;
	}

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

	return (resultNumColours > 0);
}


}
