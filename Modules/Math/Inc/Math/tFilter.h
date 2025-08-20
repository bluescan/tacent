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
// frequency at which signals begin to be attenuated (reduced in amplitude). Lower cutoff frequencies result in more
// smoothing/jitter-reduction but introduce more lag.
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
class tLowPassFilter_FixFlt
{
public:
	// If tauAttenuation is false attenuation is specified as a cutoffFrequency. If tauAttenuation is true attenuation
	// is specified as 'tau'. Cutoff-frequency is in Hz above which the signal is attenuated (reduced in amplitude). Tau
	// is specified in seconds and is the time it takes for the filter's output to reach approximately 63% of a raw
	// input value. FixedDeltaTime is in seconds.
	tLowPassFilter_FixFlt(float fixedDeltaTime, float attenuation, bool tauAttenuation = false, float initialValue = 0.0f) :
		FixedDeltaTime(fixedDeltaTime),
		Value(initialValue)
	{
		// Calculate the weight. It doesn't change since fixedDeltaTime is unchanging.
		tiClampMin(attenuation, tMath::Epsilon);
		float tau = tauAttenuation ? attenuation : 1.0f / (tMath::TwoPi * attenuation);
		Weight = 1.0f - tMath::tExp( -FixedDeltaTime / tau );
		tiSaturate(Weight);
	}

	// Given the new value returns the filtered value. Call this every fixedDeltaTime seconds.
    float Update(float inputValue)
	{
        Value = Weight * inputValue + (1.0f - Weight) * Value;
        return Value;
    }

	float GetValue() const { return Value; }

	void SetCutoffFreq(float cutoffFreq)
	{
		float tau = 1.0f / (tMath::TwoPi * cutoffFreq);
		Weight = 1.0f - tMath::tExp( -FixedDeltaTime / tau );
		tiSaturate(Weight);
	}

	void SetTau(float tau)
	{
		Weight = 1.0f - tMath::tExp( -FixedDeltaTime / tau );
		tiSaturate(Weight);
	}

private:
	float FixedDeltaTime;			// Stored so we can adjust weight dynamically.
    float Weight;					// Internal only. You cannot get or set this directly.
    float Value;					// The current filtered value.
};


class tLowPassFilter_FixDbl
{
public:
	// If tauAttenuation is false attenuation is specified as a cutoffFrequency. If tauAttenuation is true attenuation
	// is specified as 'tau'. Cutoff-frequency is in Hz above which the signal is attenuated (reduced in amplitude). Tau
	// is specified in seconds and is the time it takes for the filter's output to reach approximately 63% of a raw
	// input value. FixedDeltaTime is in seconds.
	tLowPassFilter_FixDbl(double fixedDeltaTime, double attenuation, bool tauAttenuation = false, double initialValue = 0.0f) :
		FixedDeltaTime(fixedDeltaTime),
		Value(initialValue)
	{
		// Calculate the weight. It doesn't change since fixedDeltaTime is unchanging.
		tiClampMin(attenuation, tMath::EpsilonDbl);
		double tau = tauAttenuation ? attenuation : 1.0/ (tMath::TwoPi * attenuation);
		Weight = 1.0 - tMath::tExp( -FixedDeltaTime / tau );
		tiSaturate(Weight);
	}

	// Given the new value returns the filtered value. Call this every fixedDeltaTime seconds.
    double Update(double inputValue)
	{
        Value = Weight * inputValue + (1.0 - Weight) * Value;
        return Value;
    }

	// Call this after Update if it's difficult to store the result immediately.
	double GetValue() const { return Value; }

	void SetCutoffFreq(double cutoffFreq)
	{
		double tau = 1.0 / (tMath::TwoPi * cutoffFreq);
		Weight = 1.0 - tMath::tExp( -FixedDeltaTime / tau );
		tiSaturate(Weight);
	}

	void SetTau(double tau)
	{
		Weight = 1.0 - tMath::tExp( -FixedDeltaTime / tau );
		tiSaturate(Weight);
	}

private:
	double FixedDeltaTime;		// Stored so we can adjust weight dynamically.
    double Weight;				// Internal only. You cannot get or set this directly.
    double Value;				// The current filtered value.
};


class tLowPassFilter_DynFlt
{
public:
	// If tauAttenuation is false attenuation is specified as a cutoffFrequency. If tauAttenuation is true attenuation
	// is specified as 'tau'. Cutoff-frequency is in Hz above which the signal is attenuated (reduced in amplitude). Tau
	// is specified in seconds and is the time it takes for the filter's output to reach approximately 63% of a raw
	// input value.
	tLowPassFilter_DynFlt(float attenuation, bool tauAttenuation = false, float initialValue = 0.0f) :
		Value(initialValue)
	{
		tiClampMin(attenuation, tMath::Epsilon);
		Tau = tauAttenuation ? attenuation : 1.0f / (tMath::TwoPi * attenuation);
	}

	float Update(float inputValue, float deltaTime)
	{
		float weight = 1.0f - tMath::tExp(-deltaTime / Tau);
		Value = weight * inputValue + (1.0f - weight) * Value;
		return Value;
	}

	// A less accurate but faster update that uses an approximation to compute the weight instead of exp.
	float UpdateFast(float inputValue, float deltaTime)
	{
		// Less accurate approx.
		float weight = deltaTime / (Tau + deltaTime);
		Value = weight * inputValue + (1.0f - weight) * Value;
		return Value;
	}

	// Call this after Update if it's difficult to store the result immediately.
	float GetValue() const { return Value; }

	void SetCutoffFreq(float cutoffFreq)
	{
		Tau = 1.0f / (tMath::TwoPi * cutoffFreq);
	}

	void SetTau(float tau)
	{
		Tau = tau;
	}

private:
	float Tau;
	float Value;
};


class tLowPassFilter_DynDbl
{
public:
	// If tauAttenuation is false attenuation is specified as a cutoffFrequency. If tauAttenuation is true attenuation
	// is specified as 'tau'. Cutoff-frequency is in Hz above which the signal is attenuated (reduced in amplitude). Tau
	// is specified in seconds and is the time it takes for the filter's output to reach approximately 63% of a raw
	// input value.
	tLowPassFilter_DynDbl(double attenuation, bool tauAttenuation = false, double initialValue = 0.0)
	{
		tiClampMin(attenuation, tMath::EpsilonDbl);
		Tau = tauAttenuation ? attenuation : 1.0 / (tMath::TwoPi * attenuation);
		Value = initialValue;
	}

	double Update(double inputValue, double deltaTime)
	{
		double weight = 1.0 - tMath::tExp(-deltaTime / Tau);
		Value = weight * inputValue + (1.0 - weight) * Value;
		return Value;
	}

	// A less accurate but faster update that uses an approximation to compute the weight instead of exp.
	double UpdateFast(double inputValue, double deltaTime)
	{
		// Less accurate approx.
		double weight = deltaTime / (Tau + deltaTime);
		Value = weight * inputValue + (1.0 - weight) * Value;
		return Value;
	}

	// Call this after Update if it's difficult to store the result immediately.
	double GetValue() const { return Value; }

	void SetCutoffFreq(double cutoffFreq)
	{
		Tau = 1.0 / (tMath::TwoPi * cutoffFreq);
	}

	void SetTau(double tau)
	{
		Tau = tau;
	}

private:
	float Tau;
	float Value;
};


}


// Implementation below this line.
