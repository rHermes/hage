
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${Have_SOURCE_DIR}/include/hage/*.hpp")

add_library(hage_lib lib.cpp ${HEADER_LIST})

# We need this directory, and users of our library will need it to.
target_include_directories(hage_lib PUBLIC ../include)

# Users of this library needs c++20
target_compile_features(hage_lib PUBLIC cxx_std_20)

source_group(
        TREE "${PROJECT_SOURCE_DIR}/include"
        PREFIX "Header files"
        FILES ${HEADER_LIST}
)