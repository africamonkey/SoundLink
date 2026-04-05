// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/common/math/fft.h"

#include <cmath>
#include <complex>
#include <vector>

#include "glog/logging.h"

namespace math {

bool IsPowerOf2(int n) {
  return n > 0 && (n & (n - 1)) == 0;
}

int NextPowerOf2(int n) {
  if (n <= 0) return 1;
  if (IsPowerOf2(n)) return n;
  int p = 1;
  while (p < n) {
    p *= 2;
  }
  return p;
}

FFT::FFT(int size) : size_(size) {
  CHECK(IsPowerOf2(size)) << "FFT size must be power of 2, got " << size;
  log2_size_ = 0;
  int temp = size;
  while (temp > 1) {
    temp >>= 1;
    ++log2_size_;
  }

  twiddle_factors_.resize(size_);
  for (int i = 0; i < size_; ++i) {
    double angle = -2.0 * M_PI * i / size_;
    twiddle_factors_[i] = std::complex<double>(std::cos(angle), std::sin(angle));
  }
}

FFT::~FFT() {}

void FFT::Forward(const std::vector<double>& input, std::vector<std::complex<double>>* output) {
  CHECK_EQ((int)input.size(), size_);
  output->resize(size_);
  for (int i = 0; i < size_; ++i) {
    (*output)[i] = std::complex<double>(input[i], 0.0);
  }
  ForwardInPlace(output);
}

void FFT::Inverse(const std::vector<std::complex<double>>& input, std::vector<double>* output) {
  CHECK_EQ((int)input.size(), size_);
  std::vector<std::complex<double>> temp = input;
  InverseInPlace(&temp);
  output->resize(size_);
  for (int i = 0; i < size_; ++i) {
    (*output)[i] = temp[i].real() / size_;
  }
}

void FFT::ForwardInPlace(std::vector<std::complex<double>>* data) {
  CHECK_EQ((int)data->size(), size_);

  for (int i = 0; i < size_; ++i) {
    int j = 0;
    for (int k = 0; k < log2_size_; ++k) {
      j = (j << 1) | ((i >> k) & 1);
    }
    if (j > i) {
      std::swap((*data)[i], (*data)[j]);
    }
  }

  for (int len = 2; len <= size_; len <<= 1) {
    int half_len = len >> 1;
    for (int i = 0; i < size_; i += len) {
      for (int j = 0; j < half_len; ++j) {
        int idx = i + j;
        int twiddle_idx = (size_ / len) * j;
        std::complex<double> u = (*data)[idx];
        std::complex<double> t = twiddle_factors_[twiddle_idx] * (*data)[idx + half_len];
        (*data)[idx] = u + t;
        (*data)[idx + half_len] = u - t;
      }
    }
  }
}

void FFT::InverseInPlace(std::vector<std::complex<double>>* data) {
  CHECK_EQ((int)data->size(), size_);

  for (int i = 0; i < size_; ++i) {
    int j = 0;
    for (int k = 0; k < log2_size_; ++k) {
      j = (j << 1) | ((i >> k) & 1);
    }
    if (j > i) {
      std::swap((*data)[i], (*data)[j]);
    }
  }

  for (int len = 2; len <= size_; len <<= 1) {
    int half_len = len >> 1;
    for (int i = 0; i < size_; i += len) {
      for (int j = 0; j < half_len; ++j) {
        int idx = i + j;
        int twiddle_idx = (size_ / len) * j;
        std::complex<double> u = (*data)[idx];
        std::complex<double> t = std::conj(twiddle_factors_[twiddle_idx]) * (*data)[idx + half_len];
        (*data)[idx] = u + t;
        (*data)[idx + half_len] = u - t;
      }
    }
  }
}

void ComputeFFT(const std::vector<double>& input, std::vector<std::complex<double>>* output) {
  int size = NextPowerOf2(input.size());
  std::vector<double> padded_input(size, 0.0);
  for (size_t i = 0; i < input.size(); ++i) {
    padded_input[i] = input[i];
  }
  FFT fft(size);
  fft.Forward(padded_input, output);
}

void ComputeInverseFFT(const std::vector<std::complex<double>>& input, std::vector<double>* output) {
  int size = NextPowerOf2(input.size());
  std::vector<std::complex<double>> padded_input(size);
  for (size_t i = 0; i < input.size(); ++i) {
    padded_input[i] = input[i];
  }
  for (size_t i = input.size(); i < (size_t)size; ++i) {
    padded_input[i] = std::complex<double>(0.0, 0.0);
  }
  FFT fft(size);
  std::vector<std::complex<double>> complex_output;
  complex_output = padded_input;
  fft.InverseInPlace(&complex_output);
  output->resize(input.size());
  for (size_t i = 0; i < input.size(); ++i) {
    (*output)[i] = complex_output[i].real() / size;
  }
}

void ComputeFFTRealInput(const std::vector<double>& input, std::vector<std::complex<double>>* output) {
  ComputeFFT(input, output);
}

}  // namespace math
