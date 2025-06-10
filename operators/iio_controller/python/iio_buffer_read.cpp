#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../cpp/iio_buffer_read.hpp"

namespace py = pybind11;

namespace holoscan::ops {

void init_iio_buffer_read(py::module_& m) {
  py::class_<IIOBufferRead, holoscan::Operator, std::shared_ptr<IIOBufferRead>>(m, "IIOBufferRead")
      .def(py::init<>())
      .def("setup", &IIOBufferRead::setup)
      .def("initialize", &IIOBufferRead::initialize)
      .def("compute", &IIOBufferRead::compute);
}

}  // namespace holoscan::ops
