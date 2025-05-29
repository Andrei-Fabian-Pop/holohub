#include <yaml-cpp/exceptions.h>
#include <filesystem>
#include <holoscan/core/conditions/gxf/boolean.hpp>
#include <holoscan/core/conditions/gxf/count.hpp>
#include <holoscan/core/endpoint.hpp>
#include <holoscan/core/forward_def.hpp>
#include <holoscan/core/operator.hpp>
#include "holoscan/holoscan.hpp"
#include "iio_attribute_read.hpp"
#include "iio_attribute_write.hpp"
#include "iio_buffer_read.hpp"
#include "iio_buffer_write.hpp"
#include "iio_configurator.hpp"
#include "iio_params.hpp"

static constexpr int G_NUM_READS = 10;
static constexpr const char* G_URI = "ip:192.168.2.1";

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
    uint enabled_channels = 1;  // NOTE: Setting this to 1 does not work perfectly
    ulong num_samples = 8192 * enabled_channels;
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
        generateSineWave(num_samples / enabled_channels, frequency, amplitude, sample_rate);
    std::vector<int16_t> data_vector2 =
        generateSineWave(num_samples / enabled_channels, frequency, amplitude / 2, sample_rate);

    auto buffer_info = std::shared_ptr<iio_buffer_info_t>(new iio_buffer_info_t);
    buffer_info->buffer = new int16_t[num_samples];  // pluto has a sample size of 2 bytes

    for (size_t i = 0; i < num_samples; i += enabled_channels) {
      static_cast<int16_t*>(buffer_info->buffer)[i] = data_vector[i / enabled_channels];
      iio_channel_convert_inverse(chn,
                                  static_cast<int16_t*>(buffer_info->buffer) + i,
                                  static_cast<int16_t*>(buffer_info->buffer) + i);

      if (enabled_channels == 2) {
        static_cast<int16_t*>(buffer_info->buffer)[i + 1] = data_vector2[i / enabled_channels];
        iio_channel_convert_inverse(chn2,
                                    static_cast<int16_t*>(buffer_info->buffer) + i + 1,
                                    static_cast<int16_t*>(buffer_info->buffer) + i + 1);
      }
    }

    // 1 sample contains samples for 2 channels
    buffer_info->samples_count = num_samples / enabled_channels;

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

    uint enabled_channels = 1;
    std::string device_name = "cf-ad9361-dds-core-lpc";
    std::string channel_name = "voltage0";
    std::string channel_name2 = "voltage1";

    iio_context* ctx = iio_create_context_from_uri(G_URI);
    iio_device* dev = iio_context_find_device(ctx, device_name.c_str());
    iio_channel* chn = iio_device_find_channel(dev, channel_name.c_str(), true);
    iio_channel* chn2 = iio_device_find_channel(dev, channel_name2.c_str(), true);

    for (size_t i = 0; i < buffer_info->samples_count; ++i) {
      iio_channel_convert(chn,
                          static_cast<int16_t*>(buffer_info->buffer) + i,
                          static_cast<int16_t*>(buffer_info->buffer) + i);

      if (enabled_channels == 2) {
        iio_channel_convert(chn2,
                            static_cast<int16_t*>(buffer_info->buffer) + i + 1,
                            static_cast<int16_t*>(buffer_info->buffer) + i + 1);
      }
    }

    // Print the buffer info
    HOLOSCAN_LOG_INFO("Buffer info: samples_count = {}", buffer_info->samples_count);
    for (size_t i = 0; i < buffer_info->samples_count; ++i) {
      std::cout << static_cast<int16_t*>(buffer_info->buffer)[i] << " ";
    }
    std::cout << std::endl;
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

class App : public holoscan::Application {
 public:
  void compose() override {
    HOLOSCAN_LOG_INFO("IIO Compose started");
    using namespace holoscan;

    auto stop_init_cond = make_condition<holoscan::BooleanCondition>("is_init");

    auto iio_rw_cond = make_condition<holoscan::CountCondition>("iio_read_cond", 1);
    // auto iio_read_op =
    //     make_operator<ops::IIOAttributeRead>("iio_attribute_read",
    //                                          Arg("ctx") = std::string(G_URI),
    //                                          Arg("dev") = std::string("ad9361-phy"),
    //                                          // Arg("chan") = std::string("voltage0"),
    //                                          // Arg("channel_is_output") = true,
    //                                          Arg("attr_name") = std::string("calib_mode"),
    //                                          iio_rw_cond);

    // auto iio_write_op =
    //     make_operator<ops::IIOAttributeWrite>("iio_attribute_write",
    //                                           Arg("ctx") = std::string(G_URI),
    //                                           Arg("dev") = std::string("ad9361-phy"),
    //                                           // Arg("chan") = std::string("voltage0"),
    //                                           // Arg("channel_is_output") = true,
    //                                           Arg("attr_name") = std::string("calib_mode"),
    //                                           iio_rw_cond);

    std::vector<std::string> enabled_channels_names_1 = {
        "voltage0",
        // "voltage1",
    };

    std::vector<std::string> enabled_channels_names_2 = {
        "voltage2",
    };

    std::vector<bool> enabled_channels_output = {
        true,
        // true,
    };

    std::vector<bool> enabled_channels_input = {
        false,
        // false,
    };

    auto iio_buf_write_op_1 =
        make_operator<ops::IIOBufferWrite>("iio_buffer_write_1",
                                           Arg("ctx") = std::string(G_URI),
                                           Arg("dev") = std::string("cf-ad9361-dds-core-lpc"),
                                           Arg("is_cyclic") = true,
                                           Arg("enabled_channel_names") = enabled_channels_names_1,
                                           Arg("enabled_channel_output") = enabled_channels_output,
                                           iio_rw_cond);

    auto iio_buf_write_op_2 =
        make_operator<ops::IIOBufferWrite>("iio_buffer_write_2",
                                           Arg("ctx") = std::string(G_URI),
                                           Arg("dev") = std::string("cf-ad9361-dds-core-lpc"),
                                           Arg("is_cyclic") = true,
                                           Arg("enabled_channel_names") = enabled_channels_names_2,
                                           Arg("enabled_channel_output") = enabled_channels_output,
                                           iio_rw_cond);

    auto iio_buf_read_op =
        make_operator<ops::IIOBufferRead>("iio_buffer_read",
                                          Arg("ctx") = std::string(G_URI),
                                          Arg("dev") = std::string("cf-ad9361-lpc"),
                                          Arg("is_cyclic") = true,
                                          Arg("samples_count") = static_cast<size_t>(8192),
                                          Arg("enabled_channel_names") = enabled_channels_names_1,
                                          Arg("enabled_channel_output") = enabled_channels_input,
                                          iio_rw_cond);

    // auto basic_printer_op = make_operator<ops::BasicPrinterOp>("basic_printer_op");
    // auto basic_emitter_op = make_operator<ops::BasicEmitterOp>("basic_emitter_op");
    auto basic_buffer_emitter_op =
        make_operator<ops::BasicIIOBufferEmitterOP>("basic_buffer_emitter_op");
    auto basic_buffer_printer_op =
        make_operator<ops::BasicIIOBufferPrinterOP>("basic_buffer_printer_op");

    auto basic_wait_op = make_operator<ops::BasicWaitOp>("basic_wait_op");

    // Write attr flow
    // add_flow(basic_emitter_op, iio_write_op, {{"value", "value"}});

    // Secondary flow, just for system setup
    auto config_file_path = config().config_file();
    auto iio_configurator_op = make_operator<ops::IIOConfigurator>(
        "iio_configurator_op", Arg("cfg") = std::string(config_file_path));
    add_flow(start_op(), iio_configurator_op);

    // TX flow
    // add_flow(basic_buffer_emitter_op, iio_buf_write_op_1, {{"buffer", "buffer"}});
    // add_flow(iio_buf_write_op_1, basic_wait_op);

    // add_flow(iio_buf_write_op_1, iio_buf_read_op, {{"buffer", "buffer"}});
    // add_flow(iio_buf_read_op, iio_buf_write_op_2, {{"buffer", "buffer"}});

    // RX flow
    // add_flow(iio_buf_read_op, basic_buffer_printer_op, {{"buffer", "buffer"}});
  }
};

int main(int argc, char** argv) {
  // Set the yaml config path
  auto config_path = std::filesystem::canonical(argv[0]).parent_path();
  config_path /= std::filesystem::path("iio_config.yaml");
  if (argc > 1) {
    config_path = std::filesystem::path(argv[1]);
  }

  auto app = holoscan::make_application<App>();
  app->config(config_path);
  app->run();

  return 0;
}
