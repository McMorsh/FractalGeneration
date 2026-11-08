#include "animation_worker.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <system_error>
#include <utility>
#include <vector>

#include <omp.h>
#include <raylib.h>

#include "fractal.hpp"
#include "gif_writer.hpp"
#include "view.hpp"
#include "view_defaults.hpp"

namespace {

constexpr int kGifCycleColors = 255;

}  // namespace

AnimationResult RenderAnimation(const AnimationRequest& request,
                                std::stop_token stop_token,
                                const std::function<void(int)>& progress) {
  AnimationResult result;
  result.id = request.id;
  result.path = request.path;

  const RenderSettings& settings = request.settings;
  const int frames = std::max(1, request.frame_count);
  if (settings.width <= 0 || settings.height <= 0 ||
      settings.max_iterations <= 0 || frames <= 0) {
    return result;
  }

  std::array<Color, 256> palette{};
  palette[0] = BLACK;
  request.colormap.GetCyclePalette(palette.data() + 1, kGifCycleColors);

  GifWriter writer;
  if (!writer.Open(request.path, settings.width, settings.height, palette,
                   request.delay_cs)) {
    return result;
  }

  const View start_view(settings.center_x, settings.center_y, settings.span,
                        settings.width, settings.height);
  View view = start_view;
  const double start_span = start_view.GetSpan();
  const double final_span =
      std::max(std::min(request.final_span, start_span), kMinSpan);
  const double step_factor =
      frames > 1 ? std::pow(final_span / start_span, 1.0 / (frames - 1)) : 1.0;

  const Fractal fractal(settings.max_iterations, settings.fractal,
                        settings.backend);

  int written = 0;
  bool cancelled = false;
  for (int frame = 0; frame < frames; ++frame) {
    if (stop_token.stop_requested()) {
      cancelled = true;
      break;
    }

    const std::vector<double> data =
        fractal.Generate(settings.width, settings.height, view.GetCenterX(),
                         view.GetCenterY(), view.GetSpan(), stop_token);
    if (data.empty()) {
      cancelled = stop_token.stop_requested();
      break;
    }

    std::vector<unsigned char> indices(data.size());
#pragma omp parallel for schedule(static)
    for (std::int64_t i = 0; i < static_cast<std::int64_t>(data.size()); ++i) {
      const std::size_t index = static_cast<std::size_t>(i);
      indices[index] =
          static_cast<unsigned char>(request.colormap.GetPaletteIndex(
              data[index], settings.max_iterations, kGifCycleColors,
              request.color_cycle));
    }

    if (stop_token.stop_requested() || !writer.WriteFrame(indices)) {
      cancelled = stop_token.stop_requested();
      break;
    }

    ++written;
    if (progress) {
      progress(written);
    }

    if (frame + 1 < frames) {
      view.ZoomAt(step_factor, request.anchor_x, request.anchor_y);
    }
  }

  const bool closed = writer.Close();
  result.frames_written = written;
  result.cancelled = cancelled;
  result.saved = !cancelled && closed && written == frames;

  if (!result.saved) {
    std::error_code error;
    std::filesystem::remove(request.path, error);
  }
  return result;
}

AnimationWorker::AnimationWorker(int thread_limit)
    : thread_limit_(thread_limit),
      worker_([this](std::stop_token stop_token) { Run(stop_token); }) {}

AnimationWorker::~AnimationWorker() {
  {
    std::lock_guard lock(mutex_);
    stopping_ = true;
    job_stop_.request_stop();
  }

  worker_.request_stop();
  cv_.notify_all();

  if (worker_.joinable()) {
    worker_.join();
  }
}

void AnimationWorker::Request(const AnimationRequest& request) {
  {
    std::lock_guard lock(mutex_);
    if (stopping_) {
      return;
    }
    job_stop_.request_stop();
    pending_ = request;
  }

  cv_.notify_one();
}

void AnimationWorker::Cancel() {
  {
    std::lock_guard lock(mutex_);
    job_stop_.request_stop();
    pending_.reset();
  }

  cv_.notify_all();
}

bool AnimationWorker::TryTake(AnimationResult& result) {
  std::lock_guard lock(mutex_);
  if (!finished_) {
    return false;
  }

  result = std::move(*finished_);
  finished_.reset();
  return true;
}

bool AnimationWorker::IsBusy() const {
  std::lock_guard lock(mutex_);
  return busy_.load(std::memory_order_relaxed) || pending_.has_value();
}

bool AnimationWorker::GetProgress(int& frame, int& total) const {
  const int current_total = total_.load(std::memory_order_relaxed);
  if (current_total <= 0) {
    return false;
  }

  frame = frame_.load(std::memory_order_relaxed);
  total = current_total;
  return true;
}

void AnimationWorker::Run(std::stop_token) {
  const int threads =
      thread_limit_ > 0 ? thread_limit_ : std::max(1, omp_get_num_procs() - 2);
  omp_set_num_threads(threads);

  while (true) {
    AnimationRequest request;
    std::stop_token job_token;

    {
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this] { return stopping_ || pending_.has_value(); });
      if (stopping_) {
        return;
      }

      request = *pending_;
      pending_.reset();
      job_stop_ = std::stop_source();
      job_token = job_stop_.get_token();
      busy_.store(true, std::memory_order_relaxed);
      total_.store(std::max(1, request.frame_count), std::memory_order_relaxed);
      frame_.store(0, std::memory_order_relaxed);
    }

    AnimationResult result =
        RenderAnimation(request, job_token, [this](int frames_written) {
          frame_.store(frames_written, std::memory_order_relaxed);
        });

    {
      std::lock_guard lock(mutex_);
      busy_.store(false, std::memory_order_relaxed);
      total_.store(0, std::memory_order_relaxed);
      if (stopping_) {
        return;
      }
      finished_ = std::move(result);
    }
  }
}
