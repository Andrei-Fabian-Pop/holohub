#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../cpp/iio_attribute_write.hpp"

namespace py = pybind11;

namespace holoscan::ops {
// void init_iio_attribute_write(py::module_& m) {
//   py::class_<IIOAttributeWrite, holoscan::Operator, std::shared_ptr<IIOAttributeWrite>>(
//       m, "IIOAttributeWrite")
//       .def(py::init<>())
//       .def("setup", &IIOAttributeWrite::setup)
//       .def("initialize", &IIOAttributeWrite::initialize)
//       .def("compute", &IIOAttributeWrite::compute);
// }
}  // namespace holoscan::ops
