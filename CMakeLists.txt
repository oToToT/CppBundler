cmake_minimum_required(VERSION 3.14)
project("CppBundler" VERSION 1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(test)

# Static Build Options
option(BUILD_STATIC "Build with static LLVM/Clang libraries" OFF)
option(USE_STATIC_CRT "Link against static C runtime (Windows/Linux)" OFF)

# Platform-specific static flags
if(USE_STATIC_CRT)
  # For GCC/Clang on Linux (non-musl) or MinGW, try to link static runtimes
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

# Find LLVM/Clang
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Match LLVM RTTI/EH settings. (Assuming GCC/Clang style flags)
if(NOT LLVM_ENABLE_RTTI)
  message(STATUS "LLVM built without RTTI, disabling RTTI for CppBundler")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

if(NOT LLVM_ENABLE_EH)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()

# Build the tool
add_executable(cpp-bundle CppBundler.cpp)

if(BUILD_STATIC)
  # When building static, we need to link against individual static libraries
  # because libclang-cpp is usually shared.
  
  # Basic set of Clang libs often needed for Tooling
  set(CLANG_LIBS
    clangTooling
    clangFrontend
    clangSerialization
    clangDriver
    clangParse
    clangSema
    clangAnalysis
    clangRewrite
    clangEdit
    clangAST
    clangLex
    clangBasic
  )
  
  # Map LLVM components to static libraries
  llvm_map_components_to_libnames(llvm_libs support core option native)

  target_link_libraries(cpp-bundle PRIVATE ${CLANG_LIBS} ${llvm_libs})

else()
  # Default dynamic link
  target_link_libraries(cpp-bundle PRIVATE LLVM clang-cpp)
endif()
