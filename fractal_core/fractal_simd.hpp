#ifndef FRACTALGENERATION_FRACTAL_CORE_FRACTAL_SIMD_HPP_
#define FRACTALGENERATION_FRACTAL_CORE_FRACTAL_SIMD_HPP_

#include <stop_token>

namespace fractal_simd {

/// Returns true if the CPU supports AVX2.
bool IsAvx2Available();

/// Computes a row of the Mandelbrot set.
/// @param start_real real part of the first pixel
/// @param step step between adjacent pixels on the real axis
/// @param imag imaginary part of the row
/// @param output receives width values
void GenerateMandelbrotRow(double start_real, double step, double imag,
                           int width, int max_iterations,
                           const std::stop_token& stop_token, double* output);

}  // namespace fractal_simd

#endif  // FRACTALGENERATION_FRACTAL_CORE_FRACTAL_SIMD_HPP_
