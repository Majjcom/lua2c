cmake_minimum_required(VERSION 3.20)
project(lua2c VERSION 0.1.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CTest)
option(LUA2C_SHARED_RUNTIME "Build the Lua runtime as a shared library" ON)
option(LUA2C_BUILD_CLI "Build the conversion command line tool" ON)
option(LUA2C_BUILD_TESTS "Build conversion and execution tests" ${BUILD_TESTING})
option(LUA2C_RUNTIME_ASSERTIONS "Enable Lua internal assertions and API checks" OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

include(cmake/VMTemplates.cmake)
add_subdirectory(lua)

add_library(lua2c src/converter.cpp src/bytecode.cpp src/emitter.cpp)
add_library(lua2c::lua2c ALIAS lua2c)
target_include_directories(lua2c PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(lua2c PRIVATE lualib)
set_target_properties(lua2c PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
if(MSVC)
    target_compile_options(lua2c PRIVATE /W4 /utf-8)
else()
    target_compile_options(lua2c PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(LUA2C_BUILD_CLI)
    add_executable(lua2c_cli src/main.cpp)
    set_target_properties(lua2c_cli PROPERTIES OUTPUT_NAME lua2c)
    target_link_libraries(lua2c_cli PRIVATE lua2c)
    install(TARGETS lua2c_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(TARGETS lua2c lualib EXPORT lua2cTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES README.md LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/lua2c)
install(DIRECTORY lua/lua/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lua2c/runtime
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(FILES runtime/lua2c_runtime.h "${CMAKE_CURRENT_BINARY_DIR}/generated/lua2c_vm.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lua2c/runtime)
install(EXPORT lua2cTargets NAMESPACE lua2c:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lua2c)
include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/lua2cConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/lua2cConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lua2c)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/lua2cConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lua2cConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/lua2cConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lua2c)

if(LUA2C_BUILD_TESTS)
    if(NOT LUA2C_BUILD_CLI)
        message(FATAL_ERROR "Tests require LUA2C_BUILD_CLI")
    endif()
    add_subdirectory(tests)
endif()
