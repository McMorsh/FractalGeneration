#include "gif_writer.hpp"

#include <cstddef>

#include <gif_lib.h>

namespace {

constexpr int kGifColorCount = 256;
constexpr int kGifColorResolution = 8;

}  // namespace

GifWriter::~GifWriter() {
  Close();
}

bool GifWriter::Open(const std::string& path, int width, int height,
                     const std::array<Color, 256>& palette, int delay_cs) {
  if (file_ != nullptr || path.empty() || width <= 0 || height <= 0 ||
      delay_cs < 0) {
    return false;
  }

  int error = 0;
  file_ = EGifOpenFileName(path.c_str(), false, &error);
  if (file_ == nullptr) {
    return false;
  }

  width_ = width;
  height_ = height;
  delay_cs_ = delay_cs;
  failed_ = false;

  // The graphics control block per frame makes the file GIF89a.
  EGifSetGifVersion(file_, true);

  ColorMapObject* map = GifMakeMapObject(kGifColorCount, nullptr);
  if (map == nullptr) {
    failed_ = true;
    Close();
    return false;
  }

  for (int i = 0; i < kGifColorCount; ++i) {
    map->Colors[i].Red = palette[i].r;
    map->Colors[i].Green = palette[i].g;
    map->Colors[i].Blue = palette[i].b;
  }

  // The screen descriptor copies the map, so ours is released right away.
  if (EGifPutScreenDesc(file_, width, height, kGifColorResolution, 0, map) ==
      GIF_ERROR) {
    GifFreeMapObject(map);
    failed_ = true;
    Close();
    return false;
  }
  GifFreeMapObject(map);

  return true;
}

bool GifWriter::WriteFrame(const std::vector<unsigned char>& indices) {
  if (file_ == nullptr || failed_) {
    return false;
  }
  if (indices.size() != static_cast<std::size_t>(width_) * height_) {
    failed_ = true;
    return false;
  }

  GraphicsControlBlock control{};
  control.DisposalMode = DISPOSAL_UNSPECIFIED;
  control.UserInputFlag = false;
  control.DelayTime = static_cast<GifWord>(delay_cs_);
  control.TransparentColor = NO_TRANSPARENT_COLOR;

  GifByteType extension[4];
  const std::size_t length = EGifGCBToExtension(&control, extension);
  if (EGifPutExtension(file_, GRAPHICS_EXT_FUNC_CODE, static_cast<int>(length),
                       extension) == GIF_ERROR) {
    failed_ = true;
    return false;
  }

  if (EGifPutImageDesc(file_, 0, 0, width_, height_, false, nullptr) ==
      GIF_ERROR) {
    failed_ = true;
    return false;
  }

  line_.resize(static_cast<std::size_t>(width_));
  for (int y = 0; y < height_; ++y) {
    const unsigned char* source =
        indices.data() + static_cast<std::size_t>(y) * width_;
    for (int x = 0; x < width_; ++x) {
      line_[static_cast<std::size_t>(x)] = source[x];
    }
    if (EGifPutLine(file_, line_.data(), width_) == GIF_ERROR) {
      failed_ = true;
      return false;
    }
  }

  return true;
}

bool GifWriter::Close() {
  if (file_ == nullptr) {
    return !failed_;
  }

  int error = 0;
  const bool closed =
      EGifCloseFile(file_, &error) == GIF_OK && error == E_GIF_SUCCEEDED;
  file_ = nullptr;
  return closed && !failed_;
}
