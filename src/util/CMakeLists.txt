git_submodule_build(zlib)

set(UTIL_SOURCES
        compression.cpp
        )

    set(UTIL_HEADERS
        compression.hpp
        )

add_library(pe_util SHARED ${UTIL_SOURCES})
