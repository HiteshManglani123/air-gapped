cmake_minimum_required(VERSION 3.10)

# Set the project name
project(airgap_system_security)

# Add the main source file and transmitter source file
add_executable(transmitter.x86_64
  src/main.c
  src/transmitter.c
)

# Include directories
target_include_directories(transmitter.x86_64 PRIVATE include)

# Enable testing
enable_testing()

# Add GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add test source file
add_executable(
  unit_tests
  tests/test_main.cpp
  src/transmitter.c  # Include transmitter.c for testing
)

# Link GoogleTest to the test executable
target_link_libraries(
  unit_tests
  gtest_main
)

# Include directories for tests
target_include_directories(unit_tests PRIVATE include)

# Add a test
add_test(
  NAME Unit_Tests
  COMMAND unit_test
)