project(unique)
cmake_minimum_required (VERSION 3.10)

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release")
endif()

if(NOT BUILD_STATIC)
    set(BUILD_SHARED_LIBS ON)
    message(STATUS "Building shared libraries.")
else()
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    message(STATUS "Building statically.")
endif()

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR ) 
  message (FATAL_ERROR
    "In-source builds are not allowed. Please clean your source tree and try again.")  
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
add_compile_options (-fpic)

include_directories(docopt.cpp)
add_subdirectory(docopt.cpp)

add_subdirectory (abc)
add_subdirectory (minisat)
add_subdirectory (avy)
add_subdirectory (interpolatingsolver)
add_subdirectory (unique)
