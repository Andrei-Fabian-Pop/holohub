add_custom_target(
  python_iio ALL
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/iio_app.py"
          "${CMAKE_CURRENT_BINARY_DIR}/iio_app.py"
  DEPENDS "iio_app.py"
  BYPRODUCTS "iio_app.py")
