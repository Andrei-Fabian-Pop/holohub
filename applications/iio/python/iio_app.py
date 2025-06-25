import os
import math
import struct
import iio
from typing import List

from holoscan.core import Application, Operator, OperatorSpec
from holoscan.conditions import CountCondition
from holohub.iio_controller import IIOAttributeRead, IIOAttributeWrite, IIOConfigurator, IIOBufferWrite, IIOBufferRead, IIOBufferInfo, IIOChannelInfo

G_NUM_REPETITIONS = 10
G_URI = "ip:192.168.2.1"


class BasicPrintOp(Operator):
    """A simple operator that prints a message."""

    def __init__(self, fragment, *args, **kwargs):
        super().__init__(fragment, *args, **kwargs)

    def setup(self, spec: OperatorSpec):
        """Setup the operator."""
        spec.input("value")

    def compute(self, op_input, op_output, context):
        """Compute method to print a message."""
        value = op_input.receive("value")
        print(f"Received value: {value}")


class BasicEmitOp(Operator):
    """A simple operator that emits a message."""

    def __init__(self, fragment, *args, **kwargs):
        super().__init__(fragment, *args, **kwargs)

    def setup(self, spec: OperatorSpec):
        """Setup the operator."""
        spec.output("value")

    def compute(self, op_input, op_output, context):
        """Compute method to emit a message."""
        value = "nominal"
        op_output.emit(value, "value", "std::string")


class BasicIIOBufferEmitterOp(Operator):
    """A simple operator that emits a structure for the iio_buffer"""

    def __init__(self, fragment, *args, **kwargs):
        super().__init__(fragment, *args, **kwargs)

    def setup(self, spec: OperatorSpec):
        spec.output("buffer")

    def generate_sinewave(
            self,
            num_samples: int,
            frequency: float,
            amplitude: float,
            sample_rate: float
    ) -> List[int]:
        """Generates an array with samples of a sine wave.

        Args:
            num_samples (int): The total number of samples to generate.
            frequency (float): The frequency of the sine wave in Hertz (Hz).
            amplitude (float): The amplitude of the sine wave.
            sample_rate (float): The number of samples per second (Hz).

        Returns:
            List[int]: A list containing the samples of the sine wave as int16 values.
        """
        sine_wave = []
        for i in range(num_samples):
            time = i / sample_rate
            sample_value = int(
                amplitude * math.sin(2 * math.pi * frequency * time))
            sine_wave.append(sample_value)

        return sine_wave

    def compute(self, op_input, op_output, context):
        enabled_channels: int = 1  # NOTE: Setting this to 2 does not work perfectly
        num_samples: int = 8192 * enabled_channels
        frequency: float = 8
        amplitude: float = 408
        sample_rate: float = 400

        # These variables are just for conversion (pluto does not necessarily need them)
        device_name: str = "cf-ad9361-dds-core-lpc"
        channel_name: str = "voltage0"
        channel_name2: str = "voltage1"

        ctx = iio.Context(_context=G_URI)
        dev = ctx.find_device(device_name)
        if dev is None:
            print(f"Device {device_name} was not found.")
            return

        chn = dev.find_channel(channel_name, True)  # True = output channel
        chn2 = dev.find_channel(
            channel_name2, True) if enabled_channels == 2 else None

        data_vector = self.generate_sinewave(
            num_samples // enabled_channels, frequency, amplitude, sample_rate
        )
        data_vector2 = self.generate_sinewave(
            num_samples // enabled_channels, frequency, amplitude // 2, sample_rate
        )

        # Create buffer info structure
        buffer_info = IIOBufferInfo()

        # Create buffer data - pluto has a sample size of 2 bytes (int16)
        buffer_data = bytearray(num_samples * 2)
        chn.enabled = True

        for i in range(0, num_samples, enabled_channels):
            sample_idx = i // enabled_channels
            sample0 = data_vector[sample_idx]

            # Pack the sample into the buffer
            struct.pack_into('<h', buffer_data, i * 2, sample0)

            # Apply channel conversion if needed (simplified - the actual
            # conversion would require proper IIO channel conversion

            if enabled_channels == 2:
                sample1 = data_vector2[sample_idx]
                struct.pack_into('<h', buffer_data, (i + 1) * 2, sample1)

        # Set buffer info properties
        # 1 sample contains samples for all channels
        buffer_info.samples_count = num_samples // enabled_channels
        buffer_info.buffer = bytes(buffer_data)
        buffer_info.is_cyclic = True
        buffer_info.device_name = device_name

        # Populate enabled channels
        ch1_info = IIOChannelInfo()
        ch1_info.name = channel_name
        ch1_info.is_output = True
        buffer_info.enabled_channels = [ch1_info]

        if enabled_channels == 2:
            ch2_info = IIOChannelInfo()
            ch2_info.name = channel_name2
            ch2_info.is_output = True
            buffer_info.enabled_channels.append(ch2_info)

        # Emit the buffer info with the correct type
        op_output.emit(buffer_info, "buffer",
                       "std::shared_ptr<iio_buffer_info_t>")


class BasicWaitOp(Operator):
    """A simple operator that waits for a specified duration."""

    def __init__(self, fragment, *args, **kwargs):
        super().__init__(fragment, *args, **kwargs)

    def setup(self, spec: OperatorSpec):
        """Setup the operator."""
        pass

    def compute(self, op_input, op_output, context):
        """Compute method to wait."""
        import time
        time.sleep(20)


class BasicBufferPrinterOp(Operator):
    """A simple operator that prints buffer information."""

    def __init__(self, fragment, *args, **kwargs):
        super().__init__(fragment, *args, **kwargs)

    def setup(self, spec: OperatorSpec):
        """Setup the operator."""
        spec.input("buffer")

    def compute(self, op_input, op_output, context):
        """Compute method to print buffer information."""
        buffer_info = op_input.receive("buffer")

        if buffer_info is None or buffer_info.buffer is None:
            print("Error: Buffer is null")
            return

        enabled_channels = 1
        device_name = "cf-ad9361-dds-core-lpc"
        channel_name = "voltage0"
        channel_name2 = "voltage1"

        ctx = iio.Context(_context=G_URI)
        dev = ctx.find_device(device_name)
        if dev is None:
            print(f"Device {device_name} was not found.")
            return

        chn = dev.find_channel(channel_name, True)  # True = output channel
        chn2 = dev.find_channel(
            channel_name2, True) if enabled_channels == 2 else None

        # Convert buffer data back to int16 samples
        buffer_data = buffer_info.buffer
        samples = []

        for i in range(0, len(buffer_data), 2):
            if i + 1 < len(buffer_data):
                sample = struct.unpack('<h', buffer_data[i:i+2])[0]
                samples.append(sample)

        # Print the buffer info including
        print(f"Buffer info: samples_count = {buffer_info.samples_count}, "
              f"device = {buffer_info.device_name}, "
              f"cyclic = {buffer_info.is_cyclic}, "
              f"enabled_channels = {len(buffer_info.enabled_channels)}")

        # Print channel information
        for ch in buffer_info.enabled_channels:
            print(
                f"Channel: {ch.name} ({'output' if ch.is_output else 'input'})")

        # Print first X samples (matching C++ behavior)
        samples_to_print = 100
        print(f"First {samples_to_print} samples:")
        print(" ".join(str(sample)
              for sample in samples[:min(len(samples), samples_to_print)]))


class MyApp(Application):
    def __init__(self, *args, **kwargs):
        """Init the application."""
        super().__init__(*args, *kwargs)

        self.name = "IIOController Examples"

    def attr_read_example(self):
        """Example for reading an IIO attribute."""
        iio_read = IIOAttributeRead(
            self,
            ctx="ip:192.168.2.1",
            dev="ad9361-phy",
            attr_name="trx_rate_governor",
            name="iio_read",
        )

        basic_print_op = BasicPrintOp(
            self,
            CountCondition(self, G_NUM_REPETITIONS),
            name="basic_print_op"
        )

        self.add_flow(iio_read, basic_print_op, {("value", "value")})

    def attr_write_example(self):
        """Example for writing an IIO attribute."""
        iio_write = IIOAttributeWrite(
            self,
            ctx="ip:192.168.2.1",
            dev="ad9361-phy",
            attr_name="trx_rate_governor",
            name="iio_write",
        )

        basic_emit_op = BasicEmitOp(
            self,
            CountCondition(self, G_NUM_REPETITIONS),
            name="basic_emit_op"
        )

        self.add_flow(basic_emit_op, iio_write, {("value", "value")})

    def buffer_read_example(self):
        """Example for buffer read operations matching the C++ implementation."""

        # Create condition for IIO operations
        iio_rw_cond = CountCondition(self, 1)

        # Channel configuration matching C++ implementation
        enabled_channels_names_1 = ["voltage0"]
        enabled_channels_input = [False]  # False for input channels

        # Create IIO buffer read operator
        iio_buf_read_op = IIOBufferRead(
            self,
            iio_rw_cond,
            ctx=G_URI,
            dev="cf-ad9361-lpc",
            is_cyclic=True,
            samples_count=8192,
            enabled_channel_names=enabled_channels_names_1,
            enabled_channel_input=enabled_channels_input,
            name="iio_buffer_read"
        )

        # Create buffer printer operator
        basic_buffer_printer_op = BasicBufferPrinterOp(
            self,
            name="basic_buffer_printer_op"
        )

        # RX flow - connect buffer reader to buffer printer
        self.add_flow(iio_buf_read_op, basic_buffer_printer_op,
                      {("buffer", "buffer")})

    def buffer_write_example(self):
        """Example for buffer write operations matching the C++ implementation."""

        # Create condition for IIO operations
        iio_rw_cond = CountCondition(self, 1)

        enabled_channels_names_1 = ["voltage0"]
        enabled_channels_output = [True]  # True for output channels

        # Create IIO buffer write operator 1
        iio_buf_write_op_1 = IIOBufferWrite(
            self,
            iio_rw_cond,
            ctx=G_URI,
            dev="cf-ad9361-dds-core-lpc",
            is_cyclic=True,
            enabled_channel_names=enabled_channels_names_1,
            enabled_channel_output=enabled_channels_output,
            name="iio_buffer_write_1"
        )

        # Create buffer emitter operator
        basic_buffer_emitter_op = BasicIIOBufferEmitterOp(
            self,
            name="basic_buffer_emitter_op"
        )

        # Create wait operator for timing
        basic_wait_op = BasicWaitOp(
            self,
            name="basic_wait_op"
        )

        # TX flow - connect buffer emitter to buffer writer to wait
        self.add_flow(basic_buffer_emitter_op,
                      iio_buf_write_op_1, {("buffer", "buffer")})
        self.add_flow(iio_buf_write_op_1, basic_wait_op)

    def configurator_example(self):
        config_file_path = self.config().config_file
        print(f"Config file: {config_file_path}")
        iio_configurator = IIOConfigurator(
            self, name="iio_configurator_op", cfg=config_file_path)

        # self.start_op() will only run the configurator once
        self.add_flow(self.start_op(), iio_configurator)

    def compose(self):
        """Compose the application."""
        # self.attr_read_example()
        # self.attr_write_example()
        # self.buffer_write_example()
        self.buffer_read_example()
        # self.configurator_example()


if __name__ == "__main__":
    config_file = os.path.join(os.path.dirname(__file__), "iio_config.yaml")
    app = MyApp()
    app.config(config_file)
    app.run()
