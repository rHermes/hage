# Testing library
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 10.2.1
)

FetchContent_MakeAvailable(fmt)


set(CORE_HEADER_LIST
        "${hage_SOURCE_DIR}/include/hage/core/misc.hpp"
        "${hage_SOURCE_DIR}/include/hage/core/lifetime_tester.hpp"
        "${hage_SOURCE_DIR}/include/hage/core/concepts.hpp"
        "${hage_SOURCE_DIR}/include/hage/core/assert.hpp"
        "${hage_SOURCE_DIR}/include/hage/core/charconv.hpp"
)


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
        "${hage_SOURCE_DIR}/include/hage/logging.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/ring_buffer.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/vector_buffer.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/byte_buffer.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/logger.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/serializers.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/sink.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/file_sink.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/rotating_file_sink.hpp"
        "${hage_SOURCE_DIR}/include/hage/logging/console_sink.hpp"
)


add_library(hage_core INTERFACE)

# some compilers don't support the float charconvs yet
try_compile(HAS_FLOAT_CHARCONV SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/compile_tests/float_charconv.cpp
        CXX_STANDARD 20 CXX_EXTENSIONS OFF)

if(NOT HAS_FLOAT_CHARCONV)
    target_compile_definitions(hage_core INTERFACE HAGE_NO_FLOAT_CHARCONV=1)
endif()


target_sources(hage_core INTERFACE ${CORE_HEADER_LIST})
target_include_directories(hage_core INTERFACE ../include)

add_library(hage_atomic INTERFACE)

target_sources(hage_atomic INTERFACE
        ${ATOMIC_HEADER_LIST}
)

target_include_directories(hage_atomic INTERFACE ../include)

add_library(hage_logging ${LOGGING_HEADER_LIST}
        logging/console_sink.cpp
        logging/file_sink.cpp
        logging/rotating_file_sink.cpp)

# We need this directory, and users of our library will need it to.
target_include_directories(hage_logging PUBLIC ../include)
target_link_libraries(hage_logging PUBLIC fmt::fmt hage_atomic hage_core)

foreach (target_var IN ITEMS hage_atomic hage_logging hage_core)
    get_target_property(target_type ${target_var} TYPE)

    if (target_type STREQUAL "INTERFACE_LIBRARY")
        set(pub_type INTERFACE)
        set(priv_type INTERFACE)
    else ()
        set(pub_type PUBLIC)
        set(priv_type PRIVATE)
    endif ()

    target_compile_features(${target_var} ${pub_type} cxx_std_20)
    set_target_properties(${target_var} PROPERTIES CXX_EXTENSIONS OFF)


    if (MSVC)
        target_compile_options(${target_var} ${priv_type} /W4 /utf-8)

        # These options make msvc act sane.
        # This is to make sure that the new preprocessor is chosen, which is required
        target_compile_options(${target_var} ${pub_type} /permissive- /Zc:__cplusplus /Zc:preprocessor /EHsc)
    else ()
        target_compile_options(${target_var} ${priv_type} -Wall -Wextra -Wpedantic)

        CHECK_CXX_COMPILER_FLAG("-Wno-interference-size" COMPILER_SUPPORTS_NO_INTERFERENCE_SIZE)
        if (COMPILER_SUPPORTS_NO_INTERFERENCE_SIZE)
            target_compile_options(${target_var} ${priv_type} -Wno-interference-size)
        endif ()

        # target_compile_options(${target_var} ${pub_type} -fsanitize=thread)
        # target_link_options(${target_var} ${pub_type} -fsanitize=thread)

        # target_compile_options(${target_var} ${pub_type}  "-fsanitize=address,undefined")
        # target_link_options(${target_var} ${pub_type}  "-fsanitize=address,undefined")
    endif ()
endforeach ()

source_group(
        TREE "${PROJECT_SOURCE_DIR}/include"
        PREFIX "Header files"
        FILES ${LOGGING_HEADER_LIST}
)