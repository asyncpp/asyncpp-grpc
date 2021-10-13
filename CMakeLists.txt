find_package(Threads REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake PARENT_SCOPE)
set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} PARENT_SCOPE)

add_library(asyncpp_grpc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/grpc/util.cpp
)
target_link_libraries(asyncpp_grpc PUBLIC asyncpp gRPC::grpc++ protobuf::libprotobuf Threads::Threads)
target_include_directories(asyncpp
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(BUILD_TEST)
    enable_testing()
    include(GoogleTest)
    if(HUNTER_ENABLED)
        hunter_add_package(GTest)
    endif()
    find_package(GTest REQUIRED)

    include(GenerateGRPC)
    set(PROTOBUF_GENERATE_CPP_APPEND_PATH TRUE)
    GRPC_PB_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR}/protos/
        ${CMAKE_CURRENT_SOURCE_DIR}/test/dummy.proto
    )

    add_executable(asyncpp_grpc-test
        ${PROTO_SRCS}
        ${CMAKE_CURRENT_SOURCE_DIR}/test/client_unary.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/server_bidi_streaming.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/server_client_streaming.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/server_server_streaming.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/server_unary.cpp
    )
    target_link_libraries(asyncpp_grpc-test PRIVATE asyncpp_grpc GTest::GTest GTest::Main spdlog::spdlog)
    target_include_directories(asyncpp_grpc-test PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/protos/)
endif()