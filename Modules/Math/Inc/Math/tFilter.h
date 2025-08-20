// tFilter.h
//
// Implementations for low and high pass filters.
//
// LowPass
//
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


// An abstract base class for low-pass filters.
template<typename T> class tLowPassFilter_Base
{
public:
	virtual T GetValue() const = 0;
};


template <typename T> class tLowPassFilterFixed : public tLowPassFilter_Base<T>
{
public:
	// If tauAttenuation is false attenuation is specified as a cutoffFrequency. If tauAttenuation is true attenuation
	// is specified as 'tau'. Cutoff-frequency is in Hz above which the signal is attenuated (reduced in amplitude). Tau
	// is specified in seconds and is the time it takes for the filter's output to reach approximately 63% of a raw
	// input value. FixedDeltaTime is in seconds.
	tLowPassFilterFixed(T fixedDeltaTime, T attenuation, bool tauAttenuation = false, T initialValue = T(0.0)) :
		FixedDeltaTime(fixedDeltaTime),
		Value(initialValue)
	{
		// Calculate the weight. It doesn't change since fixedDeltaTime is unchanging.
		tiClampMin(attenuation, T(tMath::Epsilon));
		T tau = tauAttenuation ? attenuation : T(1.0) / (tMath::TwoPi * attenuation);
		Weight = T(1.0) - tMath::tExp( -FixedDeltaTime / tau );
		tiSaturate(Weight);
	}

	// Given the new value returns the filtered value. Call this every fixedDeltaTime seconds.
	T Update(T inputValue)
	{
		Value = Weight * inputValue + (T(1.0) - Weight) * Value;
		return Value;
	}

	T GetValue() const override { return Value; }

	void SetCutoffFreq(T cutoffFreq)
	{
		T tau = T(1.0) / (tMath::TwoPi * cutoffFreq);
		Weight = T(1.0) - tMath::tExp( -FixedDeltaTime / tau );
		tiSaturate(Weight);
	}

	void SetTau(T tau)
	{
		Weight = T(1.0) - tMath::tExp( -FixedDeltaTime / tau );
		tiSaturate(Weight);
	}

private:
	T FixedDeltaTime;			// Stored so we can adjust weight dynamically.
	T Weight;					// Internal only. You cannot get or set this directly.
	T Value;					// The current filtered value.
};
typedef tLowPassFilterFixed<float>  tLowPassFilter_FixFlt;
typedef tLowPassFilterFixed<double> tLowPassFilter_FixDbl;
typedef tLowPassFilterFixed<long double> tLowPassFilter_FixLongDbl;


template <typename T> class tLowPassFilterDynamic : public tLowPassFilter_Base<T>
{
public:
	// If tauAttenuation is false attenuation is specified as a cutoffFrequency. If tauAttenuation is true attenuation
	// is specified as 'tau'. Cutoff-frequency is in Hz above which the signal is attenuated (reduced in amplitude). Tau
	// is specified in seconds and is the time it takes for the filter's output to reach approximately 63% of a raw
	// input value.
	tLowPassFilterDynamic(T attenuation, bool tauAttenuation = false, T initialValue = T(0.0)) :
		Value(initialValue)
	{
		tiClampMin(attenuation, tMath::Epsilon);
		Tau = tauAttenuation ? attenuation : T(1.0) / (tMath::TwoPi * attenuation);
	}

	T Update(T inputValue, T deltaTime)
	{
		T weight = T(1.0) - tMath::tExp(-deltaTime / Tau);
		Value = weight * inputValue + (T(1.0) - weight) * Value;
		return Value;
	}

	// A less accurate but faster update that uses an approximation to compute the weight instead of exp.
	T UpdateFast(T inputValue, T deltaTime)
	{
		// Less accurate approx.
		T weight = deltaTime / (Tau + deltaTime);
		Value = weight * inputValue + (T(1.0) - weight) * Value;
		return Value;
	}

	// Call this after Update if it's difficult to store the result immediately.
	T GetValue() const override { return Value; }

	void SetCutoffFreq(T cutoffFreq)
	{
		Tau = T(1.0) / (tMath::TwoPi * cutoffFreq);
	}

	void SetTau(T tau)
	{
		Tau = tau;
	}

private:
	T Tau;
	T Value;
};
typedef tLowPassFilterDynamic<float>  tLowPassFilter_DynFlt;
typedef tLowPassFilterDynamic<double> tLowPassFilter_DynDbl;
typedef tLowPassFilterDynamic<long double> tLowPassFilter_DynLongDbl;


}


// Implementation below this line.
