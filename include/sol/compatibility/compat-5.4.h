#ifndef NOT_KEPLER_PROJECT_COMPAT54_H_
#define NOT_KEPLER_PROJECT_COMPAT54_H_

#include <sol/compatibility/lua_version.hpp>

#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 504

#if !defined(LUA_ERRGCMM)
/* So Lua 5.4 actually removes this, which breaks sol2...
 man, this API is quite unstable...!
*/
#  define LUA_ERRGCMM (LUA_ERRERR + 2)
#endif /* LUA_ERRGCMM define */

#endif // Lua 5.4 only

#endif // NOT_KEPLER_PROJECT_COMPAT54_H_
