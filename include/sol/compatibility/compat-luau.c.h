#ifndef KEPLER_PROJECT_COMPATU_C_
#define KEPLER_PROJECT_COMPATU_C_

#include <sol/compatibility/compat-5.3.h>
#include <sol/compatibility/compat-luau.h>

#if SOL_IS_ON(SOL_USE_LUAU)

#include <stddef.h>

COMPATU_API int luaL_ref(lua_State* L, int t) {
	int ref;
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);     /* remove from stack */
		return LUA_REFNIL; /* 'nil' has a unique fixed reference */
	}
	t = lua_absindex(L, t);
	if (lua_rawgeti(L, t, 1) == LUA_TNUMBER) { /* already initialized? */
		ref = (int)lua_tointeger(L, -1);      /* ref = t[1] */
	}
	else {                      /* first access */
		ref = 0;               /* list is empty */
		lua_pushinteger(L, 0); /* initialize as an empty list */
		lua_rawseti(L, t, 1);  /* ref = t[1] = 0 */
	}
	lua_pop(L, 1);               /* remove element from stack */
	if (ref != 0) {              /* any free element? */
		lua_rawgeti(L, t, ref); /* remove it from list */
		lua_rawseti(L, t, 1);   /* (t[1] = t[ref]) */
	}
	else {                                /* no free elements */
		ref = (int)lua_rawlen(L, t) + 1; /* get a new reference */
	}
	lua_rawseti(L, t, ref);
	return ref;
}

#endif

#endif /* KEPLER_PROJECT_COMPATU_C_ */


/*********************************************************************
 * This file contains parts of Lua 5.2's and Lua 5.3's source code:
 *
 * Copyright (C) 1994-2014 Lua.org, PUC-Rio.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *********************************************************************/
