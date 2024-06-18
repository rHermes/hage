
find_package(Threads REQUIRED)

#add_executable(compiler_bug_1 compiler_bug_1.cpp)
#add_executable(compiler_bug_2 compiler_bug_2.cpp)
add_executable(play play.cpp)

# target_link_libraries(compiler_bug_1 PRIVATE Threads::Threads)

# target_compile_options(compiler_bug_1 PRIVATE -pthread)

foreach (target_var IN ITEMS play)
    target_compile_features(${target_var} PUBLIC cxx_std_20)
    set_target_properties(${target_var} PROPERTIES CXX_EXTENSIONS OFF)


    if (MSVC)
        target_compile_options(${target_var} PRIVATE /W4 /utf-8 /permissive- /Zc:__cplusplus)
    else ()
        target_compile_options(${target_var} PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
        #target_compile_options(compiler_bug_1 PUBLIC "-fsanitize=address,undefined")
        # target_link_options(hage_test PUBLIC -fsanitize=thread)
        # target_compile_options(hage_test PRIVATE -fconcepts-diagnostics-depth=2)
    endif ()
endforeach ()