// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/common/math/fft.h"

#include <cmath>
#include <vector>

#include "gtest/gtest.h"

namespace math {

namespace {

constexpr double kEpsilon = 1e-6;

bool ComplexNear(const std::complex<double>& a, const std::complex<double>& b, double epsilon) {
  return std::abs(a.real() - b.real()) < epsilon && std::abs(a.imag() - b.imag()) < epsilon;
}

}  // namespace

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

TEST(FFTTest, DCComponent) {
  std::vector<double> input(8, 1.0);
  std::vector<std::complex<double>> output;
  ComputeFFT(input, &output);

  EXPECT_TRUE(ComplexNear(output[0], std::complex<double>(8.0, 0.0), kEpsilon));
  for (size_t i = 1; i < output.size(); ++i) {
    EXPECT_TRUE(ComplexNear(output[i], std::complex<double>(0.0, 0.0), kEpsilon))
        << "Index " << i << " has magnitude " << std::abs(output[i]);
  }
}

TEST(FFTTest, SingleFrequency) {
  const int size = 8;
  const double frequency = 1.0;
  std::vector<double> input(size);
  for (int i = 0; i < size; ++i) {
    input[i] = std::sin(2.0 * M_PI * frequency * i / size);
  }

  std::vector<std::complex<double>> output;
  ComputeFFT(input, &output);

  int peak_idx = 0;
  double peak_mag = 0.0;
  for (int i = 0; i < size; ++i) {
    double mag = std::abs(output[i]);
    if (mag > peak_mag) {
      peak_mag = mag;
      peak_idx = i;
    }
  }
  EXPECT_EQ(peak_idx, 1);
}

TEST(FFTTest, RoundTrip) {
  std::vector<double> original = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};

  std::vector<std::complex<double>> freq_domain;
  ComputeFFT(original, &freq_domain);

  std::vector<double> recovered;
  ComputeInverseFFT(freq_domain, &recovered);

  ASSERT_EQ(recovered.size(), original.size());
  for (size_t i = 0; i < original.size(); ++i) {
    EXPECT_NEAR(recovered[i], original[i], kEpsilon);
  }
}

TEST(FFTTest, RoundTripWithPadding) {
  std::vector<double> original = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};

  std::vector<std::complex<double>> freq_domain;
  ComputeFFT(original, &freq_domain);

  std::vector<double> recovered;
  ComputeInverseFFT(freq_domain, &recovered);

  ASSERT_EQ(recovered.size(), original.size());
  for (size_t i = 0; i < original.size(); ++i) {
    EXPECT_NEAR(recovered[i], original[i], kEpsilon);
  }
}

TEST(FFTTest, ClassBasedForwardInverse) {
  const int size = 16;
  FFT fft(size);

  std::vector<double> input(size);
  for (int i = 0; i < size; ++i) {
    input[i] = std::sin(2.0 * M_PI * 2.0 * i / size);
  }

  std::vector<std::complex<double>> freq;
  fft.Forward(input, &freq);

  std::vector<double> recovered;
  fft.Inverse(freq, &recovered);

  ASSERT_EQ(recovered.size(), (size_t)size);
  for (int i = 0; i < size; ++i) {
    EXPECT_NEAR(recovered[i], input[i], kEpsilon);
  }
}

}  // namespace math
