#pragma once

#include <holoscan/core/application.hpp>
#include <holoscan/core/conditions/gxf/boolean.hpp>
#include <holoscan/core/conditions/gxf/count.hpp>
#include <holoscan/core/endpoint.hpp>
#include <holoscan/core/forward_def.hpp>
#include <holoscan/core/operator.hpp>
#include "holoscan/holoscan.hpp"
#include "iio_params.hpp"

#include <iio.h>

static constexpr int G_NUM_READS = 10;
static constexpr const char* G_URI = "ip:192.168.2.1";
static constexpr int G_NUM_CHANNELS = 2;  // Set to 1 or 2 to control number of channels

namespace holoscan::ops {

class BasicPrinterOp : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(BasicPrinterOp);
  BasicPrinterOp() = default;
  ~BasicPrinterOp() = default;

  void setup(OperatorSpec& spec) override { spec.input<std::string>("value"); }

  void compute(InputContext& op_input, OutputContext&, ExecutionContext&) override {
    auto value = op_input.receive<std::string>("value").value();
    HOLOSCAN_LOG_INFO("IIOAttributeRead value: {}", value);
  }
};

class BasicEmitterOp : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(BasicEmitterOp);
  BasicEmitterOp() = default;
  ~BasicEmitterOp() = default;

  void setup(OperatorSpec& spec) override { spec.output<std::string>("value"); }

  void compute(InputContext&, OutputContext& op_output, ExecutionContext&) override {
    auto value = std::string("manual");
    op_output.emit(value, "value");
  }
};

class BasicIIOBufferEmitterOP : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(BasicIIOBufferEmitterOP);
  BasicIIOBufferEmitterOP() = default;
  ~BasicIIOBufferEmitterOP() = default;

  void setup(OperatorSpec& spec) override { spec.output<iio_buffer_info_t>("buffer"); }

  std::vector<int16_t> generateSineWave(ulong numSamples, float frequency, float amplitude,
                                        float sampleRate) {
    std::vector<int16_t> samples(numSamples);

    for (ulong i = 0; i < numSamples; ++i) {
      float t = i / sampleRate;
      samples[i] = amplitude * std::sin(2 * M_PI * frequency * t);
    }

    return samples;
  }

  void compute(InputContext&, OutputContext& op_output, ExecutionContext&) override {
    uint enabled_channels = G_NUM_CHANNELS;
    ulong total_samples = 8192;                            // Total samples PER CHANNEL
    ulong buffer_size = total_samples * enabled_channels;  // Total buffer size
    float frequency = 8;
    float amplitude = 408;
    float sample_rate = 400;

    // These variables are just for conversion (pluto does not necessarily need them)
    std::string device_name = "cf-ad9361-dds-core-lpc";
    std::string channel_name = "voltage0";
    std::string channel_name2 = "voltage1";

    iio_context* ctx = iio_create_context_from_uri(G_URI);
    iio_device* dev = iio_context_find_device(ctx, device_name.c_str());
    iio_channel* chn = iio_device_find_channel(dev, channel_name.c_str(), true);
    iio_channel* chn2 = iio_device_find_channel(dev, channel_name2.c_str(), true);

    std::vector<int16_t> data_vector =
        generateSineWave(total_samples, frequency, amplitude, sample_rate);
    std::vector<int16_t> data_vector2 =
        generateSineWave(total_samples, frequency, amplitude / 2, sample_rate);

    auto buffer_info = std::shared_ptr<iio_buffer_info_t>(new iio_buffer_info_t);
    buffer_info->buffer = new int16_t[buffer_size];  // pluto has a sample size of 2 bytes
    buffer_info->is_cyclic = true;
    buffer_info->device_name = device_name;

    // Populate enabled channels
    iio_channel_info_t ch1_info;
    ch1_info.name = channel_name;
    ch1_info.is_output = true;
    buffer_info->enabled_channels.push_back(ch1_info);

    if (enabled_channels == 2) {
      iio_channel_info_t ch2_info;
      ch2_info.name = channel_name2;
      ch2_info.is_output = true;
      buffer_info->enabled_channels.push_back(ch2_info);
    }

    // Interleave samples for multi-channel setup
    for (size_t sample_idx = 0; sample_idx < total_samples; ++sample_idx) {
      size_t buffer_idx = sample_idx * enabled_channels;

      // Channel 0
      static_cast<int16_t*>(buffer_info->buffer)[buffer_idx] = data_vector[sample_idx];
      iio_channel_convert_inverse(chn,
                                  static_cast<int16_t*>(buffer_info->buffer) + buffer_idx,
                                  static_cast<int16_t*>(buffer_info->buffer) + buffer_idx);

      // Channel 1 (if enabled)
      if (enabled_channels == 2) {
        static_cast<int16_t*>(buffer_info->buffer)[buffer_idx + 1] = data_vector2[sample_idx];
        iio_channel_convert_inverse(chn2,
                                    static_cast<int16_t*>(buffer_info->buffer) + buffer_idx + 1,
                                    static_cast<int16_t*>(buffer_info->buffer) + buffer_idx + 1);
      }
    }

    // samples_count represents the number of samples per channel
    buffer_info->samples_count = total_samples;

    // Emit the buffer info
    op_output.emit(buffer_info, "buffer");
  }
};

class BasicIIOBufferPrinterOP : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(BasicIIOBufferPrinterOP);
  BasicIIOBufferPrinterOP() = default;
  ~BasicIIOBufferPrinterOP() = default;

  void setup(OperatorSpec& spec) override { spec.input<iio_buffer_info_t>("buffer"); }

  void compute(InputContext& op_input, OutputContext&, ExecutionContext&) override {
    auto buffer_info = op_input.receive<std::shared_ptr<iio_buffer_info_t>>("buffer").value();
    if (buffer_info->buffer == nullptr) {
      HOLOSCAN_LOG_ERROR("Buffer is null");
      return;
    }

    uint enabled_channels = buffer_info->enabled_channels.size();

    // Print the buffer info including new fields
    HOLOSCAN_LOG_INFO(
        "Buffer info: samples_count = {}, device = {}, cyclic = {}, enabled_channels = {}",
        buffer_info->samples_count,
        buffer_info->device_name,
        buffer_info->is_cyclic,
        buffer_info->enabled_channels.size());

    // Print channel information
    for (const auto& ch : buffer_info->enabled_channels) {
      HOLOSCAN_LOG_INFO("  Channel: {} ({})", ch.name, ch.is_output ? "output" : "input");
    }

    // Print first few samples
    const size_t samples_to_print = 100;  // Per channel
    HOLOSCAN_LOG_INFO("First {} samples per channel:", samples_to_print);

    if (enabled_channels == 1) {
      std::cout << "Channel 0: ";
      for (size_t i = 0; i < std::min(samples_to_print, buffer_info->samples_count); ++i) {
        std::cout << static_cast<int16_t*>(buffer_info->buffer)[i] << " ";
      }
      std::cout << std::endl;
    } else {
      // Print interleaved samples for each channel
      std::cout << "Channel 0: ";
      for (size_t i = 0; i < std::min(samples_to_print, buffer_info->samples_count); ++i) {
        std::cout << static_cast<int16_t*>(buffer_info->buffer)[i * enabled_channels] << " ";
      }
      std::cout << std::endl;

      std::cout << "Channel 1: ";
      for (size_t i = 0; i < std::min(samples_to_print, buffer_info->samples_count); ++i) {
        std::cout << static_cast<int16_t*>(buffer_info->buffer)[i * enabled_channels + 1] << " ";
      }
      std::cout << std::endl;
    }
  }
};

class BasicWaitOp : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(BasicWaitOp);
  BasicWaitOp() = default;
  ~BasicWaitOp() = default;

  void setup(OperatorSpec&) override {}
  void compute(InputContext&, OutputContext&, ExecutionContext&) override { sleep(20); }
};

}  // namespace holoscan::ops
