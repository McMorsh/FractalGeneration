#include <string>

#include <gtest/gtest.h>

#include "backend.hpp"
#include "fractal_type.hpp"

namespace {

TEST(BackendTest, ParsesNamesCaseInsensitively) {
  Backend backend = Backend::kOpenCl;

  EXPECT_TRUE(ParseBackend("AUTO", backend));
  EXPECT_EQ(backend, Backend::kAuto);
  EXPECT_TRUE(ParseBackend("Cpu", backend));
  EXPECT_EQ(backend, Backend::kCpu);
  EXPECT_TRUE(ParseBackend("opencl", backend));
  EXPECT_EQ(backend, Backend::kOpenCl);
}

TEST(BackendTest, RejectsUnknownName) {
  Backend backend = Backend::kAuto;

  EXPECT_FALSE(ParseBackend("gpu", backend));
  EXPECT_EQ(backend, Backend::kAuto);
}

TEST(BackendTest, NamesRoundTrip) {
  for (const Backend backend :
       {Backend::kAuto, Backend::kCpu, Backend::kOpenCl}) {
    Backend parsed = Backend::kOpenCl;

    ASSERT_TRUE(ParseBackend(GetBackendName(backend), parsed));
    EXPECT_EQ(parsed, backend);
  }
}

TEST(BackendTest, OpenClSupportsOnlyMandelbrot) {
  EXPECT_TRUE(BackendSupportsFractal(Backend::kAuto, FractalType::kJulia));
  EXPECT_TRUE(BackendSupportsFractal(Backend::kCpu, FractalType::kBurningShip));
  EXPECT_TRUE(
      BackendSupportsFractal(Backend::kOpenCl, FractalType::kMandelbrot));
  EXPECT_FALSE(BackendSupportsFractal(Backend::kOpenCl, FractalType::kJulia));
  EXPECT_FALSE(
      BackendSupportsFractal(Backend::kOpenCl, FractalType::kBurningShip));
}

}  // namespace
