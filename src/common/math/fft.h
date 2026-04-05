// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <complex>
#include <vector>

namespace math {

class FFT {
 public:
  explicit FFT(int size);
  ~FFT();

  void Forward(const std::vector<double>& input, std::vector<std::complex<double>>* output);
  void Inverse(const std::vector<std::complex<double>>& input, std::vector<double>* output);

  void ForwardInPlace(std::vector<std::complex<double>>* data);
  void InverseInPlace(std::vector<std::complex<double>>* data);

 private:
  int size_;
  int log2_size_;
  std::vector<std::complex<double>> twiddle_factors_;
};

bool IsPowerOf2(int n);
int NextPowerOf2(int n);

void ComputeFFT(const std::vector<double>& input, std::vector<std::complex<double>>* output);
void ComputeInverseFFT(const std::vector<std::complex<double>>& input, std::vector<double>* output);

void ComputeFFTRealInput(const std::vector<double>& input, std::vector<std::complex<double>>* output);

}  // namespace math
