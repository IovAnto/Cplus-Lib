cmake_minimum_required(VERSION 3.20)
project(Cplus VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_ASAN "Enable Address and Undefined Behavior Sanitizers" OFF)

# Compiler optimization and warning flags
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -ffunction-sections -fdata-sections
    )
    add_link_options(-Wl,--gc-sections)
    
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -march=native)
    elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3 -O0)
    endif()

    if (ENABLE_ASAN)
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# Core library sources
set(CPLUS_SOURCES
    src/cp_mem.c
    src/cp_str.c
    src/cp_fs.c
    src/cp_ds.c
    src/cp_graph.c
    src/cp_algo.c
    src/cp_math.c
    src/cp_thread.c
    src/cp_net.c
    src/cp_tui.c
    src/cp_hash.c
    src/cp_map.c
    src/cp_args.c
    src/cp_time.c
    src/cp_json.c
    src/cp_stack.c
    src/cp_os.c
)

add_library(cplus STATIC ${CPLUS_SOURCES})
target_include_directories(cplus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(cplus PUBLIC pthread m)

# Testing
enable_testing()

add_executable(test_mem tests/test_mem.c)
target_link_libraries(test_mem PRIVATE cplus)
add_test(NAME test_mem COMMAND test_mem)

add_executable(test_str_fs tests/test_str_fs.c)
target_link_libraries(test_str_fs PRIVATE cplus)
add_test(NAME test_str_fs COMMAND test_str_fs)

add_executable(test_ds tests/test_ds.c)
target_link_libraries(test_ds PRIVATE cplus)
add_test(NAME test_ds COMMAND test_ds)

add_executable(test_graph tests/test_graph.c)
target_link_libraries(test_graph PRIVATE cplus)
add_test(NAME test_graph COMMAND test_graph)

add_executable(test_algo_math tests/test_algo_math.c)
target_link_libraries(test_algo_math PRIVATE cplus)
add_test(NAME test_algo_math COMMAND test_algo_math)

add_executable(test_thread_net tests/test_thread_net.c)
target_link_libraries(test_thread_net PRIVATE cplus)
add_test(NAME test_thread_net COMMAND test_thread_net)

add_executable(test_tui tests/test_tui.c)
target_link_libraries(test_tui PRIVATE cplus)
add_test(NAME test_tui COMMAND test_tui)

add_executable(test_convenience tests/test_convenience.c)
target_link_libraries(test_convenience PRIVATE cplus)
add_test(NAME test_convenience COMMAND test_convenience)

add_executable(test_stack tests/test_stack.c)
target_link_libraries(test_stack PRIVATE cplus)
add_test(NAME test_stack COMMAND test_stack)

add_executable(test_os tests/test_os.c)
target_link_libraries(test_os PRIVATE cplus)
add_test(NAME test_os COMMAND test_os)

add_executable(test_utils tests/test_utils.c)
target_link_libraries(test_utils PRIVATE cplus)
add_test(NAME test_utils COMMAND test_utils)

# Benchmarks
add_executable(bench_mem benchmarks/bench_mem.c)
target_link_libraries(bench_mem PRIVATE cplus)

add_executable(bench_str_fs benchmarks/bench_str_fs.c)
target_link_libraries(bench_str_fs PRIVATE cplus)

add_executable(bench_ds benchmarks/bench_ds.c)
target_link_libraries(bench_ds PRIVATE cplus)

add_executable(bench_graph benchmarks/bench_graph.c)
target_link_libraries(bench_graph PRIVATE cplus)

add_executable(bench_algo_math benchmarks/bench_algo_math.c)
target_link_libraries(bench_algo_math PRIVATE cplus)

add_executable(bench_thread_net benchmarks/bench_thread_net.c)
target_link_libraries(bench_thread_net PRIVATE cplus)

add_executable(bench_tui benchmarks/bench_tui.c)
target_link_libraries(bench_tui PRIVATE cplus)

add_executable(bench_map benchmarks/bench_map.c)
target_link_libraries(bench_map PRIVATE cplus)

add_executable(bench_stack benchmarks/bench_stack.c)
target_link_libraries(bench_stack PRIVATE cplus)

add_executable(bench_utils benchmarks/bench_utils.c)
target_link_libraries(bench_utils PRIVATE cplus)

# Visual Demos & Comparative Examples
add_executable(demo_pure_c examples/demo_pure_c_monitor.c)
target_link_libraries(demo_pure_c PRIVATE m)

add_executable(demo_cplus examples/demo_cplus_monitor.c)
target_link_libraries(demo_cplus PRIVATE cplus)

add_executable(quickstart examples/quickstart.c)
target_link_libraries(quickstart PRIVATE cplus)

# -----------------------------------------------------------------------------
# Installation & Packaging
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cplus.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/cplus.pc
    @ONLY
)

install(TARGETS cplus
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/cplus
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES dist/cplus.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cplus.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
