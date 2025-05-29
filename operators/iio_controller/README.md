# IIO Controller Operator

## Overview

This operator aims to provide a simple interface for controlling IIO devices in
HoloHub applications.

## Description

This operator allows users to interact with IIO devices, enabling them to read
and write data from sensors, configure device parameters, and manage device
states. It abstracts some of the complexities involved in dealing with IIO
devices, making it easier for developers to integrate the devices into their
applications.

An IIO device is a type of device that provides an interface for reading and
writing data from sensors, ADCs (Analog-to-Digital Converters), DACs (Digital-to-Analog
Converters), and other devices. The IIO (Industrial I/O) subsystem in Linux
provides a standardized way to interact with these devices, allowing users to access
sensor data and configure device parameters through a unified interface.
It is commonly used for devices made by Analog Devices Inc.

## Requirements

- libiio (version 0.X)
- An IIO device compatible with the libiio library (or an emulator, for testing purposes)

## Example Usage

Provide a brief code snippet demonstrating how your operator can be used in a Holoscan
C++ or Python application.
Alternatively, link to a HoloHub application or tutorial showcasing how to use your operator.

There are 5 operators available in this package:

- `IIOAttributeRead`: Reads data from an IIO device.
- `IIOAttributeWrite`: Writes data to an IIO device.
- `IIOBufferRead`: Reads data from an IIO buffer.
- `IIOBufferWrite`: Writes data to an IIO buffer.
- `IIOConfigurator`: Automatically configures an IIO device based on a YAML
  configuration file.

### IIOAttributeRead Operator

#### Configuration Parameters

- **`ctx`**: (Mandatory) The URI of the IIO context to connect to the device.
- **`dev`**: (Optional) The name of the IIO device to read from. If not
    specified, it will read the context attributes.
- **`chan`**: (Optional) The name of the IIO channel to read from. If not
    specified, it will read the device attributes (the dev parameter must
    be specified then).
- **`channel_is_output`**: (Optional) If true, the channel is treated as an output
    channel. Defaults to false. If the **`chan`** parameter is set, this
    parameter must also be set.
- **`attr_name`**: (Mandatory) The name of the attribute to read from.

#### Ports

- To receive the data read from the `IIOAttributeRead` operator, use the
  output port named `value` of type `std::string`.

#### Operator Example

```cpp
auto iio_read_op = make_operator<ops::IIOAttributeRead>(
    "IIOAttributeRead",
    Arg("ctx") = std::string("ip:192.168.2.1"),
    Arg("dev") = std::string("ad9361-phy"),
    Arg("chan") = std::string("voltage0"),
    Arg("channel_is_output") = false,
    Arg("attr_name") = std::string("raw")
);

add_flow(iio_read_op, basic_printer_op, {{"value", "value"}});
```

### IIOAttributeWrite Operator

#### Configuration Parameters

- **`ctx`**: (Mandatory) The URI of the IIO context to connect to the device.
- **`dev`**: (Optional) The name of the IIO device to write to. If not
    specified, it will write to the context attributes.
- **`chan`**: (Optional) The name of the IIO channel to write to. If not
    specified, it will write to the device attributes (the dev parameter must
    be specified then).
- **`channel_is_output`**: (Optional) If true, the channel is treated as an output
    channel. Defaults to false. If the **`chan`** parameter is set, this
    parameter must also be set.
- **`attr_name`**: (Mandatory) The name of the attribute to write to.

#### Ports

- To send the data to be written to the `IIOAttributeWrite` operator, use the
  input port named `value` of type `std::string`.

#### Operator Example

```cpp
auto iio_write_op = make_operator<ops::IIOAttributeWrite>(
    "IIOAttributeWrite",
    Arg("ctx") = std::string("ip:192.168.2.1"),
    Arg("dev") = std::string("ad9361-phy"),
    Arg("chan") = std::string("voltage0"),
    Arg("channel_is_output") = false,
    Arg("attr_name") = std::string("raw")
);

add_flow(basic_emitter_op, iio_write_op, {{"value", "value"}});
```

### IIOBufferRead Operator

#### Configuration Parameters

- **`ctx`**: (Mandatory) The URI of the IIO context to connect to the device.
- **`dev`**: (Mandatory) The name of the IIO device to read from.
- **`is_cyclic`**: (Mandatory) If true, the buffer is cyclic. If false, it is
    non-cyclic.
- **`samples_count`**: (Mandatory) The number of samples to read from the buffer.
- **`enabled_channel_names`**: (Mandatory) A list of strings representing the
    names of the channels to read from. For the buffer to exist, at least one
    channel must be enabled.
- **`enabled_channel_output`**: (Mandatory) A list of booleans representing whether
    the corresponding channel is an output channel. The order must match the
    `enabled_channel_names` list.

#### Ports

- To receive the data read from the `IIOBufferRead` operator, use the output
  port named `buffer` of type `iio_buffer_info_t` as a shared pointer. This
  structure contains a the sample count and a void pointer to the data.
  This is the data read from the buffer, in order to interpret it, please
  refer to the available example application or the libiio documentation.

#### Operator Example

```cpp
std::vector<std::string> enabled_channel_names = {"voltage0", "voltage1"};
std::vector<bool> enabled_channel = {true, true};

auto iio_buffer_read_op = make_operator<ops::IIOBufferRead>(
    "IIOBufferRead",
    Arg("ctx") = std::string("ip:192.168.2.1"),
    Arg("dev") = std::string("ad9361-phy"),
    Arg("is_cyclic") = true,
    Arg("samples_count") = static_cast<size_t>(1024),
    Arg("enabled_channel_names") = enabled_channel_names,
    Arg("enabled_channel_output") = enabled_channel
);

add_flow(iio_buffer_read_op, basic_buffer_printer_op, {{"buffer", "buffer"}});
```

### IIOBufferWrite Operator

#### Configuration Parameters

- **`ctx`**: (Mandatory) The URI of the IIO context to connect to the device.
- **`dev`**: (Mandatory) The name of the IIO device to write to.
- **`is_cyclic`**: (Mandatory) If true, the buffer is cyclic. If false, it is
    non-cyclic.
- **`enabled_channel_names`**: (Mandatory) A list of strings representing the
    names of the channels to write to. For the buffer to exist, at least one
    channel must be enabled.
- **`enabled_channel_output`**: (Mandatory) A list of booleans representing whether
    the corresponding channel is an output channel. The order must match the
    `enabled_channel_names` list.

#### Ports

- To send the data to be written to the `IIOBufferWrite` operator, use the
  input port named `buffer` of type `iio_buffer_info_t` as a shared pointer.
  This structure contains a the sample count and a void pointer to the data.
  This is the data to be written to the buffer, in order to form it,
  please refer to the available example application or the libiio documentation.

#### Operator Example

```cpp
std::vector<std::string> enabled_channel_names = {"voltage0", "voltage1"};
std::vector<bool> enabled_channel = {false, false};

auto iio_buffer_write_op = make_operator<ops::IIOBufferWrite>(
    "IIOBufferWrite",
    Arg("ctx") = std::string("ip:192.168.2.1"),
    Arg("dev") = std::string("ad9361-phy"),
    Arg("is_cyclic") = true,
    Arg("enabled_channel_names") = enabled_channel_names,
    Arg("enabled_channel_output") = enabled_channel
);

add_flow(basic_buffer_emitter_op, iio_buffer_write_op, {{"buffer", "buffer"}});
```

### IIOConfigurator Operator

#### Configuration Parameters

- **`cfg`**: (Mandatory) The path to the YAML configuration file that
    contains the device configuration.

#### Ports

- This operator does not have any input or output ports. It reads the
  configuration from the specified YAML file and applies it to the IIO device.

#### Operator Example

```cpp
// Assuming the config file for the application is set
auto config_file_path = config().config_file();
auto iio_configurator_op = make_operator<ops::IIOConfigurator>(
    "IIOConfigurator",
    Arg("cfg") = std::string(config_file_path)
);

// Configure it only once, at the start of the application flow
add_flow(start_op(), iio_configurator_op);
```
