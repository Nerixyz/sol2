#ifndef NOT_KEPLER_PROJECT_COMPATLUAU_H_
#define NOT_KEPLER_PROJECT_COMPATLUAU_H_

#include <sol/compatibility/lua_version.hpp>

#if SOL_IS_ON(SOL_USE_LUAU)

#include <string_view>
#include <Luau/Bytecode.h>
#include <Luau/Compiler.h>


#ifndef COMPATU_PREFIX
#  define COMPATU_PREFIX kp_compatU
#endif // COMPATU_PREFIX

#ifndef COMPATU_API
#  if defined(COMPATU_INCLUDE_SOURCE) && COMPATU_INCLUDE_SOURCE
#    if defined(__GNUC__) || defined(__clang__)
#      define COMPATU_API __attribute__((__unused__)) static inline 
#    else
#      define COMPATU_API static inline 
#    endif /* Clang/GCC */
#  else /* COMPATU_INCLUDE_SOURCE */
/* we are not including source, so everything is extern */
#      define COMPATU_API extern
#  endif /* COMPATU_INCLUDE_SOURCE */
#endif /* COMPATU_PREFIX */


#define COMPATU_CONCAT_HELPER(a, b) a##b
#define COMPATU_CONCAT(a, b) COMPATU_CONCAT_HELPER(a, b)

#define luaL_ref COMPATU_CONCAT(COMPATU_PREFIX, L_ref)
COMPATU_API int luaL_ref(lua_State *L, int t);

namespace sol::detail::compat::luau {

inline bool is_valid_bytecode_version(uint8_t version) {
    return (version >= LBC_VERSION_MIN && version <= LBC_VERSION_MAX);
}

inline bool is_valid_bytecode(std::string_view data) {
    if (data.size() < 3) {
        return false;
    }
    // First byte is the version
    uint8_t version = data[0];
    if (!is_valid_bytecode_version(version)) {
        return false;
    }

    // version 4 adds type info
    if (version >= 4) 
    {
        uint8_t type_version = data[1];
        if (type_version < LBC_TYPE_VERSION_MIN || type_version > LBC_TYPE_VERSION_MAX) {
            return false;
        }
    }

    return true;
}

}

#define SOL_RETURN_LUA_ERROR(...) lua_error(__VA_ARGS__)
#define SOL_RETURN_LUAL_ERROR(L, fmt, ...) luaL_errorL(L, fmt, ##__VA_ARGS__)
#else
#define SOL_RETURN_LUA_ERROR(...) return lua_error(__VA_ARGS__)
#define SOL_RETURN_LUAL_ERROR(...) return luaL_error(__VA_ARGS__)
#endif

#if defined(COMPATU_INCLUDE_SOURCE) && COMPATU_INCLUDE_SOURCE == 1
#  include "compat-luau.c.h"
#endif

#endif // NOT_KEPLER_PROJECT_COMPATLUAU_H_
