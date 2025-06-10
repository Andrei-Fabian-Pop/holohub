#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../cpp/iio_params.hpp"

namespace py = pybind11;

namespace holoscan::ops {

void init_iio_params(py::module_& m) {
  py::enum_<attr_type_t>(m, "AttrType")
      .value("CONTEXT", attr_type_t::CONTEXT)
      .value("DEVICE", attr_type_t::DEVICE)
      .value("CHANNEL", attr_type_t::CHANNEL)
      .value("UNKNOWN", attr_type_t::UNKNOWN)
      .export_values();

  py::class_<iio_buffer_info_t>(m, "IIOBufferInfo")
      .def(py::init<>())
      .def_readwrite("samples_count", &iio_buffer_info_t::samples_count)
      .def_readwrite("buffer", &iio_buffer_info_t::buffer);  // raw pointer, exposed as-is
}

}  // namespace holoscan::ops
