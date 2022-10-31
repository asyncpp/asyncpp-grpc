if(HUNTER_ENABLED)
  hunter_add_package(gRPC)
  find_package(gRPC CONFIG REQUIRED)
  hunter_add_package(Protobuf)
  find_package(Protobuf CONFIG REQUIRED)
  hunter_add_package(OpenSSL)
  find_package(OpenSSL REQUIRED)
else()
  find_package(gRPC CONFIG)
  find_package(Protobuf CONFIG)
  if(NOT gRPC_FOUND OR NOT Protobuf_FOUND)
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
      pkg_search_module(gRPC IMPORTED_TARGET grpc)
      pkg_search_module(gRPCPP IMPORTED_TARGET grpc++>=1.16.0)
      pkg_search_module(GPR IMPORTED_TARGET gpr)
      find_program(gRPCPP_PB_PLUGIN grpc_cpp_plugin)
      pkg_search_module(Protobuf IMPORTED_TARGET protobuf)
      find_program(Protobuf_PROTOC protoc)
      if(gRPC_FOUND
         AND gRPCPP_FOUND
         AND Protobuf_FOUND
         AND GPR_FOUND
         AND NOT gRPCPP_PB_PLUGIN STREQUAL "gRPCPP_PB_PLUGIN-NOTFOUND"
         AND NOT Protobuf_PROTOC STREQUAL "Protobuf_PROTOC-NOTFOUND")
        message(STATUS "Using system grpc/protobuf")
        set_target_properties(PkgConfig::gRPC PROPERTIES IMPORTED_GLOBAL TRUE)
        set_target_properties(PkgConfig::gRPCPP PROPERTIES IMPORTED_GLOBAL TRUE)
        set_target_properties(PkgConfig::GPR PROPERTIES IMPORTED_GLOBAL TRUE)
        set_target_properties(PkgConfig::Protobuf PROPERTIES IMPORTED_GLOBAL
                                                             TRUE)
        add_library(gRPC::grpc ALIAS PkgConfig::gRPC)
        target_link_libraries(PkgConfig::gRPC INTERFACE PkgConfig::GPR)
        # Building with TSAN/ASAN causes runtime errors if grpc is built
        # without. See https://github.com/grpc/grpc/issues/19224 for details
        target_compile_definitions(PkgConfig::gRPC
                                   INTERFACE GRPC_ASAN_SUPPRESSED)
        add_library(gRPC::grpc++ ALIAS PkgConfig::gRPCPP)
        target_link_libraries(PkgConfig::gRPCPP INTERFACE PkgConfig::GPR)
        target_compile_definitions(PkgConfig::gRPCPP
                                   INTERFACE GRPC_ASAN_SUPPRESSED)
        add_executable(gRPC::grpc_cpp_plugin IMPORTED)
        set_property(TARGET gRPC::grpc_cpp_plugin PROPERTY IMPORTED_LOCATION
                                                           ${gRPCPP_PB_PLUGIN})
        add_library(protobuf::libprotobuf ALIAS PkgConfig::Protobuf)
        add_executable(protobuf::protoc IMPORTED)
        set_property(TARGET protobuf::protoc PROPERTY IMPORTED_LOCATION
                                                      ${Protobuf_PROTOC})
      else()
        unset(gRPC_FOUND)
      endif()
    endif()
  endif()
  if(NOT gRPC_FOUND OR NOT Protobuf_FOUND)
    message(STATUS "Using FetchContent for GRPC")
    set(gRPC_BUILD_TESTS
        OFF
        CACHE INTERNAL "" FORCE)
    set(gRPC_BUILD_CSHARP_EXT
        OFF
        CACHE INTERNAL "" FORCE)
    set(gRPC_BACKWARDS_COMPATIBILITY_MODE
        OFF
        CACHE INTERNAL "" FORCE)
    set(gRPC_BUILD_GRPC_CSHARP_PLUGIN
        OFF
        CACHE INTERNAL "" FORCE)
    set(gRPC_BUILD_GRPC_NODE_PLUGIN
        OFF
        CACHE INTERNAL "" FORCE)
    set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN
        OFF
        CACHE INTERNAL "" FORCE)
    set(gRPC_BUILD_GRPC_PHP_PLUGIN
        OFF
        CACHE INTERNAL "" FORCE)
    set(gRPC_BUILD_GRPC_PYTHON_PLUGIN
        OFF
        CACHE INTERNAL "" FORCE)
    set(gRPC_BUILD_GRPC_RUBY_PLUGIN
        OFF
        CACHE INTERNAL "" FORCE)
    set(CMAKE_POSITION_INDEPENDENT_CODE
        ON
        CACHE INTERNAL "" FORCE)
    include(FetchContent)
    FetchContent_Declare(
      gRPC
      GIT_REPOSITORY https://github.com/grpc/grpc
      GIT_TAG v1.50.1
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
      USES_TERMINAL_DOWNLOAD TRUE)
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_MakeAvailable(gRPC)
    add_library(gRPC::grpc++ ALIAS grpc++)
    add_library(gRPC::grpc ALIAS grpc)
    add_executable(gRPC::grpc_cpp_plugin ALIAS grpc_cpp_plugin)
  endif()
endif()
