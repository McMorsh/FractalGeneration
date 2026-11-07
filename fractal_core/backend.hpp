#ifndef FRACTALGENERATION_FRACTAL_CORE_BACKEND_HPP_
#define FRACTALGENERATION_FRACTAL_CORE_BACKEND_HPP_

#include <string>

#include "fractal_type.hpp"

/// Computation backend selected on the command line.
enum class Backend {
  kAuto,
  kCpu,
  kOpenCl,
};

/// Parses "auto", "cpu" or "opencl" case-insensitively.
/// @param backend receives the parsed value
/// @return false if the name is not recognized.
bool ParseBackend(const std::string& text, Backend& backend);

/// Returns the command-line name: auto, cpu or opencl.
const char* GetBackendName(Backend backend);

/// Returns false if the backend cannot compute the fractal.
/// OpenCL implements only the Mandelbrot set.
bool BackendSupportsFractal(Backend backend, FractalType type);

/// Returns true if the backend can run: OpenCL needs a GPU with FP64 support.
bool IsBackendAvailable(Backend backend);

/// Returns the backend that actually computes the frame, for example
/// "OpenCL: Intel Arc B580" or "CPU: AVX2".
std::string GetBackendDescription(Backend backend, FractalType type);

#endif  // FRACTALGENERATION_FRACTAL_CORE_BACKEND_HPP_
