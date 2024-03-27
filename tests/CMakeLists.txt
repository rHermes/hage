# Testing library
FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
)

FetchContent_MakeAvailable(doctest)

add_executable(hage_test doctest.cpp
        logging_tests.cpp)

if (MSCV)
else()
    target_compile_options(hage_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(hage_test PRIVATE -Wno-interference-size)
endif()


target_link_libraries(hage_test PRIVATE hage_lib doctest::doctest)
add_test(NAME hage_test_test COMMAND hage_test)