﻿# CMakeList.txt : CMake project for cmake, include source and define
# project specific logic here.
#

include_directories ("../..")

# Add source to this project's executable.
add_library (map SHARED
			 "map.cpp")

if (MSVC)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
  target_compile_options(map PRIVATE /bigobj)
  include_directories ("C:/msys64/usr/local/ruby-3.3.0-vc/include/ruby-3.2.0")
  include_directories ("C:/msys64/usr/local/ruby-3.3.0-vc/include/ruby-3.2.0/x64-mswin64_140")
  target_link_libraries(map "C:/msys64/usr/local/ruby-3.3.0-vc/lib/x64-vcruntime140-ruby330.lib")
endif ()
