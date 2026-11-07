#include <string>

#include <gtest/gtest.h>

#include "backend.hpp"

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

TEST(BackendTest, CpuDescriptionNamesImplementation) {
  const std::string description = GetBackendDescription(Backend::kCpu);

  EXPECT_TRUE(description == "CPU: AVX2" || description == "CPU: scalar");
}

}  // namespace
