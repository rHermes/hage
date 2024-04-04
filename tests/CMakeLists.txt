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
        test_sink.hpp)

target_compile_features(hage_test PUBLIC cxx_std_20)
set_target_properties(hage_test PROPERTIES CXX_EXTENSIONS OFF)

if (MSVC)
else()
    target_compile_options(hage_test PRIVATE -Wall -Wextra -Wpedantic)

    CHECK_CXX_COMPILER_FLAG("-Wno-interference-size" COMPILER_SUPPORTS_NO_INTERFERENCE_SIZE)
    if(COMPILER_SUPPORTS_NO_INTERFERENCE_SIZE)
        target_compile_options(hage_test PRIVATE -Wno-interference-size)
    endif()

    # target_compile_options(hage_test PRIVATE -fconcepts-diagnostics-depth=2)
endif()

target_compile_definitions(hage_test PRIVATE DOCTEST_CONFIG_SUPER_FAST_ASSERTS)

target_link_libraries(hage_test PRIVATE hage_lib doctest::doctest)
# add_test(NAME hage_test_test COMMAND hage_test)

include(doctest)

doctest_discover_tests(hage_test EXTRA_ARGS "-ni")