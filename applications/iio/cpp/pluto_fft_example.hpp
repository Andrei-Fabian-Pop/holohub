#pragma once

#include <holoscan/core/application.hpp>
#include <holoscan/core/conditions/gxf/boolean.hpp>
#include <holoscan/core/conditions/gxf/count.hpp>
#include <holoscan/core/endpoint.hpp>
#include <holoscan/core/forward_def.hpp>
#include <holoscan/core/operator.hpp>
#include "holoscan/holoscan.hpp"

#include "iio_buffer_read.hpp"
#include "iio_buffer_write.hpp"
#include "support_operators.hpp"
#include "fft.hpp"

using namespace holoscan;

// TODO: Add a separate namespace for the new files (where needed)

class PlutoFFTExample : public holoscan::Application {
 public:
  PlutoFFTExample() { name_ = "PlutoFFTExample"; }

  void compose() override {
    std::vector<std::string> enabled_channels = {"voltage0"};
    std::vector<bool> enabled_channels_output = {true};

    auto iio_buffer_emitter_op =
        make_operator<ops::BasicIIOBufferEmitterOP>("basic_buffer_emitter_op");

    auto iio_buffer_write_op =
        make_operator<ops::IIOBufferWrite>("iio_buffer_write",
                                           Arg("ctx") = std::string(G_URI),
                                           Arg("dev") = std::string("cf-ad9361-dds-core-lpc"),
                                           Arg("is_cyclic") = true,
                                           Arg("enabled_channel_names") = enabled_channels,
                                           Arg("enabled_channel_output") = enabled_channels_output);

    auto iio_buf_read_op = make_operator<ops::IIOBufferRead>(
        "iio_buffer_read",
        Arg("ctx") = std::string(G_URI),
        Arg("dev") = std::string("cf-ad9361-lpc"),
        Arg("is_cyclic") = true,
        Arg("samples_count") = static_cast<size_t>(8192),
        Arg("enabled_channel_names") = std::vector<std::string>{"voltage0"},
        Arg("enabled_channel_output") = std::vector<bool>{false});

    auto fft_op = make_operator<ops::FFT>("fft",
                                      Arg("burst_size") = 1024,
                                      Arg("num_bursts") = 8,
                                      Arg("num_channels") = static_cast<uint16_t>(1),
                                      Arg("spectrum_type") = static_cast<uint8_t>(0),
                                      Arg("averaging_type") = static_cast<uint8_t>(0),
                                      Arg("window_time") = static_cast<uint8_t>(0),
                                      Arg("window_type") = static_cast<uint8_t>(0),
                                      Arg("transform_points") = static_cast<uint32_t>(1024),
                                      Arg("window_points") = static_cast<uint32_t>(1024),
                                      Arg("resolution") = static_cast<uint64_t>(1000),
                                      Arg("span") = static_cast<uint64_t>(1000000),
                                      Arg("weighting_factor") = 1.0f,
                                      Arg("f1_index") = static_cast<int32_t>(0),
                                      Arg("f2_index") = static_cast<int32_t>(1023),
                                      Arg("window_time_delta") = static_cast<uint32_t>(1000));

    auto basic_buffer_printer_op =
        make_operator<ops::BasicIIOBufferPrinterOP>("basic_buffer_printer_op");

    // add_flow(iio_buffer_emitter_op, iio_buffer_write_op, {{"buffer", "buffer"}});
    // add_flow(iio_buffer_write_op, iio_buf_read_op);
    add_flow(iio_buf_read_op, fft_op);
    add_flow(fft_op, basic_buffer_printer_op);
  }

 private:
  std::string name_;
};

static int pluto_fft_main(int argc, char** argv) {
  auto app = holoscan::make_application<PlutoFFTExample>();
  app->run();

  return 0;
}
