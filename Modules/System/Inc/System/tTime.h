// tTimer.h
//
// Simple timer class. Like a stopwatch. Supports keeping track of time in a number of different units. Accuracy is all
// up to you - you call the update function. This code does not access low-level timer hardware.
//
// Copyright (c) 2005, 2017, 2019, 2020, 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <ctime>
#include <Foundation/tPlatform.h>
#include <Foundation/tAssert.h>
#include <Foundation/tUnits.h>
namespace tSystem
{


// High accuracy cross platform timing functions. For windows the frequency is whatever the HW reports. For other
// platforms it's 1/1ns = 1000000000Hz = 1GHz. Returns the frequency in Hz. The crystals in most computers that are used
// for the high-performance timer can generally measure down to micro-second or better resolution. Even though 1GHz
// (a 1ns period) is reported for non-Windows platforms, it does not mean you get that resolution -- it just means the
// timer count will get updated in larger chunks.
int64 tGetHardwareTimerFrequency();

// Similar to tGetHardwareTimerFrequency except returns the inverse so as to get the period. Returns the hardware timer
// period in nano-seconds. 10E-9 was chosen so that the period fits nicely in an int.
int64 tGetHardwareTimerPeriod_ns();

// The absolute value of this count is unimportant. On Windows, for example, it starts when Windows is booted up. In any
// case it is the delta you will care about. The returned count increments by whatever the internal timer resolution is.
// For example, if the timer freq is 5000000 Hz, that's 1/5000000 s = 0.000000200 s which is 200ns. This means for every
// timer count it represents 200ns.
int64 tGetHardwareTimerCount();


// Gets the number of seconds since the absolute time reference of 00:00:00 Coordinated Universal Time (UTC),
// Thursday, 1 January 1970. The std::time_t is essentially a big integer.
std::time_t tGetTimeUTC();
std::time_t tGetTimeGMT();


// Gets the current local time. Takes into account your timezone, DST, etc.
// std::tm is a field-based time format, HH, MM, SS etc.
std::tm tGetTimeLocal();
std::tm tConvertTimeToLocal(std::time_t);


// Return a timepoint as a string. The Filename format uses no special characters and if friendly for filenames
// on all filesystems: YYYY-MM-DD-HH-MM-SS.
enum class tTimeFormat
{
	Standard,		// Eg. 2020-01-14 01:47:12
	Extended,		// Eg. Tuesday January 14 2020 - 01:36:34
	Short,			// Eg. Tue Jan 14 14:38:58 2020
	Filename,		// Eg. 2023-02-14-23-55-09
};
tString tConvertTimeToString(std::tm, tTimeFormat = tTimeFormat::Standard);


// Gets the number of seconds since application start. Uses the high-performance counter,
float tGetTime();
double tGetTimeDouble();
void tSleep(int milliSeconds);


class tTimer
{
public:
	// Creates a timer. You can specify whether the timer is constructed started or not. Internally a floating-point
	// (double) member is used to keep track of the elapsed time. You can choose the units it represents, which has
	// implications for overall timer precision. In general, use an internal unit that matches your domain - timing
	// oscillations of visible light? Use nanoseconds. If unit is Unspecified seconds are used.
	tTimer(bool start = true, tUnit::tTime unit = tUnit::tTime::Second)													: UnitInternal(unit), Running(start), Time(0.0f) { if (UnitInternal == tUnit::tTime::Unspecified) UnitInternal = tUnit::tTime::Second; }

	// Call this frequently. If Unit is not specified, internal units are used.
	void Update(float timeElapsed, tUnit::tTime = tUnit::tTime::Unspecified);

	// Does nothing (idempotent) if timer already started (or stopped).
	void Start()																										{ Running = true; }
	void Stop()																											{ Running = false; }
	void Reset(bool start = true)																						{ Time = 0.0; Running = start; }

	// Returns the time in the units of Unit. If Unit is not specified, uses internal units.
	float GetTime(tUnit::tTime = tUnit::tTime::Unspecified) const;
	bool IsRunning() const																								{ return Running; }
	tUnit::tTime GetInternalUnit() const																				{ return UnitInternal; }

	static double Convert(double time, tUnit::tTime from, tUnit::tTime to);

private:
	// @todo Add getters that format the time into a number of different string formats.
	const static double UnitConversionTable[int(tUnit::tTime::NumTimeUnits)][int(tUnit::tTime::NumTimeUnits)];

	tUnit::tTime UnitInternal;
	bool Running;
	double Time;

public:
	// For developers only. Easily add a new unit and recreate the unit conversion table.
	static void PrintHighPrecisionConversionTable(const char* outputFile);
};


}
