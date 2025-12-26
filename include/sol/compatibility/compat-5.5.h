#ifndef NOT_KEPLER_PROJECT_COMPAT55_H_
#define NOT_KEPLER_PROJECT_COMPAT55_H_

#if defined(__cplusplus) && !defined(COMPAT53_LUA_CPP)
extern "C" {
#endif
#if __has_include(<lua/lua.h>)
  #include <lua/lua.h>
  #include <lua/lauxlib.h>
  #include <lua/lualib.h>
#else
  #include <lua.h>
  #include <lauxlib.h>
  #include <lualib.h>
#endif
#if defined(__cplusplus) && !defined(COMPAT53_LUA_CPP)
}
#endif

// Lua 5.5 AND ABOVE
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 505
#define sol_detail_lua_newstate(f, ud) lua_newstate(f, ud, /*seed=*/0)
#else
#define sol_detail_lua_newstate(f, ud) lua_newstate(f, ud)
#endif

#endif // NOT_KEPLER_PROJECT_COMPAT55_H_
