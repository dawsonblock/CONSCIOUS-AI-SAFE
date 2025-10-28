cmake_minimum_required(VERSION 3.20)
project(brain-ai VERSION 3.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Performance flags
if(CMAKE_BUILD_TYPE MATCHES "Release")
    add_compile_options(-O3 -march=native -flto -DNDEBUG)
    # Note: Cannot use -fno-exceptions/-fno-rtti due to FAISS/HNSWlib dependencies
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# Find dependencies
find_package(Eigen3 3.3 REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(YAMLCPP REQUIRED yaml-cpp)
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)
find_package(OpenMP)

# Find FAISS
find_library(FAISS_LIB faiss REQUIRED)

# Protobuf generation
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto_gen")
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

set(PROTO_SRCS "${PROTO_GEN_DIR}/brain.pb.cc")
set(PROTO_HDRS "${PROTO_GEN_DIR}/brain.pb.h")
set(GRPC_SRCS "${PROTO_GEN_DIR}/brain.grpc.pb.cc")
set(GRPC_HDRS "${PROTO_GEN_DIR}/brain.grpc.pb.h")

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
    COMMAND protobuf::protoc
    ARGS --cpp_out=${PROTO_GEN_DIR}
         --grpc_out=${PROTO_GEN_DIR}
         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
         -I${PROTO_DIR}
         ${PROTO_DIR}/brain.proto
    DEPENDS ${PROTO_DIR}/brain.proto
)

# Core library
add_library(brain_ai_core
    src/memory_factory.cpp
    src/memory/faiss.cpp
    src/memory/hnswlib.cpp
    src/graph/graph.cpp
    src/metrics.cpp
    src/filters.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_include_directories(brain_ai_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROTO_GEN_DIR}
    ${YAMLCPP_INCLUDE_DIRS}
)

target_link_libraries(brain_ai_core PUBLIC
    Eigen3::Eigen
    protobuf::libprotobuf
    gRPC::grpc++
    OpenSSL::SSL
    OpenSSL::Crypto
    ${FAISS_LIB}
    ${YAMLCPP_LIBRARIES}
    ${MICROHTTPD_LIBRARIES}
    sqlite3
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(brain_ai_core PUBLIC OpenMP::OpenMP_CXX)
endif()

# Server executable
add_executable(brain_server
    src/brain_server.cpp
)

target_link_libraries(brain_server PRIVATE brain_ai_core)

# Tools
add_executable(brain_search tools/search.cpp)
target_link_libraries(brain_search PRIVATE brain_ai_core)

add_executable(brain_reindex tools/reindex.cpp)
target_link_libraries(brain_reindex PRIVATE brain_ai_core)

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
        )
        FetchContent_MakeAvailable(googletest)
    endif()
    
    enable_testing()
    
    add_executable(test_qw tests/test_qw.cpp)
    target_link_libraries(test_qw PRIVATE brain_ai_core GTest::gtest_main)
    add_test(NAME test_qw COMMAND test_qw)
    
    add_executable(test_memory tests/test_memory.cpp)
    target_link_libraries(test_memory PRIVATE brain_ai_core GTest::gtest_main)
    add_test(NAME test_memory COMMAND test_memory)
    
    add_executable(test_graph tests/test_graph.cpp)
    target_link_libraries(test_graph PRIVATE brain_ai_core GTest::gtest_main)
    add_test(NAME test_graph COMMAND test_graph)
endif()

# Installation
install(TARGETS brain_server brain_search brain_reindex
    RUNTIME DESTINATION bin)
install(DIRECTORY configs/ DESTINATION etc/brain-ai)
install(FILES README.md DESTINATION share/doc/brain-ai)

message(STATUS "==========================================")
message(STATUS " Brain-AI v${PROJECT_VERSION} - Build Config")
message(STATUS "==========================================")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "OpenMP: ${OpenMP_CXX_FOUND}")
message(STATUS "==========================================")
