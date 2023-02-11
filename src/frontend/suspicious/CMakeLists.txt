#cmake_minimum_required(VERSION 3.16.0)
#project(suspicious)

#find_package(LLVM REQUIRED CONFIG)

#message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
#message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

#include_directories(${LLVM_INCLUDE_DIRS})
#separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
#add_definitions(${LLVM_DEFINITIONS_LIST})

#llvm_map_components_to_libnames(llvm_libs core mcjit native)


add_executable(instrument-suspicious
  InstrumentSuspicious.cpp
  SMTLIB2.cpp
  ../util.cpp
  )


target_link_libraries(instrument-suspicious
  PRIVATE
  #${llvm_libs}
  clangTooling
  clangBasic
  clangASTMatchers
  )

