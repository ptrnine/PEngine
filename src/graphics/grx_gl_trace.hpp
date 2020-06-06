#pragma once

#define ENABLE_DETAILED_LOG

#include <core/log.hpp>

//#define TRACE_GL_CALLS

#ifdef TRACE_GL_CALLS
    #define GL_TRACE_NO_ARG(function_name) \
        (core::logger::instance().write(core::logger::Details, "{}()", #function_name), function_name())

    #define GL_TRACE(function_name, ...) \
        ( \
        core::logger::instance().write(core::logger::Details, "{}({}) \n\t\tevaluate to: {}{}", \
            #function_name, #__VA_ARGS__, #function_name, std::make_tuple(__VA_ARGS__)), \
        function_name(__VA_ARGS__))

#else
    #define GL_TRACE_NO_ARG(function_name) \
        function_name()

    #define GL_TRACE(function_name, ...) \
        function_name(__VA_ARGS__)
#endif

