#include "iterations.hpp"

#include <algorithm>
#include <cmath>

#include "view_defaults.hpp"

namespace {

constexpr double kBaseIterations = 150.0;
constexpr double kIterationGrowth = 18.0;
constexpr double kIterationExponent = 0.3;
constexpr double kMaxAutomaticIterations = 200000.0;

int RoundToStep(int value) {
  return ((value + 5) / 10) * 10;
}

}  // namespace

int ComputeIterationsForSpan(double span) {
  const double zoom = kInitialSpan / span;
  // Measured escape times grow about twice per 10x zoom, so the exponent
  // stays below one and the cost per frame remains bounded.
  const double growth = std::pow(std::max(zoom, 1.0), kIterationExponent) - 1.0;
  const double iterations = kBaseIterations + kIterationGrowth * growth;
  return static_cast<int>(
      std::clamp(iterations, kBaseIterations, kMaxAutomaticIterations));
}

int ComputeIterationsForView(double span, int bonus) {
  return std::clamp(ComputeIterationsForSpan(span) + bonus, kMinIterations,
                    kMaxIterations);
}

int GetIterationStep(int iterations) {
  const int step = std::max(kIterationStep, iterations / 50);
  return RoundToStep(step);
}

bool TryNextIterationBonus(double span, int bonus, int delta, int& next_bonus) {
  if (delta == 0) {
    return false;
  }

  const int automatic = ComputeIterationsForSpan(span);
  const int candidate = std::clamp(bonus + delta, kMinIterations - automatic,
                                   kMaxIterations - automatic);
  if (ComputeIterationsForView(span, candidate) ==
      ComputeIterationsForView(span, bonus)) {
    return false;
  }

  next_bonus = candidate;
  return true;
}
