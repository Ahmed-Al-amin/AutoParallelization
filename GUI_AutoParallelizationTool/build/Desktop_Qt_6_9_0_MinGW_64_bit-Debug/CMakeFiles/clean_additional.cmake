# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "AutoParallelization_autogen"
  "CMakeFiles\\AutoParallelization_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\AutoParallelization_autogen.dir\\ParseCache.txt"
  )
endif()
