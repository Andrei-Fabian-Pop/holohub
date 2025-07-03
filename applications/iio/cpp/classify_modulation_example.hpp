#pragma once

#include <holoscan/core/conditions/gxf/boolean.hpp>
#include <holoscan/core/conditions/gxf/count.hpp>
#include <holoscan/core/endpoint.hpp>
#include <holoscan/core/forward_def.hpp>
#include <holoscan/core/operator.hpp>
#include "holoscan/holoscan.hpp"

class ClassifyModulationApp : public holoscan::Application {
 public:
  ClassifyModulationApp() { name_ = "IIOController Examples"; }
};
