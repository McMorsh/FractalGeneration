#include "backend.hpp"

#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>

#include "fractal_opencl.hpp"
#include "fractal_simd.hpp"

namespace {

void EraseToken(std::string& text, std::string_view token) {
  for (std::size_t pos = text.find(token); pos != std::string::npos;
       pos = text.find(token, pos)) {
    text.erase(pos, token.size());
  }
}

// Drivers report names like "Intel(R) Arc(TM) B580 Graphics".
std::string NormalizeDeviceName(std::string name) {
  EraseToken(name, "(R)");
  EraseToken(name, "(TM)");

  std::string result;
  result.reserve(name.size());
  for (const char ch : name) {
    if (ch == ' ' && !result.empty() && result.back() == ' ') {
      continue;
    }
    result.push_back(ch);
  }
  while (!result.empty() && result.back() == ' ') {
    result.pop_back();
  }

  constexpr std::string_view kGraphicsSuffix = " Graphics";
  if (result.ends_with(kGraphicsSuffix)) {
    result.resize(result.size() - kGraphicsSuffix.size());
  }
  return result;
}

}  // namespace

bool ParseBackend(const std::string& text, Backend& backend) {
  std::string lower;
  lower.reserve(text.size());
  for (const char ch : text) {
    lower.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }

  if (lower == "auto") {
    backend = Backend::kAuto;
    return true;
  }
  if (lower == "cpu") {
    backend = Backend::kCpu;
    return true;
  }
  if (lower == "opencl") {
    backend = Backend::kOpenCl;
    return true;
  }

  return false;
}

const char* GetBackendName(Backend backend) {
  switch (backend) {
    case Backend::kAuto:
      return "auto";
    case Backend::kCpu:
      return "cpu";
    case Backend::kOpenCl:
      return "opencl";
  }
  return "auto";
}

bool IsBackendAvailable(Backend backend) {
  return backend != Backend::kOpenCl || fractal_opencl::IsAvailable();
}

std::string GetBackendDescription(Backend backend) {
  if (backend != Backend::kCpu && fractal_opencl::IsAvailable()) {
    const std::string name =
        NormalizeDeviceName(fractal_opencl::GetDeviceName());
    return name.empty() ? "OpenCL" : "OpenCL: " + name;
  }
  if (fractal_simd::IsAvx2Available()) {
    return "CPU: AVX2";
  }
  return "CPU: scalar";
}
