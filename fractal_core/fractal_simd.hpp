#ifndef FRACTALGENERATION_FRACTAL_CORE_FRACTAL_SIMD_HPP_
#define FRACTALGENERATION_FRACTAL_CORE_FRACTAL_SIMD_HPP_

#include <stop_token>

#include "fractal_type.hpp"

namespace fractal_simd {

/// Returns true if the CPU supports AVX2.
bool IsAvx2Available();

/// Computes a row of the selected fractal.
/// @param type fractal formula
/// @param params Julia parameter c; ignored for other types
/// @param start_real real part of the first pixel (c or z0)
/// @param step step between adjacent pixels on the real axis
/// @param imag imaginary part of the row (c or z0)
/// @param output receives width values
void GenerateRow(FractalType type, const FractalParams& params,
                 double start_real, double step, double imag, int width,
                 int max_iterations, const std::stop_token& stop_token,
                 double* output);

}  // namespace fractal_simd

#endif  // FRACTALGENERATION_FRACTAL_CORE_FRACTAL_SIMD_HPP_
