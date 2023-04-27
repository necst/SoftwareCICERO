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