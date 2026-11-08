#include <gtest/gtest.h>

#include "iterations.hpp"
#include "view_defaults.hpp"

namespace {

TEST(IterationsTest, InitialSpanUsesBaseIterations) {
  EXPECT_EQ(ComputeIterationsForSpan(kInitialSpan), 150);
}

TEST(IterationsTest, ViewAppliesBonus) {
  EXPECT_EQ(ComputeIterationsForView(kInitialSpan, 100), 250);
}

TEST(IterationsTest, ViewClampsToLimits) {
  EXPECT_EQ(ComputeIterationsForView(kInitialSpan, -1000), kMinIterations);
  EXPECT_EQ(ComputeIterationsForView(kInitialSpan, kMaxIterations),
            kMaxIterations);
}

TEST(IterationsTest, DeepViewNeedsManyIterations) {
  EXPECT_GT(ComputeIterationsForSpan(kMinSpan), 100000);
  EXPECT_LE(ComputeIterationsForSpan(kMinSpan), kMaxIterations);
}

TEST(IterationsTest, IterationStepGrowsWithCount) {
  EXPECT_EQ(GetIterationStep(kMinIterations), kIterationStep);
  EXPECT_GT(GetIterationStep(100000), 1000);
}

TEST(IterationsTest, StepDownStopsAtFloor) {
  int next_bonus = 0;

  EXPECT_FALSE(
      TryNextIterationBonus(kInitialSpan, -100, -kIterationStep, next_bonus));
  EXPECT_EQ(ComputeIterationsForView(kInitialSpan, -100), kMinIterations);
}

TEST(IterationsTest, StepUpWorksFromFloor) {
  int next_bonus = 0;

  ASSERT_TRUE(
      TryNextIterationBonus(kInitialSpan, -100, kIterationStep, next_bonus));
  EXPECT_EQ(next_bonus, 0);
  EXPECT_GT(ComputeIterationsForView(kInitialSpan, next_bonus), kMinIterations);
}

TEST(IterationsTest, StepUpStopsAtCeiling) {
  const int automatic = ComputeIterationsForSpan(kMinSpan);
  const int ceiling_bonus = kMaxIterations - automatic;
  int next_bonus = 0;

  EXPECT_FALSE(TryNextIterationBonus(kMinSpan, ceiling_bonus, kIterationStep,
                                     next_bonus));
  EXPECT_EQ(ComputeIterationsForView(kMinSpan, ceiling_bonus), kMaxIterations);
}

TEST(IterationsTest, StepDownWorksFromCeiling) {
  const int automatic = ComputeIterationsForSpan(kMinSpan);
  const int ceiling_bonus = kMaxIterations - automatic;
  int next_bonus = 0;

  ASSERT_TRUE(TryNextIterationBonus(kMinSpan, ceiling_bonus, -kIterationStep,
                                    next_bonus));
  EXPECT_EQ(next_bonus, ceiling_bonus - kIterationStep);
  EXPECT_LT(ComputeIterationsForView(kMinSpan, next_bonus), kMaxIterations);
}

TEST(IterationsTest, ZeroDeltaDoesNothing) {
  int next_bonus = 0;

  EXPECT_FALSE(TryNextIterationBonus(kInitialSpan, 0, 0, next_bonus));
}

}  // namespace
