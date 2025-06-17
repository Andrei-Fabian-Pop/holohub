import os
import math
import iio

from holoscan.core import Application, Operator, OperatorSpec
from holoscan.conditions import CountCondition
from holohub.iio_controller import IIOAttributeRead, IIOAttributeWrite

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
    ) -> list:
        """Generates an array with samples of a sine wave.

        Args:
            num_samples (int): The total number of samples to generate.
            frequency (float): The frequency of the sine wave in Hertz (Hz).
            amplitude (float): The amplitude of the sine wave.
            sample_rate (float): The number of samples per second (Hz).

        Returns:
            list: A list containing the samples of the sine wave.
        """
        sine_wave = []
        for i in range(num_samples):
            time = i / sample_rate
            sample_value = amplitude * math.sin(2 * math.pi * frequency * time)
            sine_wave.append(sample_value)

        return sine_wave

    def compute(self, op_input, op_output, context):
        enabled_channels: int = 1
        num_samples: int = 8192 * enabled_channels
        frequency: float = 8
        amplitude: float = 408
        sample_rate: float = 400

        device_name: str = "cf-ad9361-dds-core-lpc"
        channel0_name: str = "voltage0"
        channel1_name: str = "voltage1"

        ctx: iio.Context = iio.Context(_context=G_URI)
        dev: iio.Device | iio.Trigger | None = ctx.find_device(device_name)
        if dev is None:
            print(f"Device {device_name} was not found.")

        # channel0 = dev.find_channel(channel0_name, True)


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
        pass

    def buffer_write_example(self):
        pass

    def configurator_example(self):
        pass

    def compose(self):
        """Compose the application."""
        # self.attr_read_example()
        self.attr_write_example()


if __name__ == "__main__":
    config_file = os.path.join(os.path.dirname(__file__), "../iio_config.yaml")
    app = MyApp()
    app.config(config_file)
    app.run()
