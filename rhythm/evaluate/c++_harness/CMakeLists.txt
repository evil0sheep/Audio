cmake_minimum_required(VERSION 3.0)
project(LibRhythmEvaluate CXX)


SET(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/../../")
INCLUDE_DIRECTORIES( ${INCLUDE_DIR} )

add_library(analyze_rhythm STATIC ffi_interface.cpp)

# Here is not-so-standard element.
# The target installation command has to exist
# and has to install all linkable targets
# into `.` (current) directory for cargo to find it.
install(TARGETS analyze_rhythm DESTINATION .)