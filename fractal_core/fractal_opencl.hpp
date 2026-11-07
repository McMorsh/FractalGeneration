#ifndef FRACTALGENERATION_FRACTAL_CORE_FRACTAL_OPENCL_HPP_
#define FRACTALGENERATION_FRACTAL_CORE_FRACTAL_OPENCL_HPP_

#include <stop_token>
#include <string>
#include <vector>

namespace fractal_opencl {

/// Returns true if an OpenCL GPU with FP64 support was found and the kernel
/// compiled successfully.
bool IsAvailable();

/// Returns the name reported by the OpenCL driver, or an empty string if
/// OpenCL is unavailable.
std::string GetDeviceName();

/// Computes the Mandelbrot set on the GPU.
/// @return an empty vector if OpenCL is unavailable or the computation failed.
std::vector<double> GenerateMandelbrot(int width, int height, double center_x,
                                       double center_y, double span,
                                       int max_iterations,
                                       const std::stop_token& stop_token);

}  // namespace fractal_opencl

#endif  // FRACTALGENERATION_FRACTAL_CORE_FRACTAL_OPENCL_HPP_
