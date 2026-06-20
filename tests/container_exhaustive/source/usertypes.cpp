#include <catch2/catch_all.hpp>

#include <sol/sol.hpp>
#include <unordered_map>

TEST_CASE("array") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("make", [] { return std::array { 1, 2, 3, 4 }; });

	sol::optional<sol::error> result = lua.safe_script(R"(
        local a = make()
        assert(#a == 4)
        assert(a:size() == 4)
        assert(not a:empty())

        -- find()
        assert(a:find(2) == 2)
        assert(a:find(3) == 3)
        assert(a:find(5) == nil)
        -- index_of()
        assert(a:index_of(2) == 2)
        assert(a:index_of(3) == 3)
        assert(a:index_of(5) == nil)
        -- __index
        assert(a[0] == nil)
        assert(a[1] == 1)
        assert(a[4] == 4)
        assert(a[5] == nil)
        -- at()
        assert(a:at(0) == nil)
        assert(a:at(1) == 1)
        assert(a:at(4) == 4)
        assert(a:at(5) == nil)
        -- get()
        assert(a:get(0) == nil)
        assert(a:get(1) == 1)
        assert(a:get(4) == 4)
        assert(a:get(5) == nil)

    )"
#if SOL_IS_ON(SOL_USE_LUAU)
	                                                   R"(
        -- __iter
        for k, v in a do
            assert(k == v)
        end
    )"
#else
	                                                   R"(
        -- __pairs
        for k, v in pairs(a) do
            assert(k == v)
        end
        -- __ipairs
        for k, v in ipairs(a) do
            assert(k == v)
        end
    )"
#endif
	                                                   R"(
        -- pairs()
        for k, v in a:pairs() do
            assert(k == v)
        end

        -- __newindex
        a[3] = 9
        assert(a[3] == 9)
        local ok, res = pcall(function()
            a[5] = 1
        end)
        assert(not ok)
        -- set()
        a:set(4, 5)
        assert(a[4] == 5)
        -- clear
        local ok, res = pcall(function()
            a:clear()
        end)
        assert(not ok)
        -- insert
        local ok, res = pcall(function()
            a:insert(2, 8)
        end)
        assert(not ok)
        -- add
        local ok, res = pcall(function()
            a:add(8)
        end)
        assert(not ok)
        -- erase
        local ok, res = pcall(function()
            a:erase(1)
        end)
        assert(not ok)
	)");
	REQUIRE_FALSE(result.has_value());
}

TEST_CASE("vector") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("make", [] { return std::vector { 1, 2, 3, 4 }; });

	sol::optional<sol::error> result = lua.safe_script(R"(
        local a = make()
        assert(#a == 4)
        assert(a:size() == 4)
        assert(not a:empty())

        -- find()
        assert(a:find(2) == 2)
        assert(a:find(3) == 3)
        assert(a:find(5) == nil)
        -- index_of()
        assert(a:index_of(2) == 2)
        assert(a:index_of(3) == 3)
        assert(a:index_of(5) == nil)
        -- __index
        assert(a[0] == nil)
        assert(a[1] == 1)
        assert(a[4] == 4)
        assert(a[5] == nil)
        -- at()
        assert(a:at(0) == nil)
        assert(a:at(1) == 1)
        assert(a:at(4) == 4)
        assert(a:at(5) == nil)
        -- get()
        assert(a:get(0) == nil)
        assert(a:get(1) == 1)
        assert(a:get(4) == 4)
        assert(a:get(5) == nil)

    )"
#if SOL_IS_ON(SOL_USE_LUAU)
	                                                   R"(
        -- __iter
        for k, v in a do
            assert(k == v)
        end
    )"
#else
	                                                   R"(
        -- __pairs
        for k, v in pairs(a) do
            assert(k == v)
        end
        -- __ipairs
        for k, v in ipairs(a) do
            assert(k == v)
        end
    )"
#endif
	                                                   R"(
        -- pairs()
        for k, v in a:pairs() do
            assert(k == v)
        end

        -- __newindex
        a[3] = 9
        assert(a[3] == 9)
        a[5] = 1
        assert(#a == 5)
        assert(a[5] == 1)
        -- set()
        a:set(4, 5)
        assert(a[4] == 5)
        -- insert
        a:insert(4, 4)
        assert(#a == 6)
        assert(a[1] == 1 and
               a[2] == 2 and
               a[3] == 9 and
               a[4] == 4 and
               a[5] == 5 and
               a[6] == 1)
        -- add
        a:add(8)
        assert(#a == 7)
        assert(a[7] == 8)
        -- erase
        a:erase(3)
        assert(#a == 6)
        assert(a[1] == 1 and
               a[2] == 2 and
               a[3] == 4 and
               a[4] == 5 and
               a[5] == 1 and
               a[6] == 8)
        -- clear
        a:clear()
        assert(#a == 0)
        assert(a:empty())
	)");
	REQUIRE_FALSE(result.has_value());
}

TEST_CASE("map") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("make", [] { return std::map<std::string, int> { {"foo", 1}, {"bar", 2}, {"baz", 3}}; });

	sol::optional<sol::error> result = lua.safe_script(R"(
        local a = make()
        assert(#a == 3)
        assert(a:size() == 3)
        assert(not a:empty())

        -- find()
        assert(a:find("foo") == 1)
        assert(a:find("bar") == 2)
        assert(a:find("bbb") == nil)
        -- index_of()
        assert(a:index_of("foo") == 3)
        assert(a:index_of("bar") == 1)
        assert(a:index_of("bbb") == nil)
        -- __index
        assert(a["foo"] == 1)
        assert(a["baz"] == 3)
        assert(a["fff"] == nil)
        -- at()
        assert(a:at(0) == nil)
        assert(a:at(1) == 2)
        assert(a:at(2) == 3)
        assert(a:at(4) == nil)
        -- get()
        assert(a:get("foo") == 1)
        assert(a:get("baz") == 3)
        assert(a:get("bbb") == nil)
    )"
#if SOL_IS_ON(SOL_USE_LUAU)
	                                                   R"(
        -- __iter
        local order = ""
        for k, v in a do
            assert(({foo=1,bar=2,baz=3})[k] == v)
            order = order .. k .. ","
        end
        assert(order == "bar,baz,foo,")
    )"
#else
	                                                   R"(
        -- __pairs
        local order = ""
        for k, v in pairs(a) do
            assert(({foo=1,bar=2,baz=3})[k] == v)
            order = order .. k .. ","
        end
        assert(order == "bar,baz,foo,")
        -- __ipairs (not supported since 5.4)
    )"
#endif
	                                                   R"(
        -- pairs()
        for k, v in a:pairs() do
            assert(({foo=1,bar=2,baz=3})[k] == v)
        end

        -- __newindex
        a["qox"] = 4
        assert(a["qox"] == 4)
        a["bar"] = 1
        assert(#a == 4)
        assert(a["bar"] == 1)
        -- set()
        a:set("baz", 5)
        assert(a["baz"] == 5)
        -- insert
        -- a:insert(3, "aaa", 4) -- FIXME: this is broken!
        assert(#a == 4)
        assert(a["foo"] == 1 and
               a["bar"] == 1 and
               a["baz"] == 5 and
               a["qox"] == 4)
        -- add
        a:add("bbb", 6)
        assert(#a == 5)
        assert(a["bbb"] == 6)
        -- erase
        a:erase("bbb")
        assert(#a == 4)
        assert(a["foo"] == 1 and
               a["bar"] == 1 and
               a["baz"] == 5 and
               a["qox"] == 4)
        -- clear
        a:clear()
        assert(#a == 0)
        assert(a:empty())
	)");
	REQUIRE_FALSE(result.has_value());
}

TEST_CASE("unordered_map") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.set_function("make", [] { return std::unordered_map<std::string, int> { { "foo", 1 }, { "bar", 2 }, { "baz", 3 } }; });

	sol::optional<sol::error> result = lua.safe_script(R"(
        local a = make()
        assert(#a == 3)
        assert(a:size() == 3)
        assert(not a:empty())

        -- find()
        assert(a:find("foo") == 1)
        assert(a:find("bar") == 2)
        assert(a:find("bbb") == nil)
        -- index_of()
        local ok, res = pcall(function()
            a:index_of("foo")
        end)
        assert(not ok)
        -- __index
        assert(a["foo"] == 1)
        assert(a["baz"] == 3)
        assert(a["fff"] == nil)
        -- at()
        -- unordered - we don't know
        assert(a:at(1) == 1 or a:at(1) == 2 or a:at(1) == 3)
        assert(a:at(4) == nil)
        -- get()
        assert(a:get("foo") == 1)
        assert(a:get("baz") == 3)
        assert(a:get("bbb") == nil)

    )"
#if SOL_IS_ON(SOL_USE_LUAU)
	                                                   R"(
        -- __iter
        for k, v in a do
            assert(({foo=1,bar=2,baz=3})[k] == v)
        end
    )"
#else
	                                                   R"(
        -- __pairs
        for k, v in pairs(a) do
            assert(({foo=1,bar=2,baz=3})[k] == v)
        end
        -- __ipairs (not supported since 5.4)
    )"
#endif
	                                                   R"(
        -- pairs()
        for k, v in a:pairs() do
            assert(({foo=1,bar=2,baz=3})[k] == v)
        end

        -- __newindex
        a["qox"] = 4
        assert(a["qox"] == 4)
        a["bar"] = 1
        assert(#a == 4)
        assert(a["bar"] == 1)
        -- set()
        a:set("baz", 5)
        assert(a["baz"] == 5)
        -- insert
        -- a:insert(3, "aaa", 4) -- FIXME: this is broken!
        assert(#a == 4)
        assert(a["foo"] == 1 and
               a["bar"] == 1 and
               a["baz"] == 5 and
               a["qox"] == 4)
        -- add
        a:add("bbb", 6)
        assert(#a == 5)
        assert(a["bbb"] == 6)
        -- erase
        a:erase("bbb")
        assert(#a == 4)
        assert(a["foo"] == 1 and
               a["bar"] == 1 and
               a["baz"] == 5 and
               a["qox"] == 4)
        -- clear
        a:clear()
        assert(#a == 0)
        assert(a:empty())
	)");
	REQUIRE_FALSE(result.has_value());
}
