set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")



add_executable(harness
  Harness.cpp
  ../suspicious/SMTLIB2.cpp
  ../util.cpp
  )


target_link_libraries(harness
  PRIVATE
  #${llvm_libs}
  clangTooling
  clangBasic
  clangASTMatchers
  )

