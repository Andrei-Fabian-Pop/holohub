#pragma once

#include <holoscan/core/application.hpp>
#include <holoscan/core/conditions/gxf/boolean.hpp>
#include <holoscan/core/conditions/gxf/count.hpp>
#include <holoscan/core/endpoint.hpp>
#include <holoscan/core/forward_def.hpp>
#include <holoscan/core/operator.hpp>
#include "holoscan/holoscan.hpp"

#include "fft.hpp"
#include "iio_buffer_read.hpp"
#include "iio_buffer_write.hpp"
#include "support_operators.hpp"

using namespace holoscan;

class PlutoFFTRealtimeExample : public holoscan::Application {
 public:
  PlutoFFTRealtimeExample() { name_ = "PlutoFFTRealtimeExample"; }

  void compose() override {
    // IIOBufferRead operator - reads data from Pluto SDR
    auto iio_buf_read_op = make_operator<ops::IIOBufferRead>(
        "iio_buffer_read",
        Arg("ctx") = std::string(G_URI),
        Arg("dev") = std::string("cf-ad9361-lpc"),
        Arg("is_cyclic") = true,
        // Total samples for I/Q pairs: 16384 I/Q pairs = 32768 int16 values
        Arg("samples_count") = static_cast<size_t>(16384),
        Arg("enabled_channel_names") = std::vector<std::string>{"voltage0", "voltage1"},
        Arg("enabled_channel_output") = std::vector<bool>{false, false});

    // IIOChannelConvertOp - applies IIO channel conversion before processing
    auto iio_convert_op =
        make_operator<ops::IIOChannelConvertOp>("iio_convert", Arg("convert_channels") = true);

    // IIOBuffer2CudaTensorOp - converts IIO buffer to CUDA tensor
    auto buffer_to_tensor_op = make_operator<ops::IIOBuffer2CudaTensorOp>(
        "buffer_to_tensor",
        Arg("num_channels") = 2U,
        Arg("samples_per_channel") = 16384UL,  // Number of complex samples after I/Q conversion
        Arg("data_format") = std::string("interleaved_iq"),
        Arg("burst_size") = 16384,
        Arg("num_bursts") = 1,
        Arg("adc_bits") = 12);  // Pluto SDR uses 12-bit ADC

    // FFT operator - performs FFT on the CUDA tensor with Hann window
    auto fft_op =
        make_operator<ops::FFT>("fft",
                                Arg("burst_size") = 16384,
                                Arg("num_bursts") = 1,
                                Arg("num_channels") = static_cast<uint16_t>(1),
                                Arg("spectrum_type") = static_cast<uint8_t>(0),
                                Arg("averaging_type") = static_cast<uint8_t>(0),
                                Arg("window_time") = static_cast<uint8_t>(0),
                                Arg("window_type") = static_cast<uint8_t>(2),  // Hann window
                                Arg("transform_points") = static_cast<uint32_t>(16384),
                                Arg("window_points") = static_cast<uint32_t>(16384),
                                Arg("resolution") = static_cast<uint64_t>(1875),  // 30.72MHz/16384
                                Arg("span") = static_cast<uint64_t>(30720000),
                                Arg("weighting_factor") = 1.0f,
                                Arg("f1_index") = static_cast<int32_t>(0),
                                Arg("f2_index") = static_cast<int32_t>(16383),
                                Arg("window_time_delta") = static_cast<uint32_t>(1000));

    // FFTGnuplotRealtimeOp - real-time plotting with gnuplot
    auto fft_gnuplot_realtime_op = make_operator<ops::FFTGnuplotRealtimeOp>(
        "fft_gnuplot_realtime",
        Arg("max_frequency") = 30720000.0f,  // 30.72 MHz sample rate
        Arg("log_scale") = true,
        Arg("power_offset") = 0.0f,  // Can be adjusted for calibration
        Arg("adc_bits") = 12,
        Arg("update_interval") = 100,                          // Update every 100ms
        Arg("y_range") = std::vector<float>{-160.0f, 10.0f});  // dB range

    // Connect the operators in the specified flow
    add_flow(iio_buf_read_op, iio_convert_op, {{"buffer", "buffer_in"}});
    add_flow(iio_convert_op, buffer_to_tensor_op, {{"buffer_out", "buffer"}});
    add_flow(buffer_to_tensor_op, fft_op, {{"tensor", "in"}});
    add_flow(fft_op, fft_gnuplot_realtime_op, {{"out", "buffer"}});
  }

 private:
  std::string name_;
};

static int pluto_fft_realtime_main(int argc, char** argv) {
  auto app = holoscan::make_application<PlutoFFTRealtimeExample>();
  app->run();

  return 0;
}
