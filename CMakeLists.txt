cmake_minimum_required(VERSION 3.15)
project(py_zed_open_capture LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Python 3.12 COMPONENTS Interpreter Development.Module REQUIRED)

if(NOT SKBUILD)
  execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
endif()
find_package(nanobind CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

set(ZOC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/zed-open-capture)

nanobind_add_module(
  _py_zed_open_capture_ext
  STABLE_ABI
  NB_STATIC
  csrc/bindings.cpp
  ${ZOC_DIR}/src/videocapture.cpp
)

target_include_directories(_py_zed_open_capture_ext PRIVATE ${ZOC_DIR}/include)
target_compile_definitions(_py_zed_open_capture_ext PRIVATE VIDEO_MOD_AVAILABLE)
target_link_libraries(_py_zed_open_capture_ext PRIVATE PkgConfig::LIBUSB)

install(TARGETS _py_zed_open_capture_ext LIBRARY DESTINATION py_zed_open_capture)
