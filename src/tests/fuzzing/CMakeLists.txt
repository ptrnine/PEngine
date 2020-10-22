add_afl_fuzz_test(dummy_test SOURCES dummy.cpp LINK_DIRS ${CONAN_LIB_DIRS} LIBS ${CONAN_LIBS})
add_afl_fuzz_test(config_test SOURCES config.cpp LINK_DIRS ${CONAN_LIB_DIRS} LIBS ${CONAN_LIBS})
