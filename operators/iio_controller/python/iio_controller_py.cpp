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

#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <holoscan/core/fragment.hpp>
#include <holoscan/core/operator.hpp>
#include <holoscan/core/operator_spec.hpp>
#include "../../operator_util.hpp"

#include "../cpp/iio_attribute_read.hpp"
#include "../cpp/iio_attribute_write.hpp"
#include "../cpp/iio_buffer_read.hpp"
#include "../cpp/iio_buffer_write.hpp"

#include "iio_configurator.hpp"
#include "iio_controller_pydoc.hpp"

using std::string_literals::operator""s;
using pybind11::literals::operator""_a;

namespace py = pybind11;

namespace holoscan::ops {

class PyIIOAttributeRead : public IIOAttributeRead {
 public:
  using IIOAttributeRead::IIOAttributeRead;

  PyIIOAttributeRead(Fragment* fragment, const py::args& args, std::string ctx,
                     std::string attr_name, std::string dev = "", std::string chan = "",
                     bool channel_is_output = false, const std::string& name = "iio_attribute_read")
      : IIOAttributeRead(ArgList{Arg("ctx", ctx),
                                 Arg("attr_name", attr_name),
                                 Arg("dev", dev),
                                 Arg("chan", chan),
                                 Arg("channel_is_output", channel_is_output)}) {
    add_positional_condition_and_resource_args(this, args);
    name_ = name;
    fragment_ = fragment;
    spec_ = std::make_shared<OperatorSpec>(fragment);
    setup(*spec_.get());
  }
};

class PyIIOAttributeWrite : public IIOAttributeWrite {
 public:
  using IIOAttributeWrite::IIOAttributeWrite;

  PyIIOAttributeWrite(Fragment* fragment, const py::args& args, std::string ctx,
                      std::string attr_name, std::string dev = "", std::string chan = "",
                      bool channel_is_output = false,
                      const std::string& name = "iio_attribute_read")
      : IIOAttributeWrite(ArgList{Arg("ctx", ctx),
                                  Arg("attr_name", attr_name),
                                  Arg("dev", dev),
                                  Arg("chan", chan),
                                  Arg("channel_is_output", channel_is_output)}) {
    add_positional_condition_and_resource_args(this, args);
    name_ = name;
    fragment_ = fragment;
    spec_ = std::make_shared<OperatorSpec>(fragment);
    setup(*spec_.get());
  }
};

class PyIIOBufferRead : public IIOBufferRead {
 public:
  using IIOBufferRead::IIOBufferRead;

  PyIIOBufferRead(Fragment* fragment, const py::args& args, std::string ctx, std::string dev,
                  bool is_cyclic, size_t samples_count,
                  std::vector<std::string> enabled_channel_names,
                  std::vector<bool> enabled_channel_output,
                  const std::string& name = "iio_buffer_read")
      : IIOBufferRead(ArgList{Arg("ctx", ctx),
                              Arg("dev", dev),
                              Arg("is_cyclic", is_cyclic),
                              Arg("samples_count", samples_count),
                              Arg("enabled_channel_names", enabled_channel_names),
                              Arg("enabled_channel_output", enabled_channel_output)}) {
    add_positional_condition_and_resource_args(this, args);
    name_ = name;
    fragment_ = fragment;
    spec_ = std::make_shared<OperatorSpec>(fragment);
    setup(*spec_.get());
  }
};

class PyIIOBufferWrite : public IIOBufferWrite {
 public:
  using IIOBufferWrite::IIOBufferWrite;

  PyIIOBufferWrite(Fragment* fragment, const py::args& args, std::string ctx, std::string dev,
                   bool is_cyclic, std::vector<std::string> enabled_channel_names,
                   std::vector<bool> enabled_channel_output,
                   const std::string& name = "iio_buffer_write")
      : IIOBufferWrite(ArgList{Arg("ctx", ctx),
                               Arg("dev", dev),
                               Arg("is_cyclic", is_cyclic),
                               Arg("enabled_channel_names", enabled_channel_names),
                               Arg("enabled_channel_output", enabled_channel_output)}) {
    add_positional_condition_and_resource_args(this, args);
    name_ = name;
    fragment_ = fragment;
    spec_ = std::make_shared<OperatorSpec>(fragment);
    setup(*spec_.get());
  }
};

class PyIIOConfigurator : public IIOConfigurator {
 public:
  using IIOConfigurator::IIOConfigurator;

  PyIIOConfigurator(Fragment* fragment, const py::args& args, std::string cfg,
                    const std::string& name = "iio_configurator")
      : IIOConfigurator(ArgList{Arg("cfg", cfg)}) {
    add_positional_condition_and_resource_args(this, args);
    name_ = name;
    fragment_ = fragment;
    spec_ = std::make_shared<OperatorSpec>(fragment);
    setup(*spec_.get());
  }
};

PYBIND11_MODULE(_iio_controller, m) {
  m.doc() = R"pbdoc(
    IIO Controller Python Bindings
    -----------------------------------
    .. currentmodule:: _iio_controller
  )pbdoc";

#ifdef VERSION_INFO
  m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
  m.attr("__version__") = "dev";
#endif

  py::class_<IIOAttributeRead,
             PyIIOAttributeRead,
             holoscan::Operator,
             std::shared_ptr<IIOAttributeRead>>(
      m, "IIOAttributeRead", doc::IIOAttributeRead::doc_IIOAttributeRead_python)
      .def(py::init<Fragment*,
                    const py::args&,
                    std::string,
                    std::string,
                    std::string,
                    std::string,
                    bool,
                    const std::string&>(),
           "fragment"_a,
           "ctx"_a,
           "attr_name"_a,
           "dev"_a = ""s,
           "chan"_a = ""s,
           "channel_is_output"_a = false,
           "name"_a = "iio_attribute_read"s,
           doc::IIOAttributeRead::doc_IIOAttributeRead_python)
      .def("initialize", &IIOAttributeRead::initialize, doc::IIOAttributeRead::doc_initialize);

  py::class_<IIOAttributeWrite,
             PyIIOAttributeWrite,
             holoscan::Operator,
             std::shared_ptr<IIOAttributeWrite>>(
      m, "IIOAttributeWrite", doc::IIOAttributeWrite::doc_IIOAttributeWrite_python)
      .def(py::init<Fragment*,
                    const py::args&,
                    std::string,
                    std::string,
                    std::string,
                    std::string,
                    bool,
                    const std::string&>(),
           "fragment"_a,
           "ctx"_a,
           "attr_name"_a,
           "dev"_a = ""s,
           "chan"_a = ""s,
           "channel_is_output"_a = false,
           "name"_a = "iio_attribute_write"s,
           doc::IIOAttributeWrite::doc_IIOAttributeWrite_python)
      .def("initialize", &IIOAttributeWrite::initialize, doc::IIOAttributeWrite::doc_initialize);

  py::class_<IIOBufferWrite, PyIIOBufferWrite, holoscan::Operator, std::shared_ptr<IIOBufferWrite>>(
      m, "IIOBufferWrite", doc::IIOBufferWrite::doc_IIOBufferWrite_python)
      .def(py::init<Fragment*,
                    const py::args&,
                    std::string,
                    std::string,
                    bool,
                    std::vector<std::string>,
                    std::vector<bool>,
                    const std::string&>(),
           "fragment"_a,
           "ctx"_a,
           "dev"_a,
           "is_cyclic"_a,
           "enabled_channel_names"_a,
           "enabled_channel_output"_a,
           "name"_a = "iio_buffer_write"s,
           doc::IIOBufferWrite::doc_IIOBufferWrite_python)
      .def("initialize", &IIOBufferWrite::initialize, doc::IIOBufferWrite::doc_initialize);

  py::class_<IIOBufferRead, PyIIOBufferRead, holoscan::Operator, std::shared_ptr<IIOBufferRead>>(
      m, "IIOBufferRead", doc::IIOBufferRead::doc_IIOBufferRead_python)
      .def(py::init<Fragment*,
                    const py::args&,
                    std::string,
                    std::string,
                    bool,
                    size_t,
                    std::vector<std::string>,
                    std::vector<bool>,
                    const std::string&>(),
           "fragment"_a,
           "ctx"_a,
           "dev"_a,
           "is_cyclic"_a,
           "samples_count"_a,
           "enabled_channel_names"_a,
           "enabled_channel_input"_a,
           "name"_a = "iio_buffer_read"s,
           doc::IIOBufferRead::doc_IIOBufferRead_python)
      .def("initialize", &IIOBufferRead::initialize, doc::IIOBufferRead::doc_initialize);

  py::class_<IIOConfigurator,
             PyIIOConfigurator,
             holoscan::Operator,
             std::shared_ptr<IIOConfigurator>>(
      m, "IIOConfigurator", doc::IIOConfigurator::doc_IIOConfigurator_python)
      .def(py::init<Fragment*, const py::args&, std::string, const std::string&>(),
           "fragment"_a,
           "cfg"_a,
           "name"_a = "iio_configurator"s,
           doc::IIOConfigurator::doc_IIOConfigurator_python);
}  // PYBIND11_MODULE
}  // namespace holoscan::ops
