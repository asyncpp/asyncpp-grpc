cmake_minimum_required(VERSION 3.13)
project(AsyncppGRPC)

find_package(Threads REQUIRED)

include(cmake/GetGRPC.cmake)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_directory_property(HAS_PARENT PARENT_DIRECTORY)

if(HAS_PARENT)
  set(CMAKE_MODULE_PATH
      ${CMAKE_MODULE_PATH}
      PARENT_SCOPE)
endif()

option(ASYNCPP_BUILD_TEST "Enable test builds" ON)
option(ASYNCPP_WITH_ASAN "Enable asan for test builds" ON)

if(TARGET asyncpp)
  message(STATUS "Using existing asyncpp target.")
else()
  message(STATUS "Missing asyncpp, using Fetch to import it.")
  include(cmake/Fetch_asyncpp.cmake)
endif()

add_library(asyncpp_grpc ${CMAKE_CURRENT_SOURCE_DIR}/src/grpc/client_cq.cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/grpc/server.cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/grpc/util.cpp)
target_link_libraries(
  asyncpp_grpc PUBLIC asyncpp gRPC::grpc++ protobuf::libprotobuf
                      Threads::Threads)
target_include_directories(asyncpp_grpc
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(asyncpp_grpc PUBLIC cxx_std_20)

if(ASYNCPP_BUILD_TEST)
  enable_testing()
  include(GoogleTest)

  include(cmake/GetGTest.cmake)

  include(cmake/GenerateGRPC.cmake)
  set(PROTOBUF_GENERATE_CPP_APPEND_PATH TRUE)
  grpc_pb_generate_cpp(
    PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR}/protos/
    ${CMAKE_CURRENT_SOURCE_DIR}/test/dummy.proto)

  add_executable(
    asyncpp_grpc-test
    ${PROTO_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/test/traits.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/call.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/client_cq.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/task.cpp)
  target_link_libraries(asyncpp_grpc-test PRIVATE asyncpp_grpc GTest::gtest
                                                  GTest::gtest_main)
  target_include_directories(asyncpp_grpc-test
                             PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/protos/)

  if(ASYNCPP_WITH_ASAN)
    message(STATUS "Building with asan enabled")
    target_compile_options(asyncpp_grpc-test PRIVATE -fsanitize=address)
    target_link_libraries(asyncpp_grpc-test PRIVATE asan)
  endif()

  gtest_discover_tests(asyncpp_grpc-test)
endif()
