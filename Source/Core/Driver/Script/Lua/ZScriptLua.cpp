#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "ZScriptLua.h"
#include <cassert>
#include <cstring>
#include "../../../Driver/Profiler/Optick/optick.h"

extern "C" {
	#include "Core/lua.h"
	#include "Core/lualib.h"
	#include "Core/lauxlib.h"
	#include "Core/lobject.h"
}

#ifdef _MSC_VER
#pragma warning(disable:4800)
#endif

using namespace PaintsNow;

enum {
	LUA_RIDX_BIND_KEY = LUA_RIDX_LAST + 1,
	LUA_RIDX_BIND_MANAGED_KEY,
	LUA_RIDX_BIND_INDEXED_KEY,
	LUA_RIDX_BIND_MANAGED_INDEXED_KEY,
	LUA_RIDX_DUMMY_KEY,
	LUA_RIDX_REFERENCE_KEY,
	LUA_RIDX_OBJECT_KEY,
	LUA_RIDX_WRAP_KEY,
	LUA_RIDX_STRING_KEY,
	LUA_RIDX_REQUEST_POOL_KEY
};

static int lua_typex(lua_State* L, int index) {
	int type = lua_type(L, index); // maybe faster ? ttypetag()
	if (type == LUA_TNUMBER) {
		type = lua_isinteger(L, index) ? LUA_VNUMINT : LUA_VNUMFLT;
	}

	return type;
}

static ZScriptLua* GetScript(lua_State* L) {
	return *reinterpret_cast<ZScriptLua**>(lua_getextraspace(L));
}

static void ObjectSet(lua_State* L, const IScript::BaseDelegate& b) {
	IScript::BaseDelegate& d = const_cast<IScript::BaseDelegate&>(b);
	IScript::Object* ptr = d.Get();
	int t = lua_gettop(L);
	// check if already exists
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_OBJECT_KEY);
	lua_rawgetp(L, -1, ptr);
	lua_replace(L, -2);

	if (lua_isnil(L, -1)) {
		d = IScript::BaseDelegate(nullptr);
		lua_pop(L, 1);

		void* p = lua_newuserdatauv(L, sizeof(void*), 0);
		memcpy(p, &ptr, sizeof(void*));

		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_BIND_INDEXED_KEY);
		lua_pushstring(L, ptr->GetUnique()->GetName().c_str());

		if (lua_rawget(L, -2) == LUA_TNIL) {
			lua_pop(L, 1);
			lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_BIND_KEY);
		}

		lua_replace(L, -2);
		
		lua_setmetatable(L, -2);
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_OBJECT_KEY);
		lua_pushvalue(L, -2);
		lua_rawsetp(L, -2, ptr);
		lua_pop(L, 1);
	}

	assert(lua_typex(L, -1) == LUA_TUSERDATA);
	int m = lua_gettop(L);
	assert(m == t + 1);
}

static bool IsObject(lua_State* L, void* ptr) {
	IScript::Object* object = *reinterpret_cast<IScript::Object**>(ptr);
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_OBJECT_KEY);
	lua_rawgetp(L, -1, object);
	bool checked = ptr == lua_touserdata(L, -1);
	lua_pop(L, 2);

	return checked;
}

static IScript::Object* ToObject(lua_State* L, int idx) {
	if (lua_islightuserdata(L, idx))
		return nullptr;

	IScript::Object** ptr = reinterpret_cast<IScript::Object**>(lua_touserdata(L, idx));
	if (ptr == nullptr) {
		return nullptr;
	} else {
		// is IScript::Object?
		IScript::Object* p = *ptr;
#ifdef DEBUG_SCRIPT
		if (!IsObject(L, ptr)) {
			assert(false);
			p = nullptr;
		}
#endif
		return p;
	}
}

static int CastToObject(lua_State* L) {
	IScript::Object* object = reinterpret_cast<IScript::Object*>(lua_tointeger(L, 1));
	if (object != nullptr) {
		ObjectSet(L, IScript::BaseDelegate(object));
		return 1;
	} else {
		return 0;
	}
}

static int CastToPointer(lua_State* L) {
	void* object = reinterpret_cast<void*>(lua_touserdata(L, 1));
	if (object != nullptr) {
		lua_pushinteger(L, (uint64_t)object);
		return 1;
	} else {
		return 0;
	}
}

static void HandleError(ZScriptLua* script, lua_State* L) {
	// error
	ZScriptLua::Request s(script, L);
	const char* errMsg = lua_tostring(L, -1);
	assert(script->IsLocked());
	script->UnLock();

	s.DefaultError(errMsg);
	/*
	if (strcmp(errMsg, "attempt to yield across a C-call boundary") == 0) {
		s.Error("Please use asynchronized calls in your script handler while calling lua routines that may yield themselves.");
	}*/
	script->DoLock();
	lua_pop(L, 1);
}

static int FunctionProxy(lua_State* L) {
	// const TProxy<void, IScript::Request&>* proxy = reinterpret_cast<TProxy<void, IScript::Request&>* const>(lua_touserdata(L, lua_upvalueindex(1)));
	// IHost* handler = reinterpret_cast<IHost*>(lua_touserdata(L, lua_upvalueindex(2)));
	const IScript::Request::AutoWrapperBase* wrapper = *reinterpret_cast<const IScript::Request::AutoWrapperBase**>(lua_touserdata(L, lua_upvalueindex(1)));

	ZScriptLua* pRet = GetScript(L);
	if (pRet->IsResetting()) return 0;

	int ptr = lua_gettop(L);
	int valsize = 0;
	bool hasError = false;
	{
		OPTICK_EVENT();
		ZScriptLua::Request s(pRet, L);

		// popup all locks
		assert(pRet->IsLocked());
		// pRet->UnLock(); // currently not locked in this way
		(*wrapper).Execute(s);
		// pRet->DoLock();
		assert(pRet->IsLocked());
		const String& errorMessage = s.GetError();
		hasError = errorMessage.size() != 0;

		if (hasError) {
			lua_pushlstring(L, errorMessage.c_str(), errorMessage.size());
		}
	}

	if (hasError) {
		return lua_error(L);
	} else {
		return lua_gettop(L) - ptr;
	}
}

static int FreeMem(lua_State* L) {
	void* mem = lua_touserdata(L, -1);
	if (mem == nullptr) // we only free user data
		return 0;

	ZScriptLua::Request s(GetScript(L), L);

	IScript* script = s.GetScript();
	assert(script->IsLocked());

	// printf("<<<< DELETED OBJ: %p Type: %s\n", obj, obj->GetUnique()->GetName().c_str());
	IScript::Object* obj = *reinterpret_cast<IScript::Object**>(mem);
	if (obj != nullptr) {
		obj->ScriptUninitialize(s);
	}

	assert(script->IsLocked());
	return 0;
}

static int FreeMemManaged(lua_State* L) {
	void* mem = lua_touserdata(L, -1);
	if (mem == nullptr) // we only free user data
		return 0;

	IScript* script = GetScript(L);
	assert(script->IsLocked());

	// printf("<<<< DELETED OBJ: %p Type: %s\n", obj, obj->GetUnique()->GetName().c_str());
	IReflectObject* obj = reinterpret_cast<IReflectObject*>(mem);
	if (obj != nullptr) {
		obj->~IReflectObject();
	}

	return 0;
}

static int FreeWrapper(lua_State* L) {
	IScript* script = GetScript(L);
	assert(script->IsLocked());
	script->UnLock();
	IScript::Request::AutoWrapperBase** base = reinterpret_cast<IScript::Request::AutoWrapperBase**>(lua_touserdata(L, -1));
	delete *base;
	script->DoLock();
	// printf("Freed!");
	return 0;
}

static int Cocreate(lua_State* L) {
	lua_State* NL;
	luaL_checktype(L, 1, LUA_TFUNCTION);
	NL = lua_newthread(L);
	/* copy request pool info*/
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_REQUEST_POOL_KEY);
	lua_pushvalue(L, -2);
	lua_pushthread(L);
	lua_rawget(L, -3);
	lua_rawset(L, -3);
	lua_pop(L, 1);

	lua_pushvalue(L, 1);  /* move function to top */
	lua_xmove(L, NL, 1);  /* move function from L to NL */
	return 1;
}

static int SetIndexer(lua_State* L) {
	// const char* s = lua_tostring(L, 1);
	luaL_checktype(L, 1, LUA_TSTRING);

	lua_CFunction freer[2] = {
		FreeMem,
		FreeMemManaged
	};

	const int keys[2] = {
		LUA_RIDX_BIND_INDEXED_KEY,
		LUA_RIDX_BIND_MANAGED_INDEXED_KEY,
	};

	bool setIndex = lua_istable(L, 2);
	bool setNewIndex = lua_istable(L, 3);

	for (int i = 0; i < 2; i++) {
		int top = lua_gettop(L);
		lua_newtable(L);

		if (setIndex) {
			lua_pushliteral(L, "__index");
			lua_pushvalue(L, 2);
			lua_rawset(L, -3);
		}

		if (setNewIndex) {
			lua_pushliteral(L, "__newindex");
			lua_pushvalue(L, 3);
			lua_rawset(L, -3);
		}

		lua_pushliteral(L, "__gc");
		lua_pushcfunction(L, freer[i]);
		lua_rawset(L, -3);

		lua_rawgeti(L, LUA_REGISTRYINDEX, keys[i]);
		lua_pushvalue(L, 1);
		lua_pushvalue(L, -3);
		lua_rawset(L, -3);
		lua_pop(L, 2);

		assert(top == lua_gettop(L));
	}

	return 0;
}

ZScriptLua::ZScriptLua(IThread& threadApi, lua_State* L) : IScript(threadApi), callCounter(0), runningEvent(nullptr), rawState(L) {
	resetting.store(1, std::memory_order_relaxed);
	Init();
}

bool ZScriptLua::IsResetting() const {
	return resetting.load(std::memory_order_relaxed) != 0;
}

bool ZScriptLua::IsHosting() const {
	return rawState == nullptr;
}

void ZScriptLua::Clear() {
	OPTICK_EVENT();
	// Wait for all active routines finished.
	DoLock();
	resetting.store(1, std::memory_order_release);

	if (callCounter != 0) {
		IThread::Event* e = threadApi.NewEvent();
		runningEvent = e;
		std::atomic_thread_fence(std::memory_order_release);
		threadApi.Wait(runningEvent, mutex);
		assert(runningEvent == nullptr);
		threadApi.DeleteEvent(e);
	}

	if (IsHosting()) {
		lua_close(state);
	}

	defaultRequest->Destroy();
	UnLock();

#ifdef _DEBUG
	if (totalReference != 0) {
		for (std::map<size_t, std::pair<uint32_t, String> >::iterator it = debugReferences.begin(); it != debugReferences.end(); ++it) {
			fprintf(stderr, "Possible leaked lua object at: %s, the object will be auto-released.\n", (*it).second.second.c_str());
		}
	}
#endif
}

thread_local size_t RecursiveCount = 0;

static void LuaHook(lua_State* L, lua_Debug* ar) {
	String name = "[Lua] ";

	if (lua_getinfo(L, "nS", ar)) {
		if (ar->name != nullptr) {
			name += ar->name;
		} else {
			name += "function ()";
		}
	}

	if (ar->event == LUA_HOOKCALL) {
		assert(!name.empty());
		OPTICK_PUSH_DYNAMIC(name.c_str());
		RecursiveCount++;
	} else if (ar->event == LUA_HOOKRET && RecursiveCount != 0) {
		OPTICK_POP();
		RecursiveCount--;
	}
}

static void* Realloc(void* ud, void* ptr, size_t osize, size_t nsize) {
	(void)ud; (void)osize;  /* not used */
	if (nsize == 0) {
		::operator delete ((char*)ptr);
		return nullptr;
	} else {
		if (osize >= nsize) {
			return ptr;
		} else {
			void* w = ::operator new(nsize);
			if (w == nullptr)
				return nullptr;

			if (ptr != nullptr) {
				memcpy(w, ptr, osize);
				::operator delete ((char*)ptr);
			}

			return w;
		}
	}
}

void ZScriptLua::Init() {
	DoLock();

	// attach to existing lua vm?
	if (rawState != nullptr) {
		state = rawState;
	} else {
		state = lua_newstate(Realloc, nullptr);
		luaL_openlibs(state);
	}

	lua_State* L = state;
	defaultRequest = new ZScriptLua::Request(this, L);
	callCounter = 0;

	ZScriptLua** s = reinterpret_cast<ZScriptLua**>(lua_getextraspace(L));
	*s = this;

	// make __gc metatable for default values
	lua_newtable(L);
	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, FreeMem);
	lua_rawset(L, -3);
	lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_BIND_KEY);

	lua_newtable(L);
	lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_BIND_INDEXED_KEY);

	lua_newtable(L);
	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, FreeMemManaged);
	lua_rawset(L, -3);
	lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_BIND_MANAGED_KEY);

	lua_newtable(L);
	lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_BIND_MANAGED_INDEXED_KEY);

	// make a dummy node for empty table accesses
	lua_newtable(L);
	lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_DUMMY_KEY);

	// make a reference table for references
	lua_newtable(L);
	lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_REFERENCE_KEY);

	// make a table for delegates
	lua_newtable(L);
	lua_pushliteral(L, "__mode");
	lua_pushliteral(L, "v");
	lua_rawset(L, -3);
	lua_pushvalue(L, -1);
	lua_setmetatable(L, -2);
	lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_OBJECT_KEY);

	// make a table for wrappers
	lua_newtable(L);
	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, FreeWrapper);
	lua_rawset(L, -3);
	lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_WRAP_KEY);

	lua_newtable(L);
	lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_STRING_KEY);

	// make a table for request pools
	lua_newtable(L);
	lua_pushliteral(L, "__mode");
	lua_pushliteral(L, "k");
	lua_rawset(L, -3);
	lua_pushvalue(L, -1);
	lua_setmetatable(L, -2);
	lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_REQUEST_POOL_KEY);

	// enable setmetatable for userdata
	lua_pushcfunction(L, SetIndexer);
	lua_setglobal(L, "setindexer");

	lua_getglobal(L, "coroutine");
	lua_pushliteral(L, "create");
	lua_pushcclosure(L, Cocreate, 0);
	lua_settable(L, -3);
	lua_pop(L, 1);

	lua_pushcfunction(L, CastToObject);
	lua_setglobal(L, "toobject");

	lua_pushcfunction(L, CastToPointer);
	lua_setglobal(L, "topointer");

	totalReference = 0;

	if (rawState != nullptr) {
		delete defaultRequest;
		defaultRequest = new Request(this, nullptr);
	}

	// Install hooks if needed
#if USE_OPTICK
	lua_sethook(state, LuaHook, LUA_MASKCALL | LUA_MASKRET, 0);
#endif

	resetting.store(0, std::memory_order_release);
	UnLock();
}

bool ZScriptLua::BeforeCall() {
	assert(IsLocked());
	callCounter++;

	return true;
}

void ZScriptLua::AfterCall() {
	assert(IsLocked());
	if (--callCounter == 0) {
		std::atomic_thread_fence(std::memory_order_acquire);
		if (runningEvent != nullptr) {
			threadApi.Signal(runningEvent);
			runningEvent = nullptr;
		}
	}
}

void ZScriptLua::Reset() {
	Clear();
	IScript::Reset();
	Init();
}

ZScriptLua::~ZScriptLua() {
	Clear();
}

IScript::Request::TYPE ZScriptLua::Request::GetReferenceType(const IScript::Request::Ref& g) {
	*this << g;
	int t = lua_typex(L, -1);
	lua_pop(L, 1);

	return ZScriptLua::Request::ConvertType(t);
}

bool ZScriptLua::Request::Call(const AutoWrapperBase& defer, const IScript::Request::Ref& g) {
	OPTICK_EVENT();

	assert(GetScript()->IsLocked());
	assert(tableLevel == 0);
	if (state->IsResetting()) return false;

	if (!state->BeforeCall())
		return false;

	int pos = lua_gettop(L);
	int in = pos - initCount;
	assert(in >= 0);
	// lua_State* s = state->GetState();
	assert(defer.IsSync()); // only support sync call
	*this << g;
	// printf("Type : %s\n", lua_typename(L, lua_typex(L, -1)));
	if (!lua_isfunction(L, -1)) {
		state->AfterCall();
		return false;
	}

	lua_insert(L, initCount + 1);
	assert(lua_isfunction(L, -in - 1));
	// lua_KContext context = (lua_KContext)defer.Clone();
	// dispatch deferred calls after the next sync call.
	// int status = lua_pcallk(L, in, LUA_MULTRET, 0, nullptr, ContinueProxy);

#if USE_OPTICK
	size_t savedCallCount = RecursiveCount;
#endif

	int status = lua_pcall(L, in, LUA_MULTRET, 0);

#if USE_OPTICK
	while (RecursiveCount > savedCallCount) {
		OPTICK_POP();
		RecursiveCount--;
	}
#endif

	if (status != LUA_OK && status != LUA_YIELD) {
		HandleError(static_cast<ZScriptLua*>(GetScript()), L);
		state->AfterCall();
		return false;
	} else {
		assert(lua_gettop(L) >= initCount);
		// important!
		SetIndex(initCount + 1);
		state->AfterCall();
		return true;
	}
}

const char* ZScriptLua::GetFileExt() const {
	static const String ext = "lua";
	return ext.c_str();
}

IScript::Request* ZScriptLua::NewRequest(const String& entry) {
	assert(IsLocked());
	return new ZScriptLua::Request(this, nullptr);
}

IScript::Request& ZScriptLua::Request::MoveVariables(IScript::Request& target, size_t count) {
	assert(GetScript()->IsLocked());
	lua_xmove(L, (static_cast<ZScriptLua::Request&>(target)).L, (int)count);
	return *this;
}

void ZScriptLua::Request::DefaultError(const String& msg) {
	BaseClass::Error(msg);
}

void ZScriptLua::Request::Error(const String& msg) {
	if (ref.value == 0 && L != state->state) { // borrowed request
		errorMessage = msg; // delayed.
	}

	DefaultError(String("HostError: ") + msg);
}

IScript* ZScriptLua::Request::GetScript() {
	return state;
}

int ZScriptLua::Request::GetCount() {
	assert(GetScript()->IsLocked());
	// return lua_gettop(L);
	return lua_gettop(L) - initCount;
}

const String& ZScriptLua::Request::GetError() const {
	return errorMessage;
}

IScript::Request& ZScriptLua::Request::CleanupIndex() {
	idx = initCount + 1;
	return *this;
}

void ZScriptLua::Request::SetIndex(int i) {
	assert(GetScript()->IsLocked());
	idx = i;
}

lua_State* ZScriptLua::Request::GetRequestState() {
	return L;
}

static int IncreaseTableIndex(lua_State* L, int count = 1) {
	void* t = (lua_touserdata(L, -1));
	int index = *(int*)&t;
	if (count == 0) {
		index = 0;
	} else {
		index += count;
	}
	lua_pop(L, 1);
	lua_pushlightuserdata(L, (void*)(size_t)index);
	return index;
}

static void refget(lua_State* L, const IScript::Request::Ref& ref) {
#ifdef _DEBUG
	assert(ref.hostScript == nullptr || GetScript(L) == ref.hostScript);
#endif
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_REFERENCE_KEY);
	lua_rawgeti(L, -1, ref.value);
	lua_replace(L, -2);
}

template <class F, class C>
static void Write(lua_State* L, int& tableLevel, int& idx, ZScriptLua::Request::KeyState& keyState, F f, C& value) {
	if (tableLevel != 0) {
		if (keyState == ZScriptLua::Request::KEYSTATE_IDLE) {
			int index = IncreaseTableIndex(L);
			f(L, value);
			lua_rawseti(L, -3, index);
		} else if (keyState == ZScriptLua::Request::KEYSTATE_KEY) {
			f(L, value);
			keyState = ZScriptLua::Request::KEYSTATE_VALUE;
		} else {
			assert(keyState == ZScriptLua::Request::KEYSTATE_VALUE);
			f(L, value);
			lua_rawset(L, -4);
			keyState = ZScriptLua::Request::KEYSTATE_IDLE;
		}
	} else {
		assert(keyState == ZScriptLua::Request::KEYSTATE_IDLE);
		f(L, value);
	}
}

template <class F, class C>
static void Read(lua_State* L, int& tableLevel, int& idx, ZScriptLua::Request::KeyState& keyState, F f, C& value) {
	if (tableLevel != 0) {
		if (keyState == ZScriptLua::Request::KEYSTATE_IDLE) {
			assert(lua_isuserdata(L, -1));
			assert(lua_istable(L, -2));

			int index = IncreaseTableIndex(L);
			lua_rawgeti(L, -2, index);
		} else if (keyState == ZScriptLua::Request::KEYSTATE_KEY) {
			assert(false); // not possible
			return;
		} else if (keyState == ZScriptLua::Request::KEYSTATE_KEY_ITER) {
			assert(lua_isuserdata(L, -3));
			assert(lua_istable(L, -4));
			value = f(L, -2);
			keyState = ZScriptLua::Request::KEYSTATE_VALUE_ITER;
			return;
		} else if (keyState == ZScriptLua::Request::KEYSTATE_VALUE_ITER) {
			assert(lua_isuserdata(L, -5));
			assert(lua_isuserdata(L, -3));
			assert(lua_istable(L, -4));
			value = f(L, -1);
			lua_pop(L, 1);
			keyState = ZScriptLua::Request::KEYSTATE_NEXT_ITER;
			return;
		} else {
			// TODO: verify this!
			assert(lua_isuserdata(L, -4));
			assert(lua_isuserdata(L, -2));
			assert(keyState == ZScriptLua::Request::KEYSTATE_VALUE);
			assert(lua_istable(L, -3));
			lua_rawget(L, -3);
			keyState = ZScriptLua::Request::KEYSTATE_IDLE;
		}

		if (!lua_isnil(L, -1)) {
			value = f(L, -1);
		}
		lua_pop(L, 1);
	} else {
		assert(keyState == ZScriptLua::Request::KEYSTATE_IDLE);
		value = f(L, idx++);
	}
}

IScript::Request& ZScriptLua::Request::operator >> (Arguments& args) {
	args.count = initCount - idx + 1;
	assert(args.count >= 0);
	return *this;
}

IScript::Request& ZScriptLua::Request::operator << (const IScript::Request::Global&) {
	assert(GetScript()->IsLocked());
	assert(keyState == KEYSTATE_IDLE);
	lua_pushlightuserdata(L, (void*)keyState);
	lua_pushglobaltable(L);
	// now the top of stack is object table
	// push index
	lua_pushlightuserdata(L, (void*)0);
	tableLevel++;
	return *this;
}

void ZScriptLua::Request::Dereference(IScript::Request::Ref& ref) {
	assert(GetScript()->IsLocked());
	if (ref.value != 0) {
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_REFERENCE_KEY);
		// refget(L, ref);
		// printf("UNREF: %s\n", lua_typename(L, lua_typex(L, -1)));
		// lua_pop(L, 1);

		luaL_unref(L, -1, (int)ref.value);
		lua_pop(L, 1);
		DelReference(ref);
		ref.value = 0;
	}
}

IScript::Request::AutoWrapperBase* ZScriptLua::Request::GetWrapper(const IScript::Request::Ref& ref) {
#ifdef _DEBUG
	assert(ref.hostScript == nullptr || ref.hostScript == GetScript());
#endif
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_REFERENCE_KEY);
	lua_rawgeti(L, -1, ref.value);

	if (lua_iscfunction(L, -1)) {
		lua_getupvalue(L, -1, 1);
		IScript::Request::AutoWrapperBase* wrapper = *reinterpret_cast<IScript::Request::AutoWrapperBase**>(lua_touserdata(L, -1));
		lua_pop(L, 3);
		return wrapper;
	} else {
		lua_pop(L, 2);
		return nullptr;
	}
}

static IScript::Request::Ref refadd(lua_State* L, int index) {
	if (lua_isnil(L, index) || index > lua_gettop(L)) {
		return IScript::Request::Ref(0);
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_REFERENCE_KEY);
	lua_pushvalue(L, index > 0 ? index : index - 1);
	// printf("Val type: %s\n", lua_typename(L, lua_typex(L, -1)));
	int ref = luaL_ref(L, -2);
	lua_pop(L, 1);
	return IScript::Request::Ref(ref == -1 ? 0 : ref);
}

static void wrapget(lua_State* L, const IScript::Request::AutoWrapperBase& wrapper) {
	// make a reference in wrap table
	IScript::Request::AutoWrapperBase** ptr = reinterpret_cast<IScript::Request::AutoWrapperBase**>(lua_newuserdatauv(L, sizeof(IScript::Request::AutoWrapperBase*), 0));
	*ptr = wrapper.Clone();

	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_WRAP_KEY);
	lua_setmetatable(L, -2);
	lua_pushcclosure(L, FunctionProxy, 1);
}

IScript::Request& ZScriptLua::Request::Push() {
	// save state
	assert(keyState == KEYSTATE_IDLE);
	lua_pushinteger(L, idx);
	lua_pushinteger(L, tableLevel);
	lua_pushinteger(L, initCount);
	initCount = lua_gettop(L);
	idx = initCount + 1;
	tableLevel = 0;
	return *this;
}

IScript::Request& ZScriptLua::Request::Pop() {
	// save state
	assert(keyState == KEYSTATE_IDLE);
	assert(tableLevel == 0);
	assert(lua_gettop(L) >= 3);
	lua_settop(L, initCount);
	initCount = (int)lua_tointeger(L, -1);
	tableLevel = (int)lua_tointeger(L, -2);
	idx = (int)lua_tointeger(L, -3);
	lua_pop(L, 3);
	return *this;
}

void ZScriptLua::Request::AddReference(IScript::Request::Ref& ref) {
#ifdef _DEBUG
	ref.hostScript = GetScript();
#endif
	if (ref.value == 0)
		return;

	state->totalReference++;
#ifdef _DEBUG
	if (state->debugReferences[ref.value].first++ == 0) {
		refget(L, ref);
		int type = lua_typex(L, -1);
		lua_pop(L, 1);
		state->debugReferences[ref.value].second = lua_typename(L, type);
	}
#endif
}

void ZScriptLua::Request::DelReference(IScript::Request::Ref ref) {
#ifdef _DEBUG
	assert(ref.hostScript == nullptr || ref.hostScript == GetScript());
#endif
	if (ref.value == 0)
		return;

	state->totalReference--;
#ifdef _DEBUG
	if (--state->debugReferences[ref.value].first == 0) {
		state->debugReferences.erase(ref.value);
	}
#endif
}

IScript::Request& ZScriptLua::Request::operator << (const AutoWrapperBase& wrapper) {
	assert(GetScript()->IsLocked());
	Write(L, tableLevel, idx, keyState, wrapget, wrapper);
	return *this;
}

IScript::Request& ZScriptLua::Request::operator >> (IScript::Request::Ref& ref) {
	assert(GetScript()->IsLocked());

	Read(L, tableLevel, idx, keyState, refadd, ref);
	AddReference(ref);
	return *this;
}

IScript::Request& ZScriptLua::Request::operator << (const IScript::Request::Ref& ref) {
#ifdef _DEBUG
	assert(ref.hostScript == nullptr || ref.hostScript == GetScript());
#endif
	assert(GetScript()->IsLocked());
	Write(L, tableLevel, idx, keyState, refget, ref);
	return *this;
}

IScript::Request& ZScriptLua::GetDefaultRequest() {
	return *defaultRequest;
}

IScript::Request& ZScriptLua::Request::operator >> (IScript::Request::TableStart& start) {
	assert(GetScript()->IsLocked());

	if (tableLevel == 0) {
		assert(keyState == KEYSTATE_IDLE);
		lua_pushlightuserdata(L, (void*)keyState);
		lua_pushvalue(L, idx++); // table
	} else {
		if (keyState == KEYSTATE_IDLE) {
			int index = IncreaseTableIndex(L);
			lua_pushlightuserdata(L, (void*)keyState);
			lua_rawgeti(L, -3, index);
		} else if (keyState == KEYSTATE_KEY) {
			assert(false);
			return *this;
		} else if (keyState == KEYSTATE_KEY_ITER) {
			// TODO: verify this!
			keyState = KEYSTATE_VALUE_ITER;
			lua_pushlightuserdata(L, (void*)keyState);
			lua_pushvalue(L, -3);
		} else if (keyState == KEYSTATE_VALUE_ITER) {
			// TODO: verify this!
			keyState = KEYSTATE_NEXT_ITER;
			lua_pushlightuserdata(L, (void*)keyState);
			lua_insert(L, -2);
		} else {
			// TODO: verify this!
			assert(keyState == KEYSTATE_VALUE);
			lua_rawget(L, -3);
			lua_pushlightuserdata(L, (void*)keyState);
			lua_insert(L, -2);
		}

		keyState = KEYSTATE_IDLE;
	}

	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_DUMMY_KEY);
	}

	lua_pushliteral(L, "n");
	if (lua_rawget(L, -2) == LUA_TNUMBER) {
		start.count = (size_t)lua_tointeger(L, -1);
	} else {
		start.count = (size_t)lua_rawlen(L, -2);
	}
	lua_pop(L, 1);

	lua_pushlightuserdata(L, (void*)0); // index
	tableLevel++;

	assert(lua_istable(L, -2));
	assert(lua_isuserdata(L, -1));
	assert(lua_isuserdata(L, -3));

	return *this;
}

IScript::Request& ZScriptLua::Request::operator >> (IScript::Request::ArrayStart& start) {
	TableStart ts;
	*this >> ts;
	start.count = ts.count;
	return *this;
}

IScript::Request& ZScriptLua::Request::operator << (const IScript::Request::TableStart& t) {
	assert(GetScript()->IsLocked());

	// add table
	if (tableLevel != 0) {
		if (keyState == KEYSTATE_IDLE) {
			int index = IncreaseTableIndex(L);
			lua_pushlightuserdata(L, (void*)keyState);
			lua_newtable(L);
			lua_pushvalue(L, -1);
			lua_rawseti(L, -5, index);
		} else {
			lua_newtable(L);
			lua_insert(L, -2);
			lua_pushvalue(L, -2);
			lua_rawset(L, -5);

			keyState = KEYSTATE_IDLE;
			lua_pushlightuserdata(L, (void*)keyState);
			lua_insert(L, -2);
		}
	} else {
		assert(keyState == KEYSTATE_IDLE);
		lua_newtable(L);
		lua_pushlightuserdata(L, (void*)keyState);
		lua_pushvalue(L, -2);
	}

	// load table
	assert(lua_istable(L, -1));
	lua_pushlightuserdata(L, (void*)0);
	assert(lua_isuserdata(L, -3));
	tableLevel++;

	return *this;
}

IScript::Request& ZScriptLua::Request::operator << (const IScript::Request::ArrayStart& t) {
	return *this << TableStart();
}

IScript::Request& ZScriptLua::Request::operator << (const IScript::Request::TableEnd& d) {
	assert(GetScript()->IsLocked());
	assert(tableLevel != 0);
	tableLevel--;
	assert(lua_isuserdata(L, -3));
	KeyState state = (KeyState)(size_t)lua_touserdata(L, -3);
	keyState = state;
	lua_pop(L, 3);

	return *this;
}

IScript::Request& ZScriptLua::Request::operator << (const IScript::Request::ArrayEnd& d) {
	return *this << TableEnd();
}

IScript::Request& ZScriptLua::Request::operator << (double value) {
	assert(GetScript()->IsLocked());
	Write(L, tableLevel, idx, keyState, lua_pushnumber, value);
	return *this;
}

static double ToNumber(lua_State* L, int index) {
	return lua_tonumber(L, index);
}

IScript::Request& ZScriptLua::Request::operator >> (double& value) {
	assert(GetScript()->IsLocked());
	Read(L, tableLevel, idx, keyState, ToNumber, value);

	return *this;
}

static void StrViewSet(lua_State* L, const StringView& v) {
	lua_pushlstring(L, v.data(), v.size());
}

static StringView StrViewGet(lua_State* L, int index) {
	size_t length;
	const char* ptr = lua_tolstring(L, index, &length);
	return StringView(ptr, length);
}

static void StrSet(lua_State* L, const String& v) {
	lua_pushlstring(L, v.data(), v.size());
}

static String StrGet(lua_State* L, int index) {
	size_t length;
	const char* ptr = lua_tolstring(L, index, &length);
	return String(ptr, length);
}

IScript::Request& ZScriptLua::Request::operator << (const StringView& v) {
	assert(GetScript()->IsLocked());
	Write(L, tableLevel, idx, keyState, StrViewSet, v);
	return *this;
}

IScript::Request& ZScriptLua::Request::operator >> (StringView& v) {
	assert(GetScript()->IsLocked());
	Read(L, tableLevel, idx, keyState, StrViewGet, v);

	return *this;
}

IScript::Request& ZScriptLua::Request::operator << (const String& v) {
	assert(GetScript()->IsLocked());
	Write(L, tableLevel, idx, keyState, StrSet, v);
	return *this;
}

IScript::Request& ZScriptLua::Request::operator >> (String& v) {
	assert(GetScript()->IsLocked());
	Read(L, tableLevel, idx, keyState, StrGet, v);

	return *this;
}

static void BytesSet(lua_State* L, const Bytes& v) {
	lua_pushlstring(L, (const char*)v.GetData(), v.GetSize());
}

static Bytes BytesGet(lua_State* L, int index) {
	size_t length;
	const char* ptr = lua_tolstring(L, index, &length);
	return Bytes((const uint8_t*)ptr, length);
}

IScript::Request& ZScriptLua::Request::operator << (const Bytes& v) {
	assert(GetScript()->IsLocked());
	Write(L, tableLevel, idx, keyState, BytesSet, v);
	return *this;
}

IScript::Request& ZScriptLua::Request::operator >> (Bytes& v) {
	assert(GetScript()->IsLocked());
	Read(L, tableLevel, idx, keyState, BytesGet, v);

	return *this;
}

static void ManagedSet(lua_State* L, void*& v) {
	v = lua_newuserdatauv(L, (size_t)v, 0);
	assert(((size_t)v & 0xf) == 0);
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_BIND_MANAGED_KEY);
	lua_setmetatable(L, -2);
}

static void* ManagedGet(lua_State* L, int index) {
	if (!lua_islightuserdata(L, index)) {
		void* ptr = lua_touserdata(L, index);
#ifdef DEBUG_SCRIPT
		if (IsObject(L, ptr)) {
			assert(false);
			return nullptr;
		}
#endif
		return ptr;
	} else {
		return nullptr;
	}
}

IScript::ManagedObject* ZScriptLua::Request::WriteManaged(size_t size) {
	assert(GetScript()->IsLocked());
	void* userdata = reinterpret_cast<void*>(size);
	Write(L, tableLevel, idx, keyState, ManagedSet, userdata);

	return reinterpret_cast<ManagedObject*>(userdata);
}

IScript::ManagedObject* ZScriptLua::Request::ReadManaged() {
	assert(GetScript()->IsLocked());
	void* v = nullptr;
	Read(L, tableLevel, idx, keyState, ManagedGet, v);

	return reinterpret_cast<IScript::ManagedObject*>(v);
}

static void UniqueSet(lua_State* L, Unique v) {
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_STRING_KEY);
	if (lua_rawgetp(L, -1, (void*)v.GetInfo()) == LUA_TNIL) {
		lua_pop(L, 1);
		const String& name = v->GetName();
		lua_pushlstring(L, name.c_str(), name.size());
		lua_pushvalue(L, -1);
		lua_rawsetp(L, -3, (void*)v.GetInfo());
		lua_pushvalue(L, -1);
		lua_pushlightuserdata(L, (void*)v.GetInfo());
		lua_rawset(L, -4);
	}

	lua_replace(L, -2);
}

static Unique UniqueGet(lua_State* L, int index) {
	lua_pushvalue(L, index);
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_STRING_KEY);
	lua_pushvalue(L, -2);

	if (lua_rawget(L, -2) != LUA_TNIL) {
		Unique unique = (UniqueInfo*)lua_touserdata(L, -1);
		lua_pop(L, 3);
		return unique;
	} else {
		lua_pop(L, 1);
		Unique unique(lua_tostring(L, -2));
		lua_pushvalue(L, -2);
		lua_rawsetp(L, -2, (void*)unique.GetInfo());
		lua_pushvalue(L, -2);
		lua_pushlightuserdata(L, (void*)unique.GetInfo());
		lua_rawset(L, -3);
		lua_pop(L, 2);
		
		return unique;
	}
}

IScript::Request& ZScriptLua::Request::operator << (Unique v) {
	assert(GetScript()->IsLocked());
	Write(L, tableLevel, idx, keyState, UniqueSet, v);
	return *this;
}

IScript::Request& ZScriptLua::Request::operator >> (Unique& v) {
	assert(GetScript()->IsLocked());
	Read(L, tableLevel, idx, keyState, UniqueGet, v);

	return *this;
}

// lua 5.3 now supports 64bit integer
IScript::Request& ZScriptLua::Request::operator << (int64_t value) {
	assert(GetScript()->IsLocked());
	Write(L, tableLevel, idx, keyState, lua_pushinteger, value);
	return *this;
}

static int64_t tointeger(lua_State* L, int index) {
	int isnumber;
	int64_t v = lua_tointegerx(L, index, &isnumber);
	if (isnumber) {
		return v;
	} else {
		return (int64_t)lua_tonumber(L, index);
	}
}

IScript::Request& ZScriptLua::Request::operator >> (int64_t& value) {
	assert(GetScript()->IsLocked());
	Read(L, tableLevel, idx, keyState, tointeger, value);

	return *this;
}

IScript::Request& ZScriptLua::Request::operator << (bool value) {
	assert(GetScript()->IsLocked());
	Write(L, tableLevel, idx, keyState, lua_pushboolean, value);
	return *this;
}

IScript::Request& ZScriptLua::Request::operator >> (bool& value) {
	assert(GetScript()->IsLocked());
	Read(L, tableLevel, idx, keyState, lua_toboolean, value);

	return *this;
}

IScript::Request& ZScriptLua::Request::operator << (const char* value) {
	assert(GetScript()->IsLocked());
	Write(L, tableLevel, idx, keyState, lua_pushstring, value);

	return *this;
}

static const char* tostrptr(lua_State* L, int i) {
	return lua_tostring(L, i);
}

IScript::Request& ZScriptLua::Request::operator >> (const char*& value) {
	assert(GetScript()->IsLocked());
	Read(L, tableLevel, idx, keyState, tostrptr, value);

	return *this;
}

IScript::Request& ZScriptLua::Request::operator << (const IScript::Request::Key& k) {
	assert(GetScript()->IsLocked());
	assert(keyState == KEYSTATE_IDLE);
	keyState = KEYSTATE_KEY;
	return *this;
}

IScript::Request::TYPE ZScriptLua::Request::ConvertType(int type) {
	IScript::Request::TYPE target = NIL;
	switch (type) {
	case LUA_TNIL:
		target = NIL;
		break;
	case LUA_TBOOLEAN:
		target = BOOLEAN;
		break;
	case LUA_VNUMFLT:
		target = NUMBER;
		break;
	case LUA_VNUMINT:
		target = INTEGER;
		break;
	case LUA_TSTRING:
		target = STRING;
		break;
	case LUA_TTABLE:
		target = TABLE;
		break;
	case LUA_TFUNCTION:
		target = FUNCTION;
		break;
	case LUA_TUSERDATA:
		target = OBJECT;
		break;
	default:
		target = NIL;
		break;
	}

	return target;
}

IScript::Request& ZScriptLua::Request::operator >> (IScript::Request::Iterator& it) {
	assert(GetScript()->IsLocked());
	if (it.keyType == NIL) 	{
		assert(keyState == KEYSTATE_IDLE);
		lua_pushnil(L);
	} else {
		assert(keyState == KEYSTATE_NEXT_ITER);
	}

	if (lua_next(L, -3) != 0) {
		it.keyType = ConvertType(lua_typex(L, -2));
		it.valueType = ConvertType(lua_typex(L, -1));
		keyState = KEYSTATE_KEY_ITER;
	} else {
		it.keyType = it.valueType = NIL; // iteration ended
		keyState = KEYSTATE_IDLE;
	}

	return *this;
}

IScript::Request::Ref ZScriptLua::Request::Reference(const IScript::Request::Ref& d) {
	assert(GetScript()->IsLocked());
	int idx = GetCount();
	*this << d;
	IScript::Request::Ref ref = refadd(L, -1);
	lua_pop(L, 1);

	assert(idx == GetCount());
	AddReference(ref);
	return ref;
}

static void fake_pushnil(lua_State* L, const IScript::Request::Nil&) {
	lua_pushnil(L);
}

IScript::Request& ZScriptLua::Request::operator << (const IScript::BaseDelegate& d) {
	assert(GetScript()->IsLocked());
	if (d.Get() != nullptr) {
		// write
		IScript::BaseDelegate& m = const_cast<IScript::BaseDelegate&>(d);
		IScript::Object* ptr = m.Get();
		Write(L, tableLevel, idx, keyState, ObjectSet, m);

		if (m.Get() == nullptr) {
			ptr->ScriptInitialize(*this);
		}
	} else {
		Write(L, tableLevel, idx, keyState, fake_pushnil, nil);
	}

	return *this;
}

IScript::Request& ZScriptLua::Request::operator >> (IScript::BaseDelegate& d) {
	assert(GetScript()->IsLocked());
	Object* p = nullptr;
	Read(L, tableLevel, idx, keyState, ToObject, p);
	d = IScript::BaseDelegate(p);

	return *this;
}

IScript::Request& ZScriptLua::Request::operator << (const Nil& n) {
	assert(GetScript()->IsLocked());
	Write(L, tableLevel, idx, keyState, fake_pushnil, n);
	return *this;
}

IScript::Request::TYPE ZScriptLua::Request::GetCurrentType() {
	int type = LUA_TNIL;
	if (tableLevel != 0) {
		assert(lua_isuserdata(L, -1));
		assert(lua_istable(L, -2));
		assert(keyState == KEYSTATE_IDLE);
		void* t = (lua_touserdata(L, -1));
		int index = *(int*)&t + 1;
		lua_rawgeti(L, -2, index);

		if (!lua_isnil(L, -1)) {
			type = lua_typex(L, -1);
		}

		lua_pop(L, 1);
	} else {
		type = lua_typex(L, idx);
	}

	return ConvertType(type);
}

ZScriptLua::Request::Request(ZScriptLua* s, lua_State* l) : state(s), keyState(KEYSTATE_IDLE), L(l) {
	assert(GetScript()->IsLocked());
	if (L == nullptr) {
		L = lua_newthread(s->GetState());

		// make reference so gc won't collect this request
		ref = refadd(s->GetState(), -1);
		lua_pop(s->GetState(), 1);
		AddReference(ref);
	} else {
		ref.value = 0;
	}

	ExchangeState(L);
}

ZScriptLua::IndexState::IndexState(lua_State* L) {
	idx = 1;
	tableLevel = 0;
	initCount = lua_gettop(L);
}

ZScriptLua::IndexState ZScriptLua::Request::ExchangeState(const IndexState& w) {
	IndexState state = static_cast<IndexState>(*this);
	static_cast<IndexState&>(*this) = w;
	return state;
}

IScript::Request::Ref ZScriptLua::Request::Load(const String& script, const String& path) {
	assert(GetScript()->IsLocked());
	// lua_pushlightuserdata(L, LUA_RIDX_REFERENCE_KEY);
	// assert(lua_istable(L, -1));
	IScript::Request::Ref ref;
	int error = luaL_loadbuffer(L, script.c_str(), script.length(), path.c_str());

	if (error != 0) {
		DefaultError(lua_tostring(L, -1));
		lua_pop(L, 1);
		return ref;
	}

	assert(lua_isfunction(L, -1));
	ref = refadd(L, -1);
	lua_pop(L, 1);
	AddReference(ref);

	return ref;
}

void ZScriptLua::Request::SetRequestPool(IScript::RequestPool* p) {
	assert(GetScript()->IsLocked());

	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_REQUEST_POOL_KEY);
	lua_pushthread(L);
	lua_pushlightuserdata(L, p);
	lua_rawset(L, -3);
	lua_pop(L, 1);
}

void* ZScriptLua::Request::GetNativeScript() {
	return L;
}

IScript::RequestPool* ZScriptLua::Request::GetRequestPool() {
	assert(GetScript()->IsLocked());

	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_REQUEST_POOL_KEY);
	lua_pushthread(L);
	lua_rawget(L, -2);

	IScript::RequestPool* p = reinterpret_cast<IScript::RequestPool*>(lua_touserdata(L, -1));
	lua_pop(L, 2);

	return p;
}

ZScriptLua::Request::~Request() {
	assert(GetScript()->IsLocked());
	if (ref.value != 0) {
		Dereference(ref);
	}
}

lua_State* ZScriptLua::GetState() const {
	return state;
}

TObject<IReflect>& ZScriptLua::Request::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

IScript* ZScriptLua::NewScript() const {
	return new ZScriptLua(threadApi);
}