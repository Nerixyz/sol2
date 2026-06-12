# Include guard
if (_luau_build_included)
    return()
endif (_luau_build_included)
set(_luau_build_included true)

# Import necessary standard modules
include(ExternalProject)
include(Common/Core)

# Latest version of Luau
set(LUAU_LATEST_VERSION 0.724)

# Clean up and validate the version
if (LUA_VERSION MATCHES "^Luau ([0-9]+)\\.([0-9]+)$")
    set(LUAU_VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2})
else()
    set(LUAU_VERSION ${LUAU_LATEST_VERSION})
endif()

set(LUA_BUILD_LIBRARY_TYPE "STATIC")
FIND_PACKAGE_MESSAGE(LUABUILD
    "Selecting Luau ${LUAU_VERSION} from '${LUA_VERSION}' and building a ${LUA_BUILD_LIBRARY_TYPE} library..."
    "[${LUAU_VERSION}][${LUA_VERSION}][${LUA_BUILD_LIBRARY_TYPE}]")

# Download and extract Luau source
include(FetchContent)

set(LUAU_BUILD_CLI OFF CACHE BOOL "Build CLI")
set(LUAU_BUILD_TESTS OFF CACHE BOOL "Build tests")
set(LUAU_BUILD_WEB OFF CACHE BOOL "Build Web module")
set(LUAU_WERROR OFF CACHE BOOL "Warnings as errors")
set(LUAU_STATIC_CRT OFF CACHE BOOL "Link with the static CRT (/MT)")
set(LUAU_EXTERN_C ON CACHE BOOL "Use extern C for all APIs")

FetchContent_Declare(
    luau
    URL https://github.com/luau-lang/luau/archive/refs/tags/${LUAU_VERSION}.tar.gz
)
FetchContent_MakeAvailable(luau)

add_library(sol_luau INTERFACE)
add_library(Lua::Lua ALIAS sol_luau)
target_link_libraries(sol_luau INTERFACE 
    Luau.Compiler
    Luau.VM
    Luau.VM.Internals
)

# Set include directories
set(LUA_INCLUDE_DIRS
    ${luau_SOURCE_DIR}/Common/include
    ${luau_SOURCE_DIR}/Ast/include
    ${luau_SOURCE_DIR}/Compiler/include
    ${luau_SOURCE_DIR}/Config/include
    ${luau_SOURCE_DIR}/Analysis/include
    ${luau_SOURCE_DIR}/VM/include
)
set(LUA_LIBRARIES Lua::Lua)
set(LUA_INTERPRETER "")
