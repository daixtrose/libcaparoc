cmake_minimum_required(VERSION 3.25)

project(libcaparoc
    VERSION 0.1.0
    DESCRIPTION "CAPAROC library built on top of modbus_cpp"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

if(NOT TARGET modbus_cpp)
    FetchContent_Declare(
        libmodbus_cpp_proj
        GIT_REPOSITORY https://github.com/daixtrose/libmodbus-cpp.git
        GIT_TAG v0.1.0
    )
    FetchContent_MakeAvailable(libmodbus_cpp_proj)
endif()

add_library(caparoc
    ${CMAKE_CURRENT_LIST_DIR}/src/caparoc.cpp
)

add_library(libcaparoc::caparoc ALIAS caparoc)

target_include_directories(caparoc
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(caparoc
    PUBLIC
        modbus_cpp
)

target_compile_features(caparoc PUBLIC cxx_std_23)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(caparoc PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
elseif(MSVC)
    target_compile_options(caparoc PRIVATE
        /W4
    )
endif()
