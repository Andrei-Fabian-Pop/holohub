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

#include "iio_buffer_read.hpp"
#include <iio.h>
#include "iio_params.hpp"

using namespace holoscan::ops;

IIOBufferRead::~IIOBufferRead() {
  if (buffer_) {
    iio_buffer_destroy(buffer_);
  }
  if (ctx_) {
    iio_context_destroy(ctx_);
  }
}

void IIOBufferRead::setup(OperatorSpec& spec) {
  HOLOSCAN_LOG_INFO("IIOBufferRead setup");
  spec.output<std::shared_ptr<iio_buffer_info_t>>("buffer");

  spec.param<std::string>(ctx_p_, "ctx", "IIO Context", "The URI of the IIO Context");
  spec.param<std::string>(dev_p_, "dev", "IIO Device", "Name of the IIO Device");
  spec.param<bool>(is_cyclic, "is_cyclic", "Is cyclic", "Is the buffer cyclic?");
  spec.param<size_t>(
      samples_count_p_, "samples_count", "Samples count", "Number of samples to read");
  spec.param<std::vector<std::string>>(
      enabled_channel_names_p_,
      "enabled_channel_names",
      "Names of the enabled IIO Channels",
      "The names of the channels that are enabled when pushing the buffer");
  spec.param<std::vector<bool>>(
      enabled_channel_types_p_,
      "enabled_channel_output",
      "Types of the enabled IIO Channels",
      "The types of the channels that are enabled when pushing the buffer");
}

void IIOBufferRead::initialize() {
  HOLOSCAN_LOG_INFO("IIOBufferRead initialize");
  Operator::initialize();

  if (ctx_p_.get().empty()) {
    HOLOSCAN_LOG_ERROR("IIO Context is not set. Cannot use operator.");
    return;
  }

  if (dev_p_.get().empty()) {
    HOLOSCAN_LOG_ERROR("IIO Device is not set. Cannot use operator.");
    return;
  }

  if (enabled_channel_names_p_.get().empty() || enabled_channel_types_p_.get().empty()) {
    HOLOSCAN_LOG_ERROR(
        "It is mandatory to enable at least one channel before creating the "
        "IIO buffer.");
    return;
  }

  if (samples_count_p_.get() == 0) {
    HOLOSCAN_LOG_ERROR("Samples count is not set. Cannot use operator.");
    return;
  }

  ctx_ = iio_create_context_from_uri(ctx_p_.get().c_str());
  if (!ctx_) {
    HOLOSCAN_LOG_ERROR("Failed to create context with uri {}", ctx_p_.get());
    return;
  }

  dev_ = iio_context_find_device(ctx_, dev_p_.get().c_str());
  if (!dev_) {
    HOLOSCAN_LOG_ERROR("Failed to find device {}", dev_p_.get());
    return;
  }

  std::vector<std::string>& enabled_channel_names = enabled_channel_names_p_.get();
  std::vector<bool>& enabled_channel_types = enabled_channel_types_p_.get();
  for (size_t i = 0; i < enabled_channel_names.size(); ++i) {
    std::string& chn_name = enabled_channel_names[i];
    bool chn_type = enabled_channel_types[i];
    iio_channel* chn = iio_device_find_channel(dev_, chn_name.c_str(), chn_type);
    if (!chn) {
      HOLOSCAN_LOG_ERROR("Failed to find {} channel {}", chn_type ? "output" : "input", chn_name);
      return;
    }

    if (!iio_channel_is_enabled(chn)) {
      HOLOSCAN_LOG_INFO("Enabled channel: {}", chn_name);
      iio_channel_enable(chn);
    } else {
      HOLOSCAN_LOG_INFO("Channel {} is already enabled", chn_name);
    }
  }

  ssize_t sample_size = iio_device_get_sample_size(dev_);
  if (sample_size < 0) {
    HOLOSCAN_LOG_ERROR(
        "Failed to get sample size from device {}; Err code: {}", dev_p_.get(), sample_size_);
    return;
  }
  sample_size_ = static_cast<size_t>(sample_size);

  buffer_ = nullptr;
}

void IIOBufferRead::compute(InputContext&, OutputContext& op_output, ExecutionContext&) {
  HOLOSCAN_LOG_INFO("IIOBufferRead compute");
  auto buffer_info = std::shared_ptr<iio_buffer_info_t>(new iio_buffer_info_t);
  buffer_info->buffer = nullptr;
  buffer_info->samples_count = 0;

  if (!buffer_) {
    HOLOSCAN_LOG_INFO(
        "Creating buffer with {} samples of size {} bytes", samples_count_p_.get(), sample_size_);
    buffer_ = iio_device_create_buffer(dev_, samples_count_p_.get(), false);
    if (!buffer_) {
      HOLOSCAN_LOG_ERROR("Failed to create buffer, errro code {}", errno);
      return;
    }
  }

  ssize_t res = iio_buffer_refill(buffer_);
  if (res < 0) {
    HOLOSCAN_LOG_ERROR("Failed to refill buffer. Error code: {}", res);
    return;
  }

  size_t bytes_read = static_cast<size_t>(res);
  buffer_info->samples_count = bytes_read / sample_size_;

  void* buffer_data = iio_buffer_start(buffer_);
  buffer_info->buffer = new int8_t[bytes_read];
  memcpy(buffer_info->buffer, buffer_data, bytes_read);

  op_output.emit(buffer_info, "buffer");
}
