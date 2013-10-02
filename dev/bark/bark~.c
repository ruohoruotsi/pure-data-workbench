/*

bark~ - A Bark-frequency based attack detector.

Copyright 2010 William Brent

bark~ is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

bark~ is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.


version 0.1.8, December 20, 2011

¥ 0.1.8 incorporates tIDLib.h to prepare for packing with timbreID
¥ 0.1.6 added a function to change minvel and went over NEGATIVE GROWTH issue described below. deleted the negative growth warning, as the logic seems solid there.  next version should look at the weighting function
¥ 0.1.5 adds a minvel parameter, though no method function to set it, so it's hard coded in _new.  need to fix that next.  also, "NEGATIVE GROWTH" warnings are thrown now that I have minvel in there, so I should think through that logic of have_hit, etc more carefully w.r.t. minvel.
¥ 0.1.4 removes BFCCs.  This can just be a Bark-based onset detector with good features
¥ 0.1.3 starts to revisit the BFCC side of things now that the barkspec and masking is working well.  not going to do any masking whatsoever to start - merely looking for the magnitude of change in each BFCC
¥ 0.1.2 makes loudness weighting optional (still no interpolation there), and makes loThresh == -1 a flag to report an attack as soon as growth shows any sign of decay
¥ 0.1.1 is a first stab at adding a weighting curve.  the one stored here is from Parmanen, Applied Acoustics 68 (2007): 58-70
¥ 0.1.0 adding peak as well as avg to the growth measurement function. getting rid of an "overlap" setting in favor of a "hop" in multiples of n=64.
¥ 0.0.9 tries to add masking as described in section 3.1 of puckette/apel. i think i did it right (with a maskDecay and maskPeriods to boot), but there might be some weird behavior there. i know it doesn't work correctly with BFCCs, only with bark spectrum.
¥ 0.0.8 just uses memset and memcpy where needed
¥ 0.0.6 makes barkSpec/BFCCs an option, and does some cleanup making proper functions for all the optional features
¥ time to add BFCCs to see if something like BFCC1 changes can be effective. very weird behavior. the plotting of growth in the help patch won't work properly.  i really don't understand why at all.  i think my growth calcs with BFCCs are correct, but it's time to look at this frame by frame in octave in order to make any progress
¥ need to add: masking, minvel, power adjustment based on loudness curves
¥ experiment with taking the logarithm of proportional change
¥Êtry detecting with a range of BFCCs
¥ 0.0.5 is a considerable cleanup

*/

#include "tIDLib.h"
#define NUMWEIGHTPOINTS 29
#define BLOCKSIZE 64

t_float weights_dB[] = {-69.9, -60.4, -51.4, -43.3, -36.6, -30.3, -24.3, -19.5, -14.8, -10.7, -7.5, -4.8, -2.6, -0.8, 0.0, 0.6, 0.5, 0.0, -0.1, 0.5, 1.5, 3.6, 5.9, 6.5, 4.2, -2.6, -10.2, -10.0, -2.8};

t_float weights_freqs[] = {20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500};

static t_class *bark_tilde_class;

typedef struct _bark_tilde
{
    t_object x_obj;
    int debug;
    int spew;
    int overlap;
    t_float sr;
    t_float n;
	t_float window;

    int dspTick;
    int hop;

	t_clock *clock;

	int powerSpectrum;
	int normalize;
	int filteravg;
	int useWeights;
	int windowFunction;

    t_float barkSpacing;
    int numFilters;
    int sizeFilterFreqs;
    t_float *x_filterFreqs;
    t_filter *x_filterbank;
	t_float *loudWeights;

	int measureTicks;
	t_float peakGrowth;
	t_float avgGrowth;
	t_float prevTotalGrowth;
    t_float loThresh;
    t_float hiThresh;
    t_float minvel;
    int loBin;
    int hiBin;
    t_float debounceTime;
    t_float maskDecay;
    int maskPeriods;
    int *numPeriods;

    int debounceActive;
    int haveHit;

	t_float weights_dB[NUMWEIGHTPOINTS];
	t_float weights_freqs[NUMWEIGHTPOINTS];
    t_float *blackman;
    t_float *cosine;
    t_float *hamming;
    t_float *hann;

    t_sample *signalBuf;
	t_float *analysisBuf;
    t_float *mask;
    t_float *growth;
	t_atom *growthList;

    t_outlet *x_outputList;
    t_outlet *x_growthOut;
    t_outlet *x_bangOut;
    t_float x_f;

} t_bark_tilde;


/* ---------------- utility functions ---------------------- */

static void bark_tilde_create_loudness_weighting(t_bark_tilde *x)
{
	int i;
	t_float bark_sum, *bark_freqs;

	bark_freqs = (t_float *)t_getbytes(x->numFilters * sizeof(t_float));

	bark_sum = x->barkSpacing;

	for(i=0; i<x->numFilters; i++)
	{
		bark_freqs[i] = tIDLib_bark2freq(bark_sum);
		bark_sum += x->barkSpacing;
	}

	for(i=0; i<x->numFilters; i++)
	{
		int nearIdx;
		t_float nearFreq, diffFreq, diffdB, dBint;

		nearIdx = tIDLib_nearestBinIndex(bark_freqs[i], weights_freqs, NUMWEIGHTPOINTS);
		nearFreq = weights_freqs[nearIdx];
		diffdB = 0;

		// this doesn't have to be if/else'd into a greater/less situation.  later on i should write a more general interpolation solution, and maybe move it up to 4 points instead.
		if(bark_freqs[i]>nearFreq)
		{
			if(nearIdx<=NUMWEIGHTPOINTS-2)
			{
				diffFreq = (bark_freqs[i] - nearFreq)/(weights_freqs[nearIdx+1] - nearFreq);
				diffdB = diffFreq * (weights_dB[nearIdx+1] - weights_dB[nearIdx]);
			}

			dBint = weights_dB[nearIdx] + diffdB;
		}
		else
		{
			if(nearIdx>0)
			{
				diffFreq = (bark_freqs[i] - weights_freqs[nearIdx-1])/(nearFreq - weights_freqs[nearIdx-1]);
				diffdB = diffFreq * (weights_dB[nearIdx] - weights_dB[nearIdx-1]);
			}

			dBint = weights_dB[nearIdx-1] + diffdB;
		}

		if(x->powerSpectrum)
			x->loudWeights[i] = pow(10.0, dBint*0.1);
		else
			x->loudWeights[i] = pow(10.0, dBint*0.05);

	}
}

/* ---------------- END utility functions ---------------------- */



/* ------------------------ bark~ -------------------------------- */

static void bark_tilde_overlap(t_bark_tilde *x, t_floatarg o)
{
	int overlap;

	overlap = o;

	// this change will be picked up in _dsp, where the filterbank will be recreated based on the samplerate sp[0]->s_sr/x->overlap;
	if(overlap > 0)
		x->overlap = overlap;
	else
		error("overlap must be at least 1.");

    post("overlap: %i", x->overlap);

    post("overlap: %i", x->overlap);
}


static void bark_tilde_windowFunction(t_bark_tilde *x, t_floatarg f)
{
    f = (f<0)?0:f;
    f = (f>4)?4:f;
	x->windowFunction = f;

	switch(x->windowFunction)
	{
		case 0:
			post("window function: rectangular.");
			break;
		case 1:
			post("window function: blackman.");
			break;
		case 2:
			post("window function: cosine.");
			break;
		case 3:
			post("window function: hamming.");
			break;
		case 4:
			post("window function: hann.");
			break;
		default:
			break;
	};
}

static void bark_tilde_thresh(t_bark_tilde *x, t_floatarg lo, t_floatarg hi)
{
	if(hi<lo)
	{
		post("bark~: warning: high threshold less than low threshold.");
		x->hiThresh = lo;
		x->loThresh = hi;

		x->loThresh = (x->loThresh<0)?-1:x->loThresh;
	}
	else
	{
		x->hiThresh = hi;
		x->loThresh = lo;

		x->loThresh = (x->loThresh<0)?-1:x->loThresh;
	}
}

static void bark_tilde_minvel(t_bark_tilde *x, t_floatarg mv)
{
	x->minvel = (mv<0)?0:mv;
}

static void bark_tilde_filter_range(t_bark_tilde *x, t_floatarg lo, t_floatarg hi)
{
	if(hi<lo)
	{
		post("bark~: warning: high bin less than low bin.");
		x->hiBin = lo;
		x->loBin = hi;

		x->loBin = (x->loBin<0) ? 0 : x->loBin;
		x->hiBin = (x->hiBin>=x->numFilters) ? x->numFilters-1 : x->hiBin;
	}
	else
	{
		x->hiBin = hi;
		x->loBin = lo;

		x->loBin = (x->loBin<0) ? 0 : x->loBin;
		x->hiBin = (x->hiBin>=x->numFilters) ? x->numFilters-1 : x->hiBin;
	}
}

static void bark_tilde_mask(t_bark_tilde *x, t_floatarg per, t_floatarg dec)
{
	x->maskPeriods = per;
	x->maskDecay = dec;

	x->maskPeriods = (x->maskPeriods<0) ? 0 : x->maskPeriods;
	x->maskDecay = (x->maskDecay<0.05) ? 0.05 : x->maskDecay;
	x->maskDecay = (x->maskDecay>0.95) ? 0.95 : x->maskDecay;
}

static void bark_tilde_debounce(t_bark_tilde *x, t_floatarg db)
{
	if(x->debounceTime>=0)
	{
		x->debounceTime = db;
	}
	else
		error("bark~: debounce time must be >= 0");
}

static void bark_tilde_print(t_bark_tilde *x)
{
	post("window size: %i", (int)x->window);
	post("hop: %i samples", x->hop);
	post("Bark spacing: %0.2f", x->barkSpacing);
	post("no. of filters: %i", x->numFilters);
	post("bin range: %i through %i (inclusive)", x->loBin, x->hiBin);
	post("low thresh: %0.2f, high thresh: %0.2f", x->loThresh, x->hiThresh);
	post("minvel: %f", x->minvel);
	post("mask periods: %i, mask decay: %0.2f", x->maskPeriods, x->maskDecay);
	post("debounce time: %0.2f", x->debounceTime);
	post("normalization: %i", x->normalize);
	post("filter averaging: %i", x->filteravg);
	post("power spectrum: %i", x->powerSpectrum);
	post("spew mode: %i", x->spew);
	post("debug mode: %i", x->debug);
}

static void bark_tilde_debug(t_bark_tilde *x, t_floatarg debug)
{
	debug = (debug<0)?0:debug;
	debug = (debug>1)?1:debug;
	x->debug = debug;

	if(x->debug)
		post("debug mode ON");
	else
		post("debug mode OFF");
}

static void bark_tilde_spew(t_bark_tilde *x, t_floatarg spew)
{
	spew = (spew<0)?0:spew;
	spew = (spew>1)?1:spew;
	x->spew = spew;

	post("spew mode: %i", x->spew);
}

static void bark_tilde_measure(t_bark_tilde *x, t_floatarg m)
{
	m = (m<0)?0:m;
	m = (m>1)?1:m;

	if((int)m)
	{
		post("measuring average growth...");
		x->measureTicks = 0;
	}
	else
	{
		post("no. of ticks: %i", x->measureTicks);
		post("average growth: %f", x->avgGrowth/x->measureTicks);
		post("peak growth: %f", x->peakGrowth);
		x->avgGrowth = 0.0;
		x->peakGrowth = 0.0;
		x->measureTicks = -1;
	}
}

static void bark_tilde_use_weights(t_bark_tilde *x, t_floatarg w)
{
	w = (w<0)?0:w;
	w = (w>1)?1:w;
	x->useWeights = w;

	if(x->useWeights)
		post("using loudness weighting");
	else
		post("using unweighted spectrum");
}


static void bark_tilde_hat(t_bark_tilde *x, t_floatarg filt)
{
	int i, idx;

	idx = filt;

	if(idx>=x->numFilters)
		error("filter %i does not exist.", idx);
	else if(idx < 0)
		error("filter %i does not exist.", idx);
	else
	{
		post("size[%i]: %i", idx, x->x_filterbank[idx].size);
		for(i=0; i<x->x_filterbank[idx].size; i++)
			post("val %i: %f", i, x->x_filterbank[idx].filter[i]);

		post("idxLo: %i, idxHi: %i", x->x_filterbank[idx].indices[0], x->x_filterbank[idx].indices[1]);
	}
}


static void bark_tilde_filterFreqs(t_bark_tilde *x)
{
	int i;

	for(i=1; i<=x->numFilters; i++)
		post("filterFreq[%i]: %f", i-1, x->x_filterFreqs[i]);
}


static void bark_tilde_filter_avg(t_bark_tilde *x, t_floatarg avg)
{
	avg = (avg<0) ? 0 : avg;
	avg = (avg>1) ? 1 : avg;
	x->filteravg = avg;

	if(x->filteravg)
		post("averaging energy in filters.");
	else
		post("summing energy in filters.");
}


// magnitude spectrum == 0, power spectrum == 1
static void bark_tilde_powerSpectrum(t_bark_tilde *x, t_floatarg spec)
{
	spec = (spec<0) ? 0 : spec;
	spec = (spec>1) ? 1 : spec;
	x->powerSpectrum = spec;

	bark_tilde_create_loudness_weighting(x);

	if(x->powerSpectrum)
		post("using power spectrum.");
	else
		post("using magnitude spectrum.");
}


static void bark_tilde_normalize(t_bark_tilde *x, t_floatarg norm)
{
	norm = (norm<0) ? 0 : norm;
	norm = (norm>1) ? 1 : norm;
	x->normalize = norm;

	if(x->normalize)
		post("spectrum normalization ON.");
	else
		post("spectrum normalization OFF.");
}


static void bark_tilde_clear_hit(t_bark_tilde *x)
{
    x->debounceActive = 0;
}


static void *bark_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
    t_bark_tilde *x = (t_bark_tilde *)pd_new(bark_tilde_class);
	int i, isPow2;

	x->x_bangOut = outlet_new(&x->x_obj, &s_bang);
	x->x_growthOut = outlet_new(&x->x_obj, &s_float);
	x->x_outputList = outlet_new(&x->x_obj, gensym("list"));
	s=s;

	if(argc==3)
	{
		x->window = atom_getfloat(argv);  // should perform a check for >64 && power of two
		isPow2 = (int)x->window && !( ((int)x->window-1) & (int)x->window );

		if(!isPow2)
		{
			error("requested window size is not a power of 2. default value of 1024 used instead.");
			x->window = 1024;
		};

		x->hop = atom_getfloat(argv+1);

		if(x->hop%BLOCKSIZE)
		{
			x->hop = (x->hop/(t_float)BLOCKSIZE);
			x->hop *= BLOCKSIZE;
			error("requested hop is not a multiple of 64. %i used instead.", x->hop);
		};

		x->barkSpacing = atom_getfloat(argv+2);
	}

	if(argc==2)
	{
		x->window = atom_getfloat(argv);  // should perform a check for >64 && power of two
		isPow2 = (int)x->window && !( ((int)x->window-1) & (int)x->window );

		if(!isPow2)
		{
			error("requested window size is not a power of 2. default value of 1024 used instead.");
			x->window = 1024;
		};

		x->hop = atom_getfloat(argv+1);

		if(x->hop%BLOCKSIZE)
		{
			x->hop = (x->hop/(t_float)BLOCKSIZE);
			x->hop *= BLOCKSIZE;
			error("requested hop is not a multiple of 64. %i used instead.", x->hop);
		};

		x->barkSpacing = 0.5;
	}

	if(argc==1)
	{
		x->window = atom_getfloat(argv);  // should perform a check for >64 && power of two
		isPow2 = (int)x->window && !( ((int)x->window-1) & (int)x->window );

		if(!isPow2)
		{
			error("requested window size is not a power of 2. default value of 1024 used instead.");
			x->window = 1024;
		};

		x->hop = 64;
		x->barkSpacing = 0.5;
	}

	if(argc==0)
	{
		x->window = 2048;
		x->hop = 128;
		x->barkSpacing = 0.5;
	}

	x->debug = 0;
	x->spew = 0;
	x->overlap = 1;
	x->sr = 44100.0;
	x->n = BLOCKSIZE;
	x->windowFunction = 4; // 4 is hann window
	x->powerSpectrum = 1; // choose mag (0) or power (1) spec
	x->normalize = 0;
	x->filteravg = 0;
	x->dspTick = 0;
	x->loThresh = 3;
	x->hiThresh = 7;
	x->minvel = 1.0;
	x->haveHit = 0;
	x->debounceTime = 20;
	x->debounceActive = 0;
	x->maskDecay = 0.7;
	x->maskPeriods = 4;
	x->numFilters = 0;
	x->avgGrowth = 0.0;
	x->peakGrowth = 0.0;
	x->prevTotalGrowth = 0.0;
	x->measureTicks = -1;
	x->useWeights = 0;

    x->clock = clock_new(x, (t_method)bark_tilde_clear_hit);


	x->signalBuf = (t_sample *)t_getbytes(x->window * sizeof(t_sample));
	x->analysisBuf = (t_float *)t_getbytes(x->window * sizeof(t_float));

	for(i=0; i<x->window; i++)
	{
		x->signalBuf[i] = 0.0;
		x->analysisBuf[i] = 0.0;
	}

  	x->blackman = (t_float *)t_getbytes(x->window*sizeof(t_float));
  	x->cosine = (t_float *)t_getbytes(x->window*sizeof(t_float));
  	x->hamming = (t_float *)t_getbytes(x->window*sizeof(t_float));
  	x->hann = (t_float *)t_getbytes(x->window*sizeof(t_float));

 	// initialize signal windowing functions
	tIDLib_blackmanWindow(x->blackman, x->window);
	tIDLib_cosineWindow(x->cosine, x->window);
	tIDLib_hammingWindow(x->hamming, x->window);
	tIDLib_hannWindow(x->hann, x->window);

	// grab memory
	x->x_filterbank = (t_filter *)t_getbytes(0);
	x->x_filterFreqs = (t_float *)t_getbytes(0);

	x->numFilters = tIDLib_getBarkFilterSpacing(&x->x_filterFreqs, x->sizeFilterFreqs, x->barkSpacing, x->sr);

	x->sizeFilterFreqs = x->numFilters+2;

	tIDLib_createFilterbank(x->x_filterFreqs, &x->x_filterbank, 0, x->numFilters, x->window, x->sr);

	x->loBin = 0;
	x->hiBin = x->numFilters-1;

	x->mask = (t_float *)t_getbytes(x->numFilters*sizeof(t_float));
	x->growth = (t_float *)t_getbytes(x->numFilters*sizeof(t_float));
	x->numPeriods = (int *)t_getbytes(x->numFilters*sizeof(int));
	x->growthList = (t_atom *)t_getbytes(x->numFilters*sizeof(t_atom));
	x->loudWeights = (t_float *)t_getbytes(x->numFilters*sizeof(t_float));

	for(i=0; i<x->numFilters; i++)
	{
		x->mask[i] = 0.0;
		x->growth[i] = 0.0;
		x->numPeriods[i] = 0.0;
		SETFLOAT(x->growthList+i, 0.0);
		x->loudWeights[i] = 0.0;
	}

	bark_tilde_create_loudness_weighting(x);

    post("bark~ 0.1.8: window size: %i, hop: %i", (int)x->window, x->hop);

    return (x);
}


static t_int *bark_tilde_perform(t_int *w)
{
    int i, j, n, window, windowHalf;
    t_float totalGrowth, totalVel, *windowFuncPtr;

    t_bark_tilde *x = (t_bark_tilde *)(w[1]);

    t_sample *in = (t_float *)(w[2]);
    n = w[3];

 	window = x->window;
 	windowHalf = window*0.5;

 	// shift signal buffer contents back.
	for(i=0; i<(window-n); i++)
		x->signalBuf[i] = x->signalBuf[i+n];

	// write new block to end of signal buffer.
	for(i=0; i<n; i++)
		x->signalBuf[window-n+i] = in[i];

	x->dspTick += n;

	if(x->dspTick >= x->hop)
	{
		x->dspTick = 0;
	 	totalGrowth = 0.0;
	 	totalVel = 0.0;

		// set window function
		windowFuncPtr = x->hann; //default case to get rid of compile warning

		switch(x->windowFunction)
		{
			case 0:
				break;
			case 1:
				windowFuncPtr = x->blackman;
				break;
			case 2:
				windowFuncPtr = x->cosine;
				break;
			case 3:
				windowFuncPtr = x->hamming;
				break;
			case 4:
				windowFuncPtr = x->hann;
				break;
			default:
				break;
		};

		// if windowFunction == 0, skip the windowing (rectangular)
		if(x->windowFunction>0)
			for(i=0; i<window; i++, windowFuncPtr++)
				x->analysisBuf[i] = x->signalBuf[i] * *windowFuncPtr;
		else
			for(i=0; i<window; i++, windowFuncPtr++)
				x->analysisBuf[i] = x->signalBuf[i];

		mayer_realfft(window, x->analysisBuf);

		// calculate the power spectrum in place. we'll overwrite the first N/2+1 points in x->analysisBuf with the magnitude spectrum, as this is all that's used below in filterbank_multiply()
		x->analysisBuf[0] = x->analysisBuf[0] * x->analysisBuf[0];  // DC
		x->analysisBuf[windowHalf] = x->analysisBuf[windowHalf] * x->analysisBuf[windowHalf];  // Nyquist

		for(i=(window-1), j=1; i>windowHalf; i--, j++)
			x->analysisBuf[j] = (x->analysisBuf[j]*x->analysisBuf[j]) + (x->analysisBuf[i]*x->analysisBuf[i]);

		// optional use of power/magnitude spectrum
		if(!x->powerSpectrum)
			for(i=0; i<=windowHalf; i++)
				x->analysisBuf[i] = sqrt(x->analysisBuf[i]);

 		tIDLib_filterbankMultiply(x->analysisBuf, x->normalize, x->filteravg, x->x_filterbank, x->numFilters);

 		// optional loudness weighting
 		if(x->useWeights)
			for(i=0; i<x->numFilters; i++)
				x->analysisBuf[i] *= x->loudWeights[i];

		for(i=0; i<x->numFilters; i++)
			totalVel += x->analysisBuf[i];

		// init growth list to zero
		for(i=0; i<x->numFilters; i++)
			x->growth[i] = 0.0;

		for(i=0; i<x->numFilters; i++)
		{
			// from p.3 of Puckette/Apel/Zicarelli, 1998
			// salt divisor with + 1.0e-15 in case previous power was zero
			if(x->analysisBuf[i] > x->mask[i])
				x->growth[i] = x->analysisBuf[i]/(x->mask[i] + 1.0e-15) - 1.0;

			if(i>=x->loBin && i<=x->hiBin && x->growth[i]>0)
				totalGrowth += x->growth[i];

			SETFLOAT(x->growthList+i, x->growth[i]);
		}

		if(x->measureTicks >= 0)
		{
			if(totalGrowth > x->peakGrowth)
				x->peakGrowth = totalGrowth;

			x->avgGrowth += totalGrowth;
			x->measureTicks++;
		}

		if(totalVel >= x->minvel && totalGrowth > x->hiThresh && !x->haveHit && !x->debounceActive)
		{
 			if(x->debug)
 				post("peak: %f", totalGrowth);

			x->haveHit = 1;
			x->debounceActive = 1;
			clock_delay(x->clock, x->debounceTime); // wait debounceTime ms before allowing another attack
		}
		else if(x->haveHit && x->loThresh>0 && totalGrowth < x->loThresh) // if loThresh is an actual value (not -1), then wait until growth drops below that value before reporting attack
		{
			if(x->debug)
				post("drop: %f", totalGrowth);

			x->haveHit = 0;

			// don't output data if spew will do it anyway below
			if(!x->spew)
			{
				outlet_list(x->x_outputList, 0, x->numFilters, x->growthList);
				outlet_float(x->x_growthOut, totalGrowth);
			}

			outlet_bang(x->x_bangOut);
		}
		else if(x->haveHit && x->loThresh<0 && totalGrowth < x->prevTotalGrowth) // if loThresh == -1, report attack as soon as growth shows any decay at all
		{
			if(x->debug)
				post("drop: %f", totalGrowth);

			x->haveHit = 0;

			// don't output data if spew will do it anyway below
			if(!x->spew)
			{
				outlet_list(x->x_outputList, 0, x->numFilters, x->growthList);
				outlet_float(x->x_growthOut, totalGrowth);
			}

			outlet_bang(x->x_bangOut);
		}


		if(x->spew)
		{
			outlet_list(x->x_outputList, 0, x->numFilters, x->growthList);
			outlet_float(x->x_growthOut, totalGrowth);
		}

		// update mask
		for(i=0; i<x->numFilters; i++)
		{
			if(x->analysisBuf[i] > x->mask[i])
			{
				x->mask[i] = x->analysisBuf[i];
				x->numPeriods[i] = 0;
			}
			else
				if(++x->numPeriods[i] >= x->maskPeriods)
					x->mask[i] *= x->maskDecay;
		}

		x->prevTotalGrowth = totalGrowth;
	}

    return (w+4);
}


static void bark_tilde_dsp(t_bark_tilde *x, t_signal **sp)
{
	dsp_add(
		bark_tilde_perform,
		3,
		x,
		sp[0]->s_vec,
		sp[0]->s_n
	);

	if(sp[0]->s_sr != x->sr*x->overlap)
	{
	    int i, oldNumFilters;

		x->sr = sp[0]->s_sr/x->overlap;

        oldNumFilters = x->numFilters;
        x->numFilters = tIDLib_getBarkFilterSpacing(&x->x_filterFreqs, x->sizeFilterFreqs, x->barkSpacing, x->sr);

        x->sizeFilterFreqs = x->numFilters+2;

        tIDLib_createFilterbank(x->x_filterFreqs, &x->x_filterbank, oldNumFilters, x->numFilters, x->window, x->sr);

        x->loBin = 0;
        x->hiBin = x->numFilters-1;

        x->mask = (t_float *)t_resizebytes(x->mask, oldNumFilters*sizeof(t_float), x->numFilters*sizeof(t_float));
        x->growth = (t_float *)t_resizebytes(x->growth, oldNumFilters*sizeof(t_float), x->numFilters*sizeof(t_float));
        x->numPeriods = (int *)t_resizebytes(x->numPeriods, oldNumFilters*sizeof(int), x->numFilters*sizeof(int));
        x->growthList = (t_atom *)t_resizebytes(x->growthList, oldNumFilters*sizeof(t_atom), x->numFilters*sizeof(t_atom));
        x->loudWeights = (t_float *)t_resizebytes(x->loudWeights, oldNumFilters*sizeof(t_float), x->numFilters*sizeof(t_float));

        for(i=0; i<x->numFilters; i++)
        {
	        x->mask[i] = 0.0;
	        x->growth[i] = 0.0;
	        x->numPeriods[i] = 0.0;
	        SETFLOAT(x->growthList+i, 0.0);
	        x->loudWeights[i] = 0.0;
        }
	}
};


static void bark_tilde_free(t_bark_tilde *x)
{
	int i;

	// free the output list
	t_freebytes(x->growthList, x->numFilters * sizeof(t_atom));

	// free the input buffer memory
    t_freebytes(x->signalBuf, x->window*sizeof(t_sample));

	// free the analysis buffer memory
    t_freebytes(x->analysisBuf, x->window*sizeof(t_float));

	// free the mask memory
    t_freebytes(x->mask, x->numFilters*sizeof(t_float));

	// free the growth record memory
    t_freebytes(x->growth, x->numFilters*sizeof(t_float));

	// free the mask counter memory
    t_freebytes(x->numPeriods, x->numFilters*sizeof(int));

	// free the loudness weights memory
    t_freebytes(x->loudWeights, x->numFilters*sizeof(t_float));

	// free the window memory
	t_freebytes(x->blackman, x->window*sizeof(t_float));
	t_freebytes(x->cosine, x->window*sizeof(t_float));
	t_freebytes(x->hamming, x->window*sizeof(t_float));
	t_freebytes(x->hann, x->window*sizeof(t_float));

	// free the filterFreqs memory
	t_freebytes(x->x_filterFreqs, x->sizeFilterFreqs*sizeof(t_float));

	// free the filterbank memory
	for(i=0; i<x->numFilters; i++)
		t_freebytes(x->x_filterbank[i].filter, x->x_filterbank[i].size*sizeof(t_float));

	t_freebytes(x->x_filterbank, x->numFilters*sizeof(t_filter));

	clock_free(x->clock);
}


void bark_tilde_setup(void)
{
    bark_tilde_class =
    class_new(
    	gensym("bark~"),
    	(t_newmethod)bark_tilde_new,
    	(t_method)bark_tilde_free,
        sizeof(t_bark_tilde),
        CLASS_DEFAULT,
        A_GIMME,
		0
    );

    CLASS_MAINSIGNALIN(bark_tilde_class, t_bark_tilde, x_f);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_overlap,
		gensym("overlap"),
		A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_windowFunction,
		gensym("window_function"),
		A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_thresh,
		gensym("thresh"),
        A_DEFFLOAT,
        A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_minvel,
		gensym("minvel"),
        A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_filter_range,
		gensym("filter_range"),
        A_DEFFLOAT,
        A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_mask,
		gensym("mask"),
        A_DEFFLOAT,
        A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_debounce,
		gensym("debounce"),
		A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_print,
		gensym("print"),
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_debug,
		gensym("debug"),
		A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_spew,
		gensym("spew"),
		A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_measure,
		gensym("measure"),
		A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_use_weights,
		gensym("loudness"),
		A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_filterFreqs,
		gensym("filter_freqs"),
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_hat,
		gensym("hat"),
        A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_filter_avg,
		gensym("filter_avg"),
		A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_powerSpectrum,
		gensym("power_spectrum"),
		A_DEFFLOAT,
		0
	);

	class_addmethod(
		bark_tilde_class,
        (t_method)bark_tilde_normalize,
		gensym("normalize"),
		A_DEFFLOAT,
		0
	);

    class_addmethod(
    	bark_tilde_class,
    	(t_method)bark_tilde_dsp,
    	gensym("dsp"),
    	0
    );
}

