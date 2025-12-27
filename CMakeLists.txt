cmake_minimum_required(VERSION 3.14)
project("CppBundler" VERSION 1.1 LANGUAGES C CXX)

add_subdirectory(test)

# Static Build Options
option(BUILD_STATIC "Build with static LLVM/Clang libraries" OFF)
option(USE_STATIC_CRT "Link against static C runtime (Windows/Linux)" OFF)

# Platform-specific static flags
if(USE_STATIC_CRT)
  if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  else()
    # For GCC/Clang on Linux (non-musl), link libgcc/libstdc++ statically
    # For Musl, the compiler wrapper usually handles this, but explicit flags don't hurt if check fails
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
  endif()
endif()

# Find LLVM/Clang
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Match LLVM RTTI/EH settings.
if(NOT LLVM_ENABLE_RTTI)
  message(STATUS "LLVM built without RTTI, disabling RTTI for CppBundler")
  if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
  endif()
endif()

if(NOT LLVM_ENABLE_EH)
  if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
  endif()
endif()

# Build the tool
add_executable(cpp-bundle CppBundler.cpp)

if(BUILD_STATIC)
  # When building static, we need to link against individual static libraries
  # because libclang-cpp is usually shared.
  # We map the components we use. Based on code:
  # Tooling, Frontend, Rewrite, Serialization, Support, etc.
  
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
  
  if(UNIX AND NOT APPLE)
      # On Linux static builds often need pthreads, zlib, and maybe libxml2/zstd depending on LLVM build
      target_link_libraries(cpp-bundle PRIVATE pthread dl z)
      if (USE_STATIC_CRT)
        # Often static LLVM needs these if not provided by compiler implicitly
        # target_link_libraries(cpp-bundle PRIVATE atomic) 
      endif()
  endif()
else()
  # Default dynamic link
  target_link_libraries(cpp-bundle PRIVATE LLVM clang-cpp)
endif()
