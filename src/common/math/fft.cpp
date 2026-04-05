// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#include "src/common/math/fft.h"

#include <cmath>
#include <complex>
#include <iostream>
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
  CHECK(IsPowerOf2(size_)) << "FFT size must be power of 2, got " << size_;
  log2_size_ = 0;
  int temp = size_;
  while (temp > 1) {
    temp >>= 1;
    ++log2_size_;
  }

  rev_.resize(size_);
  std::fill(rev_.begin(), rev_.end(), 0);
  for (int i = 0; i < size_; ++i) {
    rev_[i] = (rev_[i >> 1] >> 1) | ((i & 1) << (log2_size_ - 1));
  }
}

FFT::~FFT() {}

void FFT::Forward(const std::vector<double>& input, std::vector<std::complex<double>>* output) const {
  CHECK_LE((int)input.size(), size_);
  output->resize(size_);
  std::fill(output->begin(), output->end(), std::complex<double>(0.0, 0.0));
  for (int i = 0; i < input.size(); ++i) {
    (*output)[i] = std::complex<double>(input[i], 0.0);
  }
  DFT(output, 1);
}

void FFT::Inverse(const std::vector<std::complex<double>>& input, std::vector<double>* output) const {
  CHECK_EQ((int)input.size(), size_);
  std::vector<std::complex<double>> temp = input;
  DFT(&temp, -1);
  output->resize(size_);
  for (int i = 0; i < size_; ++i) {
    (*output)[i] = temp[i].real();
  }
}

void FFT::DFT(std::vector<std::complex<double>>* data, int v) const {
  for (int i = 0; i < size_; ++i) {
    if (i < rev_[i]) {
      std::swap(data->at(i), data->at(rev_[i]));
    }
  }
  for (int s = 2; s <= size_; s <<= 1) {
    std::complex<double> g(std::cos(2 * M_PI / s) , v * std::sin(2 * M_PI / s));
    for (int k = 0; k < size_; k += s) {
      std::complex<double> w(1.0, 0.0);
      for (int j = 0; j < s / 2; ++j) {
        std::complex<double> &u = data->at(k + j + s / 2), &v = data->at(k + j);
        std::complex<double> t = w * u;
        u = v - t;
        v = v + t;
        w = w * g;
      }
    }
  }
	if (v == -1) {
		for (int i = 0; i < size_; ++i) {
      data->at(i) /= size_;
    }
	}
}

std::vector<double> ComputeConvolution(const std::vector<double>& a, const std::vector<double>& b) {
  FFT fft(NextPowerOf2(a.size() + b.size()));
  std::vector<std::complex<double>> a_dft, b_dft;
  fft.Forward(a, &a_dft);
  fft.Forward(b, &b_dft);
  std::cout << "\n";
  for (int i = 0; i < std::min(a_dft.size(), b_dft.size()); ++i) {
    a_dft[i] *= b_dft[i];
  }
  std::vector<double> output;
  fft.Inverse(a_dft, &output);
  output.resize(std::max(static_cast<int>(a.size() + b.size()) - 1, 0));
  return output;
}

}  // namespace math
