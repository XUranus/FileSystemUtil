cmake_minimum_required (VERSION 3.8)

project ("fsutil")

add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
add_executable (fsutil "FileSystemUtil.cpp" "FileSystemUtil.h" "Demo.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET fsutil PROPERTY CXX_STANDARD 17)
endif()

target_link_libraries(fsutil mpr)