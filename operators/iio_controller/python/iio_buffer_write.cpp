#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../cpp/iio_buffer_write.hpp"

namespace py = pybind11;

namespace holoscan::ops {

void init_iio_buffer_write(py::module_& m) {
  py::class_<IIOBufferWrite, holoscan::Operator, std::shared_ptr<IIOBufferWrite>>(m,
                                                                                  "IIOBufferWrite")
      .def(py::init<>())
      .def("setup", &IIOBufferWrite::setup)
      .def("initialize", &IIOBufferWrite::initialize)
      .def("compute", &IIOBufferWrite::compute);
}

}  // namespace holoscan::ops
