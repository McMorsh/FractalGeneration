#include "fractal_simd.hpp"

#include <cmath>

#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#define FRACTALGENERATION_HAS_AVX2 1
#include <immintrin.h>
#else
#define FRACTALGENERATION_HAS_AVX2 0
#endif

namespace {

double MandelbrotValue(double real, double imag, int max_iterations) {
  double zr = 0.0;
  double zi = 0.0;
  int iterations = 0;
  while (zr * zr + zi * zi <= 4.0 && iterations < max_iterations) {
    const double zr2 = zr * zr;
    const double zi2 = zi * zi;
    zi = zr * zi + zi * zr + imag;
    zr = zr2 - zi2 + real;
    ++iterations;
  }

  if (iterations >= max_iterations) {
    return static_cast<double>(max_iterations);
  }

  const double mag2 = zr * zr + zi * zi;
  const double nu = std::log2(0.5 * std::log2(mag2));
  return iterations + 1.0 - nu;
}

#if FRACTALGENERATION_HAS_AVX2
__attribute__((target("avx2"))) void GenerateMandelbrotRowAvx2(
    double start_real, double step, double imag, int width, int max_iterations,
    const std::stop_token& stop_token, double* output) {
  const __m256d four = _mm256_set1_pd(4.0);
  const __m256d all_ones = _mm256_castsi256_pd(_mm256_set1_epi64x(-1));
  const __m256i one = _mm256_set1_epi64x(1);

  int x = 0;
  for (; x + 4 <= width; x += 4) {
    if ((x & 63) == 0 && stop_token.stop_requested()) {
      return;
    }

    const __m256d cr =
        _mm256_set_pd(start_real + (x + 3) * step, start_real + (x + 2) * step,
                      start_real + (x + 1) * step, start_real + x * step);
    const __m256d ci = _mm256_set1_pd(imag);
    __m256d zr = _mm256_setzero_pd();
    __m256d zi = _mm256_setzero_pd();
    __m256i iterations = _mm256_setzero_si256();

    for (int n = 0; n < max_iterations; ++n) {
      const __m256d zr2 = _mm256_mul_pd(zr, zr);
      const __m256d zi2 = _mm256_mul_pd(zi, zi);
      const __m256d mag2 = _mm256_add_pd(zr2, zi2);
      const __m256d escaped = _mm256_cmp_pd(mag2, four, _CMP_GT_OQ);
      const __m256d active = _mm256_andnot_pd(escaped, all_ones);
      if (_mm256_movemask_pd(active) == 0) {
        break;
      }

      const __m256d new_zi = _mm256_add_pd(
          _mm256_add_pd(_mm256_mul_pd(zr, zi), _mm256_mul_pd(zi, zr)), ci);
      const __m256d new_zr = _mm256_add_pd(_mm256_sub_pd(zr2, zi2), cr);
      zr = _mm256_blendv_pd(zr, new_zr, active);
      zi = _mm256_blendv_pd(zi, new_zi, active);
      const __m256i active_int = _mm256_castpd_si256(active);
      iterations =
          _mm256_add_epi64(iterations, _mm256_and_si256(active_int, one));
    }

    alignas(32) double zr_out[4];
    alignas(32) double zi_out[4];
    alignas(32) long long iter_out[4];
    _mm256_store_pd(zr_out, zr);
    _mm256_store_pd(zi_out, zi);
    _mm256_store_si256(reinterpret_cast<__m256i*>(iter_out), iterations);

    for (int lane = 0; lane < 4; ++lane) {
      const int it = static_cast<int>(iter_out[lane]);
      if (it >= max_iterations) {
        output[x + lane] = static_cast<double>(max_iterations);
      } else {
        const double mag2 =
            zr_out[lane] * zr_out[lane] + zi_out[lane] * zi_out[lane];
        const double nu = std::log2(0.5 * std::log2(mag2));
        output[x + lane] = it + 1.0 - nu;
      }
    }
  }

  for (; x < width; ++x) {
    if ((x & 63) == 0 && stop_token.stop_requested()) {
      return;
    }
    output[x] = MandelbrotValue(start_real + x * step, imag, max_iterations);
  }
}
#endif

}  // namespace

bool fractal_simd::IsAvx2Available() {
#if FRACTALGENERATION_HAS_AVX2
  static const bool available = [] {
#if defined(__GNUC__) && !defined(__clang__)
    __builtin_cpu_init();
#endif
    return __builtin_cpu_supports("avx2") != 0;
  }();
  return available;
#else
  return false;
#endif
}

void fractal_simd::GenerateMandelbrotRow(double start_real, double step,
                                         double imag, int width,
                                         int max_iterations,
                                         const std::stop_token& stop_token,
                                         double* output) {
#if FRACTALGENERATION_HAS_AVX2
  if (IsAvx2Available()) {
    GenerateMandelbrotRowAvx2(start_real, step, imag, width, max_iterations,
                              stop_token, output);
    return;
  }
#endif

  for (int x = 0; x < width; ++x) {
    if ((x & 63) == 0 && stop_token.stop_requested()) {
      return;
    }
    output[x] = MandelbrotValue(start_real + x * step, imag, max_iterations);
  }
}
