/*

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.


version 0.6.0C, November 24, 2011

¥ 0.6.0C is adding mel compatibility for the create_filterbank function.  It currently does so with a barkFlag to choose between bark/mel.  Also changing all variable names to cut down on underscores. should also change FFT_unpack so that it includes nyquist (window/2 + 1)
¥ 0.6.0 marks the use of a timbreID.h header file

*/

#include <math.h>	// for fabs(), cos(), etc
//#include <string.h>	// for memset()
//#include <limits.h>
#include <float.h>
#include "m_pd.h"

#define MAXWINDOWSIZE 131072
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

typedef struct filter
{
	t_float *filter;
	int indices[2];
	int size;
} t_filter;


/* ---------------- filterbank functions ---------------------- */

int tIDLib_nearestBinIndex(t_float target, t_float *binFreqs, int n)
{
	int i, idx;
	t_float dist, bestDist;

	bestDist = FLT_MAX;
	dist = 0.0;
	idx = 0;

	for(i=0; i<n; i++)
		if((dist=fabs(binFreqs[i]-target)) < bestDist)
		{
			bestDist = dist;
			idx = i;
		}

	return(idx);
}


t_float tIDLib_bark2freq(t_float bark)
{
	t_float freq;

	freq = 1960/(26.81/(bark+0.53) - 1);
	freq = (freq<0)?0:freq;

	return(freq);
}


t_float tIDLib_mel2freq(t_float mel)
{
	t_float freq;

	freq = 700 * (exp(mel/1127.01048) - 1);
	freq = (freq<0)?0:freq;

	return(freq);
}


int tIDLib_getBarkFilterSpacing(t_float **filterFreqs, int oldSizeFilterFreqs, t_float spacing, t_float sr)
{
	int sizeFilterFreqs, newNumFilters;
	t_float sum, sumFreq;

	if(spacing<0.05)
	{
		spacing = 0.05;
		post("Bark spacing must be between 0.05 and 6.5 Barks");
	}
	else if (spacing>6.5)
	{
		spacing = 6.5;
		post("Bark spacing must be between 0.05 and 6.5 Barks");
	}

	sum = sumFreq = 0.0;
	sizeFilterFreqs = 0;

	(*filterFreqs) = (t_float *)t_resizebytes((*filterFreqs), oldSizeFilterFreqs*sizeof(t_float), 0);

	while(sumFreq<=sr*0.5)
	{
		(*filterFreqs) = (t_float *)t_resizebytes((*filterFreqs), sizeFilterFreqs*sizeof(t_float), (sizeFilterFreqs+1)*sizeof(t_float));

		if(sum==0.0)
			(*filterFreqs)[sizeFilterFreqs] = 0.0;
		else
			(*filterFreqs)[sizeFilterFreqs] = sumFreq;

		sizeFilterFreqs++;
		sum += spacing;

		// don't go over 26 Barks, as it's near Nyquist for 44.1kHz, and the formula returns negative results for anything over about 26 Barks. The function here clips anything below 0 to 0, so the while() could be infinite if we only test for < sr*0.5 as with mels
		if(sum>=26.0)
			break;
		else
			sumFreq = tIDLib_bark2freq(sum);
	}

	newNumFilters = sizeFilterFreqs-2;  // sizeFilterFreqs-2 is the correct number of filters, since we don't count the start point of the first filter, or the finish point of the last filter

	return(newNumFilters);
}


int tIDLib_getMelFilterSpacing(t_float **filterFreqs, int oldSizeFilterFreqs, t_float spacing, t_float sr)
{
	int sizeFilterFreqs, newNumFilters;
	t_float sum, sumFreq;

	if(spacing<5)
	{
		spacing = 5;
		post("mel spacing must be between 5 and 1000 mels");
	}
	else if (spacing>1000)
	{
		spacing = 1000;
		post("mel spacing must be between 5 and 1000 mels");
	}

	sum = sumFreq = 0.0;
	sizeFilterFreqs = 0;

	(*filterFreqs) = (t_float *)t_resizebytes((*filterFreqs), 	oldSizeFilterFreqs*sizeof(t_float), 0);

	while(sumFreq<=sr*0.5)
	{
		(*filterFreqs) = (t_float *)t_resizebytes((*filterFreqs), 	sizeFilterFreqs*sizeof(t_float), (sizeFilterFreqs+1)*sizeof(t_float));

		if(sum==0.0)
			(*filterFreqs)[sizeFilterFreqs] = 0.0;
		else
			(*filterFreqs)[sizeFilterFreqs] = sumFreq;

		sizeFilterFreqs++;
		sum += spacing;

		sumFreq = tIDLib_mel2freq(sum);
	}

	newNumFilters = sizeFilterFreqs-2;  // sizeFilterFreqs-2 is the correct number of filters, since we don't count the start point of the first filter, or the finish point of the last filter

	return(newNumFilters);
}


void tIDLib_createFilterbank(t_float *filterFreqs, t_filter **filterbank, int oldNumFilters, int newNumFilters, t_float window, t_float sr)
{
	int i, j, k, windowHalfPlus1;
	t_float *binFreqs;

	windowHalfPlus1 = (window*0.5)+1;

	// create local memory
	binFreqs = (t_float *)t_getbytes(windowHalfPlus1*sizeof(t_float));

	// free memory for each filter
	for(i=0; i<oldNumFilters; i++)
		t_freebytes((*filterbank)[i].filter, (*filterbank)[i].size*sizeof(t_float));

	(*filterbank) = (t_filter *)t_resizebytes((*filterbank), oldNumFilters*sizeof(t_filter), newNumFilters*sizeof(t_filter));

	// initialize indices
	for(i=0; i<newNumFilters; i++)
		for(j=0; j<2; j++)
			(*filterbank)[i].indices[j] = 0;

	// initialize filterbank sizes
	for(i=0; i<newNumFilters; i++)
		(*filterbank)[i].size = 0;

	// first, find the actual freq for each bin based on current window size
	for(i=0; i<windowHalfPlus1; i++)
		binFreqs[i] = (sr*i)/window;


	// finally, build the filterbank
	for(i=1; i<=newNumFilters; i++)
	{
		int startIdx, peakIdx, finishIdx, filterWidth;
		startIdx = peakIdx = finishIdx = 0;

		startIdx = tIDLib_nearestBinIndex(filterFreqs[i-1], binFreqs, windowHalfPlus1);
		peakIdx = tIDLib_nearestBinIndex(filterFreqs[i], binFreqs, windowHalfPlus1);
		finishIdx = tIDLib_nearestBinIndex(filterFreqs[i+1], binFreqs, windowHalfPlus1);

		// grab memory for this filter
 		filterWidth = finishIdx-startIdx + 1;
  		(*filterbank)[i-1].size = filterWidth; // store the sizes for freeing memory later

 		(*filterbank)[i-1].filter = (t_float *)t_getbytes((*filterbank)[i-1].size*sizeof(t_float));

 		// initialize this filter
		for(j=0; j<(*filterbank)[i-1].size; j++)
			(*filterbank)[i-1].filter[j] = 0.0;

 		// SPECIAL CASE FOR DUPLICATE START/PEAK/FINISH
		if(filterWidth==1)
			(*filterbank)[i-1].filter[0] = 1;
		else if(filterWidth==2)
		{
			(*filterbank)[i-1].filter[0] = 0.5;
			(*filterbank)[i-1].filter[1] = 0.5;
		}
		else
		{
			// UPWARD
			for(j=startIdx, k=0; j<=peakIdx; j++, k++)
			{
				// (binFreqs(j)-start)/(peak-start);
				(*filterbank)[i-1].filter[k] = (binFreqs[j] - filterFreqs[i-1])/(filterFreqs[i] - filterFreqs[i-1]);

				// all references to filterbank or indices must use [i-1] since we start at 1 and end at numFilters

				(*filterbank)[i-1].filter[k] = ((*filterbank)[i-1].filter[k]<0.0)?0.0:(*filterbank)[i-1].filter[k];

				(*filterbank)[i-1].filter[k] = ((*filterbank)[i-1].filter[k]>1.0)?1.0:(*filterbank)[i-1].filter[k];
			};


			// DOWNWARD (k continues where it is...)
			for(j=(peakIdx+1); j<finishIdx; j++, k++)
			{
				(*filterbank)[i-1].filter[k] = (filterFreqs[i+1] - binFreqs[j])/(filterFreqs[i+1] - filterFreqs[i]);

				(*filterbank)[i-1].filter[k] = ((*filterbank)[i-1].filter[k]<0.0)?0.0:(*filterbank)[i-1].filter[k];

				(*filterbank)[i-1].filter[k] = ((*filterbank)[i-1].filter[k]>1.0)?1.0:(*filterbank)[i-1].filter[k];
			};
		};

		(*filterbank)[i-1].indices[0] = startIdx;
		(*filterbank)[i-1].indices[1] = finishIdx;
	}

	// free local memory
	t_freebytes(binFreqs, windowHalfPlus1*sizeof(t_float));
}
/* ---------------- END filterbank functions ---------------------- */




/* ---------------- dsp utility functions ---------------------- */

void tIDLib_blackmanWindow(t_float *wptr, int n)
{
	int i;

	for(i=0; i<n; i++, wptr++)
    	*wptr = 0.42 - (0.5 * cos(2*M_PI*i/n)) + (0.08 * cos(4*M_PI*i/n));
}

void tIDLib_cosineWindow(t_float *wptr, int n)
{
	int i;

	for(i=0; i<n; i++, wptr++)
    	*wptr = sin(M_PI*i/n);
}

void tIDLib_hammingWindow(t_float *wptr, int n)
{
	int i;

	for(i=0; i<n; i++, wptr++)
    	*wptr = 0.5 - (0.46 * cos(2*M_PI*i/n));
}

void tIDLib_hannWindow(t_float *wptr, int n)
{
	int i;

	for(i=0; i<n; i++, wptr++)
    	*wptr = 0.5 * (1 - cos(2*M_PI*i/n));
}

int tIDLib_signum(t_sample sample)
{
	int sign, crossings;
	
	sign=0;
	crossings=0;
	
	if(sample>0)
		sign = 1;
	else if(sample<0)
		sign = -1;
	else
		sign = 0;
	
	return(sign);
}

t_float tIDLib_zeroCrossingRate(int n, t_sample *input)
{
	int i;
	t_float crossings;
	
	crossings = 0.0;
	
	for(i=1; i<n; i++)
		crossings += fabs(tIDLib_signum(input[i]) - tIDLib_signum(input[i-1]));
	
	crossings *= 0.5;
		
	return(crossings);
}

void tIDLib_realfftUnpack(int n, int nHalf, t_sample *input, t_sample *imag)
{
	int i, j;

	imag[0]=0.0;  // DC

	for(i=(n-1), j=1; i>nHalf; i--, j++)
		imag[j] = input[i];

	imag[nHalf]=0.0;  // Nyquist
}

void tIDLib_power(int n, t_sample *real, t_sample *imag)
{
	while (n--)
    {
        *real = (*real * *real) + (*imag * *imag);
        real++;
        imag++;
    };
}

void tIDLib_mag(int n, t_sample *power)
{
	while (n--)
	{
	    *power = sqrt(*power);
	    power++;
	}
}

void tIDLib_normal(int n, t_sample *input)
{
	int i;
	t_float sum, normScalar;
	
	sum=0;
	normScalar=0;
	
	for(i=0; i<n; i++)
		sum += input[i];

	sum = (sum==0)?1.0:sum;
			
	normScalar = 1.0/sum;
	
	for(i=0; i<n; i++)
		input[i] *= normScalar;
}

void tIDLib_log(int n, t_sample *spectrum)
{
	while (n--)
    {
		// if to protect against log(0)
    	if(*spectrum==0)
    		*spectrum = 0;
    	else
	        *spectrum = log(*spectrum);
	        
        spectrum++;
    };
}

void tIDLib_realifft(int n, int nHalf, t_sample *real, t_sample *imag)
{
    int i, j;
    t_float nRecip;
    
	for(i=(nHalf+1), j=(nHalf-1); i<n; i++, j--)
		real[i] = imag[j];
			
    mayer_realifft(n, real);
    
    nRecip = 1.0/(t_float)n;
    
    // normalize by 1/N, since mayer doesn't
	for(i=0; i<n; i++)
		real[i] *= nRecip;
}

void tIDLib_cosineTransform(t_float *output, t_sample *input, int numFilters)
{
	int i, k;
	t_float piOverNfilters;

	piOverNfilters = M_PI/numFilters; // save multiple divides below

	for(i=0; i<numFilters; i++)
    {
	   	output[i] = 0;

		for(k=0; k<numFilters; k++)
 	    	output[i] += input[k] * cos(i * (k+0.5) * piOverNfilters);  // DCT-II
	};
}


void tIDLib_filterbankMultiply(t_sample *spectrum, int normalize, int filterAvg, t_filter *filterbank, int numFilters)
{
	int i, j, k;
	t_float sum, sumsum, *filterPower;

	// create local memory
	filterPower = (t_float *)t_getbytes(numFilters*sizeof(t_float));

 	sumsum = 0;

	for(i=0; i<numFilters; i++)
	{
	   	sum = 0;

		for(j=filterbank[i].indices[0], k=0; j<=filterbank[i].indices[1]; j++, k++)
	    	sum += spectrum[j] * filterbank[i].filter[k];

		if(filterAvg)
			sum /= k;

		filterPower[i] = sum;  // get the total power.  another weighting might be better.

 		sumsum += sum;  // normalize so power in all bands sums to 1
	};

	if(normalize)
	{
		// prevent divide by 0
		if(sumsum==0)
			sumsum=1;
		else
			sumsum = 1/sumsum; // take the reciprocal here to save a divide below
	}
	else
		sumsum=1;

	for(i=0; i<numFilters; i++)
		spectrum[i] = filterPower[i]*sumsum;

	// free local memory
	t_freebytes(filterPower, numFilters*sizeof(t_float));
}

/* ---------------- END dsp utility functions ---------------------- */

