// TestMath.cpp
//
// Math module tests.
//
// Copyright (c) 2017, 2019-2021, 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tFundamentals.h>
#include <Math/tSpline.h>
#include <Math/tRandom.h>
#include <Math/tQuaternion.h>
#include <Math/tColour.h>
#include <Math/tInterval.h>
#include "UnitTests.h"
using namespace tMath;
namespace tUnitTest
{


tTestUnit(Fundamentals)
{
	int val = 256;
	bool isPow2 = tIsPower2(val);
	tPrintf("Val:%d   Pow2:%s\n", val, isPow2 ? "true" : "false");
	tRequire(isPow2);

	val = 257;
	isPow2 = tIsPower2(val);
	tPrintf("Val:%d   Pow2:%s\n", val, isPow2 ? "true" : "false");
	tRequire(!isPow2);

	val = 0;
	isPow2 = tIsPower2(val);
	tPrintf("Val:%d   Pow2:%s\n", val, isPow2 ? "true" : "false");
	tRequire(!isPow2);

	val = 1;
	isPow2 = tIsPower2(val);
	tPrintf("Val:%d   Pow2:%s\n", val, isPow2 ? "true" : "false");
	tRequire(isPow2);

	val = 16;
	int nextPow2 = tNextLowerPower2(val);
	tPrintf("Val:%d   NextLowerPower2:%d\n", val, nextPow2);
	tRequire(nextPow2 == 8);

	val = 16;
	nextPow2 = tNextHigherPower2(val);
	tPrintf("Val:%d   NextHigherPower2:%d\n", val, nextPow2);
	tRequire(nextPow2 == 32);

	val = 127;
	nextPow2 = tNextLowerPower2(val);
	tPrintf("Val:%d   NextLowerPower2:%d\n", val, nextPow2);
	tRequire(nextPow2 == 64);

	val = 127;
	nextPow2 = tNextHigherPower2(val);
	tPrintf("Val:%d   NextHigherPower2:%d\n", val, nextPow2);
	tRequire(nextPow2 == 128);

	val = 0;
	nextPow2 = tNextLowerPower2(val);
	tPrintf("Val:%d   NextLowerPower2:%d\n", val, nextPow2);

	val = 0;
	nextPow2 = tNextHigherPower2(val);
	tPrintf("Val:%d   NextHigherPower2:%d\n", val, nextPow2);

	val = 1;
	nextPow2 = tNextLowerPower2(val);
	tPrintf("Val:%d   NextLowerPower2:%d\n", val, nextPow2);

	val = 1;
	nextPow2 = tNextHigherPower2(val);
	tPrintf("Val:%d   NextHigherPower2:%d\n", val, nextPow2);

	tPrintf("Log2 Tests.\n");
	for (int v = -3; v < 257; v++)
		tPrintf("Log2(%d) = %d\n", v, tMath::tLog2(v));

	for (uint v = 0x7FFFFFF0; v != 0x80000000; v++)
		tPrintf("Log2(%d) = %d\n", v, tMath::tLog2(v));

	tPrintf("tCeiling(-2.5f) : %f\n", tCeiling(-2.5f));
	tRequire(tCeiling(-2.5f) == -2.0f);

	// Test rounding.
	tPrintf("tRound(0.0f) : %f\n", tRound(0.0f));
	tRequire(tRound(0.0f) == 0.0f);

	tPrintf("tRound(2.0f) : %f\n", tRound(2.0f));
	tRequire(tRound(2.0f) == 2.0f);

	tPrintf("tRound(2.1f) : %f\n", tRound(2.1f));
	tRequire(tRound(2.1f) == 2.0f);

	tPrintf("tRound(2.5f) : %f\n", tRound(2.5f));
	tRequire(tRound(2.5f) == 3.0f);

	tPrintf("tRound(2.9f) : %f\n", tRound(2.9f));
	tRequire(tRound(2.9f) == 3.0f);

	tPrintf("tRound(-1.5f) : %f\n", tRound(-1.5f));
	tRequire(tRound(-1.5f) == -1.0f);

	// I know of these plats int is 32 bits.
	#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX)
	tRequire(MinInt == MinInt32);
	tRequire(MaxInt == MaxInt32);
	#endif

	//
	// Test Greatest Common Divisor.
	//
	int gcd = 0;
	gcd = tGCD(30,12);							// 2*3*5=30 and 2*2*3=12
	tPrintf("tGCD(30,12) : %d\n", gcd);
	tRequire(6 == tGCD(30,12));					// Explicit value check.
	tRequire(tGCD(30,12) == tGCD(-30,12));
	tRequire(tGCD(-30,12) == tGCD(30,-12));
	tRequire(tGCD(30,-12) == tGCD(-30,-12));
	tRequire(tGCD(-30,-12) == tGCD(12,30));		// Swap.
	tRequire(tGCD(12,30) == tGCD(-12,30));
	tRequire(tGCD(-12,30) == tGCD(12,-30));
	tRequire(tGCD(12,-30) == tGCD(-12,-30));

	gcd = tGCD(12,8);
	tPrintf("tGCD(12,8) : %d\n", gcd);
	tRequire(gcd == 4);

	gcd = tGCD(8,12);
	tPrintf("tGCD(8,12) : %d\n", gcd);
	tRequire(gcd == 4);

	gcd = tGCD(-12,8);
	tPrintf("tGCD(-12,8) : %d\n", gcd);
	tRequire(gcd == 4);

	gcd = tGCD(-8,12);
	tPrintf("tGCD(-8,12) : %d\n", gcd);
	tRequire(gcd == 4);

	gcd = tGCD(12,0);
	tPrintf("tGCD(12,0) : %d\n", gcd);
	tRequire(gcd == 12);

	gcd = tGCD(0,12);
	tPrintf("tGCD(0,12) : %d\n", gcd);
	tRequire(gcd == 12);

	gcd = tGCD(0, 0);
	tPrintf("tGCD(0,0) : %d\n", gcd);
	tRequire(gcd == MaxInt);

	//
	// Test Least Common Multiple.
	//
	int lcm = 0;
	lcm = tLCM(6,9);
	tPrintf("tLCM(6,9) : %d\n", lcm);
	tRequire(18 == tLCM(6,9));					// Explicit value check.
	tRequire(tLCM(6,9) == tLCM(-6,9));
	tRequire(tLCM(-6,9) == tLCM(6,-9));
	tRequire(tLCM(6,-9) == tLCM(-6,-9));
	tRequire(tLCM(-6,-9) == tLCM(9,6));			// Swap.
	tRequire(tLCM(9,6) == tLCM(-9,6));
	tRequire(tLCM(-9,6) == tLCM(9,-6));
	tRequire(tLCM(9,-6) == tLCM(-9,-6));
}


tTestUnit(Interval)
{
	tString intstr;
	tString recstr;
	tInterval inter;

	intstr = "(4,6)";
	inter.Set(intstr);
	tPrintf("A:%d B:%d Bias:%d\n", inter.A, inter.B, int(inter.Bias));
	tRequire(inter.IsValid());
	tRequire(!inter.Contains(4));
	tRequire( inter.Contains(5));
	tRequire(!inter.Contains(6));
	recstr = inter.Get();
	tRequire(recstr == intstr);

	// The interval [0,5) -> { 0 1 2 3 4 }
	intstr = "[0,5)";
	inter.Set(intstr);
	tPrintf("A:%d B:%d Bias:%d\n", inter.A, inter.B, int(inter.Bias));
	tRequire(inter.Contains(0) && inter.Contains(1) && inter.Contains(2) && inter.Contains(3) && inter.Contains(4));
	tRequire(!inter.Contains(-1) && !inter.Contains(5));
	recstr = inter.Get();
	tRequire(recstr == intstr);

	// The interval (5,5) -> empty
	inter.Set("(5,5)");
	tRequire(inter.IsEmpty());

	// The interval [5,5) -> empty
	inter.Set("[5,5)");
	tRequire(inter.IsEmpty());

	// The interval (5,5] -> empty
	inter.Set("(5,5]");
	tRequire(inter.IsEmpty());

	// The interval [5,5] -> { 5 }
	inter.Set("[5,5]");
	tRequire(!inter.IsEmpty());
	tRequire(!inter.Contains(4) && inter.Contains(5) && !inter.Contains(6));

	// The interval 5 -> [5,5] -> { 5 }
	inter.Set("5");
	tRequire(!inter.IsEmpty());
	tRequire(!inter.Contains(4) && inter.Contains(5) && !inter.Contains(6));

	// The interval (4,5] -> { 5 }
	intstr = "(4,5]";
	inter.Set(intstr);
	tRequire(!inter.IsEmpty());
	tRequire(!inter.Contains(4) && inter.Contains(5) && !inter.Contains(6));
	tRequire(!inter.Contains(4));
	recstr = inter.Get();
	tPrintf("Recstr:%s Expect:%s\n", recstr.Chr(), intstr.Chr());
	tRequire(recstr == intstr);

	// The interval (4,5) -> empty
	inter.Set("(4,5)");
	tRequire(inter.IsEmpty());

	// Tests for an interval containing another.
	tInterval test;
	inter.Set(3, 10, tBias::Right);	// (3,10]
	test.Set("[3,9]");
	tRequire(!inter.Contains(test));

	test.Set("(3,9]");
	tRequire(inter.Contains(test));

	test.Set("(3,10]");
	tRequire(inter.Contains(test));

	test.Set("(3,10)");
	tRequire(inter.Contains(test));

	test.Set("(3,11)");
	tRequire(inter.Contains(test));

	test.Set("(3,11]");
	tRequire(!inter.Contains(test));

	// Test overlaps.
	// Testing if contender overlaps with (3,10]
	test.Set("[0,3]");
	tRequire(!inter.Overlaps(test));

	test.Set("[0,4)");
	tRequire(!inter.Overlaps(test));

	test.Set("[0,5)");
	tRequire(inter.Overlaps(test));

	test.Set("[0,4]");
	tRequire(inter.Overlaps(test));

	test.Set("[5,5]");
	tRequire(inter.Overlaps(test));

	test.Set("[5,8]");
	tRequire(inter.Overlaps(test));

	test.Set("[0,12]");
	tRequire(inter.Overlaps(test));

	test.Set("(10,12]");
	tRequire(!inter.Overlaps(test));

	test.Set("[10,12]");
	tRequire(inter.Overlaps(test));

	test.Set("(9,14]");
	tRequire(inter.Overlaps(test));

	test.Set("(10,14]");
	tRequire(!inter.Overlaps(test));

	test.Set("(12,14]");
	tRequire(!inter.Overlaps(test));

	// Now test collections of intervals inside a tIntervals object.
	tIntervalSet intervals;

	tPrintf("Set intervals: [4,6)U[5,8]\n");
	intervals.Set("[4,6)U[5,8]");
	tPrintf("Get intervals: %s\n", intervals.Get().Chr());
	tRequire(intervals.Get() == "[4,8]");

	tPrintf("Set intervals: (4,6]|[6,8]\n");
	intervals.Set("(4,6]|[6,8]");
	tPrintf("Get intervals: %s\n", intervals.Get().Chr());
	tRequire(intervals.Get() == "[5,8]");

	tPrintf("Set intervals: [0,3]|[4,8]\n");
	intervals.Set("[0,3]|[4,8]");
	tPrintf("Get intervals: %s\n", intervals.Get().Chr());
	tRequire(intervals.Get() == "[0,8]");

	tPrintf("Set intervals: [5,8]U[4,6)\n");
	intervals.Set("[5,8]U[4,6)");
	tPrintf("Get intervals: %s\n", intervals.Get().Chr());
	tRequire(intervals.Get() == "[4,8]");

	tPrintf("Set intervals: [0,2]U[4,8]\n");
	intervals.Set("[0,2]U[4,8]");
	tPrintf("Get intervals: %s\n", intervals.Get().Chr());
	tRequire(intervals.Get() == "[0,2]|[4,8]");

	tPrintf("Set intervals: [4,8]U[0,2]\n");
	intervals.Set("[4,8]U[0,2]");
	tPrintf("Get intervals: %s\n", intervals.Get(tIntervalRep::Set).Chr());
	tRequire(intervals.Get(tIntervalRep::Set) == "[0,2]U[4,8]");

	tPrintf("Set intervals: [10,12]U[0,2]U[6,8]\n");
	intervals.Set("[10,12]U[0,2]U[6,8]");
	tPrintf("Get intervals: %s\n", intervals.Get(tIntervalRep::Set).Chr());
	tRequire(intervals.Get(tIntervalRep::Set) == "[0,2]U[6,8]U[10,12]");

	tPrintf("Set intervals: [10,12]U[0,2]U[6,8]\n");
	intervals.Set("[10,12]U[0,2]U[6,8]");
	tPrintf("Get intervals: %s\n", intervals.Get(tIntervalRep::Range).Chr());
	tRequire(intervals.Get(tIntervalRep::Range) == "0-2:6-8:10-12");
}


tTestUnit(Spline)
{
	tVector3 CVs[4];
	CVs[0] = tVector3(0.0, 0.0, 0.0);
	CVs[1] = tVector3(1.0, 1.0, 0.0);
	CVs[2] = tVector3(2.0, 1.0, 0.0);
	CVs[3] = tVector3(3.0, 0.0, 0.0);

	// Testing path.
	tBezierPath path;
	path.SetControlVerts(CVs, 4, tBezierPath::tMode::ExternalCVs, tBezierPath::tType::Open);
	tRequire(!path.IsClosed());

	for (int n = 0; n <= 10; n++)
	{
		float t = float(n)/10.0f;
		tVector3 p;
		path.GetPoint(p, t);
		tPrintf("Path: Param=%f  Point=%v\n", t, p.Pod());
	}

	// Testing curve.
	tBezierCurve curve(CVs);
	for (int n = 0; n <= 10; n++)
	{
		float t = float(n)/10.0f;
		tVector3 p;
		curve.GetPoint(p, t);
		tPrintf("Curve: Param=%f  Point=%v\n", t, p.Pod());
	}
	float closestParam = curve.GetClosestParam(tVector3(4.0, 0.0, 0.0));
	tRequire(tMath::tApproxEqual(closestParam, 1.0f));	
	tPrintf("Closest Param=%f\n", closestParam);
}


tTestUnit(Random)
{
	uint32 seeds[1024];
	for (int i = 0; i < 1024; i++)
		seeds[i] = i;

	tRandom::tDefaultGeneratorType gen(seeds, 1025);

	tPrintf("Random Bits\n");
	for (int i = 0; i < 16; i++)
		tPrintf("Bit %02d     : %08X\n", i, gen.GetBits());

	tPrintf("Random Bits Again\n");
	for (int i = 0; i < 16; i++)
		tPrintf("Bit %02d     : %08X\n", i, tRandom::tGetBits());

	tPrintf("Random Integers in [-10, 10]\n");
	for (int i = 0; i < 16; i++)
	{
		int randomInt = tRandom::tGetBounded(-10, 10, gen);
		tPrintf("Integer %02d : %d\n", i, randomInt);
		tRequire(tInRange(randomInt, -10, 10));
	}

	tPrintf("Random Floats in [0.0f, 1.0f]\n");
	for (int i = 0; i < 16; i++)
	{
		float randomFloat = tRandom::tGetFloat(gen);
		tPrintf("Float %02d   : %f\n", i, randomFloat);
		tRequire(tInRange(randomFloat, 0.0f, 1.0f));
	}

	tPrintf("Random Doubles in [0.0, 1.0]\n");
	for (int i = 0; i < 16; i++)
	{
		double randomDouble = tRandom::tGetDouble(gen);
		tPrintf("Double %02d  : %f\n", i, randomDouble);
		tRequire(tInRange(randomDouble, 0.0, 1.0));
	}

	tPrintf("Random Bounded Vector2s in [(-10.0, -10.0), (10.0, 10.0)]\n");
	for (int i = 0; i < 16; i++)
	{
		tVector2 r = tRandom::tGetBounded(tVector2(-10.0f), tVector2(10.0f));
		tPrintf("Vector2 %02d : %:2v\n", i, tPod(r));
		tRequire(tInRange(r.x, -10.0f, 10.0f));
		tRequire(tInRange(r.y, -10.0f, 10.0f));
	}

	tPrintf("Random Extent Bounded Vector2s in [(40.0, 40.0), (60.0, 60.0)]\n");
	for (int i = 0; i < 16; i++)
	{
		tVector2 r = tRandom::tGetExtentBounded(tVector2(50.0f), tVector2(10.0f));
		tPrintf("Vector2 %02d : %:2v\n", i, tPod(r));
		tRequire(tInRange(r.x, 40.0f, 60.0f));
		tRequire(tInRange(r.y, 40.0f, 60.0f));
	}
}


tTestUnit(Matrix)
{
	tMatrix4 a;
	tMatrix4 b;
	a.Identity();
	b.Identity();
	a.a11 = 0.0f; a.a12 = 1.0f;
	a.a21 = -1.0f;
	b.a11 = 4.0f; b.a12 = 3.0f;
	b.a21 = 5.0f;

	a += b;
	a -= b;
	a *= b;
	a = a + b;
	a = a - b;
	a = a * b;

	a /= 0.5f;
	a = a / 3.0f;

	a *= 3.0f;
	a = a * 0.2f;

	if (a == b)
		tPrintf("Mats equal\n");
	tRequire(!(a == b));

	if (a != b)
		tPrintf("Mats not equal\n");
	tRequire(a != b);

	a = +a;
	a = -a;

	tVector3 v3; v3.Zero();
	tVector4 v4; v4.Zero();
	v3 = a*v3;
	v4 = a*v4;
	tPrintf("Vector3: %:3v\n", v3.Pod());

	tMatrix4 prod = a * b;
	tPrintf("Prod matrix4: %m\n", prod);

	tMatrix4 m1;
	tMatrix4 m2;

	m1.MakeRotate( tVector3(3.0f, 4.0f, 5.0f), 2.6436f );
	tPrintf("m1 matrix4: %m\n", m1);

	m2 = m1;
	m2.Invert();
	tPrintf("m2 (inverse of m1): %m\n", m2);

	m2 = m2 * m1;
	tPrintf("Product of inverses: %m\n", m2);
	tRequire(m2.ApproxEqual(tMatrix4::identity));

	tPrintf("Test matrix multiply.\n");
	tMatrix4 m;
	m.MakeRotateY(2.0f);
	tSet(m.C4, 2.0f, 3.0f, 4.0f, 1.0f);

	tVector4 v(20.0f, 30.0f, 40.0f, 0.0f);
	tVector4 r = m*v;
	tPrintf("Mult result: %4v\n", r);

	tVector4 e = m.Col1()*v.x + tVector4(m.C2)*v.y + tVector4(m.C3)*v.z + tVector4(m.C4)*v.w;
	tPrintf("Explicit result: %4v\n", e);
	tRequire(r == e);
}


tTestUnit(Quaternion)
{
	tMatrix4 identMat;
	identMat.Identity();

	tQuaternion cq(identMat);
	tPrintf("Quat from ident mat %q\n", cq);

	tVector4 v = tVector4::zero;
	tVector4 v2(v);

	identMat.Set(cq);
	tPrintf("Mat from ident quat: %m\n", identMat);

	tQuaternion qi(0.5f, 0.5f, 0.5f, 0.5f);
	tRequire(tApproxEqual(qi.Length(), 1.0f));
	qi.Normalize();
	tPrintf("Quat before %q\n", qi);

	tMatrix4 m(qi);
	tQuaternion qf(m);
	tPrintf("Quat after %q\n\n", qf);
	tRequire(qi.ApproxEqual(qf));

	tMatrix4 matBefore = tMatrix4::identity;
	matBefore.E[5] = -1.0f;
	matBefore.E[10] = -1.0f;
	tPrintf("Mat Before conversion:\n%_m\n\n", matBefore);

	tQuaternion qm(matBefore);
	tMatrix4 matAfter(qm);
	tPrintf("Mat After conversion:\n%_m\n\n", matAfter);
	tRequire(matBefore.ApproxEqual(matAfter));
}


tTestUnit(Geometry)
{
	tRay ray;
	ray.Start.Set(1.0f, 0.5f, 0.5f);
	ray.Dir.Set(-1.0f, 0.0f, 0.0f);

	// CCW winding.
	tTriangle tri;
	tri.A.Set(0.0f, 0.0f, 0.0f);
	tri.B.Set(0.0f, 1.0f, 0.0f);
	tri.C.Set(0.0f, 0.0f, 1.0f);

	bool intersects = tIntersectTestRayTriangle(ray, tri);
	tPrintf("Ray intersects triangle: %s\n", intersects ? "true" : "false");
	tRequire(intersects);

	ray.Start.Set(10.0f, 10.5f, 10.5f);
	ray.Dir.Set(1.0f, 2.0f, 3.0f);
	intersects = tIntersectTestRayTriangle(ray, tri);
	tPrintf("Ray intersects triangle: %s\n", intersects ? "true" : "false");
	tRequire(!intersects);
}


tTestUnit(Colour)
{
	tColouri a = tColouri::black;
	tColouri b = tColouri::white;
	float colDiffLinSq = tColourDiffEuclideanSq(a, b);
	tPrintf("Colour Diff (black white) Euclidean Squared: %f\n", colDiffLinSq);
	float colDiffLin = tColourDiffEuclidean(a, b);
	tPrintf("Colour Diff (black white) Euclidean: %f\n", colDiffLin);
	float colDiffRedmean = tColourDiffRedmean(a, b);
	tPrintf("Colour Diff (black white) Redmean: %f\n", colDiffRedmean);
	tRequire(colDiffLinSq >= colDiffLin);

	a = tColouri::grey;
	b = tColouri::cyan;
	colDiffLinSq = tColourDiffEuclideanSq(a, b);
	tPrintf("Colour Diff (grey cyan) Euclidean Squared: %f\n", colDiffLinSq);
	colDiffLin = tColourDiffEuclidean(a, b);
	tPrintf("Colour Diff (grey cyan) Euclidean: %f\n", colDiffLin);
	colDiffRedmean = tColourDiffRedmean(a, b);
	tPrintf("Colour Diff (grey cyan) Redmean: %f\n", colDiffRedmean);
	tRequire(colDiffLinSq >= colDiffLin);

	a = tColouri::yellow;
	b = tColouri::yellow;
	colDiffLinSq = tColourDiffEuclideanSq(a, b);
	tPrintf("Colour Diff (yellow yellow) Euclidean Squared: %f\n", colDiffLinSq);
	colDiffLin = tColourDiffEuclidean(a, b);
	tPrintf("Colour Diff (yellow yellow) Euclidean: %f\n", colDiffLin);
	colDiffRedmean = tColourDiffRedmean(a, b);
	tPrintf("Colour Diff (yellow yellow) Redmean: %f\n", colDiffRedmean);
	tRequire(colDiffLinSq >= colDiffLin);
}


}
