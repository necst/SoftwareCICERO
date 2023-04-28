cmake_minimum_required(VERSION 3.13.4)
project(SoftwareCICERO LANGUAGES CXX C)

include_directories(
  include/
)

add_library(
  CiceroMulti
  SHARED
  lib/CiceroMulti.cpp
  lib/Core.cpp
  lib/CoreOUT.cpp
  lib/Instruction.cpp
  lib/Engine.cpp
  lib/Buffer.cpp
)

add_executable(
  cicero
  src/cicero.cpp
)

target_link_libraries(
  cicero
  CiceroMulti
)

add_executable(
  test_multi
  test/newTestMulti.cpp
)

target_link_libraries(
  test_multi
  CiceroMulti
)

include(CTest)

add_test(
  NAME test_multi
  COMMAND test_multi
)

target_compile_definitions(
  test_multi
  PRIVATE
  TEST_INPUT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test/"
)