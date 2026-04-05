// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <complex>
#include <vector>

namespace math {

class FFT {
 public:
  explicit FFT(int size);
  ~FFT();

  void Forward(const std::vector<double>& input, std::vector<std::complex<double>>* output) const;
  void Inverse(const std::vector<std::complex<double>>& input, std::vector<double>* output) const;

 private:
  void DFT(std::vector<std::complex<double>>* data, int v) const;

  int size_;
  int log2_size_;
  std::vector<int> rev_;
};

bool IsPowerOf2(int n);
int NextPowerOf2(int n);
// Convolution: output[i] = \sigma_{k=0}^{i} a[k] * b[i-k]
std::vector<double> ComputeConvolution(const std::vector<double>& a, const std::vector<double>& b);

}  // namespace math
