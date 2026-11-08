#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <raylib.h>

#include "gif_writer.hpp"

namespace {

std::filesystem::path MakeTestPath() {
  return std::filesystem::temp_directory_path() / "fractal_gif_writer_test.gif";
}

std::string ReadSignature(const std::filesystem::path& path) {
  std::array<char, 6> signature{};
  std::ifstream file(path, std::ios::binary);
  file.read(signature.data(), signature.size());
  return std::string(signature.begin(), signature.end());
}

TEST(GifWriterTest, WritesTwoFrames) {
  const std::filesystem::path path = MakeTestPath();
  std::filesystem::remove(path);

  std::array<Color, 256> palette{};
  palette[1] = Color{255, 0, 0, 255};
  palette[2] = Color{0, 255, 0, 255};

  GifWriter writer;
  ASSERT_TRUE(writer.Open(path.string(), 4, 3, palette, 4));

  std::vector<unsigned char> frame(12, 0);
  frame[0] = 1;
  frame[5] = 2;
  ASSERT_TRUE(writer.WriteFrame(frame));
  ASSERT_TRUE(writer.WriteFrame(frame));
  ASSERT_TRUE(writer.Close());

  ASSERT_TRUE(std::filesystem::exists(path));
  EXPECT_EQ(ReadSignature(path), "GIF89a");
  std::filesystem::remove(path);
}

TEST(GifWriterTest, RejectsWrongFrameSize) {
  const std::filesystem::path path = MakeTestPath();
  std::filesystem::remove(path);

  const std::array<Color, 256> palette{};
  GifWriter writer;
  ASSERT_TRUE(writer.Open(path.string(), 4, 3, palette, 4));

  const std::vector<unsigned char> frame(11, 0);
  EXPECT_FALSE(writer.WriteFrame(frame));
  writer.Close();

  std::filesystem::remove(path);
}

}  // namespace
