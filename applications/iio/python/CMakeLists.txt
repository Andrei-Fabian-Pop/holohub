add_custom_target(
  python_iio ALL
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/iio_app.py"
          "${CMAKE_CURRENT_BINARY_DIR}/iio_app.py"
  DEPENDS "iio_app.py"
  BYPRODUCTS "iio_app.py")

add_custom_target(
  py_iio_config_yaml
  COMMAND
    ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/../iio_config.yaml"
    ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../iio_config.yaml"
  BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/iio_config.yaml")

add_dependencies(python_iio py_iio_config_yaml)
