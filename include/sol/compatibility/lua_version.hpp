// sol2

// The MIT License (MIT)

// Copyright (c) 2013-2022 Rapptz, ThePhD and contributors

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

#ifndef SOL_COMPATIBILITY_LUA_VERSION_HPP
#define SOL_COMPATIBILITY_LUA_VERSION_HPP

#include <sol/version.hpp>

// clang-format off

#if SOL_IS_ON(SOL_USE_LUAU) 
	#include <lua.h>
	#include <lualib.h>
	#include <lobject.h>
	#include <lobject.h>
	#include <lstate.h>
	#include <lstring.h>
	#include <lapi.h>
	#include <ltable.h>
	#include <lgc.h>
	#include <lnumutils.h>
	#include <luacode.h>
	#include <lbytecode.h>
	#include <lvm.h>
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

	#define LUAU_STATE_IS_DEAD (void*)0xBEEFFEED
	#define LUAU_VALIDATE_CHUNK_SIZE 2
	#define LUAU_USERDATA_GC_TAG	 LUA_UTAG_LIMIT - 1

	#define SOL_LUA_BIT32_LIB		 1

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

	namespace LuauUtil {
		using luaTable = decltype(std::declval<GCObject>().h);

		namespace details {
			template<typename T>
			static T read(const char* data, size_t size, size_t& offset)
			{
				(void)size;
				T result;
				memcpy(&result, data + offset, sizeof(T));
				offset += sizeof(T);

				return result;
			}

			static bool isValidBytecode(const char* data, size_t size) {
				size_t offset = 0;

				uint8_t version = read<uint8_t>(data, size, offset);
				if (version == 0)
				{
					return false;
				}

				if (version < LBC_VERSION_MIN || version > LBC_VERSION_MAX)
				{
					return false;
				}

				uint8_t typesversion = 0;

				if (version >= 4)
				{
					typesversion = read<uint8_t>(data, size, offset);

					if (typesversion < LBC_TYPE_VERSION_MIN || typesversion > LBC_TYPE_VERSION_MAX)
					{
						return false;
					}
				}

				return true;
			}

			static bool readFileChunk(const std::string& filename, std::string& outBuffer, std::streamsize numBytes, bool binary = true) {
				std::ifstream file;

				std::ios::openmode mode = std::ios::in;
				if (binary) {
					mode |= std::ios::binary;
				}

				file.open(filename.c_str(), mode);
				if (!file) {
					return false;
				}

				outBuffer.resize(static_cast<std::string::size_type>(numBytes));

				file.read(&outBuffer[0], numBytes);
				std::streamsize bytesRead = file.gcount();
				outBuffer.resize(static_cast<std::string::size_type>(bytesRead));

				file.close();

				return bytesRead == numBytes;
			}

			static bool readFile(const std::string& filename, std::string& outBuffer, bool binary = true) {
				std::ifstream file;

				std::ios::openmode mode = std::ios::in;
				if (binary) {
					mode |= std::ios::binary;
				}

				file.open(filename.c_str(), mode);
				if (!file) {
					return false;
				}

				file.seekg(0, std::ios::end);
				std::ifstream::pos_type size = file.tellg();
				if (size < 0) {
					file.close();
					return false;
				}

				outBuffer.resize(static_cast<std::string::size_type>(size));
				file.seekg(0, std::ios::beg);
				file.read(&outBuffer[0], size);
				file.close();

				return true;
			}

			namespace Dumper {
				namespace details {
					class StateBase {
					public:
						StateBase(lua_State* L, const Proto* proto) : mL(L), mProto(proto) {}
						virtual ~StateBase() = default;
						
						// Non-copyable, non-movable
						StateBase(const StateBase&) = delete;
						StateBase& operator=(const StateBase&) = delete;
						StateBase(StateBase&&) = delete;
						StateBase& operator=(StateBase&&) = delete;

						template<typename T>
						void write(T value) {
							static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
							write(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
						}

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

						virtual void write(const uint8_t* data, size_t size) = 0;
						virtual std::vector<uint8_t> data() const { return {}; }

						void collectString(const TString* str) {
							if (str && mStrings.ids.find(str) == mStrings.ids.end()) {
								mStrings.ids[str] = static_cast<uint32_t>(mStrings.arr.size() + 1);
								mStrings.arr.push_back(str);
							}
						}

						void collectProto(const Proto* proto) {
							if (proto) {
								mProtos.push_back(proto);
							}
						}

						void writeString(const TString* str) {
							if (auto it = mStrings.ids.find(str); it != mStrings.ids.end()) {
								writeVarInt(it->second);
							}
							else {
								// Handle missing string gracefully
								writeVarInt(0);
							}
						}

						// Getters
						const Proto* proto() const { return mProto; }
						std::unordered_map<const Proto*, std::vector<uint32_t>>& imports() { return mImports; }
						std::vector<const Proto*>& protos() { return mProtos; }
						std::unordered_map<const TString*, uint32_t>& stringIds() { return mStrings.ids; }
						std::vector<const TString*>& stringArray() { return mStrings.arr; }

						uint8_t getVersion() const { return mVersion; }
						uint8_t getTypesVersion() const { return mTypesVersion; }
						int getStatus() const { return mStatus; }
						int getStrip() const { return mStrip; }

						// Setters
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
							std::vector<const TString*> arr;
						} mStrings;

						std::unordered_map<const Proto*, std::vector<uint32_t>> mImports;
						std::vector<const Proto*> mProtos;
					};

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

					constexpr int getOpcodeLength(LuauOpcode op) {
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
							return 0; // Return default value for invalid import
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

						// Collect debug name
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

						// Recursively collect from nested protos
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
						const uint32_t* insn = proto->code;
						const uint32_t* insnEnd = proto->code + proto->sizecode;

						while (insn < insnEnd) {
							LuauOpcode op = static_cast<LuauOpcode>(LUAU_INSN_OP(*insn));

							if (op == LOP_GETIMPORT && insn + 1 < insnEnd) {
								uint32_t aux = *(insn + 1);
								importList.push_back(aux);
							}

							insn += getOpcodeLength(op);
						}

						for (int i = 0; i < proto->sizep; ++i) {
							collectImports(state, proto->p[i]);
						}
					}

					static void dumpHeader(StateBase& state) {
						state.write<uint8_t>(state.getVersion());

						if (state.getVersion() >= 4) {
							state.setTypesVersion(LBC_TYPE_VERSION_TARGET);
							state.write<uint8_t>(LBC_TYPE_VERSION_TARGET);
						}

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
							// userdataRemapping stops when index is 0
							state.write<uint8_t>(0x0);
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
							// Handle 4-component vectors
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

					static void dumpFunction(StateBase& state, const TString* source, const Proto* proto) {
						(void)source;
						if (!proto) return;

						state.write<uint8_t>(proto->maxstacksize);
						state.write<uint8_t>(proto->numparams);
						state.write<uint8_t>(proto->nups);
						state.write<uint8_t>(proto->is_vararg);

						if (state.getVersion() >= 4) {
							state.write<uint8_t>(proto->flags);

							if (state.getTypesVersion() == 1) {
								if (proto->typeinfo) {
									uint32_t headerSize = (proto->typeinfo[0] & 0x80) ? 4 : 3;
									uint32_t typeSize = proto->sizetypeinfo - headerSize;
									if (typeSize > 0) {
										state.write(proto->typeinfo + headerSize, typeSize);
									}
								}
							} else if (state.getTypesVersion() == 2 || state.getTypesVersion() == 3) {
								state.writeVarInt(proto->sizetypeinfo);
								if (proto->sizetypeinfo > 0 && proto->typeinfo) {
									state.write(proto->typeinfo, proto->sizetypeinfo);
								}
							}
						}

						// Dump bytecode
						state.writeVarInt(proto->sizecode);
						for (int i = 0; i < proto->sizecode; ++i) {
							state.write<uint32_t>(proto->code[i]);
						}

						// Dump constants
						state.writeVarInt(proto->sizek);
						for (int i = 0; i < proto->sizek; ++i) {
							dumpConstant(state, &proto->k[i], proto);
						}

						// Dump nested protos
						state.writeVarInt(proto->sizep);
						for (int i = 0; i < proto->sizep; ++i) {
							state.writeVarInt(proto->p[i]->bytecodeid);
						}

						// Dump debug info
						state.writeVarInt(proto->linedefined);
						if (proto->debugname && !state.getStrip()) {
							state.writeString(proto->debugname);
						} else {
							state.writeVarInt(0);
						}

						// Dump line info
						if (proto->sizelineinfo > 0 && !state.getStrip()) {
							state.write<uint8_t>(1);
							state.write(static_cast<uint8_t>(proto->linegaplog2));

							for (int i = 0; i < proto->sizecode; ++i) {
								uint8_t lineInfo = (i == 0) ? proto->lineinfo[i] :
									(proto->lineinfo[i] - proto->lineinfo[i - 1]);
								state.write<uint8_t>(lineInfo);
							}

							int intervals = ((proto->sizecode - 1) >> proto->linegaplog2) + 1;
							for (int i = 0; i < intervals; ++i) {
								uint32_t absLineInfo = (i == 0) ? proto->abslineinfo[i] :
									(proto->abslineinfo[i] - proto->abslineinfo[i - 1]);
								state.write<uint32_t>(absLineInfo);
							}
						} else {
							state.write<uint8_t>(0);
						}

						// Dump variable info
						if ((proto->sizelocvars > 0 || proto->sizeupvalues > 0) && !state.getStrip()) {
							state.write<uint8_t>(1);

							state.writeVarInt(proto->sizelocvars);
							for (int i = 0; i < proto->sizelocvars; ++i) {
								state.writeString(proto->locvars[i].varname);
								state.writeVarInt(proto->locvars[i].startpc);
								state.writeVarInt(proto->locvars[i].endpc);
								state.write<uint8_t>(proto->locvars[i].reg);
							}

							state.writeVarInt(proto->sizeupvalues);
							for (int i = 0; i < proto->sizeupvalues; ++i) {
								state.writeString(proto->upvalues[i]);
							}
						} else {
							state.write<uint8_t>(0);
						}
					}

					static int dump(StateBase& state) {
						try {
							collectStrings(state);
							collectProtos(state);
							collectImports(state);

							// Sort protos by bytecode ID
							auto& protos = state.protos();
							std::sort(protos.begin(), protos.end(),
								[](const Proto* a, const Proto* b) {
									return a && b && a->bytecodeid < b->bytecodeid;
								});

							dumpHeader(state);

							for (const Proto* proto : protos) {
								if (proto) {
									dumpFunction(state, proto->source, proto);
								}
							}

							if (state.proto()) {
								state.writeVarInt(state.proto()->bytecodeid);
							}

							return state.getStatus();
						}
						catch (...) {
							return -1; // Error occurred
						}
					}
				}

				template<bool UseWriter>
				using State = typename std::conditional<UseWriter, details::StateWriter, details::StateVector>::type;

				static int luaU_dumpWriter(lua_State* L, const Proto* proto, lua_Writer writer, void* data, int strip) {
					if (!L || !proto || !writer) return -1;

					State<true> state{ L, proto, writer, data };
					state.setStrip(strip);
					return details::dump(state);
				}

				static int luaU_dumpVector(lua_State* L, const Proto* proto, std::vector<uint8_t>& data, int strip) {
					if (!L || !proto) return -1;

					State<false> state{ L, proto, &data };
					state.setStrip(strip);
					return details::dump(state);
				}
			}
		}

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

		static bool readCodeFile(const std::string& filename, std::string& outBuffer) {
			std::string validateBuffer;
			if (!details::readFileChunk(filename, validateBuffer, LUAU_VALIDATE_CHUNK_SIZE, true)) {
				return false;
			}

			bool isBinaryFile = details::isValidBytecode(validateBuffer.data(), validateBuffer.size());
			return details::readFile(filename, outBuffer, isBinaryFile);
		}
	}

	static void lua_panicHandler(lua_State* L, int errcode) {
		(void)errcode;
		
		lua_Callbacks* callbackState = lua_callbacks(L);

		if (!callbackState || !callbackState->userdata) {
			return;
		}

		lua_CFunction callback = reinterpret_cast<lua_CFunction>(callbackState->userdata);
		callback(L);
	}

	static lua_CFunction lua_atpanic(lua_State* L, lua_CFunction panicf) {
		lua_Callbacks* callbackState = lua_callbacks(L);

		lua_CFunction old = reinterpret_cast<lua_CFunction>(callbackState->userdata);
		callbackState->userdata = reinterpret_cast<void*>(panicf);
		callbackState->panic = lua_panicHandler;

		return old;
	}

	namespace luaU_garbageCollection {
		// We need to undefine checkliveness, since userdata's might already be marked as dead.
#pragma push_macro("checkliveness")
#define checkliveness(g, obj)

		static void invokeDestructor(lua_State* L, LuauUtil::luaTable* table) {
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

		/* this is only called when a state is closing. */
		static void invoke(lua_State* L) {
			L->userdata = LUAU_STATE_IS_DEAD;
			luaM_visitgco(L, L, invokeDestructors);
		}
	
		// Note: Apologies for using the raw LuaC API here.
		// Accessing L during GC traversal is inherently unsafe, but in this case, it's the most stable option.
		// This function is called during internal GC traversal — see luaU_freeudata for context.
		// P.S. Until Luau provides a safer alternative, we'll stick with this approach.
		static void dtor(lua_State* L, void* userdataPointer) {
			if (L->userdata == LUAU_STATE_IS_DEAD) {
				// All of the destructors have been called by luaU_garbageCollection::invoke already.
				return;
			}

			Udata* userdata = reinterpret_cast<Udata*>(
				reinterpret_cast<uint8_t*>(userdataPointer) - offsetof(Udata, data)
			);

			luaU_garbageCollection::invokeDestructor(L, userdata);
		}

		// restore macros.
#pragma pop_macro("checkliveness")
	};

	static int lua_dump(lua_State* L, lua_Writer writer, void* data, int strip) {
		int status;
		TValue* o;
		api_checknelems(L, 1);
		o = L->top - 1;
		if (isLfunction(o))
			status = LuauUtil::details::Dumper::luaU_dumpWriter(L, clvalue(o)->l.p, writer, data, strip);
		else
			status = 1;
		return status;
	}

	static int luaL_ref_comp(lua_State* L, int t) {
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

	static void luaL_unref_comp(lua_State* L, int t, int ref) {
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

	#define luaL_ref luaL_ref_comp
	#define luaL_unref luaL_unref_comp
#elif SOL_IS_ON(SOL_USING_CXX_LUA)
	#if __has_include(<lua/lua.h>)
		#include <lua/lua.h>
		#include <lua/lauxlib.h>
		#include <lua/lualib.h>
	#else
		#include <lua.h>
		#include <lauxlib.h>
		#include <lualib.h>
	#endif
#elif SOL_IS_ON(SOL_USE_LUA_HPP)
	#if __has_include(<lua/lua.hpp>)
		#include <lua/lua.hpp>
	#else
		#include <lua.hpp>
	#endif
#else
	extern "C" {
		#if __has_include(<lua/lua.h>)
			#include <lua/lua.h>
			#include <lua/lauxlib.h>
			#include <lua/lualib.h>
		#else
			#include <lua.h>
			#include <lauxlib.h>
			#include <lualib.h>
		#endif
	}
#endif // C++ Mangling for Lua vs. Not

#if SOL_IS_ON(SOL_USE_LUAU)
	#define return_luaL_error(L, fmt, ...) luaL_errorL(L, fmt, ##__VA_ARGS__);
	#define return_lua_error(L) lua_error(L);
#else
	#define return_luaL_error(L, fmt, ...) return luaL_errorL(L, fmt, ##__VA_ARGS__)
	#define return_lua_error(L) return lua_error(L)
#endif

#if defined(SOL_LUAJIT)
	#if (SOL_LUAJIT != 0)
		#define SOL_USE_LUAJIT_I_ SOL_ON
	#else
		#define SOL_USE_LUAJIT_I_ SOL_OFF
	#endif
#elif defined(LUAJIT_VERSION)
	#define SOL_USE_LUAJIT_I_ SOL_ON
#elif SOL_IS_ON(SOL_USING_CXX_LUAJIT)
	#define SOL_USE_LUAJIT_I_ SOL_ON
#else
	#define SOL_USE_LUAJIT_I_ SOL_DEFAULT_OFF
#endif // luajit

#if SOL_IS_ON(SOL_USING_CXX_LUAJIT)
	#include <luajit.h>
#elif SOL_IS_ON(SOL_USE_LUAJIT)
	extern "C" {
		#include <luajit.h>
	}
#endif // C++ LuaJIT ... whatever that means

#if defined(SOL_LUAJIT_VERSION)
	#define SOL_LUAJIT_VERSION_I_ SOL_LUAJIT_VERSION
#elif SOL_IS_ON(SOL_USE_LUAJIT)
	#define SOL_LUAJIT_VERSION_I_ LUAJIT_VERSION_NUM
#else
	#define SOL_LUAJIT_VERSION_I_ 0
#endif

#if defined(SOL_LUAJIT_FFI_DISABLED)
	#define SOL_LUAJIT_FFI_DISABLED_I_ SOL_ON
#elif defined(LUAJIT_DISABLE_FFI)
	#define SOL_LUAJIT_FFI_DISABLED_I_ SOL_ON
#else
	#define SOL_LUAJIT_FFI_DISABLED_I_ SOL_DEFAULT_OFF
#endif

#if defined(MOONJIT_VERSION)
	#define SOL_USE_MOONJIT_I_ SOL_ON
#else
	#define SOL_USE_MOONJIT_I_ SOL_OFF
#endif

#if !defined(SOL_LUA_VERSION)
	#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 502
		#define SOL_LUA_VERSION LUA_VERSION_NUM
	#elif defined(LUA_VERSION_NUM) && LUA_VERSION_NUM == 501
		#define SOL_LUA_VERSION LUA_VERSION_NUM
	#elif !defined(LUA_VERSION_NUM) || !(LUA_VERSION_NUM)
		// Definitely 5.0
		#define SOL_LUA_VERSION 500
	#else
		// ??? Not sure, assume latest?
		#define SOL_LUA_VERSION 504
	#endif // Lua Version 503, 502, 501 || luajit, 500
#endif // SOL_LUA_VERSION

#if defined(SOL_LUA_VERSION)
	#define SOL_LUA_VERSION_I_ SOL_LUA_VERSION
#else
	#define SOL_LUA_VERSION_I_ 504
#endif

// Exception safety / propagation, according to Lua information
// and user defines. Note this can sometimes change based on version information...
#if defined(SOL_EXCEPTIONS_ALWAYS_UNSAFE)
	#if (SOL_EXCEPTIONS_ALWAYS_UNSAFE != 0)
		#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_OFF
	#else
		#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_ON
	#endif
#elif defined(SOL_EXCEPTIONS_SAFE_PROPAGATION)
	#if (SOL_EXCEPTIONS_SAFE_PROPAGATION != 0)
		#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_ON
	#else
		#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_USE_LUAJIT)
		#if SOL_USE(SOL_LUAJIT_VERSION) >= 20100
			// LuaJIT 2.1.0-beta3 and better have exception support locked in for all platforms (mostly)
			#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_DEFAULT_ON
		#elif SOL_USE(SOL_LUAJIT_VERSION) >= 20000
			// LuaJIT 2.0.x have exception support only on x64 builds
			#if SOL_IS_ON(SOL_PLATFORM_X64)
				#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_DEFAULT_ON
			#else
				#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_DEFAULT_OFF
			#endif
		#endif
	#else
		// otherwise, there is no exception safety for
		// shoving exceptions through Lua and errors should
		// always be serialized
		#define SOL_PROPAGATE_EXCEPTIONS_I_ SOL_DEFAULT_OFF
	#endif
#endif

// Some configurations work with exceptions,
// but cannot catch-all everything...
#if defined(SOL_EXCEPTIONS_CATCH_ALL)
	#if (SOL_EXCEPTIONS_CATCH_ALL != 0)
		#define SOL_EXCEPTIONS_CATCH_ALL_I_ SOL_ON
	#else
		#define SOL_EXCEPTIONS_CATCH_ALL_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_USE_LUAJIT) || SOL_IS_ON(SOL_USING_CXX_LUAJIT)
		#define SOL_EXCEPTIONS_CATCH_ALL_I_ SOL_DEFAULT_OFF
	#elif SOL_IS_ON(SOL_USING_CXX_LUA)
		// C++ builds of Lua will throw an exception to implement its `yield` behavior;
		// it is irresponsible to "catch all" on this setting.
		#define SOL_EXCEPTIONS_CATCH_ALL_I_ SOL_DEFAULT_OFF
	#else
		// Otherwise, by default, everyhting should be caught.
		#define SOL_EXCEPTIONS_CATCH_ALL_I_ SOL_DEFAULT_ON
	#endif
#endif

#if defined(SOL_LUAJIT_USE_EXCEPTION_TRAMPOLINE)
	#if (SOL_LUAJIT_USE_EXCEPTION_TRAMPOLINE != 0)
		#define SOL_USE_LUAJIT_EXCEPTION_TRAMPOLINE_I_ SOL_ON
	#else
		#define SOL_USE_LUAJIT_EXCEPTION_TRAMPOLINE_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_OFF(SOL_PROPAGATE_EXCEPTIONS) && SOL_IS_ON(SOL_USE_LUAJIT)
		#define SOL_USE_LUAJIT_EXCEPTION_TRAMPOLINE_I_ SOL_ON
	#else
		#define SOL_USE_LUAJIT_EXCEPTION_TRAMPOLINE_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined(SOL_LUAL_STREAM_HAS_CLOSE_FUNCTION)
	#if (SOL_LUAL_STREAM_HAS_CLOSE_FUNCTION != 0)
		#define SOL_LUAL_STREAM_USE_CLOSE_FUNCTION_I_ SOL_ON
	#else
		#define SOL_LUAL_STREAM_USE_CLOSE_FUNCTION_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_OFF(SOL_USE_LUAJIT) && (SOL_LUA_VERSION > 501)
		#define SOL_LUAL_STREAM_USE_CLOSE_FUNCTION_I_ SOL_ON
	#else
		#define SOL_LUAL_STREAM_USE_CLOSE_FUNCTION_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined (SOL_LUA_BIT32_LIB)
	#if SOL_LUA_BIT32_LIB != 0
		#define SOL_LUA_BIT32_LIB_I_ SOL_ON
	#else
		#define SOL_LUA_BIT32_LIB_I_ SOL_OFF
	#endif
#else
	// Lua 5.2 only (deprecated in 5.3 (503)) (Can be turned on with Compat flags)
	// Lua 5.2, or other versions of Lua with the compat flag, or Lua that is not 5.2 with the specific define (5.4.1 either removed it entirely or broke it)
	#if (SOL_LUA_VERSION_I_ == 502)
		#define SOL_LUA_BIT32_LIB_I_ SOL_ON
	#elif defined(LUA_COMPAT_BITLIB)
		#define SOL_LUA_BIT32_LIB_I_ SOL_ON
	#elif (SOL_LUA_VERSION_I_ < 504 && defined(LUA_COMPAT_5_2))
		#define SOL_LUA_BIT32_LIB_I_ SOL_ON
	#else
		#define SOL_LUA_BIT32_LIB_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined (SOL_LUA_NIL_IN_TABLES)
	#if SOL_LUA_NIL_IN_TABLES != 0
		#define SOL_LUA_NIL_IN_TABLES_I_ SOL_ON
	#else
		#define SOL_LUA_NIL_IN_TABLES_I_ SOL_OFF
	#endif
#else
	#if defined(LUA_NILINTABLE) && (LUA_NILINTABLE != 0)
		#define SOL_LUA_NIL_IN_TABLES_I_ SOL_DEFAULT_ON
	#else
		#define SOL_LUA_NIL_IN_TABLES_I_ SOL_DEFAULT_OFF
	#endif
#endif

// clang-format on

#endif // SOL_COMPATIBILITY_LUA_VERSION_HPP
