#include <print>
#include <sol/sol.hpp>

// why exactly did i write this again?
// Sleep deprevation does some questionable things.
namespace Seperator {
    #define DEFAULT_SEPERATOR "----------------"
    namespace details {
        static size_t gDepth = 0;

        class BaseInit {
        public:
            BaseInit(const std::string& name, const std::string& seperator)
                : mSeperator(seperator), mName(name), mDepth(std::string(gDepth++, '\t')) {
                if (mName.empty()) {
                    std::println("{}{} BEGIN {}", mDepth, mSeperator, mSeperator);
                }
                else {
                    std::println("{}{} BEGIN OF '{}' {}", mDepth, mSeperator, mName, mSeperator);
                }
            }
            ~BaseInit() {
                gDepth--;
                if (mName.empty()) {
                    std::println("{}{} END {}", mDepth, mSeperator, mSeperator);
                }
                else {
                    std::println("{}{} END OF '{}' {}", mDepth, mSeperator, mName, mSeperator);
                }
                std::println();
            }
        protected:
            std::string mDepth;
            std::string mSeperator;
            std::string mName;
        };
    }

    class Init : public details::BaseInit {
    public:
        Init() :
            details::BaseInit("", DEFAULT_SEPERATOR) {}

        Init(const std::string& Name, const std::string& Seperator = DEFAULT_SEPERATOR)
            : details::BaseInit(Name, Seperator) {}
    };
#undef DEFAULT_SEPERATOR
}

struct vars {
    int boop = 0;
};

void test_a() {
    sol::state lua;
    lua.open_libraries(sol::lib::base);
    lua.new_usertype<vars>("vars", "boop", &vars::boop);

    lua.script("beep = vars.new(); beep.boop = 1");
    assert(lua.get<vars>("beep").boop == 1);

    printf("beep.boop: %d\n", lua.get<vars>("beep").boop);

    lua.script("print(1, _VERSION)", "main");
}

struct cls {
public:
    cls() {
        std::println("[C] cls::cls()");
    }

    cls(int a, int b, int c) {
        std::println("[C] cls::cls({}, {}, {})", a, b, c);
    }

    int fn(int a, int b, int c) {
        std::println("[C] cls::fn({}, {}, {}) -> {}", a, b, c, (a * b * c) ^ key);
        return (a * b * c) ^ key;
    }

    ~cls() {
        std::println("[C] cls::~cls()");
    }

    int key = 200;
};

static int lua_collectgarbage(lua_State* L)
{
    const char* option = luaL_optstring(L, 1, "collect");

    if (strcmp(option, "collect") == 0)
    {
        lua_gc(L, LUA_GCCOLLECT, 0);
        return 0;
    }

    if (strcmp(option, "count") == 0)
    {
        int c = lua_gc(L, LUA_GCCOUNT, 0);
        lua_pushnumber(L, c);
        return 1;
    }

    luaL_error(L, "collectgarbage must be called with 'count' or 'collect'");
}

void test_b() {
    sol::state L;
    L.open_libraries(sol::lib::base);
    L["collectgarbage"] = lua_collectgarbage;
    L.new_usertype<cls>("cls",
        sol::constructors<cls(), cls(int, int, int)>(),
        "key", &cls::key,
        "fn", &cls::fn
    );

    L.script("print('[L] cls:', cls); o = cls.new(1, 2, 3);");
    auto& o = L.get<cls>("o");
    printf("[C] o: %p\n", &o);
    L.script("print('[L] fn with default key:', o:fn(1,2,3))");
    o.key = 1;
    L.script("print('[L] fn with different key:', o:fn(1,2,3))");
    L.script("print('[L] Setting o -> nil'); o = nil");
    L.script("print('[L] collecting garbage');");
    L.script("collectgarbage('collect');");
    printf("%d\n", L.is_gc_on());
    for (int i = 0; i < 3; i++) {
        printf("waiting!\n");
        _sleep(1000);
    }
    L.script("print('[L] guH!') collectgarbage('collect');");
    L.script("print('[L] cls:', cls); o2 = cls.new(1, 2, 3);");
    auto& o2 = L.get<cls>("o2");
    printf("[C] o2: %p\n", &o2);

    _sleep(1000);
}

void test_c() {
    sol::state lua;
    int x = 0;
    lua.set_function("beep", [&x]{ ++x; });
    lua.script("beep()");
    assert(x == 1);
}

int main() {
	std::println("[BEGIN] Luau Sol Test!");
    {
        Seperator::Init _("Test A");

        test_a();
    }
    
    {
        Seperator::Init _("Test B");

        test_b();
    }

    {
        Seperator::Init _("Test C");

        test_c();
    }

	std::println("[END] Luau Sol Test!");
}