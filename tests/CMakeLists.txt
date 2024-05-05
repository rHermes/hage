# Testing library
FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
)

FetchContent_MakeAvailable(doctest)

list(APPEND CMAKE_MODULE_PATH ${doctest_SOURCE_DIR}/scripts/cmake)

add_executable(hage_test doctest.cpp
        logging_tests.cpp
        test_sink.cpp
        test_sink.hpp
        test_utils.hpp
        test_utils.cpp
        ring_buffer_tests.cpp
        atomics_tests.cpp
        rotating_file_sink_tests.cpp
        core_charconv_tests.cpp)

find_package(Threads REQUIRED)

#target_compile_features(hage_test PUBLIC cxx_std_20)
# set_target_properties(hage_test PROPERTIES CXX_EXTENSIONS OFF)

if (MSVC)
    target_compile_options(hage_test PRIVATE /W4 /utf-8 /permissive- /Zc:__cplusplus)
else ()
    target_compile_options(hage_test PRIVATE -Wall -Wextra -Wpedantic)

    CHECK_CXX_COMPILER_FLAG("-Wno-interference-size" COMPILER_SUPPORTS_NO_INTERFERENCE_SIZE)
    if (COMPILER_SUPPORTS_NO_INTERFERENCE_SIZE)
        target_compile_options(hage_test PRIVATE -Wno-interference-size)
    endif ()

    # target_compile_options(hage_test PRIVATE -fconcepts-diagnostics-depth=2)


    # target_compile_options(hage_test PUBLIC -fsanitize=thread)
    #target_link_options(hage_test PUBLIC -fsanitize=thread)


    #target_compile_options(hage_test PUBLIC "-fsanitize=address,undefined")
    #target_link_options(hage_test PUBLIC "-fsanitize=address,undefined")

    # target_compile_options(hage_test PUBLIC "-fsanitize=address" )
    #target_compile_options(hage_test PUBLIC "-fsanitize=undefined" )
    # target_link_options(hage_test PUBLIC "-fsanitize=address")
    #target_link_options(hage_test PUBLIC  "-fsanitize=undefined")
endif ()

# target_compile_definitions(hage_test PRIVATE DOCTEST_CONFIG_SUPER_FAST_ASSERTS)

target_link_libraries(hage_test PRIVATE hage_logging hage_atomic doctest::doctest
    Threads::Threads)
# add_test(NAME hage_test_test COMMAND hage_test)

include(doctest)

doctest_discover_tests(hage_test EXTRA_ARGS "-ni")