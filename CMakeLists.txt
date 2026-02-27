cmake_minimum_required(VERSION 3.25...4.2)

project(libcaparoc
    VERSION 0.1.1
    DESCRIPTION "CAPAROC library built on top of modbus_cpp"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(FetchContent)

option(LIBCAPAROC_ENABLE_CPACK "Enable CPack packaging support" ${PROJECT_IS_TOP_LEVEL})

if(NOT TARGET modbus_cpp)
    FetchContent_Declare(
        libmodbus_cpp_proj
        GIT_REPOSITORY https://github.com/daixtrose/libmodbus-cpp.git
        GIT_TAG v1.1.1
        GIT_SHALLOW TRUE
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

if(LIBCAPAROC_ENABLE_CPACK)
    install(TARGETS caparoc
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    set(CPACK_PACKAGE_NAME libcaparoc)
    set(CPACK_PACKAGE_VENDOR "Markus Werle")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CAPAROC library built on top of modbus_cpp")
    set(CPACK_PACKAGE_CONTACT "daixtrose")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Markus Werle")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_VERBATIM_VARIABLES YES)

    # Standard naming with architecture: <pkg>_<ver>_<arch>.deb / <pkg>-<ver>-<rel>.<arch>.rpm
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
    set(CPACK_RPM_PACKAGE_RELEASE "1")

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(CPACK_GENERATOR TGZ DEB RPM)
    else()
        set(CPACK_GENERATOR TGZ)
    endif()

    include(CPack)
endif()
