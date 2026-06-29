// sol2

// The MIT License (MIT)

// Copyright (c) 2013-2026 Rapptz, ThePhD and contributors

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "sol_test.hpp"

#include <catch2/catch_all.hpp>

#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
#define SOL_TEST_HAS_SPAN
#include <span>
#endif

#if SOL_IS_ON(SOL_USE_LUAU)

TEST_CASE("luau/buffer", "get values in and out as a buffer") {
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::buffer);

	lua["mybuf"] = sol::copy_buffer(std::string_view("my buffer"));
	auto ret = lua.safe_script(R"(
        assert(buffer.readstring(mybuf, 0, buffer.len(mybuf)) == "my buffer")
        luabuf = buffer.fromstring("in lua")
        return luabuf
    )");
	auto luabuf = ret.get<sol::luau::buffer_view>();
	REQUIRE(luabuf.string_view() == "in lua");
	char* s = static_cast<char*>(luabuf.data());
	s[3] = 'c';
	s[4] = '+';
	s[5] = '+';
	lua.safe_script(R"(assert(buffer.readstring(luabuf, 0, 6) == "in c++"))");

	auto sv = ret.get<sol::as_buffer_t<std::string_view>>();
	REQUIRE(sv == "in c++");

#ifdef SOL_TEST_HAS_SPAN
	auto sp = ret.get<sol::as_buffer_t<std::span<char>>>();
	REQUIRE(std::string_view(sp.data(), sp.size()) == "in c++");

	auto sp2 = ret.get<sol::as_buffer_t<std::span<unsigned char>>>();
	REQUIRE(std::string_view(reinterpret_cast<const char*>(sp2.data()), sp2.size()) == "in c++");
#endif
}

TEST_CASE("luau/vector", "get values in and out as vectors") {
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::vector);

#if LUA_VECTOR_SIZE == 3
	lua["myvec"] = sol::luau::vector(1, 2, 3);
	auto ret = lua.safe_script(R"(
        assert(myvec == vector.create(1, 2, 3))
        return vector.create(4, 5, 6)
    )");
	REQUIRE(ret.get<sol::luau::vector>() == sol::luau::vector(4, 5, 6));
#else
	lua["myvec"] = sol::luau::vector(1, 2, 3, 4);
	auto ret = lua.safe_script(R"(
        assert(myvec == vector.create(1, 2, 3, 4))
        return vector.create(5, 6, 7, 8)
    )");
	REQUIRE(ret.get<sol::luau::vector>() == sol::luau::vector(5, 6, 7, 8));
#endif
}

#endif
