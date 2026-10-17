execute_process(COMMAND "${LUA}" "${SCRIPT}" alpha beta
    RESULT_VARIABLE reference_status OUTPUT_VARIABLE reference ERROR_VARIABLE reference_error)
execute_process(COMMAND "${NATIVE}" alpha beta
    RESULT_VARIABLE native_status OUTPUT_VARIABLE native ERROR_VARIABLE native_error)
if(NOT "${reference_status}" STREQUAL "${native_status}" OR NOT "${reference}" STREQUAL "${native}")
    message(FATAL_ERROR "Execution differs for ${SCRIPT}\nLua (${reference_status}):\n${reference}\n${reference_error}\nNative (${native_status}):\n${native}\n${native_error}")
endif()
if(NOT native_status EQUAL 0)
    message(FATAL_ERROR "Test script failed: ${native_error}")
endif()
