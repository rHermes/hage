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
        "${hage_SOURCE_DIR}/include/hage/logging/ring_buffer.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/queue_buffer.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/byte_buffer.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/logger.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/serializers.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/sink.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/console_sink.hpp"
)

add_library(hage_lib ${HEADER_LIST}
        logging/console_sink.cpp)

# We need this directory, and users of our library will need it to.
target_include_directories(hage_lib PUBLIC ../include)

# Users of this library needs c++20
target_compile_features(hage_lib PUBLIC cxx_std_20)
set_target_properties(hage_lib PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(hage_lib PUBLIC fmt::fmt)

if (MSVC)
else()
    target_compile_options(hage_lib PRIVATE -Wall -Wextra -Wpedantic)

    CHECK_CXX_COMPILER_FLAG("-Wno-interference-size" COMPILER_SUPPORTS_NO_INTERFERENCE_SIZE)
    if(COMPILER_SUPPORTS_NO_INTERFERENCE_SIZE)
        target_compile_options(hage_lib PRIVATE -Wno-interference-size)
    endif()

    # target_compile_options(hage_lib PUBLIC -fsanitize=thread)
    # target_link_options(hage_lib PUBLIC -fsanitize=thread)
endif()




source_group(
        TREE "${PROJECT_SOURCE_DIR}/include"
        PREFIX "Header files"
        FILES ${HEADER_LIST}
)