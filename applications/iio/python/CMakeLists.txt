add_custom_target(
  python_iio ALL
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/main.py"
          "${CMAKE_CURRENT_BINARY_DIR}/iio.py"
  DEPENDS "main.py"
  BYPRODUCTS "main.py")
