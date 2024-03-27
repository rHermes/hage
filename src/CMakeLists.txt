# Testing library
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 10.2.1
)

FetchContent_MakeAvailable(fmt)


set(HEADER_LIST
        "${hage_SOURCE_DIR}/include/hage/misc.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging.hpp"
)

add_library(hage_lib lib.cpp ${HEADER_LIST})

# We need this directory, and users of our library will need it to.
target_include_directories(hage_lib PUBLIC ../include)

# Users of this library needs c++20
target_compile_features(hage_lib PUBLIC cxx_std_20)

target_link_libraries(hage_lib PRIVATE fmt::fmt)

if (MSCV)
else()
    target_compile_options(hage_lib PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(hage_lib PRIVATE -Wno-interference-size)
    # target_compile_options(hage_lib PUBLIC -fsanitize=thread)
    # target_link_options(hage_lib PUBLIC -fsanitize=thread)
endif()




source_group(
        TREE "${PROJECT_SOURCE_DIR}/include"
        PREFIX "Header files"
        FILES ${HEADER_LIST}
)