#ifndef FRACTALGENERATION_FRACTAL_CORE_ITERATIONS_HPP_
#define FRACTALGENERATION_FRACTAL_CORE_ITERATIONS_HPP_

// Base iteration range and the smallest manual adjustment step.
inline constexpr int kMinIterations = 50;
inline constexpr int kMaxIterations = 200000;
inline constexpr int kIterationStep = 100;

/// Chooses the iteration count from the window width in complex units.
/// Grows with the zoom factor, but sublinearly, so deep views get enough
/// iterations without making every frame equally expensive.
int ComputeIterationsForSpan(double span);

/// Adds the manual offset to the automatic count and clamps the result.
int ComputeIterationsForView(double span, int bonus);

/// Manual step for the current iteration count: about 2% of it, but not less
/// than kIterationStep. A fixed step is useless once the count reaches tens of
/// thousands.
int GetIterationStep(int iterations);

/// Checks whether adding delta changes the final iteration count for span.
/// @param bonus current manual offset
/// @param next_bonus receives the new offset
/// @return false at the kMinIterations and kMaxIterations boundaries so
/// accumulation does not move the working point and the reverse step takes
/// effect immediately.
bool TryNextIterationBonus(double span, int bonus, int delta, int& next_bonus);

#endif  // FRACTALGENERATION_FRACTAL_CORE_ITERATIONS_HPP_
