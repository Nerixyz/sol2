#ifndef KEPLER_PROJECT_COMPATLUAU_H_
#define KEPLER_PROJECT_COMPATLUAU_H_
#if SOL_IS_ON(SOL_USE_LUAU) 
#include <lua.h>
#include <lualib.h>
#include <lobject.h>
#include <lstate.h>
#include <luacode.h>
#include <lbytecode.h>
#include <lmem.h>

#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <vector>

#define LUA_QL(x)				 "'" x "'"
#define LUA_QS					 LUA_QL("%s")
#define LUA_ERRFILE				 (LUA_ERRERR + 1)
#define COMPAT53_INCLUDE_SOURCE  1
#define LUA_FILEHANDLE           "FILE*"

#define LUAU_STATE_IS_CLOSING (void*)0xBEEFFEED
#define LUAU_VALIDATE_CHUNK_SIZE 2
#define LUAU_USERDATA_GC_TAG	 LUA_UTAG_LIMIT - 1

#ifdef lua_pushcclosure 
	#undef lua_pushcclosure
#endif

#ifdef lua_pushcfunction
	#undef lua_pushcfunction 
#endif

#define lua_pushcfunction(L, fn) lua_pushcclosurek(L, fn, NULL, 0, NULL)
#define lua_pushcclosure(L, fn, nup) lua_pushcclosurek(L, fn, NULL, nup, NULL)

#ifndef api_checknelems
#define api_checknelems(L, n) api_check(L, (n) <= (L->top - L->base))
#endif

using lua_Writer = int (*)(lua_State* L, const void* Value, size_t Size, void* Userdata);
using lua_Reader = const char* (*)(lua_State* L, const void* Data, size_t* Size);

namespace LuauCompat {
	// Early Luau versions called it Table, newer ones call it LuaTable.
	using luaTable = decltype(std::declval<GCObject>().h);

	namespace details {
		template<typename T>
		static T read(const char* data, size_t size, size_t& offset) {
			if (offset + sizeof(T) > size)
				throw std::out_of_range("Attempted to read past the end of buffer.");

			T result;
			memcpy(&result, data + offset, sizeof(T));
			offset += sizeof(T);

			return result;
		}

		static bool isValidBytecode(const char* data, size_t size) {
			try {
				size_t offset = 0;

				uint8_t version = read<uint8_t>(data, size, offset);
				if (version == 0 || version < LBC_VERSION_MIN || version > LBC_VERSION_MAX)
					return false;

				if (version >= 4) {
					uint8_t typesversion = read<uint8_t>(data, size, offset);
					if (typesversion < LBC_TYPE_VERSION_MIN || typesversion > LBC_TYPE_VERSION_MAX)
						return false;
				}

				return true;
			} catch (const std::out_of_range&) {
				return false;
			}
		}

        namespace Unload {
            namespace details {
                // Base class for handling bytecode serialization state
                class StateBase {
                public:
                    StateBase(lua_State* L, const Proto* proto) : mL(L), mProto(proto) {}
                    virtual ~StateBase() = default;

                    // Non-copyable, non-movable
                    StateBase(const StateBase&) = delete;
                    StateBase& operator=(const StateBase&) = delete;
                    StateBase(StateBase&&) = delete;
                    StateBase& operator=(StateBase&&) = delete;

                    // Template method for writing trivially copyable types
                    template<typename T>
                    void write(T value) {
                        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
                        write(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
                    }

                    // Write variable-length integers using LEB128 encoding
                    void writeVarInt(uint32_t value) {
                        do {
                            uint8_t byte = value & 0x7F;
                            value >>= 7;
                            if (value != 0) {
                                byte |= 0x80;
                            }
                            write(&byte, 1);
                        } while (value != 0);
                    }

                    // Pure virtual methods implemented by derived classes
                    virtual void write(const uint8_t* data, size_t size) = 0;
                    virtual std::vector<uint8_t> data() const { return {}; }

                    // String table management
                    void collectString(const TString* str) {
                        if (str && mStringTable.ids.find(str) == mStringTable.ids.end()) {
                            mStringTable.ids[str] = static_cast<uint32_t>(mStringTable.strings.size() + 1);
                            mStringTable.strings.push_back(str);
                        }
                    }

                    void collectProto(const Proto* proto) {
                        if (proto) {
                            mProtos.push_back(proto);
                        }
                    }

                    void writeString(const TString* str) {
                        if (auto it = mStringTable.ids.find(str); it != mStringTable.ids.end()) {
                            writeVarInt(it->second);
                        }
                        else {
                            writeVarInt(0); // Handle missing string gracefully
                        }
                    }

                    // Accessors
                    const Proto* proto() const { return mProto; }
                    auto& imports() { return mImports; }
                    auto& protos() { return mProtos; }
                    auto& stringIds() { return mStringTable.ids; }
                    auto& stringArray() { return mStringTable.strings; }

                    uint8_t getVersion() const { return mVersion; }
                    uint8_t getTypesVersion() const { return mTypesVersion; }
                    int getStatus() const { return mStatus; }
                    int getStrip() const { return mStrip; }

                    // Mutators
                    void setVersion(uint8_t version) { mVersion = version; }
                    void setTypesVersion(uint8_t version) { mTypesVersion = version; }
                    void setStrip(int strip) { mStrip = strip; }

                protected:
                    lua_State* mL = nullptr;
                    const Proto* mProto = nullptr;
                    int mStatus = 0;
                    int mStrip = 0;

                private:
                    uint8_t mVersion = LBC_VERSION_TARGET;
                    uint8_t mTypesVersion = 0;

                    struct StringTable {
                        std::unordered_map<const TString*, uint32_t> ids;
                        std::vector<const TString*> strings;
                    } mStringTable;

                    std::unordered_map<const Proto*, std::vector<uint32_t>> mImports;
                    std::vector<const Proto*> mProtos;
                };

                // Implementation for writing to a vector buffer
                class StateVector final : public StateBase {
                public:
                    StateVector(lua_State* L, const Proto* proto, std::vector<uint8_t>* buffer)
                        : StateBase(L, proto), mBuffer(buffer) {}

                    void write(const uint8_t* data, size_t size) override {
                        if (mBuffer && data) {
                            mBuffer->insert(mBuffer->end(), data, data + size);
                        }
                    }

                    std::vector<uint8_t> data() const override {
                        return mBuffer ? *mBuffer : std::vector<uint8_t>{};
                    }

                private:
                    std::vector<uint8_t>* mBuffer;
                };

                // Implementation for writing using lua_Writer callback
                class StateWriter final : public StateBase {
                public:
                    StateWriter(lua_State* L, const Proto* proto, lua_Writer writer, void* userdata)
                        : StateBase(L, proto), mWriter(writer), mUserdata(userdata) {}

                    void write(const uint8_t* data, size_t size) override {
                        if (mWriter) {
                            mStatus = mWriter(mL, data, size, mUserdata);
                        }
                    }

                private:
                    lua_Writer mWriter = nullptr;
                    void* mUserdata = nullptr;
                };

                // Utility functions
                static constexpr int getOpcodeLength(LuauOpcode op) {
                    switch (op) {
                    case LOP_GETGLOBAL:
                    case LOP_SETGLOBAL:
                    case LOP_GETIMPORT:
                    case LOP_GETTABLEKS:
                    case LOP_SETTABLEKS:
                    case LOP_NAMECALL:
                    case LOP_JUMPIFEQ:
                    case LOP_JUMPIFLE:
                    case LOP_JUMPIFLT:
                    case LOP_JUMPIFNOTEQ:
                    case LOP_JUMPIFNOTLE:
                    case LOP_JUMPIFNOTLT:
                    case LOP_NEWTABLE:
                    case LOP_SETLIST:
                    case LOP_FORGLOOP:
                    case LOP_LOADKX:
                    case LOP_FASTCALL2:
                    case LOP_FASTCALL2K:
                    case LOP_FASTCALL3:
                    case LOP_JUMPXEQKNIL:
                    case LOP_JUMPXEQKB:
                    case LOP_JUMPXEQKN:
                    case LOP_JUMPXEQKS:
                        return 2;
                    default:
                        return 1;
                    }
                }

                constexpr uint8_t convertConstantType(const TValue* value) {
                    if (!value) return LBC_CONSTANT_NIL;

                    switch (ttype(value)) {
                    case LUA_TNIL: return LBC_CONSTANT_NIL;
                    case LUA_TBOOLEAN: return LBC_CONSTANT_BOOLEAN;
                    case LUA_TNUMBER: return LBC_CONSTANT_NUMBER;
                    case LUA_TVECTOR: return LBC_CONSTANT_VECTOR;
                    case LUA_TSTRING: return LBC_CONSTANT_STRING;
                    case LUA_TTABLE: return LBC_CONSTANT_TABLE;
                    case LUA_TFUNCTION: return LBC_CONSTANT_CLOSURE;
                    default: return LBC_CONSTANT_IMPORT;
                    }
                }

                static uint32_t resolveImport(StateBase& state, size_t importIndex, const Proto* proto) {
                    auto& importList = state.imports()[proto];
                    if (importIndex >= importList.size()) {
                        return 0; // Invalid import
                    }
                    return importList[importIndex];
                }

                static uint32_t getConstantKeyIndex(const Proto* proto, const TKey* key, const luaTable* table) {
                    if (!proto || !key || !table) return 0;

                    for (int i = 0; i < proto->sizek; ++i) {
                        const TValue* value = &proto->k[i];
                        if (luaO_rawequalKey(key, value)) {
                            return static_cast<uint32_t>(i);
                        }
                    }
                    return 0;
                }

                // Collection phase - gather all strings, protos, and imports
                static void collectStrings(StateBase& state, const Proto* proto = nullptr) {
                    proto = proto ? proto : state.proto();
                    if (!proto) return;

                    // Collect string constants
                    for (int i = 0; i < proto->sizek; ++i) {
                        const TValue* value = &proto->k[i];
                        if (ttisstring(value)) {
                            state.collectString(tsvalue(value));
                        }
                    }

                    // Collect debug information strings
                    if (proto->debugname) {
                        state.collectString(proto->debugname);
                    }

                    // Collect local variable names
                    for (int i = 0; i < proto->sizelocvars; ++i) {
                        state.collectString(proto->locvars[i].varname);
                    }

                    // Collect upvalue names
                    for (int i = 0; i < proto->sizeupvalues; ++i) {
                        state.collectString(proto->upvalues[i]);
                    }

                    // Recursively process nested prototypes
                    for (int i = 0; i < proto->sizep; ++i) {
                        collectStrings(state, proto->p[i]);
                    }
                }

                static void collectProtos(StateBase& state, const Proto* proto = nullptr) {
                    proto = proto ? proto : state.proto();
                    if (!proto) return;

                    state.collectProto(proto);

                    for (int i = 0; i < proto->sizep; ++i) {
                        collectProtos(state, proto->p[i]);
                    }
                }

                static void collectImports(StateBase& state, const Proto* proto = nullptr) {
                    proto = proto ? proto : state.proto();
                    if (!proto) return;

                    auto& importList = state.imports()[proto];

                    const uint32_t* instruction = proto->code;
                    const uint32_t* codeEnd = proto->code + proto->sizecode;

                    while (instruction < codeEnd) {
                        LuauOpcode opcode = static_cast<LuauOpcode>(LUAU_INSN_OP(*instruction));

                        if (opcode == LOP_GETIMPORT) {
                            const uint32_t* auxData = instruction + 1;
                            if (auxData < codeEnd) {
                                importList.push_back(*auxData);
                            }
                        }

                        instruction += getOpcodeLength(opcode);
                    }

                    for (int i = 0; i < proto->sizep; ++i) {
                        collectImports(state, proto->p[i]);
                    }
                }

                // Serialization functions
                static void dumpHeader(StateBase& state) {
                    state.write<uint8_t>(state.getVersion());

                    if (state.getVersion() >= 4) {
                        state.setTypesVersion(LBC_TYPE_VERSION_TARGET);
                        state.write<uint8_t>(LBC_TYPE_VERSION_TARGET);
                    }

                    // Write string table
                    const auto& stringArray = state.stringArray();
                    state.writeVarInt(static_cast<uint32_t>(stringArray.size()));

                    for (const auto* str : stringArray) {
                        if (str) {
                            state.writeVarInt(str->len);
                            state.write(reinterpret_cast<const uint8_t*>(getstr(str)), str->len);
                        }
                        else {
                            state.writeVarInt(0);
                        }
                    }

                    if (state.getTypesVersion() == 3) {
                        state.write<uint8_t>(0x0); // userdataRemapping terminator, not restorable afaik.
                    }

                    state.writeVarInt(static_cast<uint32_t>(state.protos().size()));
                }

                static void dumpConstant(StateBase& state, const TValue* value, const Proto* proto) {
                    if (!value || !proto) return;

                    uint8_t constType = convertConstantType(value);
                    state.write<uint8_t>(constType);

                    static size_t importCount = 0;

                    switch (constType) {
                    case LBC_CONSTANT_NIL:
                        break;

                    case LBC_CONSTANT_BOOLEAN:
                        state.write(static_cast<uint8_t>(bvalue(value)));
                        break;

                    case LBC_CONSTANT_NUMBER:
                        state.write<double>(nvalue(value));
                        break;

                    case LBC_CONSTANT_VECTOR: {
                        const float* v = vvalue(value);
                        state.write(v[0]);
                        state.write(v[1]);
                        state.write(v[2]);
                        float w = (sizeof(value->extra) == 2) ? v[3] : 0.0f;
                        state.write(w);
                        break;
                    }

                    case LBC_CONSTANT_STRING:
                        state.writeString(tsvalue(value));
                        break;

                    case LBC_CONSTANT_TABLE: {
                        const luaTable* table = hvalue(value);
                        int nodeCount = sizenode(table);
                        state.writeVarInt(nodeCount);

                        for (int i = 0; i < nodeCount; ++i) {
                            const LuaNode& node = table->node[i];
                            if (node.key.tt != LUA_TNIL) {
                                state.writeVarInt(getConstantKeyIndex(proto, &node.key, table));
                            }
                        }
                        break;
                    }

                    case LBC_CONSTANT_CLOSURE: {
                        const Closure* closure = clvalue(value);
                        state.writeVarInt(closure->l.p->bytecodeid);
                        break;
                    }

                    case LBC_CONSTANT_IMPORT:
                        state.write(resolveImport(state, importCount++, proto));
                        break;
                    }
                }

                static void dumpTypeInfo(StateBase& state, const Proto* proto) {
                    if (state.getTypesVersion() == 1) {
                        if (proto->typeinfo) {
                            uint32_t headerSize = (proto->typeinfo[0] & 0x80) ? 4 : 3;
                            uint32_t typeSize = proto->sizetypeinfo - headerSize;
                            if (typeSize > 0) {
                                state.write(proto->typeinfo + headerSize, typeSize);
                            }
                        }
                    }
                    else if (state.getTypesVersion() == 2 || state.getTypesVersion() == 3) {
                        state.writeVarInt(proto->sizetypeinfo);
                        if (proto->sizetypeinfo > 0 && proto->typeinfo) {
                            state.write(proto->typeinfo, proto->sizetypeinfo);
                        }
                    }
                }

                static void dumpLineInfo(StateBase& state, const Proto* proto) {
                    if (proto->sizelineinfo > 0 && !state.getStrip()) {
                        state.write<uint8_t>(1);
                        state.write(static_cast<uint8_t>(proto->linegaplog2));

                        // Write line deltas
                        for (int i = 0; i < proto->sizecode; ++i) {
                            uint8_t lineInfo = (i == 0) ? proto->lineinfo[i] :
                                (proto->lineinfo[i] - proto->lineinfo[i - 1]);
                            state.write<uint8_t>(lineInfo);
                        }

                        // Write absolute line info
                        int intervals = ((proto->sizecode - 1) >> proto->linegaplog2) + 1;
                        for (int i = 0; i < intervals; ++i) {
                            uint32_t absLineInfo = (i == 0) ? proto->abslineinfo[i] :
                                (proto->abslineinfo[i] - proto->abslineinfo[i - 1]);
                            state.write<uint32_t>(absLineInfo);
                        }
                    }
                    else {
                        state.write<uint8_t>(0);
                    }
                }

                static void dumpVariableInfo(StateBase& state, const Proto* proto) {
                    if ((proto->sizelocvars > 0 || proto->sizeupvalues > 0) && !state.getStrip()) {
                        state.write<uint8_t>(1);

                        // Dump local variables
                        state.writeVarInt(proto->sizelocvars);
                        for (int i = 0; i < proto->sizelocvars; ++i) {
                            state.writeString(proto->locvars[i].varname);
                            state.writeVarInt(proto->locvars[i].startpc);
                            state.writeVarInt(proto->locvars[i].endpc);
                            state.write<uint8_t>(proto->locvars[i].reg);
                        }

                        // Dump upvalues
                        state.writeVarInt(proto->sizeupvalues);
                        for (int i = 0; i < proto->sizeupvalues; ++i) {
                            state.writeString(proto->upvalues[i]);
                        }
                    }
                    else {
                        state.write<uint8_t>(0);
                    }
                }

                static void dumpFunction(StateBase& state, const TString* source, const Proto* proto) {
                    (void)source;
                    if (!proto) return;

                    // Write function header
                    state.write<uint8_t>(proto->maxstacksize);
                    state.write<uint8_t>(proto->numparams);
                    state.write<uint8_t>(proto->nups);
                    state.write<uint8_t>(proto->is_vararg);

                    // Write version-specific data
                    if (state.getVersion() >= 4) {
                        state.write<uint8_t>(proto->flags);
                        dumpTypeInfo(state, proto);
                    }

                    // Write bytecode
                    state.writeVarInt(proto->sizecode);
                    for (int i = 0; i < proto->sizecode; ++i) {
                        state.write<uint32_t>(proto->code[i]);
                    }

                    // Write constants
                    state.writeVarInt(proto->sizek);
                    for (int i = 0; i < proto->sizek; ++i) {
                        dumpConstant(state, &proto->k[i], proto);
                    }

                    // Write nested function references
                    state.writeVarInt(proto->sizep);
                    for (int i = 0; i < proto->sizep; ++i) {
                        state.writeVarInt(proto->p[i]->bytecodeid);
                    }

                    // Write debug information
                    state.writeVarInt(proto->linedefined);
                    if (proto->debugname && !state.getStrip()) {
                        state.writeString(proto->debugname);
                    }
                    else {
                        state.writeVarInt(0);
                    }

                    dumpLineInfo(state, proto);
                    dumpVariableInfo(state, proto);
                }

                // Main dump function
                static int dump(StateBase& state) {
                    try {
                        // Collection phase
                        collectStrings(state);
                        collectProtos(state);
                        collectImports(state);

                        // Sort prototypes by bytecode ID for consistent output
                        auto& protos = state.protos();
                        std::sort(protos.begin(), protos.end(),
                            [](const Proto* a, const Proto* b) {
                                return a && b && a->bytecodeid < b->bytecodeid;
                            });

                        // Serialization phase
                        dumpHeader(state);

                        for (const Proto* proto : protos) {
                            if (proto) {
                                dumpFunction(state, proto->source, proto);
                            }
                        }

                        // Write main function ID
                        if (state.proto()) {
                            state.writeVarInt(state.proto()->bytecodeid);
                        }

                        return state.getStatus();
                    }
                    catch (...) {
                        // This should never happen, but better be safe than sorry
                        return -1;
                    }
                }
            } // namespace details

            template<bool UseWriter>
            using State = typename std::conditional<UseWriter, details::StateWriter, details::StateVector>::type;

            static int dumpWithWriter(lua_State* L, const Proto* proto, lua_Writer writer, void* data, int strip) {
                if (!L || !proto || !writer) return -1;

                State<true> state{ L, proto, writer, data };
                state.setStrip(strip);
                return details::dump(state);
            }

            static int dumpWithVector(lua_State* L, const Proto* proto, std::vector<uint8_t>& data, int strip) {
                if (!L || !proto) return -1;

                State<false> state{ L, proto, &data };
                state.setStrip(strip);
                return details::dump(state);
            }
        } // namespace Dumper
	}

    namespace garbageCollection {
        // We need to undefine checkliveness, since userdata's might already be marked as dead by luau.
#pragma push_macro("checkliveness")
#define checkliveness(g, obj)

        static void invokeDestructor(lua_State* L, luaTable* table) {
            auto* metatablePointer = table->metatable;
            if (!metatablePointer) {
                return;
            }

            // push metatable onto the stack
            sethvalue(L, L->top++, metatablePointer);

            // push the __gc method onto the stack
            int __gc_type = lua_rawgetfield(L, -1, "__gc");

            if (__gc_type != LUA_TFUNCTION) {
                // Abort! __gc method doesnt exist.
                lua_pop(L, 2);
                return;
            }

            // push table onto the stack
            sethvalue(L, L->top++, table);

            // __gc(table)
            int callResult = lua_pcall(L, 1, 0, 0);

            if (callResult != LUA_OK) {
                // -1: errorMsg, -2: metatable;
                const char* err = lua_tostring(L, -1);
                fprintf(stderr, "Error calling __gc: %s\n", err);

                // pop errorMsg from the stack
                lua_pop(L, 1);
            }

            // pop metatable from the stack
            lua_pop(L, 1);
        }

        static void invokeDestructor(lua_State* L, Udata* userdata) {
            auto* metatablePointer = userdata->metatable;
            if (!metatablePointer) {
                return;
            }

            // push metatable onto the stack
            sethvalue(L, L->top++, metatablePointer);

            // push the __gc method onto the stack
            int __gc_type = lua_rawgetfield(L, -1, "__gc");

            if (__gc_type != LUA_TFUNCTION) {
                // Abort! __gc method doesnt exist.
                lua_pop(L, 2);
                return;
            }

            // push userdata onto the stack
            setuvalue(L, L->top++, userdata);

            // __gc(userdata)
            int callResult = lua_pcall(L, 1, 0, 0);

            if (callResult != LUA_OK) {
                // -1: errorMsg, -2: metatable;
                const char* err = lua_tostring(L, -1);
                fprintf(stderr, "Error calling __gc: %s\n", err);

                // pop errorMsg from the stack
                lua_pop(L, 1);
            }

            // pop metatable from the stack
            lua_pop(L, 1);
        }

        static void invokeDestructor(lua_State* L, GCObject* o, lua_Page* page) {
            switch (o->gch.tt)
            {
            case LUA_TTABLE:
                invokeDestructor(L, gco2h(o));
                break;
            case LUA_TUSERDATA:
                invokeDestructor(L, gco2u(o));
                break;
            }
        }

        static bool invokeDestructors(void* context, lua_Page* page, GCObject* gco) {
            lua_State* L = (lua_State*)context;
            invokeDestructor(L, gco, page);
            return true;
        }

        // This will be called before a state is freed.
        // This only gets called on states that sol2 closes, is that safe? what if sol2 abandons a state, that could cause memory leaks, no?
        static void invoke(lua_State* L) {
            L->userdata = LUAU_STATE_IS_CLOSING;
            luaM_visitgco(L, L, invokeDestructors);
        }

        // Note: Apologies for using the raw LuaC API here.
        // Accessing L during GC traversal is inherently unsafe, but in this case, it's the most stable option.
        // This function is called during internal GC traversal — see luaU_freeudata for context.
        // P.S. Until Luau provides a safer alternative, we'll stick with this approach.
        static void dtor(lua_State* L, void* userdataPointer) {
            if (L->userdata == LUAU_STATE_IS_CLOSING) {
                // All of the destructors have been called by garbageCollection::invoke already.
                return;
            }

            Udata* userdata = reinterpret_cast<Udata*>(
                reinterpret_cast<uint8_t*>(userdataPointer) - offsetof(Udata, data)
            );

            invokeDestructor(L, userdata);
        }

        // restore macros.
#pragma pop_macro("checkliveness")
    };

	static std::string compile(const std::string& source, int optimizationLevel = 1, int debugLevel = 1) {
		lua_CompileOptions opts = {};
		opts.optimizationLevel = optimizationLevel;
		opts.debugLevel = debugLevel;
		opts.typeInfoLevel = 0;
		opts.coverageLevel = 0;

		size_t outSize = 0;
		std::unique_ptr<char, decltype(&free)> bytecodePtr(
			luau_compile(source.data(), source.size(), &opts, &outSize),
			&free
		);

		if (!bytecodePtr) {
			// are we allowed to throw exceptions?
			throw std::runtime_error("Failed to compile Lua source");
		}

		return std::string(bytecodePtr.get(), outSize);
	}
	
    static void panicHandler(lua_State* L, int errcode) {
        (void)errcode;

        lua_Callbacks* callbackState = lua_callbacks(L);

        if (!callbackState || !callbackState->userdata) {
            return;
        }

        lua_CFunction callback = reinterpret_cast<lua_CFunction>(callbackState->userdata);
        callback(L);
    }

}

static lua_CFunction lua_atpanic(lua_State* L, lua_CFunction panicf) {
	lua_Callbacks* callbackState = lua_callbacks(L);

	lua_CFunction old = reinterpret_cast<lua_CFunction>(callbackState->userdata);
	callbackState->userdata = reinterpret_cast<void*>(panicf);
	callbackState->panic = LuauCompat::panicHandler;

	return old;
}

static int lua_dump(lua_State* L, lua_Writer writer, void* data, int strip) {
	int status;
	TValue* o;
	api_checknelems(L, 1);
	o = L->top - 1;
	if (isLfunction(o))
		status = LuauCompat::details::Unload::dumpWithWriter(L, clvalue(o)->l.p, writer, data, strip);
	else
		status = 1;
	return status;
}

static int luaL_ref_compat(lua_State* L, int t) {
	if (t == LUA_REGISTRYINDEX) {
		int ref = lua_ref(L, -1);
		lua_pop(L, 1);
		return ref;
	}

	t = lua_absindex(L, t);

	int ref = LUA_REFNIL;
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return ref;
	}

	lua_rawgeti(L, t, 0);
	ref = static_cast<int>(lua_tointeger(L, -1));

	lua_pop(L, 1);
	if (ref) {
		lua_rawgeti(L, t, ref);
		lua_rawseti(L, t, 0);
	}
	else {
		ref = static_cast<int>(lua_objlen(L, t));
		ref++;
	}
	lua_rawseti(L, t, ref);
	return ref;
}

static void luaL_unref_compat(lua_State* L, int t, int ref) {
	if (ref <= LUA_REFNIL)
		return;

	if (t == LUA_REGISTRYINDEX) {
		lua_unref(L, ref);
	}
	else {
		t = lua_absindex(L, t);
		lua_rawgeti(L, t, 0);
		lua_rawseti(L, t, ref);
		lua_pushinteger(L, ref);
		lua_rawseti(L, t, 0);
	}
}

#undef registry
#undef globals

#define luaL_ref luaL_ref_compat
#define luaL_unref luaL_unref_compat
#endif
#endif KEPLER_PROJECT_COMPATLUAU_H_
