foreach(flag --help -h)
    execute_process(COMMAND "${CLI}" "${flag}"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT status EQUAL 0 OR NOT error STREQUAL "" OR output STREQUAL "")
        message(FATAL_ERROR "CLI help failed for ${flag}: ${status}\n${output}\n${error}")
    endif()
    foreach(fragment "Usage:" "Input:" "Options:" "Examples:" "--source" "--bytecode"
            "-s, --strip" "-o FILE" "--header" "--main-name NAME" "--module NAME" "--chunk-name NAME"
            "Default: lua2c_main" "Default: lua2c_module" "default: stdout" "default: @INPUT"
            "Requires -o" "compile-time options" "LUA2C_MAIN=1" "LUA2C_API=1")
        string(FIND "${output}" "${fragment}" position)
        if(position EQUAL -1)
            message(FATAL_ERROR "CLI help is missing: ${fragment}")
        endif()
    endforeach()
    if(flag STREQUAL "--help")
        set(long_help "${output}")
    elseif(NOT output STREQUAL long_help)
        message(FATAL_ERROR "-h and --help produce different output")
    endif()
endforeach()

execute_process(COMMAND "${CLI}" --header --module cli_header -o "${OUTPUT}" "${INPUT}"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 0 OR NOT output STREQUAL "")
    message(FATAL_ERROR "Header CLI failed: ${status}\n${output}\n${error}")
endif()
get_filename_component(directory "${OUTPUT}" DIRECTORY)
get_filename_component(stem "${OUTPUT}" NAME_WE)
set(header "${directory}/${stem}.h")
if(NOT EXISTS "${OUTPUT}" OR NOT EXISTS "${header}")
    message(FATAL_ERROR "CLI did not generate both C and header files")
endif()
file(READ "${header}" declarations)
if(NOT declarations MATCHES "int lua2c_main\\(lua_State \\*L\\);" OR
   NOT declarations MATCHES "int luaopen_cli_header\\(lua_State \\*L\\);")
    message(FATAL_ERROR "Generated header lacks public declarations")
endif()
execute_process(COMMAND "${CLI}" --header --module cli_header --main-name cli_entry -o "${OUTPUT}" "${INPUT}"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 0 OR NOT output STREQUAL "")
    message(FATAL_ERROR "Custom-entry CLI failed: ${status}\n${output}\n${error}")
endif()
file(READ "${OUTPUT}" generated)
file(READ "${header}" declarations)
if(NOT declarations MATCHES "int cli_entry\\(lua_State \\*L\\);" OR
   NOT declarations MATCHES "int luaopen_cli_header\\(lua_State \\*L\\);" OR
   NOT generated MATCHES "int cli_entry\\(lua_State \\*L\\)" OR
   NOT generated MATCHES "return cli_entry\\(L\\);" OR
   generated MATCHES "int lua2c_main\\(lua_State \\*L\\)")
    message(FATAL_ERROR "CLI did not consistently rename the entry definition, header and module wrapper")
endif()
execute_process(COMMAND "${CLI}" --main-name stdout_entry "${INPUT}"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 0 OR NOT output MATCHES "int stdout_entry\\(lua_State \\*L\\)")
    message(FATAL_ERROR "Custom-entry stdout conversion failed: ${status}\n${error}")
endif()
execute_process(COMMAND "${CLI}" --header --main-name for -o "${OUTPUT}" "${INPUT}"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(status EQUAL 0 OR NOT output STREQUAL "" OR NOT error MATCHES "main_name")
    message(FATAL_ERROR "CLI accepted an invalid main name: ${status}\n${output}\n${error}")
endif()
file(READ "${OUTPUT}" after_error)
file(READ "${header}" header_after_error)
if(NOT after_error STREQUAL generated OR NOT header_after_error STREQUAL declarations)
    message(FATAL_ERROR "Invalid entry name overwrote C/header outputs")
endif()
execute_process(COMMAND "${CLI}" --main-name
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(status EQUAL 0 OR NOT error MATCHES "missing value after --main-name")
    message(FATAL_ERROR "CLI accepted --main-name without a value: ${status}\n${error}")
endif()
execute_process(COMMAND "${CLI}" --header "${INPUT}"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(status EQUAL 0 OR NOT output STREQUAL "" OR NOT error MATCHES "--header requires -o")
    message(FATAL_ERROR "CLI must reject --header without -o: ${status}\n${output}\n${error}")
endif()
