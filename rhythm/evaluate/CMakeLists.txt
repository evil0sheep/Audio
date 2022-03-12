cmake_minimum_required(VERSION 3.0)
project(LibRhythmEvaluate CXX)


set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/../../")
if(DEFINED ENV{INCLUDE_PATH})
  string(REPLACE ":" ";" INCLUDE_LIST $ENV{INCLUDE_PATH})
endif()
include_directories( ${INCLUDE_LIST} "${PROJECT_SOURCE_DIR}/" "${PROJECT_SOURCE_DIR}/../../" "${PROJECT_SOURCE_DIR}/../../../" )


add_library(analyze_rhythm STATIC ffi_interface.cpp)
target_compile_options(analyze_rhythm PRIVATE -Wno-c++11-extensions)
add_definitions(-DTRAIN_PARAMETERS)
target_link_libraries(analyze_rhythm kissfft-float)


# Here is not-so-standard element.
# The target installation command has to exist
# and has to install all linkable targets
# into `.` (current) directory for cargo to find it.
install(TARGETS analyze_rhythm DESTINATION .)