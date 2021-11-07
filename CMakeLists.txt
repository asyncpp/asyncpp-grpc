cmake_minimum_required(VERSION 3.13)
if(HUNTER_ENABLED)
    include("cmake/HunterGate.cmake")
    HunterGate(
        URL "https://github.com/cpp-pm/hunter/archive/v0.23.314.tar.gz"
        SHA1 "95c47c92f68edb091b5d6d18924baabe02a6962a"
        LOCAL
    )
endif()
project(AsyncppGRPC)

find_package(Threads REQUIRED)

if(HUNTER_ENABLED)
    hunter_add_package(gRPC)
endif()
find_package(gRPC CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
    set(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} PARENT_SCOPE)
endif()

option(BUILD_TEST "Enable test builds" ON)
option(WITH_ASAN "Enable asan for test builds" ON)

if (TARGET asyncpp)
    message(STATUS "Using existing asyncpp target.")
else()
    message(STATUS "Missing asyncpp, using Fetch to import it.")
    include(cmake/Fetch_asyncpp.cmake)
endif()

add_library(asyncpp_grpc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/grpc/util.cpp
)
target_link_libraries(asyncpp_grpc PUBLIC asyncpp gRPC::grpc++ protobuf::libprotobuf Threads::Threads)
target_include_directories(asyncpp_grpc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(asyncpp_grpc PUBLIC cxx_std_20)

if(BUILD_TEST)
    enable_testing()
    include(GoogleTest)
    if(HUNTER_ENABLED)
        hunter_add_package(GTest)
    endif()
    find_package(GTest REQUIRED)

    include(cmake/GenerateGRPC.cmake)
    set(PROTOBUF_GENERATE_CPP_APPEND_PATH TRUE)
    GRPC_PB_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR}/protos/
        ${CMAKE_CURRENT_SOURCE_DIR}/test/dummy.proto
    )

    add_executable(asyncpp_grpc-test
        ${PROTO_SRCS}
        ${CMAKE_CURRENT_SOURCE_DIR}/test/unary_call.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/bidi_streaming_task.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/client_streaming_task.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/server_streaming_task.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/unary_task.cpp
    )
    target_link_libraries(asyncpp_grpc-test PRIVATE asyncpp_grpc GTest::GTest GTest::Main)
    target_include_directories(asyncpp_grpc-test PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/protos/)

    if(WITH_ASAN)
        message(STATUS "Building with asan enabled")
        target_compile_options(asyncpp_grpc-test PRIVATE -fsanitize=address)
        target_link_libraries(asyncpp_grpc-test PRIVATE asan)
    endif()

    gtest_discover_tests(asyncpp_grpc-test)
endif()