
add_executable(compiler_bug_1 compiler_bug_1.cpp)
add_executable(compiler_bug_2 compiler_bug_2.cpp)

target_compile_features(compiler_bug_1 PUBLIC cxx_std_20)
set_target_properties(compiler_bug_1 PROPERTIES CXX_EXTENSIONS OFF)

target_compile_features(compiler_bug_2 PUBLIC cxx_std_20)
set_target_properties(compiler_bug_2 PROPERTIES CXX_EXTENSIONS OFF)

if (MSVC)
    target_compile_options(compiler_bug_1 PRIVATE /W4 /utf-8 /permissive- /Zc:__cplusplus )
    target_compile_options(compiler_bug_2 PRIVATE /W4 /utf-8 /permissive- /Zc:__cplusplus )
else()
    target_compile_options(compiler_bug_1 PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(compiler_bug_2 PRIVATE -Wall -Wextra -Wpedantic)

    # target_compile_options(hage_test PRIVATE -fconcepts-diagnostics-depth=2)


    # target_compile_options(hage_test PUBLIC -fsanitize=thread)
    # target_link_options(hage_test PUBLIC -fsanitize=thread)


    #target_compile_options(compiler_bug_1 PUBLIC "-fsanitize=address,undefined")
    # target_link_options(compiler_bug_1 PUBLIC "-fsanitize=address,undefined")

    target_compile_options(compiler_bug_1 PUBLIC "-fsanitize=address" )
    target_compile_options(compiler_bug_1 PUBLIC "-fsanitize=undefined" )
    target_link_options(compiler_bug_1 PUBLIC "-fsanitize=address")
    target_link_options(compiler_bug_1 PUBLIC  "-fsanitize=undefined")

    target_compile_options(compiler_bug_2 PUBLIC "-fsanitize=address")
    target_compile_options(compiler_bug_2 PUBLIC "-fsanitize=undefined" )
    target_link_options(compiler_bug_2 PUBLIC  "-fsanitize=undefined")
    target_link_options(compiler_bug_2 PUBLIC "-fsanitize=address")
endif()