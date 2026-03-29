// Copyright (c) 2023. Kaiqi Wang - All Rights Reserved

#pragma once

#include <string>

namespace wav {

void AddWhiteNoise(const std::string &input_filename, const std::string &output_filename, double noise_level);

void AddUniformNoise(const std::string &input_filename, const std::string &output_filename, double noise_level);

void AddPinkNoise(const std::string &input_filename, const std::string &output_filename, double noise_level);

void AddBrownNoise(const std::string &input_filename, const std::string &output_filename, double noise_level);

}  // namespace wav