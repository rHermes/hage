# Testing library
FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
)

FetchContent_MakeAvailable(doctest)

set(HEADER_LIST
        "${hage_SOURCE_DIR}/include/hage/misc.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging.hpp"
)

add_library(hage_lib lib.cpp ${HEADER_LIST})

# We need this directory, and users of our library will need it to.
target_include_directories(hage_lib PUBLIC ../include)

# Users of this library needs c++20
target_compile_features(hage_lib PUBLIC cxx_std_20)

add_executable(hage_test doctest.cpp
        logging_tests.cpp)
target_link_libraries(hage_test PRIVATE hage_lib doctest::doctest)
add_test(NAME hage_test_test COMMAND hage_test)



source_group(
        TREE "${PROJECT_SOURCE_DIR}/include"
        PREFIX "Header files"
        FILES ${HEADER_LIST}
)