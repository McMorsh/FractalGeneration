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
  EXPECT_EQ(ComputeIterationsForView(kInitialSpan, 5000), kMaxIterations);
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
  int next_bonus = 0;

  EXPECT_FALSE(
      TryNextIterationBonus(kMinSpan, 2000, kIterationStep, next_bonus));
  EXPECT_EQ(ComputeIterationsForView(kMinSpan, 2000), kMaxIterations);
}

TEST(IterationsTest, StepDownWorksFromCeiling) {
  int next_bonus = 0;

  ASSERT_TRUE(
      TryNextIterationBonus(kMinSpan, 2000, -kIterationStep, next_bonus));
  EXPECT_EQ(next_bonus, 1900);
  EXPECT_LT(ComputeIterationsForView(kMinSpan, next_bonus), kMaxIterations);
}

TEST(IterationsTest, ZeroDeltaDoesNothing) {
  int next_bonus = 0;

  EXPECT_FALSE(TryNextIterationBonus(kInitialSpan, 0, 0, next_bonus));
}

}  // namespace
