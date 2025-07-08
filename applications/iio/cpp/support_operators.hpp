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

#include <holoscan/core/application.hpp>
#include <holoscan/core/conditions/gxf/boolean.hpp>
#include <holoscan/core/conditions/gxf/count.hpp>
#include <holoscan/core/endpoint.hpp>
#include <holoscan/core/forward_def.hpp>
#include <holoscan/core/operator.hpp>
#include "holoscan/holoscan.hpp"
#include "iio_params.hpp"

#include <cuda_runtime.h>
#include <iio.h>
#include <matx.h>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <vector>

using namespace matx;
using complex = cuda::std::complex<float>;

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

class IIOChannelConvertOp : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(IIOChannelConvertOp);
  IIOChannelConvertOp() = default;
  ~IIOChannelConvertOp() = default;

  void setup(OperatorSpec& spec) override {
    spec.input<iio_buffer_info_t>("buffer_in");
    spec.output<iio_buffer_info_t>("buffer_out");
    spec.param(convert_channels_,
               "convert_channels",
               "Convert channels",
               "Apply IIO channel conversion",
               true);
  }

  void initialize() override {
    Operator::initialize();

    // Store IIO context and channels for conversion
    iio_context_ = nullptr;
    iio_device_ = nullptr;
  }

  void compute(InputContext& op_input, OutputContext& op_output, ExecutionContext&) override {
    auto buffer_info = op_input.receive<std::shared_ptr<iio_buffer_info_t>>("buffer_in").value();

    if (!buffer_info || !buffer_info->buffer) {
      HOLOSCAN_LOG_ERROR("IIOChannelConvertOp: Invalid buffer received");
      return;
    }

    // Create output buffer info (copy input structure)
    auto output_buffer_info = std::make_shared<iio_buffer_info_t>();
    output_buffer_info->samples_count = buffer_info->samples_count;
    output_buffer_info->is_cyclic = buffer_info->is_cyclic;
    output_buffer_info->device_name = buffer_info->device_name;
    output_buffer_info->enabled_channels = buffer_info->enabled_channels;

    // Get buffer properties
    const size_t num_channels = buffer_info->enabled_channels.size();
    const size_t samples_per_channel = buffer_info->samples_count;
    const size_t total_samples = samples_per_channel * num_channels;

    // Allocate output buffer
    output_buffer_info->buffer = new int16_t[total_samples];

    if (convert_channels_.get()) {
      // Apply IIO channel conversion
      convertChannelData(buffer_info, output_buffer_info, num_channels, samples_per_channel);
    } else {
      // Just copy data without conversion
      std::memcpy(output_buffer_info->buffer, buffer_info->buffer, total_samples * sizeof(int16_t));
    }

    // Emit the converted buffer
    op_output.emit(output_buffer_info, "buffer_out");
  }

 private:
  void convertChannelData(std::shared_ptr<iio_buffer_info_t> input_buffer,
                          std::shared_ptr<iio_buffer_info_t> output_buffer, size_t num_channels,
                          size_t samples_per_channel) {
    // Get or create IIO context if needed
    if (!iio_context_) {
      iio_context_ = iio_create_context_from_uri(G_URI);
      if (!iio_context_) {
        HOLOSCAN_LOG_ERROR("Failed to create IIO context for channel conversion");
        return;
      }
    }

    // Get device
    if (!iio_device_) {
      iio_device_ = iio_context_find_device(iio_context_, input_buffer->device_name.c_str());
      if (!iio_device_) {
        HOLOSCAN_LOG_ERROR("Failed to find IIO device: {}", input_buffer->device_name);
        return;
      }
    }

    // Get IIO channels for conversion
    std::vector<iio_channel*> iio_channels;
    for (const auto& ch_info : input_buffer->enabled_channels) {
      iio_channel* ch =
          iio_device_find_channel(iio_device_, ch_info.name.c_str(), ch_info.is_output);
      if (ch) {
        iio_channels.push_back(ch);
      } else {
        HOLOSCAN_LOG_WARN("Could not find IIO channel: {}", ch_info.name);
      }
    }

    const int16_t* input_data = static_cast<const int16_t*>(input_buffer->buffer);
    int16_t* output_data = static_cast<int16_t*>(output_buffer->buffer);

    if (num_channels == 1) {
      // Single channel conversion
      if (!iio_channels.empty()) {
        iio_channel* ch = iio_channels[0];

        for (size_t i = 0; i < samples_per_channel; ++i) {
          // Apply IIO channel conversion (handles scaling, offset, etc.)
          iio_channel_convert(ch, &output_data[i], &input_data[i]);
        }

        HOLOSCAN_LOG_DEBUG("Applied IIO channel conversion for single channel: {}",
                           input_buffer->enabled_channels[0].name);
      } else {
        // Fallback: copy without conversion
        std::memcpy(output_data, input_data, samples_per_channel * sizeof(int16_t));
      }
    } else {
      // Multi-channel conversion with interleaved data
      for (size_t sample = 0; sample < samples_per_channel; ++sample) {
        for (size_t ch = 0; ch < num_channels && ch < iio_channels.size(); ++ch) {
          size_t idx = sample * num_channels + ch;

          // Apply IIO channel conversion for each channel
          iio_channel_convert(iio_channels[ch], &output_data[idx], &input_data[idx]);
        }
      }

      HOLOSCAN_LOG_DEBUG("Applied IIO channel conversion for {} channels", num_channels);
    }
  }

  Parameter<bool> convert_channels_;

  // IIO context and device for channel conversion
  iio_context* iio_context_;
  iio_device* iio_device_;
};

class IIOBuffer2CudaTensorOp : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(IIOBuffer2CudaTensorOp);
  IIOBuffer2CudaTensorOp() = default;
  ~IIOBuffer2CudaTensorOp() = default;

  void setup(OperatorSpec& spec) override {
    spec.input<iio_buffer_info_t>("buffer");
    spec.output<std::tuple<tensor_t<complex, 2>, cudaStream_t>>("tensor");
    spec.param(
        num_channels_, "num_channels", "Number of channels", "Number of channels in the data", 1U);
    spec.param(samples_per_channel_,
               "samples_per_channel",
               "Samples per channel",
               "Number of samples per channel",
               8192UL);
    spec.param(data_format_,
               "data_format",
               "Data format",
               "Format of the input data (interleaved_iq)",
               std::string("interleaved_iq"));
    spec.param(
        burst_size_, "burst_size", "Burst size", "Number of samples per burst for FFT", 1024);
    spec.param(num_bursts_, "num_bursts", "Number of bursts", "Number of bursts for FFT", 8);
    spec.param(adc_bits_, "adc_bits", "ADC bits", "ADC resolution in bits", 11);
  }

  void initialize() override {
    Operator::initialize();

    // Create CUDA stream
    cudaStreamCreate(&stream_);

    // Pre-allocate output tensor with shape matching FFT expectations
    // For FFT operator: (num_bursts, burst_size)
    make_tensor(output_tensor_,
                {static_cast<index_t>(num_bursts_.get()), static_cast<index_t>(burst_size_.get())});
  }

  void compute(InputContext& op_input, OutputContext& op_output, ExecutionContext&) override {
    auto buffer_info = op_input.receive<std::shared_ptr<iio_buffer_info_t>>("buffer").value();

    if (!buffer_info || !buffer_info->buffer) {
      HOLOSCAN_LOG_ERROR("IIOBuffer2CudaTensorOp: Invalid buffer received");
      return;
    }

    // Get buffer properties
    const int16_t* samples = static_cast<const int16_t*>(buffer_info->buffer);
    const size_t num_channels = buffer_info->enabled_channels.size();
    const size_t samples_per_channel = buffer_info->samples_count;

    // Validate dimensions
    if (num_channels != num_channels_.get() || samples_per_channel != samples_per_channel_.get()) {
      HOLOSCAN_LOG_WARN("Buffer dimensions mismatch: expected {}x{}, got {}x{}",
                        num_channels_.get(),
                        samples_per_channel_.get(),
                        num_channels,
                        samples_per_channel);
    }

    // Convert data based on format
    if (data_format_.get() == "interleaved_iq") {
      convertInterleavedIQToComplex(samples, samples_per_channel * 2);
      // For single channel with interleaved I/Q data
      // if (num_channels == 1) {
      //   // samples_per_channel is the number of I/Q pairs (complex samples)
      //   // But the raw buffer has 2x that many int16 values (I and Q separate)
      // } else {
      //   // For multiple channels with interleaved channel data
      //   convertMultiChannelToComplex(samples, num_channels, samples_per_channel);
      // }
    }

    // Emit the tensor with stream
    op_output.emit(std::make_tuple(output_tensor_, stream_), "tensor");
  }

 private:
  void convertInterleavedIQToComplex(const int16_t* samples, size_t num_samples) {
    // Use proper ADC scaling based on bit depth (like GNU Radio)
    // Scale by the actual ADC resolution, not the int16 container size
    float adc_scale = 1.0f / (1 << adc_bits_.get());

    // Create temporary host buffer
    size_t total_complex_samples = num_samples / 2;  // I/Q pairs to complex
    std::vector<complex> host_data(total_complex_samples);

    // Convert interleaved I/Q samples to complex with proper ADC scaling
    for (size_t i = 0; i < total_complex_samples; ++i) {
      // Apply ADC scaling instead of full-scale int16 scaling
      float real = static_cast<float>(samples[i * 2]) * adc_scale;
      float imag = static_cast<float>(samples[i * 2 + 1]) * adc_scale;
      host_data[i] = complex(real, imag);
    }

    // Copy all data to GPU at once
    cudaMemcpyAsync(output_tensor_.Data(),
                    host_data.data(),
                    total_complex_samples * sizeof(complex),
                    cudaMemcpyHostToDevice,
                    stream_);
  }

  void convertMultiChannelToComplex(const int16_t* samples, size_t num_channels,
                                    size_t samples_per_channel) {
    // Scale factor to convert int16 to float [-1.0, 1.0]
    constexpr float scalar = 1.0f / 32767.0f;

    // Create temporary host buffer
    std::vector<complex> host_data(num_channels * samples_per_channel);

    // Convert multi-channel interleaved data
    for (size_t ch = 0; ch < num_channels; ++ch) {
      for (size_t s = 0; s < samples_per_channel; ++s) {
        // Assuming real-only data for multi-channel (not I/Q pairs)
        float real = samples[s * num_channels + ch] * scalar;
        host_data[ch * samples_per_channel + s] = complex(real, 0.0f);
      }
    }

    // Copy to GPU
    cudaMemcpyAsync(output_tensor_.Data(),
                    host_data.data(),
                    num_channels * samples_per_channel * sizeof(complex),
                    cudaMemcpyHostToDevice,
                    stream_);
  }

  Parameter<unsigned int> num_channels_;
  Parameter<size_t> samples_per_channel_;
  Parameter<std::string> data_format_;
  Parameter<int> burst_size_;
  Parameter<int> num_bursts_;
  Parameter<int> adc_bits_;

  cudaStream_t stream_;
  tensor_t<complex, 2> output_tensor_;
};

class FFTTensorPrinterOp : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(FFTTensorPrinterOp);
  FFTTensorPrinterOp() = default;
  ~FFTTensorPrinterOp() = default;

  void setup(OperatorSpec& spec) override {
    spec.input<std::tuple<tensor_t<complex, 2>, cudaStream_t>>("buffer");
    spec.param(samples_to_print_,
               "samples_to_print",
               "Samples to print",
               "Number of FFT samples to print",
               20UL);
  }

  void compute(InputContext& op_input, OutputContext&, ExecutionContext&) override {
    auto tensor_data =
        op_input.receive<std::tuple<tensor_t<complex, 2>, cudaStream_t>>("buffer").value();
    auto& tensor = std::get<0>(tensor_data);
    auto stream = std::get<1>(tensor_data);

    // Synchronize stream to ensure data is ready
    cudaStreamSynchronize(stream);

    // Get tensor dimensions
    auto shape = tensor.Shape();
    size_t num_bursts = shape[0];  // First dimension is bursts, not channels
    size_t burst_size = shape[1];  // FFT size per burst

    HOLOSCAN_LOG_INFO("FFT Output - Shape: {}x{} (bursts x FFT bins)", num_bursts, burst_size);

    // Copy data to host for printing
    size_t samples_to_print = std::min(samples_to_print_.get(), burst_size);
    std::vector<complex> host_data(num_bursts * samples_to_print);

    cudaMemcpy(host_data.data(),
               tensor.Data(),
               num_bursts * samples_to_print * sizeof(complex),
               cudaMemcpyDeviceToHost);

    // Print FFT results for each burst
    for (size_t burst = 0; burst < num_bursts; ++burst) {
      HOLOSCAN_LOG_INFO("Burst {}: First {} FFT bins (magnitude):", burst, samples_to_print);
      std::cout << "  ";
      for (size_t s = 0; s < samples_to_print; ++s) {
        complex val = host_data[burst * samples_to_print + s];
        float magnitude = cuda::std::abs(val);
        std::cout << std::fixed << std::setprecision(3) << magnitude << " ";
      }
      std::cout << std::endl;

      // Also print phase information for first few samples
      HOLOSCAN_LOG_INFO("Burst {}: Phase (radians) for first 10 bins:", burst);
      std::cout << "  ";
      for (size_t s = 0; s < std::min(10UL, samples_to_print); ++s) {
        complex val = host_data[burst * samples_to_print + s];
        float phase = cuda::std::arg(val);
        std::cout << std::fixed << std::setprecision(3) << phase << " ";
      }
      std::cout << std::endl;
    }

    // Print DC component and max magnitude
    complex dc_component = host_data[0];
    HOLOSCAN_LOG_INFO("DC Component: {} + {}i (magnitude: {})",
                      dc_component.real(),
                      dc_component.imag(),
                      cuda::std::abs(dc_component));

    // Find max magnitude across all bursts
    float max_magnitude = 0.0f;
    size_t max_burst = 0;
    size_t max_bin = 0;
    for (size_t burst = 0; burst < num_bursts; ++burst) {
      for (size_t bin = 0; bin < samples_to_print; ++bin) {
        float mag = cuda::std::abs(host_data[burst * samples_to_print + bin]);
        if (mag > max_magnitude) {
          max_magnitude = mag;
          max_burst = burst;
          max_bin = bin;
        }
      }
    }
    HOLOSCAN_LOG_INFO("Max magnitude: {} at burst {} bin {}", max_magnitude, max_burst, max_bin);
  }

 private:
  Parameter<size_t> samples_to_print_;
};

class FFTGnuplotOp : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(FFTGnuplotOp);
  FFTGnuplotOp() = default;
  ~FFTGnuplotOp() = default;

  void setup(OperatorSpec& spec) override {
    spec.input<std::tuple<tensor_t<complex, 2>, cudaStream_t>>("buffer");
    spec.param(output_file_,
               "output_file",
               "Output file",
               "Base name for output files",
               std::string("fft_spectrum"));
    spec.param(selected_burst_, "selected_burst", "Selected burst", "Which burst to plot", 0);
    spec.param(
        max_frequency_, "max_frequency", "Max frequency", "Maximum frequency (Hz)", 1000000.0f);
    spec.param(log_scale_, "log_scale", "Log scale", "Use logarithmic magnitude scale", true);
    spec.param(power_offset_, "power_offset", "Power offset", "Power offset in dB", 0.0f);
    spec.param(adc_bits_, "adc_bits", "ADC bits", "ADC resolution in bits", 12);
  }

  void compute(InputContext& op_input, OutputContext&, ExecutionContext&) override {
    auto tensor_data =
        op_input.receive<std::tuple<tensor_t<complex, 2>, cudaStream_t>>("buffer").value();
    auto& tensor = std::get<0>(tensor_data);
    auto stream = std::get<1>(tensor_data);

    // Synchronize stream to ensure data is ready
    cudaStreamSynchronize(stream);

    // Get tensor dimensions
    auto shape = tensor.Shape();
    size_t num_bursts = shape[0];
    size_t burst_size = shape[1];

    // Select which burst to visualize
    size_t selected_burst = std::min(static_cast<size_t>(selected_burst_.get()), num_bursts - 1);

    HOLOSCAN_LOG_INFO(
        "Generating gnuplot for burst {} with {} FFT bins", selected_burst, burst_size);

    // Copy data for selected burst to host
    std::vector<complex> host_data(burst_size);
    cudaMemcpy(host_data.data(),
               tensor.Data() + selected_burst * burst_size,
               burst_size * sizeof(complex),
               cudaMemcpyDeviceToHost);

    // Convert to magnitude spectrum using GNU Radio method
    std::vector<float> magnitude_spectrum(burst_size);

    // FFT normalization factor (like GNU Radio's mult_const1)
    float fft_normalization =
        1.0f / (static_cast<float>(burst_size) * static_cast<float>(burst_size));

    for (size_t i = 0; i < burst_size; ++i) {
      // Calculate magnitude squared (like GNU Radio's complex_to_mag_squared)
      float real = host_data[i].real();
      float imag = host_data[i].imag();
      float mag_squared = real * real + imag * imag;

      // Apply FFT size normalization
      float normalized_power = mag_squared * fft_normalization;

      // Apply logarithmic scale if enabled (like GNU Radio's nlog10)
      if (log_scale_.get()) {
        float db_value =
            (normalized_power > 1e-10f) ? 10.0f * std::log10(normalized_power) : -200.0f;
        magnitude_spectrum[i] = db_value + power_offset_.get();  // Add power offset
      } else {
        magnitude_spectrum[i] = std::sqrt(normalized_power);  // Convert back to magnitude
      }
    }

    // Write data file for gnuplot with frequency axis from -fs/2 to +fs/2
    std::string data_file = output_file_.get() + ".dat";
    std::ofstream data_stream(data_file);

    float freq_step = max_frequency_.get() / static_cast<float>(burst_size);
    float freq_step_mhz = freq_step / 1e6f;  // Convert to MHz

    for (size_t i = 0; i < burst_size; ++i) {
      // Map frequency axis to [-fs/2, +fs/2] range in MHz
      float frequency_mhz =
          (static_cast<float>(i) - static_cast<float>(burst_size) / 2.0f) * freq_step_mhz;
      data_stream << frequency_mhz << " " << magnitude_spectrum[i] << std::endl;
    }
    data_stream.close();

    // Find peak frequency for title
    auto peak_it = std::max_element(magnitude_spectrum.begin(), magnitude_spectrum.end());
    size_t peak_bin = std::distance(magnitude_spectrum.begin(), peak_it);
    // Calculate peak frequency in MHz using the same axis mapping
    float peak_freq_mhz =
        (static_cast<float>(peak_bin) - static_cast<float>(burst_size) / 2.0f) * freq_step_mhz;

    // Create gnuplot script
    std::string script_file = output_file_.get() + ".gp";
    std::ofstream script_stream(script_file);

    script_stream << "set terminal png size 1200,800\n";
    script_stream << "set output '" << output_file_.get() << ".png'\n";
    script_stream << "set title 'Pluto SDR FFT Spectrum - Peak at " << std::fixed
                  << std::setprecision(2) << peak_freq_mhz << " MHz (" << std::setprecision(2)
                  << *peak_it << " dB)'\n";
    script_stream << "set xlabel 'Frequency (MHz)'\n";

    if (log_scale_.get()) {
      script_stream << "set ylabel 'Magnitude (dB)'\n";
    } else {
      script_stream << "set ylabel 'Magnitude'\n";
    }

    script_stream << "set grid\n";
    script_stream << "set style line 1 linecolor rgb '#00ff00' linewidth 2\n";
    script_stream << "plot '" << data_file << "' with lines linestyle 1 title 'FFT Magnitude'\n";
    script_stream.close();

    // Execute gnuplot
    std::string gnuplot_cmd = "gnuplot " + script_file;
    int result = std::system(gnuplot_cmd.c_str());

    if (result == 0) {
      HOLOSCAN_LOG_INFO("Gnuplot spectrum saved to: {}.png", output_file_.get());
      HOLOSCAN_LOG_INFO(
          "Peak frequency: {:.2f} MHz with magnitude: {:.2f} dB", peak_freq_mhz, *peak_it);
    } else {
      HOLOSCAN_LOG_ERROR("Gnuplot execution failed with return code: {}", result);
    }

    // Display the plot using system image viewer (optional)
    std::string display_cmd = "xdg-open " + output_file_.get() + ".png &";
    std::system(display_cmd.c_str());

    // Clean up temporary files
    std::remove(data_file.c_str());
    std::remove(script_file.c_str());

    HOLOSCAN_LOG_INFO("FFT gnuplot visualization complete. Application will now exit.");

    // Request application shutdown after processing one spectrum
    std::exit(0);
  }

 private:
  Parameter<std::string> output_file_;
  Parameter<int> selected_burst_;
  Parameter<float> max_frequency_;
  Parameter<bool> log_scale_;
  Parameter<float> power_offset_;
  Parameter<int> adc_bits_;
};

class FFTGnuplotRealtimeOp : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(FFTGnuplotRealtimeOp);
  FFTGnuplotRealtimeOp() = default;
  ~FFTGnuplotRealtimeOp() {
    // Clean up gnuplot process
    if (gnuplot_pipe_) {
      pclose(gnuplot_pipe_);
    }
  }

  void setup(OperatorSpec& spec) override {
    spec.input<std::tuple<tensor_t<complex, 2>, cudaStream_t>>("buffer");
    spec.param(
        max_frequency_, "max_frequency", "Max frequency", "Maximum frequency (Hz)", 1000000.0f);
    spec.param(log_scale_, "log_scale", "Log scale", "Use logarithmic magnitude scale", true);
    spec.param(power_offset_, "power_offset", "Power offset", "Power offset in dB", 0.0f);
    spec.param(adc_bits_, "adc_bits", "ADC bits", "ADC resolution in bits", 12);
    spec.param(update_interval_,
               "update_interval",
               "Update interval",
               "Update interval in milliseconds",
               100);
    spec.param(y_range_,
               "y_range",
               "Y-axis range",
               "Y-axis range [min, max]",
               std::vector<float>{-200.0f, 0.0f});
  }

  void initialize() override {
    Operator::initialize();

    // Open persistent gnuplot pipe
    gnuplot_pipe_ = popen("gnuplot", "w");
    if (!gnuplot_pipe_) {
      HOLOSCAN_LOG_ERROR("Failed to open gnuplot pipe");
      return;
    }

    // Initialize gnuplot for real-time plotting
    fprintf(gnuplot_pipe_, "set terminal x11 noraise\n");
    fprintf(gnuplot_pipe_, "set title 'Pluto SDR Real-Time FFT Spectrum'\n");
    fprintf(gnuplot_pipe_, "set xlabel 'Frequency (MHz)'\n");

    if (log_scale_.get()) {
      fprintf(gnuplot_pipe_, "set ylabel 'Magnitude (dB)'\n");
      fprintf(gnuplot_pipe_, "set yrange [%f:%f]\n", y_range_.get()[0], y_range_.get()[1]);
    } else {
      fprintf(gnuplot_pipe_, "set ylabel 'Magnitude'\n");
    }

    fprintf(gnuplot_pipe_, "set grid\n");
    fprintf(gnuplot_pipe_, "set style line 1 linecolor rgb '#00ff00' linewidth 2\n");
    fflush(gnuplot_pipe_);

    // Initialize timing
    last_update_time_ = std::chrono::steady_clock::now();
  }

  void compute(InputContext& op_input, OutputContext&, ExecutionContext&) override {
    // Check update interval
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_update_time_)
            .count();

    if (elapsed < update_interval_.get()) {
      return;  // Skip this update
    }
    last_update_time_ = current_time;

    auto tensor_data =
        op_input.receive<std::tuple<tensor_t<complex, 2>, cudaStream_t>>("buffer").value();
    auto& tensor = std::get<0>(tensor_data);
    auto stream = std::get<1>(tensor_data);

    // Synchronize stream to ensure data is ready
    cudaStreamSynchronize(stream);

    // Get tensor dimensions
    auto shape = tensor.Shape();
    size_t num_bursts = shape[0];
    size_t burst_size = shape[1];

    // Select first burst for real-time display
    size_t selected_burst = 0;

    // Copy data for selected burst to host
    std::vector<complex> host_data(burst_size);
    cudaMemcpy(host_data.data(),
               tensor.Data() + selected_burst * burst_size,
               burst_size * sizeof(complex),
               cudaMemcpyDeviceToHost);

    // Convert to magnitude spectrum using GNU Radio method
    std::vector<float> magnitude_spectrum(burst_size);

    // FFT normalization factor
    float fft_normalization =
        1.0f / (static_cast<float>(burst_size) * static_cast<float>(burst_size));

    for (size_t i = 0; i < burst_size; ++i) {
      // Calculate magnitude squared
      float real = host_data[i].real();
      float imag = host_data[i].imag();
      float mag_squared = real * real + imag * imag;

      // Apply FFT size normalization
      float normalized_power = mag_squared * fft_normalization;

      // Apply logarithmic scale if enabled
      if (log_scale_.get()) {
        float db_value =
            (normalized_power > 1e-20f) ? 10.0f * std::log10(normalized_power) : -200.0f;
        magnitude_spectrum[i] = db_value + power_offset_.get();
      } else {
        magnitude_spectrum[i] = std::sqrt(normalized_power);
      }
    }

    // Send data to gnuplot
    fprintf(gnuplot_pipe_, "plot '-' with lines linestyle 1 title 'FFT Magnitude'\n");

    float freq_step = max_frequency_.get() / static_cast<float>(burst_size);
    float freq_step_mhz = freq_step / 1e6f;

    for (size_t i = 0; i < burst_size; ++i) {
      // Map frequency axis to [-fs/2, +fs/2] range in MHz
      float frequency_mhz =
          (static_cast<float>(i) - static_cast<float>(burst_size) / 2.0f) * freq_step_mhz;
      fprintf(gnuplot_pipe_, "%f %f\n", frequency_mhz, magnitude_spectrum[i]);
    }
    fprintf(gnuplot_pipe_, "e\n");
    fflush(gnuplot_pipe_);
  }

 private:
  Parameter<float> max_frequency_;
  Parameter<bool> log_scale_;
  Parameter<float> power_offset_;
  Parameter<int> adc_bits_;
  Parameter<int> update_interval_;
  Parameter<std::vector<float>> y_range_;

  FILE* gnuplot_pipe_ = nullptr;
  std::chrono::steady_clock::time_point last_update_time_;
};

}  // namespace holoscan::ops
