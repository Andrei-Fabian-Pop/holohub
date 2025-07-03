// Copyright 2025 Analog Devices, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include <iio.h>
#include <string>
#include <vector>

enum class attr_type_t {
  CONTEXT,
  DEVICE,
  CHANNEL,
  UNKNOWN,
};

struct iio_channel_info_t {
  std::string name;
  bool is_output;
};

struct iio_buffer_info_t {
  size_t samples_count;
  void* buffer;
  std::vector<iio_channel_info_t> enabled_channels;
  bool is_cyclic;
  std::string device_name;
};
