enable_testing()

# Stage 1: Ensure cpp-bundler runs successfully
add_test(
  NAME CppBundlerRuns
  COMMAND sh -c "\"${CMAKE_BINARY_DIR}/cpp-bundle\" \"${CMAKE_CURRENT_SOURCE_DIR}/input/test.cpp\" -- -I \"${CMAKE_CURRENT_SOURCE_DIR}/input\" > \"${CMAKE_BINARY_DIR}/actual_output.cpp\""
)

# Stage 2: Compare the output with the expected output
add_test(
  NAME CppBundlerOutputCheck
  COMMAND diff "${CMAKE_CURRENT_SOURCE_DIR}/expected_output.cpp" "${CMAKE_BINARY_DIR}/actual_output.cpp"
)

set_tests_properties(CppBundlerOutputCheck PROPERTIES DEPENDS CppBundlerRuns)
