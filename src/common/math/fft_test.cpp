// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/common/math/fft.h"

#include <cmath>
#include <random>
#include <vector>

#include "gtest/gtest.h"

#include "src/common/math/math_utils.h"

namespace math {

TEST(FFTTest, IsPowerOf2) {
  EXPECT_TRUE(IsPowerOf2(1));
  EXPECT_TRUE(IsPowerOf2(2));
  EXPECT_TRUE(IsPowerOf2(4));
  EXPECT_TRUE(IsPowerOf2(8));
  EXPECT_TRUE(IsPowerOf2(16));
  EXPECT_TRUE(IsPowerOf2(1024));
  EXPECT_FALSE(IsPowerOf2(0));
  EXPECT_FALSE(IsPowerOf2(3));
  EXPECT_FALSE(IsPowerOf2(5));
  EXPECT_FALSE(IsPowerOf2(6));
  EXPECT_FALSE(IsPowerOf2(7));
}

TEST(FFTTest, NextPowerOf2) {
  EXPECT_EQ(NextPowerOf2(1), 1);
  EXPECT_EQ(NextPowerOf2(2), 2);
  EXPECT_EQ(NextPowerOf2(3), 4);
  EXPECT_EQ(NextPowerOf2(4), 4);
  EXPECT_EQ(NextPowerOf2(5), 8);
  EXPECT_EQ(NextPowerOf2(7), 8);
  EXPECT_EQ(NextPowerOf2(8), 8);
  EXPECT_EQ(NextPowerOf2(9), 16);
}

TEST(FFTTest, ComputeConvolution) {
  std::vector<double> a = {1.0, 2.0, 3.0};
  std::vector<double> b = {2.0, 1.0};
  std::vector<double> c = ComputeConvolution(a, b);
  std::vector<double> c_ans = {2.0, 5.0, 8.0, 3.0};
  ASSERT_EQ(c.size(), c_ans.size());
  for (int i = 0; i < c_ans.size(); ++i) {
    EXPECT_NEAR(c[i], c_ans[i], kEpsilon);
  }
}

TEST(FFTTest, ComputeConvolutionHard) {
  constexpr int kNumOfTestCases = 100;
  std::mt19937 gen(0);
  std::uniform_int_distribution<int> int_dist(1, 10);
  std::uniform_real_distribution<double> double_dist(-100.0, 100.0);
  for (int test_case = 0; test_case < kNumOfTestCases; ++test_case) {
    std::vector<double> a, b;
    a.resize(int_dist(gen));
    b.resize(int_dist(gen));
    for (int i = 0; i < a.size(); ++i) {
      a[i] = double_dist(gen);
    }
    for (int i = 0; i < b.size(); ++i) {
      b[i] = double_dist(gen);
    }
    std::vector<double> c_ans;
    c_ans.resize(a.size() + b.size() - 1);
    std::fill(c_ans.begin(), c_ans.end(), 0.0);
    for (int i = 0; i < a.size(); ++i) {
      for (int j = 0; j < b.size(); ++j) {
        ASSERT_GE(i + j, 0);
        ASSERT_LT(i + j, c_ans.size());
        c_ans[i + j] += a[i] * b[j];
      }
    }
    std::vector<double> c = ComputeConvolution(a, b);
    ASSERT_EQ(c.size(), c_ans.size());
    for (int i = 0; i < c_ans.size(); ++i) {
      if (std::abs(c[i] - c_ans[i]) > kEpsilon) {
        ASSERT_EQ(0, 1) << a.size() << " " << b.size();
      }
      EXPECT_NEAR(c[i], c_ans[i], kEpsilon);
    }
  }
}

}  // namespace math
