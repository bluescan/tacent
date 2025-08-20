// tFilter.h
//
// Implementations for low and high pass filters.
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
#include <Foundation/tConstants.h>
#include <Foundation/tFundamentals.h>
namespace tMath
{


// A simple first order low-pass filter using exponential smoothing. Uses a weight E [0.0, 1.0] to combine the current
// input with the previous filtered value -- the bigger the weight, the less lag and less filtering (higher cutoff
// frequency). Specifying the weight directly is not useful as it doesn't take into account how often the update
// function is called. For this reason delta-time and cutoff-frequency are used instead. Cutoff-frequency determines the
// frequency at which signals begin to be attenuated. Lower cutoff frequencies result in more smoothing/jitter-reduction
// but introduce more lag.
//
// Optionally the delta-time and a time-constant (Tau) can be used. Tau is related to cutoff-frequency and represents
// the time (in seconds) it takes for the filter's output to reach approximately 63% of a raw input value. Tau can be
// useful if you have imperical jitter data on a hardware sensor that specifies jitter in seconds or milli-seconds.
//
// In some cases you know the update period or frequency and it doesn't change between updates. In some cases the time
// delta between updates is variable. To accomodate this there are separate classes for fixed or dynamic time-steps.
// Classes with Fix in the name should be used if you know the timestep and it is constant. Classes with Dyn should be
// used if you need to supply the delta-time each update. There are also variants that use either float or double (for
// more accuracy) -- these have either Dbl or Flt in their name.
//
// @todo Tau constructors.
class tLowPassFilter_FixFlt
{
public:
	tLowPassFilter_FixFlt(float cutoffFrequency, float fixedDeltaTime)
	{
		// Calculate the weight. It doesn't change since fixedDeltaTime is unchanging.
		// Tau = 1 / (2 * pi * cutoffFrequency).  Weight = 1 - exp(-deltaTime / tau).
		tiClampMin(cutoffFrequency, tMath::Epsilon);
		float tau = 1.0f / (tMath::TwoPi * cutoffFrequency);
		Weight = 1.0f - tMath::tExp( -fixedDeltaTime / tau );
		tiSaturate(Weight);
		Value = 0.0f;
	}

	// Given the new value returns the filtered value. Call this every fixedDeltaTime seconds.
    float Update(float inputValue)
	{
        Value = Weight * inputValue + (1.0f - Weight) * Value;
        return Value;
    }

private:
    double Weight;
    double Value;
};


class tLowPassFilter_FixDbl
{
public:
	tLowPassFilter_FixDbl(double cutoffFrequency, double fixedDeltaTime)
	{
		// Calculate the weight. It doesn't change since fixedDeltaTime is unchanging.
		// Tau = 1 / (2 * pi * cutoffFrequency).  Weight = 1 - exp(-deltaTime / tau).
		tiClampMin(cutoffFrequency, tMath::EpsilonDbl);
		double tau = 1.0 / (tMath::TwoPi * cutoffFrequency);
		Weight = 1.0 - tMath::tExp( -fixedDeltaTime / tau );
		tiSaturate(Weight);
		Value = 0.0;
	}

	// Given the new value returns the filtered value. Call this every fixedDeltaTime seconds.
    double Update(double inputValue)
	{
        Value = Weight * inputValue + (1.0 - Weight) * Value;
        return Value;
    }

private:
    double Weight;
    double Value;
};


class LowPassFilter_DynFlt
{
public:
	LowPassFilter_DynFlt(float cutoffFrequency)
	{
		tiClampMin(cutoffFrequency, tMath::Epsilon);
		Tau = 1.0f / (tMath::TwoPi * cutoffFrequency);
		Value = 0.0f;
	}

	float Update(float inputValue, float deltaTime)
	{
		// Less accurate approx.
		// weight = deltaTime / (Tau + deltaTime);
		float weight = 1.0f - tMath::tExp(-deltaTime / Tau);
		Value = weight * inputValue + (1.0f - weight) * Value;
		return Value;
	}

private:
	double Tau;
	double Value;
};


class LowPassFilter_DynDbl
{
public:
	LowPassFilter_DynDbl(double cutoffFrequency)
	{
		tiClampMin(cutoffFrequency, tMath::EpsilonDbl);
		Tau = 1.0 / (tMath::TwoPi * cutoffFrequency);
		Value = 0.0;
	}

	double Update(double inputValue, double deltaTime)
	{
		// Less accurate approx.
		// weight = deltaTime / (Tau + deltaTime);
		double weight = 1.0 - tMath::tExp(-deltaTime / Tau);
		Value = weight * inputValue + (1.0 - weight) * Value;
		return Value;
	}

private:
	double Tau;
	double Value;
};


}


// Implementation below this line.
