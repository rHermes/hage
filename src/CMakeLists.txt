# Testing library
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 10.2.1
)

FetchContent_MakeAvailable(fmt)


set(ATOMIC_HEADER_LIST
        "${hage_SOURCE_DIR}/include/hage/atomic/atomic.hpp"
        "${hage_SOURCE_DIR}/include/hage/atomic/atomic_base.hpp"
        "${hage_SOURCE_DIR}/include/hage/atomic/atomic_flag.hpp"
        "${hage_SOURCE_DIR}/include/hage/atomic/atomic_floats.hpp"
        "${hage_SOURCE_DIR}/include/hage/atomic/atomic_ints.hpp"
        "${hage_SOURCE_DIR}/include/hage/atomic/atomic_ptr.hpp"
        "${hage_SOURCE_DIR}/include/hage/atomic/concepts.hpp"
)


set(LOGGING_HEADER_LIST
        "${hage_SOURCE_DIR}/include/hage/misc.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/ring_buffer.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/vector_buffer.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/byte_buffer.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/logger.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/serializers.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/sink.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/file_sink.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/console_sink.hpp"
)

add_library(hage_atomic INTERFACE)

target_sources(hage_atomic INTERFACE
        ${ATOMIC_HEADER_LIST}
)

target_include_directories(hage_atomic INTERFACE ../include)

# Users of this library needs c++20
target_compile_features(hage_atomic INTERFACE cxx_std_20)
set_target_properties(hage_atomic PROPERTIES CXX_EXTENSIONS OFF)

add_library(hage_logging ${LOGGING_HEADER_LIST}
        logging/console_sink.cpp
        logging/file_sink.cpp)


# We need this directory, and users of our library will need it to.
target_include_directories(hage_logging PUBLIC ../include)

# Users of this library needs c++20
target_compile_features(hage_logging PUBLIC cxx_std_20)
set_target_properties(hage_logging PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(hage_logging PUBLIC fmt::fmt hage_atomic)

if (MSVC)
    target_compile_options(hage_atomic PRIVATE /W4 /utf-8 /permissive- /Zc:__cplusplus)
    target_compile_options(hage_logging PRIVATE /W4 /utf-8 /permissive- /Zc:__cplusplus)
else ()
    target_compile_options(hage_atomic INTERFACE -Wall -Wextra -Wpedantic)
    target_compile_options(hage_logging PRIVATE -Wall -Wextra -Wpedantic)

    CHECK_CXX_COMPILER_FLAG("-Wno-interference-size" COMPILER_SUPPORTS_NO_INTERFERENCE_SIZE)
    if (COMPILER_SUPPORTS_NO_INTERFERENCE_SIZE)
        target_compile_options(hage_atomic INTERFACE -Wno-interference-size)
        target_compile_options(hage_logging PRIVATE -Wno-interference-size)
    endif ()


    #target_compile_options(hage_logging PUBLIC -fsanitize=thread)
    #target_link_options(hage_logging PUBLIC -fsanitize=thread)

    target_compile_options(hage_logging PUBLIC "-fsanitize=address,undefined")
    target_link_options(hage_logging PUBLIC "-fsanitize=address,undefined")

endif ()


source_group(
        TREE "${PROJECT_SOURCE_DIR}/include"
        PREFIX "Header files"
        FILES ${LOGGING_HEADER_LIST}
)