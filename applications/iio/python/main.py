import os

from holoscan.core import Application, Operator, OperatorSpec
from holohub.iio_controller import IIOAttributeRead


class BasicPrintOp(Operator):
    """A simple operator that prints a message."""

    def __init__(self, fragment, *args, **kwargs):
        super().__init__(fragment, *args, **kwargs)

    def setup(self, spec: OperatorSpec):
        """Setup the operator."""
        spec.input("value")

    def compute(self, op_input, op_output, context):
        """Compute method to print a message."""
        if not self._initialized:
            self._initialize()

        value = op_input.receive("value")
        print(f"Received value: {value}")


class MyApp(Application):
    def __init__(self, *args, **kwargs):
        """Init the application."""
        super().__init__(*args, *kwargs)

        self.name = "IIOAttributeRead Example"

    def compose(self):
        """Compose the application."""
        # Create an IIOAttributeRead operator
        iio_read = IIOAttributeRead(
            name="iio_read",
            attribute_name="voltage0",
            device_name="iio:device0",
            output_stream_name="output_stream",
        )

        self.add_flow(iio_read)


if __name__ == "__main__":
    config_file = os.path.join(os.path.dirname(__file__), "../iio_config.yaml")
    app = MyApp()
    app.config(config_file)
    app.run()
