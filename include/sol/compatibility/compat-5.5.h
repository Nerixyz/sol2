#ifndef NOT_KEPLER_PROJECT_COMPAT55_H_
#define NOT_KEPLER_PROJECT_COMPAT55_H_

#include <sol/compatibility/lua_version.hpp>

// Lua 5.5 AND ABOVE
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 505
#define sol_detail_lua_newstate(f, ud) lua_newstate(f, ud, /*seed=*/0)
#else
#define sol_detail_lua_newstate(f, ud) lua_newstate(f, ud)
#endif

#endif // NOT_KEPLER_PROJECT_COMPAT55_H_
