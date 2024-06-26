// tQuantizeSpatial.cpp
//
// This module implements a spatial quantize algorithm written by Derrick Coetzee. Only the parts modified by me are
// under the ISC licence. The majority is under what looks like the MIT licence, The original author's licence can be
// found below. Modifications include placing it in a namespace, putting what used to be in 'main' into a function,
// modifying it so that all random numbers and operators are deterministic, indenting it for readability, and making
// sure there is no global state so calls are threadsafe.
//
// The algrithm works well for smaller numbers of colours (generally 32 or fewer) but it can handle from 2 to 256
// colours. The official name of this algorith is 'scolorq'. Running on colours more than 32 takes a LONG time.
// See https://github.com/samhocevar/scolorq/blob/master/spatial_color_quant.cpp
//
// Modifications Copyright (c) 2022-2024 Tristan Grimmer.
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
// Copyright (c) 2006 Derrick Coetzee
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright
// notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <deque>
#include <random>
#include <math.h>
#include <algorithm>
#include <Math/tColour.h>
#include <Math/tRandom.h>
#include "Image/tQuantize.h"
using namespace std;
namespace tImage {
	
	
namespace tQuantizeSpatial
{
	template <typename T, int length> class vector_fixed
	{
	public:
		vector_fixed()																									{ for (int i=0; i<length; i++) data[i] = 0; }
		vector_fixed(const vector_fixed<T, length>& rhs)																{ for (int i=0; i<length; i++) data[i] = rhs.data[i]; }
		vector_fixed(const vector<T>& rhs)																				{ for (int i=0; i<length; i++) data[i] = rhs[i]; }

		T& operator()(int i)																							{ return data[i]; }
		int get_length()																								{ return length; }
		T norm_squared()																								{ T result = 0; for (int i=0; i<length; i++) result += (*this)(i) * (*this)(i); return result; }

		vector_fixed<T, length>& operator=(const vector_fixed<T, length> rhs)											{ for (int i=0; i<length; i++) data[i] = rhs.data[i]; return *this; }
		vector_fixed<T, length> direct_product(vector_fixed<T, length>& rhs)											{ vector_fixed<T, length> result; for (int i=0; i<length; i++) result(i) = (*this)(i) * rhs(i); return result; }

		double dot_product(vector_fixed<T, length> rhs)																	{ T result = 0; for (int i=0; i<length; i++) result += (*this)(i) * rhs(i); return result; }
		vector_fixed<T, length>& operator+=(vector_fixed<T, length> rhs)												{ for (int i=0; i<length; i++) data[i] += rhs(i); return *this; }
		vector_fixed<T, length> operator+(vector_fixed<T, length> rhs)													{ vector_fixed<T, length> result(*this); result += rhs; return result; }
		vector_fixed<T, length>& operator-=(vector_fixed<T, length> rhs)												{ for (int i=0; i<length; i++) data[i] -= rhs(i); return *this; }
		vector_fixed<T, length> operator-(vector_fixed<T, length> rhs)													{ vector_fixed<T, length> result(*this); result -= rhs; return result; }
		vector_fixed<T, length>& operator*=(T scalar)																	{ for (int i=0; i<length; i++) data[i] *= scalar; return *this; }
		vector_fixed<T, length> operator*(T scalar)																		{ vector_fixed<T, length> result(*this); result *= scalar; return result; }

	private:
		T data[length];
	};

	template <typename T, int length> vector_fixed<T, length> operator*(T scalar, vector_fixed<T, length> vec)			{ return vec*scalar; }

	template <typename T> class array2d
	{
	public:
		// @tacent Added the () to initialize when new called.
		array2d(int w, int h)																							{ width = w; height = h; data = new T[width * height](); }

		array2d(const array2d<T>& rhs)																					{ width = rhs.width; height = rhs.height; data = new T[width * height]; for (int i=0; i<width; i++) for (int j=0; j<height; j++) (*this)(i,j) = rhs.data[j*width + i]; }
		~array2d()																										{ delete [] data; }

		T& operator()(int col, int row)																					{ return data[row*width + col]; }
		int get_width()																									{ return width; }
		int get_height()																								{ return height; }

		array2d<T>& operator*=(T scalar)																				{ for (int i=0; i<width; i++) for (int j=0; j<height; j++) (*this)(i,j) *= scalar; return *this; }
		array2d<T> operator*(T scalar)																					{ array2d<T> result(*this); result *= scalar; return result; }
		vector<T> operator*(vector<T> vec)																				{ vector<T> result; T sum; for(int row=0; row<get_height(); row++) { sum = 0; for (int col=0; col<get_width(); col++) sum += (*this)(col,row) * vec[col]; result.push_back(sum); } return result; }
		array2d<T>& multiply_row_scalar(int row, double mult)															{ for (int i=0; i<get_width(); i++) { (*this)(i,row) *= mult; } return *this; }
		array2d<T>& add_row_multiple(int from_row, int to_row, double mult)												{ for (int i=0; i<get_width(); i++) { (*this)(i,to_row) += mult*(*this)(i,from_row); } return *this; }

		// We use simple Gaussian elimination - perf doesn't matter since the matrices will be K x K, where K = number of palette entries.
		array2d<T> matrix_inverse()
		{
			array2d<T> result(get_width(), get_height());
			array2d<T>& a = *this;

			// Set result to identity matrix
			result *= 0;
			for (int i=0; i<get_width(); i++)
				result(i,i) = 1;

			// Reduce to echelon form, mirroring in result
			for (int i=0; i<get_width(); i++)
			{
				result.multiply_row_scalar(i, 1/a(i,i));
				multiply_row_scalar(i, 1/a(i,i));
				for (int j=i+1; j<get_height(); j++)
				{
					result.add_row_multiple(i, j, -a(i,j));
					add_row_multiple(i, j, -a(i,j));
				}
			}
			// Back substitute, mirroring in result
			for (int i=get_width()-1; i>=0; i--)
			{
				for (int j=i-1; j>=0; j--)
				{
					result.add_row_multiple(i, j, -a(i,j));
					add_row_multiple(i, j, -a(i,j));
				}
			}

			// result is now the inverse
			return result;
		}

	private:
		T* data;
		int width, height;
	};

	template <typename T> array2d<T> operator*(T scalar, array2d<T> a)													{ return a*scalar; }

	template <typename T> class array3d
	{
	public:
		// @tacent Added the () to initialize when new called.
		array3d(int w, int h, int d)																					{ width = w; height = h; depth = d; data = new T[width * height * depth](); }

		array3d(const array3d<T>& rhs)																					{ width = rhs.width; height = rhs.height; depth = rhs.depth; data = new T[width * height * depth]; for (int i=0; i<width; i++) for (int j=0; j<height; j++) for (int k=0; k<depth; k++) (*this)(i,j,k) = rhs.data[j*width*depth + i*depth + k]; }
		~array3d()																										{ delete [] data; }

		T& operator()(int col, int row, int layer)																		{ return data[row*width*depth + col*depth + layer]; }

		int get_width()																									{ return width; }
		int get_height()																								{ return height; }
		int get_depth()																									{ return depth; }

	private:
		T* data;
		int width, height, depth;
	};

	int compute_max_coarse_level(int width, int height);
	void fill_random(array3d<double>& a, tMath::tRandom::tGenerator& gen);
	double get_initial_temperature();
	double get_final_temperature();
	void random_permutation(int count, vector<int>& result, std::default_random_engine& randEng);
	void random_permutation_2d(int width, int height, deque< pair<int, int> >& result, std::default_random_engine& randEng);
	void compute_b_array(array2d< vector_fixed<double, 3> >& filter_weights, array2d< vector_fixed<double, 3> >& b);
	vector_fixed<double, 3> b_value(array2d< vector_fixed<double, 3> >& b, int i_x, int i_y, int j_x, int j_y);
	void compute_a_image(array2d< vector_fixed<double, 3> >& image, array2d< vector_fixed<double, 3> >& b, array2d< vector_fixed<double, 3> >& a);
	void sum_coarsen(array2d< vector_fixed<double, 3> >& fine, array2d< vector_fixed<double, 3> >& coarse);
	template <typename T, int length> array2d<T> extract_vector_layer_2d(array2d< vector_fixed<T, length> > s, int k);
	template <typename T, int length> vector<T> extract_vector_layer_1d(vector< vector_fixed<T, length> > s, int k);
	int best_match_color(array3d<double>& vars, int i_x, int i_y, vector< vector_fixed<double, 3> >& palette);
	void zoom_double(array3d<double>& small, array3d<double>& big);
	void compute_initial_s(array2d< vector_fixed<double,3> >& s, array3d<double>& coarse_variables, array2d< vector_fixed<double, 3> >& b);
	void update_s(array2d< vector_fixed<double,3> >& s, array3d<double>& coarse_variables, array2d< vector_fixed<double, 3> >& b, int j_x, int j_y, int alpha, double delta);
	void refine_palette(array2d< vector_fixed<double,3> >& s, array3d<double>& coarse_variables, array2d< vector_fixed<double, 3> >& a, vector< vector_fixed<double, 3> >& palette);
	void compute_initial_j_palette_sum(array2d< vector_fixed<double, 3> >& j_palette_sum, array3d<double>& coarse_variables, vector< vector_fixed<double, 3> >& palette);
	bool spatial_color_quant
	(
		array2d< vector_fixed<double, 3> >& image, array2d< vector_fixed<double, 3> >& filter_weights, array2d< int >& quantized_image,
		vector< vector_fixed<double, 3> >& palette, array3d<double>*& p_coarse_variables,
		double initial_temperature, double final_temperature, int temps_per_level, int repeats_per_temp,
		tMath::tRandom::tGenerator& randGen, std::default_random_engine& randEng
	);
}


int tQuantizeSpatial::compute_max_coarse_level(int width, int height)
{
	// We want the coarsest layer to have at most MAX_PIXELS pixels
	const int MAX_PIXELS = 4000;
	int result = 0;
	while (width * height > MAX_PIXELS)
	{
		width  >>= 1;
		height >>= 1;
		result++;
	}
	return result;
}


void tQuantizeSpatial::fill_random(array3d<double>& a, tMath::tRandom::tGenerator& gen)
{
	for (int i=0; i<a.get_width(); i++)
		for (int j=0; j<a.get_height(); j++)
			for (int k=0; k<a.get_depth(); k++)
				//a(i,j,k) = ((double)rand())/RAND_MAX;
				a(i,j,k) = tMath::tRandom::tGetDouble(gen);
}


double tQuantizeSpatial::get_initial_temperature()
{
	return 2.0; // TODO: Figure out what to make this
}


double tQuantizeSpatial::get_final_temperature()
{
	return 0.02; // TODO: Figure out what to make this
}


void tQuantizeSpatial::random_permutation(int count, vector<int>& result, std::default_random_engine& randEng)
{
	result.clear();
	for(int i=0; i<count; i++)
		result.push_back(i);

	std::shuffle(result.begin(), result.end(), randEng);
}


void tQuantizeSpatial::random_permutation_2d(int width, int height, deque< pair<int, int> >& result, std::default_random_engine& randEng)
{
	vector<int> perm1d;
	random_permutation(width*height, perm1d, randEng);
	while(!perm1d.empty())
	{
		int idx = perm1d.back();
		perm1d.pop_back();
		result.push_back(pair<int,int>(idx % width, idx / width));
	}
}


void tQuantizeSpatial::compute_b_array(array2d< vector_fixed<double, 3> >& filter_weights, array2d< vector_fixed<double, 3> >& b)
{
	// Assume that the pixel i is always located at the center of b, and vary pixel j's location through each location in b.
	int radius_width = (filter_weights.get_width() - 1)/2, radius_height = (filter_weights.get_height() - 1)/2;
	int offset_x = (b.get_width() - 1)/2 - radius_width;
	int offset_y = (b.get_height() - 1)/2 - radius_height;
	for (int j_y = 0; j_y < b.get_height(); j_y++)
	{
		for (int j_x = 0; j_x < b.get_width(); j_x++)
		{
			for (int k_y = 0; k_y < filter_weights.get_height(); k_y++)
			{
				for (int k_x = 0; k_x < filter_weights.get_width(); k_x++)
				{
					if (k_x+offset_x >= j_x - radius_width &&
					k_x+offset_x <= j_x + radius_width &&
					k_y+offset_y >= j_y - radius_width &&
					k_y+offset_y <= j_y + radius_width)
					{
						b(j_x,j_y) += filter_weights(k_x,k_y).direct_product(filter_weights(k_x+offset_x-j_x+radius_width,k_y+offset_y-j_y+radius_height));
					}
				}
			}		
		}
	}
}


tQuantizeSpatial::vector_fixed<double, 3> tQuantizeSpatial::b_value(array2d< vector_fixed<double, 3> >& b, int i_x, int i_y, int j_x, int j_y)
{
	int radius_width = (b.get_width() - 1)/2, radius_height = (b.get_height() - 1)/2;
	int k_x = j_x - i_x + radius_width;
	int k_y = j_y - i_y + radius_height;
	if (k_x >= 0 && k_y >= 0 && k_x < b.get_width() && k_y < b.get_height())
		return b(k_x, k_y);
	else
		return vector_fixed<double, 3>();
}


void tQuantizeSpatial::compute_a_image(array2d< vector_fixed<double, 3> >& image, array2d< vector_fixed<double, 3> >& b, array2d< vector_fixed<double, 3> >& a)
{
	int radius_width = (b.get_width() - 1)/2, radius_height = (b.get_height() - 1)/2;
	for (int i_y = 0; i_y < a.get_height(); i_y++)
	{
		for (int i_x = 0; i_x < a.get_width(); i_x++)
		{
			for (int j_y = i_y - radius_height; j_y <= i_y + radius_height; j_y++)
			{
				if (j_y < 0) j_y = 0;
				if (j_y >= a.get_height()) break;

				for (int j_x = i_x - radius_width; j_x <= i_x + radius_width; j_x++)
				{
					if (j_x < 0) j_x = 0;
					if (j_x >= a.get_width()) break;

					a(i_x,i_y) += b_value(b, i_x, i_y, j_x, j_y).direct_product(image(j_x,j_y));
				}
			}
			a(i_x, i_y) *= -2.0;
		}
	}
}


void tQuantizeSpatial::sum_coarsen(array2d< vector_fixed<double, 3> >& fine, array2d< vector_fixed<double, 3> >& coarse)
{
	for (int y=0; y<coarse.get_height(); y++)
	{
		for (int x=0; x<coarse.get_width(); x++)
		{
			double divisor = 1.0;
			vector_fixed<double, 3> val = fine(x*2, y*2);
			if (x*2 + 1 < fine.get_width())
			{
				divisor += 1; val += fine(x*2 + 1, y*2);
			}
			if (y*2 + 1 < fine.get_height())
			{
				divisor += 1; val += fine(x*2, y*2 + 1);
			}
			if (x*2 + 1 < fine.get_width() && y*2 + 1 < fine.get_height())
			{
				divisor += 1; val += fine(x*2 + 1, y*2 + 1);
			}
			coarse(x, y) = /*(1/divisor)**/val;
		}
	}
}


template <typename T, int length> tQuantizeSpatial::array2d<T> tQuantizeSpatial::extract_vector_layer_2d(array2d< vector_fixed<T, length> > s, int k)
{
	array2d<T> result(s.get_width(), s.get_height());
	for (int i=0; i < s.get_width(); i++)
	{
		for (int j=0; j < s.get_height(); j++)
		{
			result(i,j) = s(i,j)(k);
		}
	}
	return result;
}


template <typename T, int length> vector<T> tQuantizeSpatial::extract_vector_layer_1d(vector< vector_fixed<T, length> > s, int k)
{
	vector<T> result;
	for (unsigned int i=0; i < s.size(); i++)
	{
		result.push_back(s[i](k));
	}
	return result;
}


int tQuantizeSpatial::best_match_color(array3d<double>& vars, int i_x, int i_y, vector< vector_fixed<double, 3> >& palette)
{
	int max_v = 0;
	double max_weight = vars(i_x, i_y, 0);
	for (unsigned int v=1; v < palette.size(); v++)
	{
		if (vars(i_x, i_y, v) > max_weight)
		{
			max_v = v;
			max_weight = vars(i_x, i_y, v);
		}
	}
	return max_v;
}


void tQuantizeSpatial::zoom_double(array3d<double>& small, array3d<double>& big)
{
	// Simple scaling of the weights array based on mixing the four pixels falling under each fine pixel, weighted by area.
	// To mix the pixels a little, we assume each fine pixel is 1.2 fine pixels wide and high.
	for (int y=0; y<big.get_height()/2*2; y++)
	{
		for (int x=0; x<big.get_width()/2*2; x++)
		{
			double left = max(0.0, (x-0.1)/2.0), right  = min(small.get_width()-0.001, (x+1.1)/2.0);
			double top  = max(0.0, (y-0.1)/2.0), bottom = min(small.get_height()-0.001, (y+1.1)/2.0);
			int x_left = (int)floor(left), x_right  = (int)floor(right);
			int y_top  = (int)floor(top),  y_bottom = (int)floor(bottom);
			double area = (right-left)*(bottom-top);
			double top_left_weight  = (ceil(left) - left)*(ceil(top) - top)/area;
			double top_right_weight = (right - floor(right))*(ceil(top) - top)/area;
			double bottom_left_weight  = (ceil(left) - left)*(bottom - floor(bottom))/area;
			double bottom_right_weight = (right - floor(right))*(bottom - floor(bottom))/area;
			double top_weight	 = (right-left)*(ceil(top) - top)/area;
			double bottom_weight  = (right-left)*(bottom - floor(bottom))/area;
			double left_weight	= (bottom-top)*(ceil(left) - left)/area;
			double right_weight   = (bottom-top)*(right - floor(right))/area;
			for(int z=0; z<big.get_depth(); z++)
			{
				if (x_left == x_right && y_top == y_bottom)
				{
					big(x, y, z) = small(x_left, y_top, z);
				}
				else if (x_left == x_right)
				{
					big(x, y, z) = top_weight*small(x_left, y_top, z) + bottom_weight*small(x_left, y_bottom, z);
				}
				else if (y_top == y_bottom)
				{
					big(x, y, z) = left_weight*small(x_left, y_top, z) + right_weight*small(x_right, y_top, z);
				}
				else
				{
					big(x, y, z) = top_left_weight*small(x_left, y_top, z) + top_right_weight*small(x_right, y_top, z) + bottom_left_weight*small(x_left, y_bottom, z) + bottom_right_weight*small(x_right, y_bottom, z);
				}
			}
		}
	}
}


void tQuantizeSpatial::compute_initial_s(array2d< vector_fixed<double,3> >& s, array3d<double>& coarse_variables, array2d< vector_fixed<double, 3> >& b)
{
	int palette_size  = s.get_width();
	int coarse_width  = coarse_variables.get_width();
	int coarse_height = coarse_variables.get_height();
	int center_x = (b.get_width()-1)/2, center_y = (b.get_height()-1)/2;
	vector_fixed<double,3> center_b = b_value(b,0,0,0,0);
	vector_fixed<double,3> zero_vector;
	for (int v=0; v<palette_size; v++)
	{
		for (int alpha=v; alpha<palette_size; alpha++)
		{
			s(v,alpha) = zero_vector;
		}
	}
	for (int i_y=0; i_y<coarse_height; i_y++)
	{
		for (int i_x=0; i_x<coarse_width; i_x++)
		{
			int max_j_x = min(coarse_width,  i_x - center_x + b.get_width());
			int max_j_y = min(coarse_height, i_y - center_y + b.get_height());
			for (int j_y=max(0, i_y - center_y); j_y<max_j_y; j_y++)
			{
				for (int j_x=max(0, i_x - center_x); j_x<max_j_x; j_x++)
				{
					if (i_x == j_x && i_y == j_y) continue;
					vector_fixed<double,3> b_ij = b_value(b,i_x,i_y,j_x,j_y);
					for (int v=0; v<palette_size; v++)
					{
						for (int alpha=v; alpha<palette_size; alpha++)
						{
							double mult = coarse_variables(i_x,i_y,v)*coarse_variables(j_x,j_y,alpha);
							s(v,alpha)(0) += mult * b_ij(0);
							s(v,alpha)(1) += mult * b_ij(1);
							s(v,alpha)(2) += mult * b_ij(2);
						}
					}
				}
			}		
			for (int v=0; v<palette_size; v++)
			{
				s(v,v) += coarse_variables(i_x,i_y,v)*center_b;
			}
		}
	}
}


void tQuantizeSpatial::update_s(array2d< vector_fixed<double,3> >& s, array3d<double>& coarse_variables, array2d< vector_fixed<double, 3> >& b, int j_x, int j_y, int alpha, double delta)
{
	int palette_size  = s.get_width();
	int coarse_width  = coarse_variables.get_width();
	int coarse_height = coarse_variables.get_height();
	int center_x = (b.get_width()-1)/2, center_y = (b.get_height()-1)/2;
	int max_i_x = min(coarse_width,  j_x + center_x + 1);
	int max_i_y = min(coarse_height, j_y + center_y + 1);
	for (int i_y=max(0, j_y - center_y); i_y<max_i_y; i_y++)
	{
		for (int i_x=max(0, j_x - center_x); i_x<max_i_x; i_x++)
		{
			vector_fixed<double,3> delta_b_ij = delta*b_value(b,i_x,i_y,j_x,j_y);
			if (i_x == j_x && i_y == j_y) continue;
			for (int v=0; v <= alpha; v++)
			{
				double mult = coarse_variables(i_x,i_y,v);
				s(v,alpha)(0) += mult * delta_b_ij(0);
				s(v,alpha)(1) += mult * delta_b_ij(1);
				s(v,alpha)(2) += mult * delta_b_ij(2);
			}
			for (int v=alpha; v<palette_size; v++)
			{
				double mult = coarse_variables(i_x,i_y,v);
				s(alpha,v)(0) += mult * delta_b_ij(0);
				s(alpha,v)(1) += mult * delta_b_ij(1);
				s(alpha,v)(2) += mult * delta_b_ij(2);
			}
		}
	}
	s(alpha,alpha) += delta*b_value(b,0,0,0,0);
}


void tQuantizeSpatial::refine_palette(array2d< vector_fixed<double,3> >& s, array3d<double>& coarse_variables, array2d< vector_fixed<double, 3> >& a, vector< vector_fixed<double, 3> >& palette)
{
	// We only computed the half of S above the diagonal - reflect it
	for (int v=0; v<s.get_width(); v++)
	{
		for (int alpha=0; alpha<v; alpha++)
		{
			s(v,alpha) = s(alpha,v);
		}
	}

	vector< vector_fixed<double,3> > r(palette.size());
	for (unsigned int v=0; v<palette.size(); v++)
	{
		for (int i_y=0; i_y<coarse_variables.get_height(); i_y++)
		{
			for (int i_x=0; i_x<coarse_variables.get_width(); i_x++)
			{
				r[v] += coarse_variables(i_x,i_y,v)*a(i_x,i_y);
			}
		}
	}

	for (unsigned int k=0; k<3; k++)
	{
		array2d<double> S_k = extract_vector_layer_2d(s, k);
		vector<double> R_k = extract_vector_layer_1d(r, k);
		vector<double> palette_channel = -1.0*((2.0*S_k).matrix_inverse())*R_k;
		for (unsigned int v=0; v<palette.size(); v++)
		{
			double val = palette_channel[v];
			if (val < 0) val = 0;
			if (val > 1) val = 1;
			palette[v](k) = val;
		}		
	}
}


void tQuantizeSpatial::compute_initial_j_palette_sum(array2d< vector_fixed<double, 3> >& j_palette_sum, array3d<double>& coarse_variables, vector< vector_fixed<double, 3> >& palette)
{
	for (int j_y=0; j_y<coarse_variables.get_height(); j_y++)
	{
		for (int j_x=0; j_x<coarse_variables.get_width(); j_x++)
		{
			vector_fixed<double, 3> palette_sum = vector_fixed<double, 3>();
			for (unsigned int alpha=0; alpha < palette.size(); alpha++)
			{
				palette_sum += coarse_variables(j_x,j_y,alpha)*palette[alpha];
			}
			j_palette_sum(j_x, j_y) = palette_sum;
		}
	}
}


bool tQuantizeSpatial::spatial_color_quant
(
	array2d< vector_fixed<double, 3> >& image, array2d< vector_fixed<double, 3> >& filter_weights, array2d< int >& quantized_image,
	vector< vector_fixed<double, 3> >& palette, array3d<double>*& p_coarse_variables,
	double initial_temperature, double final_temperature, int temps_per_level, int repeats_per_temp,
	tMath::tRandom::tGenerator& randGen, std::default_random_engine& randEng
)
{
	int max_coarse_level = compute_max_coarse_level(image.get_width(), image.get_height()); //1;
	p_coarse_variables = new array3d<double>(image.get_width()  >> max_coarse_level, image.get_height() >> max_coarse_level, palette.size());
	// For syntactic convenience
	array3d<double>& coarse_variables = *p_coarse_variables;
	fill_random(coarse_variables, randGen);

	double temperature = initial_temperature;

	// Compute a_i, b_{ij} according to (11)
	int extended_neighborhood_width = filter_weights.get_width()*2 - 1;
	int extended_neighborhood_height = filter_weights.get_height()*2 - 1;
	array2d< vector_fixed<double, 3> > b0(extended_neighborhood_width, extended_neighborhood_height);
	compute_b_array(filter_weights, b0);

	array2d< vector_fixed<double, 3> > a0(image.get_width(), image.get_height());
	compute_a_image(image, b0, a0);

	// Compute a_I^l, b_{IJ}^l according to (18)
	vector< array2d< vector_fixed<double, 3> > > a_vec, b_vec;
	a_vec.push_back(a0);
	b_vec.push_back(b0);

	int coarse_level;
	for (coarse_level=1; coarse_level <= max_coarse_level; coarse_level++)
	{
		int radius_width  = (filter_weights.get_width() - 1)/2, radius_height = (filter_weights.get_height() - 1)/2;
		array2d< vector_fixed<double, 3> >
		bi(max(3, b_vec.back().get_width()-2), max(3, b_vec.back().get_height()-2));
		for (int J_y=0; J_y<bi.get_height(); J_y++)
		{
			for (int J_x=0; J_x<bi.get_width(); J_x++)
			{
				for (int i_y=radius_height*2; i_y<radius_height*2+2; i_y++)
				{
					for (int i_x=radius_width*2; i_x<radius_width*2+2; i_x++)
					{
						for (int j_y=J_y*2; j_y<J_y*2+2; j_y++)
						{
							for (int j_x=J_x*2; j_x<J_x*2+2; j_x++)
							{
								bi(J_x,J_y) += b_value(b_vec.back(), i_x, i_y, j_x, j_y);
							}
						}
					}
				}
			}
		}
		b_vec.push_back(bi);
		array2d< vector_fixed<double, 3> > ai(image.get_width() >> coarse_level, image.get_height() >> coarse_level);
		sum_coarsen(a_vec.back(), ai);
		a_vec.push_back(ai);
	}

	// Multiscale annealing
	coarse_level = max_coarse_level;
	const int iters_per_level = temps_per_level;
	double temperature_multiplier = pow(final_temperature/initial_temperature, 1.0/(max(3, max_coarse_level*iters_per_level)));

	int iters_at_current_level = 0;
	bool skip_palette_maintenance = false;
	array2d< vector_fixed<double,3> > s(palette.size(), palette.size());
	compute_initial_s(s, coarse_variables, b_vec[coarse_level]);
	array2d< vector_fixed<double, 3> >* j_palette_sum =
	new array2d< vector_fixed<double, 3> >(coarse_variables.get_width(), coarse_variables.get_height());
	compute_initial_j_palette_sum(*j_palette_sum, coarse_variables, palette);
	while (coarse_level >= 0 || temperature > final_temperature)
	{
		// Need to reseat this reference in case we changed p_coarse_variables
		array3d<double>& coarse_variables = *p_coarse_variables;
		array2d< vector_fixed<double, 3> >& a = a_vec[coarse_level];
		array2d< vector_fixed<double, 3> >& b = b_vec[coarse_level];
		vector_fixed<double,3> middle_b = b_value(b,0,0,0,0);
		int center_x = (b.get_width()-1)/2, center_y = (b.get_height()-1)/2;
		int step_counter = 0;
		for (int repeat=0; repeat<repeats_per_temp; repeat++)
		{
			int pixels_changed = 0, pixels_visited = 0;
			deque< pair<int, int> > visit_queue;
			random_permutation_2d(coarse_variables.get_width(), coarse_variables.get_height(), visit_queue, randEng);

			// Compute 2*sum(j in extended neighborhood of i, j != i) b_ij
			while (!visit_queue.empty())
			{
				// If we get to 10% above initial size, just revisit them all
				if ((int)visit_queue.size() > coarse_variables.get_width()*coarse_variables.get_height()*11/10)
				{
					visit_queue.clear();
					random_permutation_2d(coarse_variables.get_width(), coarse_variables.get_height(), visit_queue, randEng);
				}

				int i_x = visit_queue.front().first, i_y = visit_queue.front().second;
				visit_queue.pop_front();

				// Compute (25)
				vector_fixed<double,3> p_i;
				for (int y=0; y<b.get_height(); y++)
				{
					for (int x=0; x<b.get_width(); x++)
					{
						int j_x = x - center_x + i_x, j_y = y - center_y + i_y;
						if (i_x == j_x && i_y == j_y) continue;
						if (j_x < 0 || j_y < 0 || j_x >= coarse_variables.get_width() || j_y >= coarse_variables.get_height()) continue;
						vector_fixed<double,3> b_ij = b_value(b, i_x, i_y, j_x, j_y);
						vector_fixed<double,3> j_pal = (*j_palette_sum)(j_x,j_y);
						p_i(0) += b_ij(0)*j_pal(0);
						p_i(1) += b_ij(1)*j_pal(1);
						p_i(2) += b_ij(2)*j_pal(2);
					}
				}
				p_i *= 2.0;
				p_i += a(i_x, i_y);

				vector<double> meanfield_logs, meanfields;
				double max_meanfield_log = -numeric_limits<double>::infinity();
				double meanfield_sum = 0.0;
				for (unsigned int v=0; v < palette.size(); v++)
				{
					// Update m_{pi(i)v}^I according to (23). We can subtract an arbitrary factor to prevent overflow,
					// since only the weight relative to the sum matters, so we will choose a value that makes the maximum e^100.
					meanfield_logs.push_back(-(palette[v].dot_product(
					p_i + middle_b.direct_product(palette[v])))/temperature);
					if (meanfield_logs.back() > max_meanfield_log)
					{
						max_meanfield_log = meanfield_logs.back();
					}
				}

				for (unsigned int v=0; v < palette.size(); v++)
				{
					meanfields.push_back(exp(meanfield_logs[v]-max_meanfield_log+100));
					meanfield_sum += meanfields.back();
				}


				if (meanfield_sum == 0.0)
					return false;
				int old_max_v = best_match_color(coarse_variables, i_x, i_y, palette);
				vector_fixed<double,3> & j_pal = (*j_palette_sum)(i_x,i_y);
				for (unsigned int v=0; v < palette.size(); v++)
				{
					double new_val = meanfields[v]/meanfield_sum;
					// Prevent the matrix S from becoming singular
					if (new_val <= 0) new_val = 1e-10;
					if (new_val >= 1) new_val = 1 - 1e-10;
					double delta_m_iv = new_val - coarse_variables(i_x,i_y,v);
					coarse_variables(i_x,i_y,v) = new_val;
					j_pal(0) += delta_m_iv*palette[v](0);
					j_pal(1) += delta_m_iv*palette[v](1);
					j_pal(2) += delta_m_iv*palette[v](2);
					if (abs(delta_m_iv) > 0.001 && !skip_palette_maintenance)
						update_s(s, coarse_variables, b, i_x, i_y, v, delta_m_iv);
				}
				int max_v = best_match_color(coarse_variables, i_x, i_y, palette);
				// Only consider it a change if the colors are different enough
				if ((palette[max_v]-palette[old_max_v]).norm_squared() >= 1.0/(255.0*255.0))
				{
					pixels_changed++;
					// We don't add the outer layer of pixels , because there isn't much weight there, and if it does need
					// to be visited, it'll probably be added when we visit neighboring pixels. The commented out loops are
					// faster but cause a little bit of distortion
					//for (int y=center_y-1; y<center_y+1; y++) {
					//   for (int x=center_x-1; x<center_x+1; x++) {
					for (int y=min(1,center_y-1); y<max(b.get_height()-1,center_y+1); y++)
					{
						for (int x=min(1,center_x-1); x<max(b.get_width()-1,center_x+1); x++)
						{
							int j_x = x - center_x + i_x, j_y = y - center_y + i_y;
							if (j_x < 0 || j_y < 0 || j_x >= coarse_variables.get_width() || j_y >= coarse_variables.get_height()) continue;
							visit_queue.push_back(pair<int,int>(j_x,j_y));
						}
					}
				}
				pixels_visited++;

				// Show progress with dots - in a graphical interface, we'd show progressive refinements of the image instead, and maybe a palette preview.
				#if 0
				step_counter++;
				if ((step_counter % 10000) == 0)
				{
					cout << ".";
					cout.flush();
				}
				#endif
			}

			if (skip_palette_maintenance)
			{
				compute_initial_s(s, *p_coarse_variables, b_vec[coarse_level]);
			}
			refine_palette(s, coarse_variables, a, palette);
			compute_initial_j_palette_sum(*j_palette_sum, coarse_variables, palette);
		}

		iters_at_current_level++;
		skip_palette_maintenance = false;
		if ((temperature <= final_temperature || coarse_level > 0) && iters_at_current_level >= iters_per_level)
		{
			coarse_level--;
			if (coarse_level < 0) break;
			array3d<double>* p_new_coarse_variables = new array3d<double>(image.get_width()  >> coarse_level, image.get_height() >> coarse_level, palette.size());
			zoom_double(coarse_variables, *p_new_coarse_variables);
			delete p_coarse_variables;
			p_coarse_variables = p_new_coarse_variables;
			iters_at_current_level = 0;
			delete j_palette_sum;
			j_palette_sum = new array2d< vector_fixed<double, 3> >((*p_coarse_variables).get_width(), (*p_coarse_variables).get_height());
			compute_initial_j_palette_sum(*j_palette_sum, *p_coarse_variables, palette);
			skip_palette_maintenance = true;
		}
		if (temperature > final_temperature)
		{
			temperature *= temperature_multiplier;
		}
	}

	// This is normally not used, but is handy sometimes for debugging
	while (coarse_level > 0)
	{
		coarse_level--;
		array3d<double>* p_new_coarse_variables = new array3d<double>(image.get_width()  >> coarse_level, image.get_height() >> coarse_level, palette.size());
		zoom_double(*p_coarse_variables, *p_new_coarse_variables);
		delete p_coarse_variables;
		p_coarse_variables = p_new_coarse_variables;
	}

	{
		// Need to reseat this reference in case we changed p_coarse_variables
		array3d<double>& coarse_variables = *p_coarse_variables;

		for (int i_x = 0; i_x < image.get_width(); i_x++)
		{
			for(int i_y = 0; i_y < image.get_height(); i_y++)
			{
				quantized_image(i_x,i_y) =
				best_match_color(coarse_variables, i_x, i_y, palette);
			}
		}
		for (unsigned int v=0; v<palette.size(); v++)
		{
			for (unsigned int k=0; k<3; k++)
			{
				if (palette[v](k) > 1.0) palette[v](k) = 1.0;
				if (palette[v](k) < 0.0) palette[v](k) = 0.0;
			}
		}
	}

	return true;
}


//
// The functions below make up the external interface.
//


double tQuantizeSpatial::ComputeBaseDither(int width, int height, int numColours)
{	
	double ditherLevel = 0.09*log(double(width*height)) - 0.04*log(double(numColours)) + 0.001;
	if (ditherLevel < 0.0)
		ditherLevel = 0.001;

	return ditherLevel;
}


bool tQuantizeSpatial::QuantizeImage
(
	int numColours, int width, int height, const tPixel3b* pixels, tColour3b* destPalette, uint8* destIndices,
	bool checkExact, double ditherLevel, int filterSize
)
{
	if ((numColours < 2) || (numColours > 256) || (width <= 0) || (height <= 0) || !pixels || !destPalette || !destIndices)
		return false;

	if ((filterSize != 1) && (filterSize != 3) && (filterSize != 5))
		return false;

	if (ditherLevel <= 0.0)
		ditherLevel = tQuantizeSpatial::ComputeBaseDither(width, height, numColours);

	if (checkExact)
	{
		bool success = tQuantize::QuantizeImageExact(numColours, width, height, pixels, destPalette, destIndices);
		if (success)
			return true;
	}

	// Seeding the generator with the same value every time guarantees repeatability.
	tMath::tRandom::tGeneratorMersenneTwister randGen(uint32(147));
	std::default_random_engine randEng(137);

	array2d< vector_fixed<double, 3> > image(width, height);
	array2d< vector_fixed<double, 3> > filter1_weights(1, 1);
	array2d< vector_fixed<double, 3> > filter3_weights(3, 3);
	array2d< vector_fixed<double, 3> > filter5_weights(5, 5);
	array2d< int > quantized_image(width, height);
	vector< vector_fixed<double, 3> > palette;

	for(int k=0; k<3; k++)
		filter1_weights(0,0)(k) = 1.0;

	for (int i = 0; i < numColours; i++)
	{
		vector_fixed<double, 3> v;
		v(0) = tMath::tRandom::tGetDouble(randGen);		// ((double)rand())/RAND_MAX;
		v(1) = tMath::tRandom::tGetDouble(randGen);		// ((double)rand())/RAND_MAX;
		v(2) = tMath::tRandom::tGetDouble(randGen);		// ((double)rand())/RAND_MAX;
		palette.push_back(v);
	}

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			image(x,y)(0) = (pixels[x + y*width].R)/255.0;
			image(x,y)(1) = (pixels[x + y*width].G)/255.0;
			image(x,y)(2) = (pixels[x + y*width].B)/255.0;
		}
	}	

	array3d<double>* coarse_variables;
	double stddev = ditherLevel;
	double sum = 0.0;
	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
			for(int k = 0; k < 3; k++)
				sum += filter3_weights(i,j)(k) = exp(-sqrt((double)((i-1)*(i-1) + (j-1)*(j-1)))/(stddev*stddev));

	sum /= 3;
	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
			for(int k = 0; k < 3; k++)
				filter3_weights(i,j)(k) /= sum;

	sum = 0.0;
	for(int i = 0; i < 5; i++)
		for(int j = 0; j < 5; j++)
			for(int k = 0; k < 3; k++)
				sum += filter5_weights(i,j)(k) = exp(-sqrt((double)((i-2)*(i-2) + (j-2)*(j-2)))/(stddev*stddev));

	sum /= 3;
	for(int i = 0; i < 5; i++)
		for(int j = 0; j < 5; j++)
			for(int k = 0; k < 3; k++)
				filter5_weights(i,j)(k) /= sum;

	array2d< vector_fixed<double, 3> >* filters[] = {NULL, &filter1_weights, NULL, &filter3_weights, NULL, &filter5_weights};
	bool success = spatial_color_quant(image, *filters[filterSize], quantized_image, palette, coarse_variables, 1.0, 0.001, 3, 1, randGen, randEng);
	// bool success = spatial_color_quant(image, filter3_weights, quantized_image, palette, coarse_variables, 0.05, 0.02);
	if (!success)
		return false;

	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			destIndices[x + y*width] = uint8(quantized_image(x,y));

	for (int c = 0; c < numColours; c++)
	{
		destPalette[c].R = uint8(255.0 * palette[c](0));
		destPalette[c].G = uint8(255.0 * palette[c](1));
		destPalette[c].B = uint8(255.0 * palette[c](2));
	}

	return true;
}


bool tQuantizeSpatial::QuantizeImage
(
	int numColours, int width, int height, const tPixel4b* pixels, tColour3b* destPalette, uint8* destIndices,
	bool checkExact, double ditherLevel, int filterSize
)
{
	if ((numColours < 2) || (numColours > 256) || (width <= 0) || (height <= 0) || !pixels || !destPalette || !destIndices)
		return false;

	tPixel3b* pixels3 = new tPixel3b[width*height];
	for (int p = 0; p < width*height; p++)
		pixels3[p].Set( pixels[p].R, pixels[p].G, pixels[p].B );

	bool success = QuantizeImage(numColours, width, height, pixels3, destPalette, destIndices, checkExact, ditherLevel, filterSize);
	delete[] pixels3;
	return success;
}


}
