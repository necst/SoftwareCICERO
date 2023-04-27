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
  lib/Manager.cpp
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

add_test(
  NAME test_multi
  COMMAND test_multi
)