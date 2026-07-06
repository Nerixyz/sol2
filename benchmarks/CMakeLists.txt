set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.28.0")
    set(_efa_flag EXCLUDE_FROM_ALL)
else()
    set(_efa_flag)
endif()
set(BENCHMARK_ENABLE_WERROR OFF CACHE INTERNAL "")
set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "")
FetchContent_Declare(
	GoogleBenchmark
	GIT_REPOSITORY https://github.com/google/benchmark.git
	GIT_TAG v1.9.5
    ${_efa_flag}
)
FetchContent_MakeAvailable(GoogleBenchmark)

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/DEBUG /INCREMENTAL:NO /OPT:REF,NOICF,LBR")
    # Use the function inlining level from 'Release' mode (2).
    string(REPLACE "/Ob1" "/Ob2" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()

add_executable(sol-benchmarks
    src/bench.cpp
    src/bench_solpp.cpp
)

target_link_libraries(sol-benchmarks PRIVATE
    sol2::sol2
    benchmark::benchmark
    benchmark::benchmark_main
    Lua::Lua
)
