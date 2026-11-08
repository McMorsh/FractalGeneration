#ifndef FRACTALGENERATION_RENDERER_GIF_WRITER_HPP_
#define FRACTALGENERATION_RENDERER_GIF_WRITER_HPP_

#include <array>
#include <string>
#include <vector>

#include <raylib.h>

struct GifFileType;

/// Writes a GIF89a animation with one shared 256-color table.
class GifWriter {
 public:
  GifWriter() = default;
  ~GifWriter();

  GifWriter(const GifWriter&) = delete;
  GifWriter& operator=(const GifWriter&) = delete;

  /// Creates the file and writes the screen descriptor.
  /// @param palette 256 colors; alpha is ignored
  /// @param delay_cs frame delay in hundredths of a second
  /// @return false if the file cannot be opened or written.
  bool Open(const std::string& path, int width, int height,
            const std::array<Color, 256>& palette, int delay_cs);

  /// Appends one frame of width * height palette indices.
  /// @return false if the frame size does not match or writing fails.
  bool WriteFrame(const std::vector<unsigned char>& indices);

  /// Writes the trailer and closes the file.
  /// @return false if any write failed.
  bool Close();

 private:
  GifFileType* file_ = nullptr;
  int width_ = 0;
  int height_ = 0;
  int delay_cs_ = 4;
  bool failed_ = false;
  std::vector<unsigned char> line_;
};

#endif  // FRACTALGENERATION_RENDERER_GIF_WRITER_HPP_
