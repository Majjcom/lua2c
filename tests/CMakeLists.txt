add_executable(lua2c_unit unit.cpp)
target_link_libraries(lua2c_unit PRIVATE lua2c lualib)
add_test(NAME library COMMAND lua2c_unit)

set(entry_c "${CMAKE_CURRENT_BINARY_DIR}/generated/entry.c")
set(entry_h "${CMAKE_CURRENT_BINARY_DIR}/generated/entry.h")
add_custom_command(OUTPUT "${entry_c}" "${entry_h}"
    COMMAND lua2c_cli --header --module entry -o "${entry_c}" "${CMAKE_CURRENT_SOURCE_DIR}/entry.lua"
    DEPENDS lua2c_cli "${CMAKE_CURRENT_SOURCE_DIR}/entry.lua" VERBATIM)
add_executable(lua2c_host host.c "${entry_c}" "${entry_h}")
target_include_directories(lua2c_host PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(lua2c_host PRIVATE lualib)
add_test(NAME host_entry COMMAND lua2c_host)
add_executable(lua2c_both "${entry_c}")
target_compile_definitions(lua2c_both PRIVATE LUA2C_MAIN=1 LUA2C_API=1)
target_link_libraries(lua2c_both PRIVATE lualib)
add_test(NAME both_macros COMMAND lua2c_both normal 7)
foreach(mode main api)
    add_executable(lua2c_header_${mode} header_cpp.cpp "${entry_c}" "${entry_h}")
    target_include_directories(lua2c_header_${mode} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(lua2c_header_${mode} PRIVATE lualib)
    if(mode STREQUAL "api")
        target_compile_definitions(lua2c_header_${mode} PRIVATE LUA2C_API=1)
    endif()
    add_test(NAME "header_cpp.${mode}" COMMAND lua2c_header_${mode})
endforeach()
add_test(NAME cli_header
    COMMAND "${CMAKE_COMMAND}" "-DCLI=$<TARGET_FILE:lua2c_cli>"
        "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/entry.lua"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/generated/cli_header.c"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/header_cli.cmake")
add_test(NAME cli_strip
    COMMAND "${CMAKE_COMMAND}" "-DCLI=$<TARGET_FILE:lua2c_cli>" "-DLUAC=$<TARGET_FILE:luac>"
        "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/entry.lua"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/generated/cli_strip.c"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/strip_cli.cmake")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/renamed")
set(renamed_c "${CMAKE_CURRENT_BINARY_DIR}/generated/renamed/entry.c")
set(renamed_h "${CMAKE_CURRENT_BINARY_DIR}/generated/renamed/entry.h")
add_custom_command(OUTPUT "${renamed_c}" "${renamed_h}"
    COMMAND lua2c_cli --strip --header --module entry --main-name run_custom_chunk
        -o "${renamed_c}" "${CMAKE_CURRENT_SOURCE_DIR}/entry.lua"
    DEPENDS lua2c_cli "${CMAKE_CURRENT_SOURCE_DIR}/entry.lua" VERBATIM)
add_executable(lua2c_renamed_host host.c "${renamed_c}" "${renamed_h}")
target_include_directories(lua2c_renamed_host PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/renamed")
target_compile_definitions(lua2c_renamed_host PRIVATE LUA2C_TEST_ENTRY=run_custom_chunk)
target_link_libraries(lua2c_renamed_host PRIVATE lualib)
add_test(NAME renamed_host_entry COMMAND lua2c_renamed_host)
foreach(mode main api)
    add_executable(lua2c_renamed_${mode} header_cpp.cpp "${renamed_c}" "${renamed_h}")
    target_include_directories(lua2c_renamed_${mode} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/renamed")
    target_compile_definitions(lua2c_renamed_${mode} PRIVATE LUA2C_TEST_ENTRY=run_custom_chunk)
    target_link_libraries(lua2c_renamed_${mode} PRIVATE lualib)
    if(mode STREQUAL "api")
        target_compile_definitions(lua2c_renamed_${mode} PRIVATE LUA2C_API=1)
    endif()
    add_test(NAME "renamed_header_cpp.${mode}" COMMAND lua2c_renamed_${mode})
endforeach()
add_executable(lua2c_renamed_both "${renamed_c}")
target_compile_definitions(lua2c_renamed_both PRIVATE LUA2C_MAIN=1 LUA2C_API=1)
target_link_libraries(lua2c_renamed_both PRIVATE lualib)
add_test(NAME renamed_both_macros COMMAND lua2c_renamed_both normal 7)
add_executable(lua2c_multiple_entries multiple_entries.cpp "${entry_c}" "${entry_h}" "${renamed_c}" "${renamed_h}")
target_include_directories(lua2c_multiple_entries PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(lua2c_multiple_entries PRIVATE lualib)
add_test(NAME multiple_entries COMMAND lua2c_multiple_entries)

file(GLOB LUA2C_TEST_SCRIPTS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/lua/*.lua" "${CMAKE_CURRENT_SOURCE_DIR}/semantics/*.lua")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
add_executable(lua2c_fixtures fixtures.cpp)
target_link_libraries(lua2c_fixtures PRIVATE lua2c lualib)
add_custom_command(OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/generated/loadkx.c"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/loadkx.luac"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/large_operands.c"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/large_operands.lua"
    COMMAND lua2c_fixtures "${CMAKE_CURRENT_BINARY_DIR}/generated"
    DEPENDS lua2c_fixtures VERBATIM)
foreach(fixture loadkx large_operands)
    set(extension lua)
    if(fixture STREQUAL "loadkx")
        set(extension luac)
    endif()
    add_executable(test_${fixture} "${CMAKE_CURRENT_BINARY_DIR}/generated/${fixture}.c")
    target_link_libraries(test_${fixture} PRIVATE lualib)
    target_compile_definitions(test_${fixture} PRIVATE LUA2C_MAIN=1)
    add_test(NAME "${fixture}"
        COMMAND "${CMAKE_COMMAND}" "-DLUA=$<TARGET_FILE:lua>"
            "-DNATIVE=$<TARGET_FILE:test_${fixture}>"
            "-DSCRIPT=${CMAKE_CURRENT_BINARY_DIR}/generated/${fixture}.${extension}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/compare.cmake")
endforeach()
foreach(script IN LISTS LUA2C_TEST_SCRIPTS)
    get_filename_component(name "${script}" NAME_WE)
    foreach(mode source bytecode stripped strip_source strip_bytecode)
        set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated/${name}_${mode}.c")
        set(input "${script}")
        set(reference "${script}")
        set(conversion_options)
        if(mode STREQUAL "bytecode" OR mode STREQUAL "stripped")
            set(input "${CMAKE_CURRENT_BINARY_DIR}/generated/${name}_${mode}.luac")
            set(strip_option)
            if(mode STREQUAL "stripped")
                set(strip_option -s)
            endif()
            add_custom_command(OUTPUT "${input}"
                COMMAND luac ${strip_option} -o "${input}" "${script}"
                DEPENDS luac "${script}" VERBATIM)
            set(reference "${input}")
        elseif(mode STREQUAL "strip_source" OR mode STREQUAL "strip_bytecode")
            set(reference "${CMAKE_CURRENT_BINARY_DIR}/generated/${name}_stripped.luac")
            set(conversion_options --strip --source)
            if(mode STREQUAL "strip_bytecode")
                set(input "${CMAKE_CURRENT_BINARY_DIR}/generated/${name}_bytecode.luac")
                set(conversion_options -s --bytecode)
            endif()
        endif()
        add_custom_command(OUTPUT "${generated}"
            COMMAND lua2c_cli ${conversion_options} --module "test_${name}" -o "${generated}" "${input}"
            DEPENDS lua2c_cli "${input}" "${reference}" VERBATIM)
        set(target "test_${name}_${mode}")
        add_executable(${target} "${generated}")
        target_compile_definitions(${target} PRIVATE LUA2C_MAIN=1)
        target_link_libraries(${target} PRIVATE lualib)
        # Names beginning with a digit are not legal module identifiers.
        set_property(SOURCE "${generated}" PROPERTY GENERATED TRUE)
        add_test(NAME "${name}.${mode}"
            COMMAND "${CMAKE_COMMAND}" "-DLUA=$<TARGET_FILE:lua>"
                "-DNATIVE=$<TARGET_FILE:${target}>" "-DSCRIPT=${reference}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/compare.cmake")
        set_tests_properties("${name}.${mode}" PROPERTIES TIMEOUT 60)
    endforeach()
endforeach()

if(LUA2C_SHARED_RUNTIME)
    set(module_c "${CMAKE_CURRENT_BINARY_DIR}/generated/native_module.c")
    add_custom_command(OUTPUT "${module_c}"
        COMMAND lua2c_cli --strip --module native_module --main-name run_native_module -o "${module_c}"
            "${CMAKE_CURRENT_SOURCE_DIR}/module.lua"
        DEPENDS lua2c_cli "${CMAKE_CURRENT_SOURCE_DIR}/module.lua" VERBATIM)
    add_library(native_module MODULE "${module_c}")
    set_target_properties(native_module PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    target_compile_definitions(native_module PRIVATE LUA2C_API=1)
    target_link_libraries(native_module PRIVATE lualib)
    add_test(NAME require_module COMMAND lua "${CMAKE_CURRENT_SOURCE_DIR}/require.lua" "$<TARGET_FILE:native_module>")
endif()
