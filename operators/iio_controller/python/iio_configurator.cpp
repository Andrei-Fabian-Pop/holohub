#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../cpp/iio_configurator.hpp"

namespace py = pybind11;

namespace holoscan::ops {

void init_iio_configurator(py::module_& m) {
  py::class_<IIOConfigurator, holoscan::Operator, std::shared_ptr<IIOConfigurator>>(
      m, "IIOConfigurator")
      .def(py::init<>())
      .def("setup", &IIOConfigurator::setup)
      .def("compute", &IIOConfigurator::compute);
}

}  // namespace holoscan::ops
