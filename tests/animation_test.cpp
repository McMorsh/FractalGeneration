#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "animation_worker.hpp"
#include "backend.hpp"

namespace {

std::filesystem::path MakeTestPath() {
  return std::filesystem::temp_directory_path() / "fractal_animation_test.gif";
}

std::string ReadSignature(const std::filesystem::path& path) {
  char signature[6] = {};
  std::ifstream file(path, std::ios::binary);
  file.read(signature, sizeof(signature));
  return std::string(signature, sizeof(signature));
}

AnimationRequest MakeRequest(const std::filesystem::path& path) {
  AnimationRequest request;
  request.id = 7;
  request.settings.width = 16;
  request.settings.height = 9;
  request.settings.max_iterations = 20;
  request.settings.center_x = -0.6;
  request.settings.center_y = 0.0;
  request.settings.span = 1.0;
  request.settings.backend = Backend::kCpu;
  request.anchor_x = 8.0;
  request.anchor_y = 4.5;
  request.final_span = 0.5;
  request.frame_count = 3;
  request.delay_cs = 4;
  request.path = path.string();
  return request;
}

TEST(AnimationTest, RendersZoomFramesAndWritesGif) {
  const std::filesystem::path path = MakeTestPath();
  std::filesystem::remove(path);

  const AnimationResult result = RenderAnimation(MakeRequest(path), {});

  EXPECT_TRUE(result.saved);
  EXPECT_FALSE(result.cancelled);
  EXPECT_EQ(result.id, 7);
  EXPECT_EQ(result.frames_written, 3);
  ASSERT_TRUE(std::filesystem::exists(path));
  EXPECT_EQ(ReadSignature(path), "GIF89a");

  std::filesystem::remove(path);
}

TEST(AnimationTest, CancelledJobRemovesPartialFile) {
  const std::filesystem::path path = MakeTestPath();
  std::filesystem::remove(path);

  std::stop_source stop;
  stop.request_stop();
  const AnimationResult result =
      RenderAnimation(MakeRequest(path), stop.get_token());

  EXPECT_TRUE(result.cancelled);
  EXPECT_FALSE(result.saved);
  EXPECT_FALSE(std::filesystem::exists(path));
}

}  // namespace
