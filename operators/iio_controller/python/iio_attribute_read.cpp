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
#include "iio_attribute_read_pydoc.hpp"

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

PYBIND11_MODULE(_iio_attribute_read, m) {
  m.doc() = R"pbdoc(TODO)pbdoc";

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
}  // PYBIND11_MODULE
}  // namespace holoscan::ops
