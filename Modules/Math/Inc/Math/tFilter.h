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


// An abstract base class for 1st order low-pass filters.
template<typename T> class tLowPassFilter
{
public:
	tLowPassFilter(T initialValue)																						: Value(initialValue) { }
	
	// Call this after Update if it's difficult to store the result immediately.
	T GetValue() const																									{ return Value; }
	virtual void SetCutoffFreq(T cutoffFreq)																			= 0;
	virtual void SetTau(T tau)																							= 0;

protected:
	T ComputeTau(T cutoffFreq) const																					{ return T(1.0) / (tMath::TwoPi * cutoffFreq); }
	T ComputeWeight(T dt, T tau) const																					{ return tSaturate(T(1.0) - tMath::tExp(-dt / tau)); }
	T ComputeWeightFast(T dt, T tau) const																				{ return tSaturate(dt / (tau + dt)); }
	T UpdateValue(T weight, T inputValue)																				{ Value = weight*inputValue + (T(1.0) - weight)*Value; return Value; }

	// The current filtered value.
	T Value;
};


template <typename T> class tLowPassFilterFixed : public tLowPassFilter<T>
{
public:
	// If tauAttenuation is false attenuation is specified as a cutoffFrequency. If tauAttenuation is true attenuation
	// is specified as 'tau'. Cutoff-frequency is in Hz above which the signal is attenuated (reduced in amplitude). Tau
	// is specified in seconds and is the time it takes for the filter's output to reach approximately 63% of a raw
	// input value. FixedDeltaTime is in seconds.
	tLowPassFilterFixed(T fixedDeltaTime, T attenuation, bool tauAttenuation = false, T initialValue = T(0.0));

	// Given the new value returns the filtered value. Call this every fixedDeltaTime seconds.
	T Update(T inputValue);

	void SetCutoffFreq(T cutoffFreq) override;
	void SetTau(T tau) override;

private:
	// We need to bring required base class functions into scope. The base template class hasn't been instantiated yet.
	using tLowPassFilter<T>::ComputeTau;
	using tLowPassFilter<T>::ComputeWeight;
	using tLowPassFilter<T>::UpdateValue;

	T FixedDeltaTime;						// Stored so we can adjust weight dynamically.
	T Weight;								// Internal only. You cannot get or set this directly.
};
typedef tLowPassFilterFixed<float>  tLowPassFilter_FixFlt;
typedef tLowPassFilterFixed<double> tLowPassFilter_FixDbl;
typedef tLowPassFilterFixed<long double> tLowPassFilter_FixLongDbl;


template <typename T> class tLowPassFilterDynamic : public tLowPassFilter<T>
{
public:
	// If tauAttenuation is false attenuation is specified as a cutoffFrequency. If tauAttenuation is true attenuation
	// is specified as 'tau'. Cutoff-frequency is in Hz above which the signal is attenuated (reduced in amplitude). Tau
	// is specified in seconds and is the time it takes for the filter's output to reach approximately 63% of a raw
	// input value.
	tLowPassFilterDynamic(T attenuation, bool tauAttenuation = false, T initialValue = T(0.0));

	// Given the new value returns the filtered value. Call this often and specify the deltaTime in seconds.
	T Update(T inputValue, T deltaTime);

	// A less accurate but faster update that uses an approximation to compute the weight instead of exp.
	T UpdateFast(T inputValue, T deltaTime);

	void SetCutoffFreq(T cutoffFreq) override;
	void SetTau(T tau) override;

private:
	// We need to bring required base class functions into scope. The base template class hasn't been instantiated yet.
	using tLowPassFilter<T>::ComputeTau;
	using tLowPassFilter<T>::ComputeWeight;
	using tLowPassFilter<T>::UpdateValue;

	T Tau;
};
typedef tLowPassFilterDynamic<float>  tLowPassFilter_DynFlt;
typedef tLowPassFilterDynamic<double> tLowPassFilter_DynDbl;
typedef tLowPassFilterDynamic<long double> tLowPassFilter_DynLongDbl;


}


// Implementation below this line.


template<typename T> inline tMath::tLowPassFilterFixed<T>::tLowPassFilterFixed(T fixedDeltaTime, T attenuation, bool tauAttenuation, T initialValue) :
	tLowPassFilter<T>(initialValue),
	FixedDeltaTime(fixedDeltaTime)
{
	// Calculate the weight. It doesn't change since fixedDeltaTime is unchanging.
	tiClampMin<T>(attenuation, T(tMath::Epsilon));
	T tau = tauAttenuation ? attenuation : ComputeTau(attenuation);
	Weight = ComputeWeight(FixedDeltaTime, tau);
}


template<typename T> inline T tMath::tLowPassFilterFixed<T>::Update(T inputValue)
{
	return UpdateValue(Weight, inputValue);
}


template<typename T> inline void tMath::tLowPassFilterFixed<T>::SetCutoffFreq(T cutoffFreq)
{
	T tau = ComputeTau(cutoffFreq);
	Weight = ComputeWeight(FixedDeltaTime, tau);
}


template<typename T> inline void tMath::tLowPassFilterFixed<T>::SetTau(T tau)
{
	Weight = ComputeWeight(FixedDeltaTime, tau);
}


template<typename T> inline tMath::tLowPassFilterDynamic<T>::tLowPassFilterDynamic(T attenuation, bool tauAttenuation, T initialValue) :
	tLowPassFilter<T>(initialValue)
{
	tiClampMin<T>(attenuation, tMath::Epsilon);
	Tau = tauAttenuation ? attenuation : ComputeTau(attenuation);
}


template<typename T> inline T tMath::tLowPassFilterDynamic<T>::Update(T inputValue, T deltaTime)
{
	T weight = ComputeWeight(deltaTime, Tau);
	return UpdateValue(weight, inputValue);
}


template<typename T> inline T tMath::tLowPassFilterDynamic<T>::UpdateFast(T inputValue, T deltaTime)
{
	// Less accurate but faster approximation.
	T weight = ComputeWeightFast(deltaTime, Tau);
	return UpdateValue(weight, inputValue);
}


template<typename T> inline void tMath::tLowPassFilterDynamic<T>::SetCutoffFreq(T cutoffFreq)
{
	Tau = ComputeTau(cutoffFreq);
}


template<typename T> inline void tMath::tLowPassFilterDynamic<T>::SetTau(T tau)
{
	Tau = tau;
}
