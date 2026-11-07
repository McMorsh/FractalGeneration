#ifndef FRACTALGENERATION_FRACTAL_CORE_VIEW_DEFAULTS_HPP_
#define FRACTALGENERATION_FRACTAL_CORE_VIEW_DEFAULTS_HPP_

/// Initial Mandelbrot view: the whole set fits into the window.
inline constexpr double kInitialCenterX = -0.6;
inline constexpr double kInitialCenterY = 0.0;
inline constexpr double kInitialSpan = 3.5;

/// At smaller scales double no longer distinguishes adjacent pixels on a
/// typical window width.
inline constexpr double kMinSpan = 1e-11;
inline constexpr double kMaxSpan = 8.0;

#endif  // FRACTALGENERATION_FRACTAL_CORE_VIEW_DEFAULTS_HPP_
