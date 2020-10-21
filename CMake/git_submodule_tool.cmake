macro(git_submodule_build_tool _tool_name)
    message("${PE_TOOLS_DIR}/${_tool_name}")

    if (NOT ${_tool_name}_ALREADY_BUILT)
        execute_process(COMMAND make CC="${CMAKE_C_COMPILER}"
                RESULT_VARIABLE result
                WORKING_DIRECTORY "${PE_TOOLS_DIR}/${_tool_name}"
                )

        if(result)
            message(FATAL_ERROR "Build step for ${_tool_name} failed: ${result}")
        endif()

        set(${_tool_name}_ALREADY_BUILT ON CACHE STRING "Is tool already built")
    endif()
endmacro()
