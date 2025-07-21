cmake_minimum_required(VERSION 3.10.0)
project(LuauTest VERSION 0.1.0 LANGUAGES C CXX)

add_executable(LuauTest main.cpp)
target_compile_features(LuauTest PUBLIC cxx_std_23)
target_compile_definitions(LuauTest PRIVATE SOL_USE_LUAU=1 SOL_LUA_BIT32_LIB=1 SOL_SAFE_STACK_CHECK=1)