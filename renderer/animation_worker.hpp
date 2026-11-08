#ifndef FRACTALGENERATION_RENDERER_ANIMATION_WORKER_HPP_
#define FRACTALGENERATION_RENDERER_ANIMATION_WORKER_HPP_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>

#include "color_map.hpp"
#include "render_settings.hpp"

/// One zoom-to-cursor GIF export request.
struct AnimationRequest {
  std::uint64_t id = 0;
  RenderSettings settings;
  /// Screen point that stays fixed while the span shrinks.
  double anchor_x = 0.0;
  double anchor_y = 0.0;
  double final_span = 0.0;
  int frame_count = 0;
  /// Frame delay in hundredths of a second.
  int delay_cs = 4;
  /// Color cycle in iterations; 0 derives it from max_iterations.
  double color_cycle = 0.0;
  std::string path;
  ColorMap colormap;
};

/// Animation export result.
struct AnimationResult {
  std::uint64_t id = 0;
  std::string path;
  int frames_written = 0;
  bool saved = false;
  bool cancelled = false;
};

/// Renders the zoom animation synchronously and writes a GIF.
/// Exposed for tests; the worker calls it from its own thread.
/// @param progress called after each written frame with the frame count
AnimationResult RenderAnimation(const AnimationRequest& request,
                                std::stop_token stop_token,
                                const std::function<void(int)>& progress = {});

/// Renders a GIF in the background so the main thread keeps responding.
class AnimationWorker {
 public:
  /// Creates the worker and starts the background thread.
  /// @param thread_limit limits the number of OpenMP threads; 0 leaves two
  /// cores for the main thread
  explicit AnimationWorker(int thread_limit = 0);
  /// Stops the worker, cancels the job, and joins the background thread.
  ~AnimationWorker();

  AnimationWorker(const AnimationWorker&) = delete;
  AnimationWorker& operator=(const AnimationWorker&) = delete;

  /// Cancels the current job and queues a new one.
  void Request(const AnimationRequest& request);

  /// Cancels the current job and drops the pending one.
  void Cancel();

  /// Takes a finished result if one is available. Does not block.
  bool TryTake(AnimationResult& result);

  /// Reports whether a task is active or pending.
  bool IsBusy() const;

  /// Returns the progress of the current job.
  /// @return false when no job is running.
  bool GetProgress(int& frame, int& total) const;

 private:
  void Run(std::stop_token stop_token);

  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::optional<AnimationRequest> pending_;
  std::optional<AnimationResult> finished_;
  std::stop_source job_stop_;
  std::atomic<bool> busy_{false};
  std::atomic<int> frame_{0};
  std::atomic<int> total_{0};
  bool stopping_ = false;
  int thread_limit_ = 0;
  std::jthread worker_;
};

#endif  // FRACTALGENERATION_RENDERER_ANIMATION_WORKER_HPP_
