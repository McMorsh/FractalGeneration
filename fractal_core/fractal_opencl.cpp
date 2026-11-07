#include "fractal_opencl.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

#include "fractal_type.hpp"

#if defined(FRACTALGENERATION_HAS_OPENCL)
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#endif

#if defined(FRACTALGENERATION_HAS_OPENCL)
namespace {

constexpr const char* kMandelbrotKernel = R"CLC(
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void mandelbrot(__global double* output,
                         const double center_x,
                         const double center_y,
                         const double scale,
                         const double half_width,
                         const double half_height,
                         const int width,
                         const int max_iterations) {
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  if (x >= width)
    return;

  const double real = center_x + (x - half_width) * scale;
  const double imag = center_y - (y - half_height) * scale;
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

  double value = (double)max_iterations;
  if (iterations < max_iterations) {
    const double mag2 = zr * zr + zi * zi;
    value = iterations + 1.0 - log2(0.5 * log2(mag2));
  }
  output[y * width + x] = value;
}
)CLC";

constexpr const char* kJuliaKernel = R"CLC(
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void julia(__global double* output,
                    const double center_x,
                    const double center_y,
                    const double scale,
                    const double half_width,
                    const double half_height,
                    const int width,
                    const int max_iterations,
                    const double julia_real,
                    const double julia_imag) {
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  if (x >= width)
    return;

  double zr = center_x + (x - half_width) * scale;
  double zi = center_y - (y - half_height) * scale;
  int iterations = 0;

  while (zr * zr + zi * zi <= 4.0 && iterations < max_iterations) {
    const double zr2 = zr * zr;
    const double zi2 = zi * zi;
    zi = zr * zi + zi * zr + julia_imag;
    zr = zr2 - zi2 + julia_real;
    ++iterations;
  }

  double value = (double)max_iterations;
  if (iterations < max_iterations) {
    const double mag2 = zr * zr + zi * zi;
    value = iterations + 1.0 - log2(0.5 * log2(mag2));
  }
  output[y * width + x] = value;
}
)CLC";

constexpr const char* kBurningShipKernel = R"CLC(
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void burning_ship(__global double* output,
                           const double center_x,
                           const double center_y,
                           const double scale,
                           const double half_width,
                           const double half_height,
                           const int width,
                           const int max_iterations) {
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  if (x >= width)
    return;

  const double real = center_x + (x - half_width) * scale;
  const double imag = -(center_y - (y - half_height) * scale);
  double zr = 0.0;
  double zi = 0.0;
  int iterations = 0;

  while (zr * zr + zi * zi <= 4.0 && iterations < max_iterations) {
    zr = fabs(zr);
    zi = fabs(zi);
    const double zr2 = zr * zr;
    const double zi2 = zi * zi;
    zi = zr * zi + zi * zr + imag;
    zr = zr2 - zi2 + real;
    ++iterations;
  }

  double value = (double)max_iterations;
  if (iterations < max_iterations) {
    const double mag2 = zr * zr + zi * zi;
    value = iterations + 1.0 - log2(0.5 * log2(mag2));
  }
  output[y * width + x] = value;
}
)CLC";

bool SupportsFp64(cl_device_id device) {
  std::size_t size = 0;
  if (clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, 0, nullptr, &size) !=
          CL_SUCCESS ||
      size == 0) {
    return false;
  }

  std::string extensions(size, '\0');
  if (clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, size, extensions.data(),
                      nullptr) != CL_SUCCESS) {
    return false;
  }
  if (extensions.find("cl_khr_fp64") == std::string::npos) {
    return false;
  }

  cl_device_fp_config config = 0;
  if (clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(config),
                      &config, nullptr) != CL_SUCCESS) {
    return false;
  }
  return config != 0;
}

struct OpenClState {
  std::once_flag init_flag;
  std::mutex mutex;
  cl_context context = nullptr;
  cl_command_queue queue = nullptr;
  cl_program program = nullptr;
  cl_kernel mandelbrot_kernel = nullptr;
  cl_kernel julia_kernel = nullptr;
  cl_kernel burning_ship_kernel = nullptr;
  std::string device_name;
  bool available = false;

  ~OpenClState() {
    if (burning_ship_kernel != nullptr) {
      clReleaseKernel(burning_ship_kernel);
    }
    if (julia_kernel != nullptr) {
      clReleaseKernel(julia_kernel);
    }
    if (mandelbrot_kernel != nullptr) {
      clReleaseKernel(mandelbrot_kernel);
    }
    if (program != nullptr) {
      clReleaseProgram(program);
    }
    if (queue != nullptr) {
      clReleaseCommandQueue(queue);
    }
    if (context != nullptr) {
      clReleaseContext(context);
    }
  }
};

void Initialize(OpenClState& state) {
  cl_uint platform_count = 0;
  if (clGetPlatformIDs(0, nullptr, &platform_count) != CL_SUCCESS ||
      platform_count == 0) {
    return;
  }

  std::vector<cl_platform_id> platforms(platform_count);
  if (clGetPlatformIDs(platform_count, platforms.data(), nullptr) !=
      CL_SUCCESS) {
    return;
  }

  cl_device_id device = nullptr;
  for (const cl_platform_id platform : platforms) {
    cl_uint device_count = 0;
    if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr,
                       &device_count) != CL_SUCCESS ||
        device_count == 0) {
      continue;
    }

    std::vector<cl_device_id> devices(device_count);
    if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, device_count,
                       devices.data(), nullptr) != CL_SUCCESS) {
      continue;
    }

    for (const cl_device_id candidate : devices) {
      if (SupportsFp64(candidate)) {
        device = candidate;
        break;
      }
    }
    if (device != nullptr) {
      break;
    }
  }
  if (device == nullptr) {
    return;
  }

  std::size_t name_size = 0;
  if (clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &name_size) ==
          CL_SUCCESS &&
      name_size > 0) {
    state.device_name.resize(name_size);
    if (clGetDeviceInfo(device, CL_DEVICE_NAME, name_size,
                        state.device_name.data(), nullptr) == CL_SUCCESS) {
      // The driver counts the trailing null byte in name_size.
      if (!state.device_name.empty() && state.device_name.back() == '\0') {
        state.device_name.pop_back();
      }
    } else {
      state.device_name.clear();
    }
  }

  cl_int error = CL_SUCCESS;
  state.context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
  if (state.context == nullptr) {
    return;
  }

  state.queue = clCreateCommandQueue(state.context, device, 0, &error);
  if (state.queue == nullptr) {
    return;
  }

  const char* mandelbrot_source = kMandelbrotKernel;
  const char* julia_source = kJuliaKernel;
  const char* burning_ship_source = kBurningShipKernel;
  const char* sources[] = {mandelbrot_source, julia_source,
                           burning_ship_source};
  state.program =
      clCreateProgramWithSource(state.context, 3, sources, nullptr, &error);
  if (state.program == nullptr) {
    return;
  }

  error = clBuildProgram(state.program, 1, &device, nullptr, nullptr, nullptr);
  if (error != CL_SUCCESS) {
    std::fprintf(stderr, "OpenCL: failed to build fractal kernels\n");
    return;
  }

  state.mandelbrot_kernel = clCreateKernel(state.program, "mandelbrot", &error);
  if (state.mandelbrot_kernel == nullptr) {
    return;
  }
  state.julia_kernel = clCreateKernel(state.program, "julia", &error);
  if (state.julia_kernel == nullptr) {
    return;
  }
  state.burning_ship_kernel =
      clCreateKernel(state.program, "burning_ship", &error);
  if (state.burning_ship_kernel == nullptr) {
    return;
  }

  state.available = true;
}

OpenClState& GetState() {
  static OpenClState instance;
  return instance;
}

cl_kernel SelectKernel(const OpenClState& state, FractalType type) {
  switch (type) {
    case FractalType::kMandelbrot:
      return state.mandelbrot_kernel;
    case FractalType::kJulia:
      return state.julia_kernel;
    case FractalType::kBurningShip:
      return state.burning_ship_kernel;
  }
  return nullptr;
}

}  // namespace
#endif

bool fractal_opencl::IsAvailable() {
#if defined(FRACTALGENERATION_HAS_OPENCL)
  OpenClState& cl = GetState();
  std::call_once(cl.init_flag, [&cl] { Initialize(cl); });
  return cl.available;
#else
  return false;
#endif
}

std::string fractal_opencl::GetDeviceName() {
#if defined(FRACTALGENERATION_HAS_OPENCL)
  OpenClState& cl = GetState();
  std::call_once(cl.init_flag, [&cl] { Initialize(cl); });
  return cl.device_name;
#else
  return {};
#endif
}

std::vector<double> fractal_opencl::Generate(
    const FractalParams& params, int width, int height, double center_x,
    double center_y, double span, int max_iterations,
    const std::stop_token& stop_token) {
#if defined(FRACTALGENERATION_HAS_OPENCL)
  OpenClState& cl = GetState();
  std::call_once(cl.init_flag, [&cl] { Initialize(cl); });
  if (!cl.available || width <= 0 || height <= 0 || span <= 0.0 ||
      max_iterations <= 0) {
    return {};
  }
  if (stop_token.stop_requested()) {
    return {};
  }

  std::lock_guard lock(cl.mutex);
  if (stop_token.stop_requested()) {
    return {};
  }

  cl_kernel kernel = SelectKernel(cl, params.type);
  if (kernel == nullptr) {
    return {};
  }

  const double scale = span / width;
  const double half_width = width * 0.5;
  const double half_height = height * 0.5;
  const std::size_t count = static_cast<std::size_t>(width) * height;

  cl_int error = CL_SUCCESS;
  cl_mem output = clCreateBuffer(cl.context, CL_MEM_WRITE_ONLY,
                                 count * sizeof(double), nullptr, &error);
  if (output == nullptr) {
    return {};
  }

  clSetKernelArg(kernel, 0, sizeof(output), &output);
  clSetKernelArg(kernel, 1, sizeof(center_x), &center_x);
  clSetKernelArg(kernel, 2, sizeof(center_y), &center_y);
  clSetKernelArg(kernel, 3, sizeof(scale), &scale);
  clSetKernelArg(kernel, 4, sizeof(half_width), &half_width);
  clSetKernelArg(kernel, 5, sizeof(half_height), &half_height);
  clSetKernelArg(kernel, 6, sizeof(width), &width);
  clSetKernelArg(kernel, 7, sizeof(max_iterations), &max_iterations);
  if (params.type == FractalType::kJulia) {
    const double julia_real = params.julia_real;
    const double julia_imag = params.julia_imag;
    clSetKernelArg(kernel, 8, sizeof(julia_real), &julia_real);
    clSetKernelArg(kernel, 9, sizeof(julia_imag), &julia_imag);
  }

  constexpr int kRowsPerBatch = 32;
  for (int row = 0; row < height; row += kRowsPerBatch) {
    if (stop_token.stop_requested()) {
      clReleaseMemObject(output);
      return {};
    }

    const int rows = std::min(kRowsPerBatch, height - row);
    const std::size_t global_offset[2] = {0, static_cast<std::size_t>(row)};
    const std::size_t global_size[2] = {static_cast<std::size_t>(width),
                                        static_cast<std::size_t>(rows)};
    error = clEnqueueNDRangeKernel(cl.queue, kernel, 2, global_offset,
                                   global_size, nullptr, 0, nullptr, nullptr);
    if (error != CL_SUCCESS) {
      clReleaseMemObject(output);
      return {};
    }
  }

  std::vector<double> result(count);
  error =
      clEnqueueReadBuffer(cl.queue, output, CL_TRUE, 0, count * sizeof(double),
                          result.data(), 0, nullptr, nullptr);
  clReleaseMemObject(output);
  if (error != CL_SUCCESS || stop_token.stop_requested()) {
    return {};
  }
  return result;
#else
  (void)params;
  (void)width;
  (void)height;
  (void)center_x;
  (void)center_y;
  (void)span;
  (void)max_iterations;
  (void)stop_token;
  return {};
#endif
}
