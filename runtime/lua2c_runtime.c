#define LUA_CORE
#include "lprefix.h"
#include <stdio.h>
#include <string.h>
#include "lua2c_runtime.h"
#include "lauxlib.h"
#include "lualib.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"

void lua2c_checkabi(lua_State *L, unsigned long version,
                   size_t proto_size, size_t state_size,
                   size_t integer_size, size_t number_size,
                   const unsigned char *integer_probe,
                   const unsigned char *number_probe) {
  lua_Integer integer = 0x5678;
  lua_Number number = 370.5;
  if (version != LUA2C_ABI_VERSION || proto_size != sizeof(Proto) ||
      state_size != sizeof(lua_State) || integer_size != sizeof(integer) ||
      number_size != sizeof(number) ||
      memcmp(integer_probe, &integer, sizeof(integer)) != 0 ||
      memcmp(number_probe, &number, sizeof(number)) != 0)
    luaL_error(L, "lua2c runtime ABI/numeric configuration mismatch");
}

static TString *newstring(lua_State *L, Proto *p, Lua2cBytes bytes) {
  TString *s;
  if (bytes.data == NULL) return NULL;
  s = luaS_newlstr(L, (const char *)bytes.data, bytes.size);
  luaC_objbarrier(L, p, s);
  return s;
}

/* The prototype is rooted before allocation; every GC-visible array is
   initialized before another allocation can trigger emergency collection. */
static void fillproto(lua_State *L, Proto *p, const Lua2cProto *d, TString *source) {
  int i;
  p->numparams = (lu_byte)d->numparams;
  p->is_vararg = (lu_byte)d->is_vararg;
  p->maxstacksize = (lu_byte)d->maxstacksize;
  p->linedefined = d->linedefined;
  p->lastlinedefined = d->lastlinedefined;
  p->lua2c_entry = d->entry;
  p->lua2c_resume = d->resume;
  p->source = d->source.data ? newstring(L, p, d->source) : source;
  if (p->source) luaC_objbarrier(L, p, p->source);

  if (d->code != NULL) {
    p->code = luaM_newvectorchecked(L, d->sizecode, Instruction);
    p->sizecode = d->sizecode;
    memcpy(p->code, d->code, d->sizecode * sizeof(Instruction));
  }
  else {
    p->sizecode = d->sizecode;  /* logical positions, not an allocated array */
  }
  p->k = luaM_newvectorchecked(L, d->sizek, TValue);
  p->sizek = d->sizek;
  for (i = 0; i < p->sizek; ++i) setnilvalue(&p->k[i]);
  for (i = 0; i < p->sizek; ++i) {
    const Lua2cConstant *c = &d->constants[i];
    TValue *v = &p->k[i];
    switch (c->tag) {
      case LUA_VNIL: break;
      case LUA_VFALSE: setbfvalue(v); break;
      case LUA_VTRUE: setbtvalue(v); break;
      case LUA_VNUMINT: {
        lua_Integer n;
        memcpy(&n, c->bytes.data, sizeof(n));
        setivalue(v, n);
        break;
      }
      case LUA_VNUMFLT: {
        lua_Number n;
        memcpy(&n, c->bytes.data, sizeof(n));
        setfltvalue(v, n);
        break;
      }
      case LUA_VSHRSTR: case LUA_VLNGSTR: {
        TString *s = newstring(L, p, c->bytes);
        setsvalue2n(L, v, s);
        break;
      }
      default: luaL_error(L, "invalid lua2c constant descriptor");
    }
  }

  p->upvalues = luaM_newvectorchecked(L, d->sizeupvalues, Upvaldesc);
  p->sizeupvalues = d->sizeupvalues;
  for (i = 0; i < p->sizeupvalues; ++i) p->upvalues[i].name = NULL;
  for (i = 0; i < p->sizeupvalues; ++i) {
    p->upvalues[i].instack = d->upvalues[i].instack;
    p->upvalues[i].idx = d->upvalues[i].idx;
    p->upvalues[i].kind = d->upvalues[i].kind;
    p->upvalues[i].name = newstring(L, p, d->upvalues[i].name);
  }
  p->p = luaM_newvectorchecked(L, d->sizep, Proto *);
  p->sizep = d->sizep;
  for (i = 0; i < p->sizep; ++i) p->p[i] = NULL;
  for (i = 0; i < p->sizep; ++i) {
    p->p[i] = luaF_newproto(L);
    luaC_objbarrier(L, p, p->p[i]);
    fillproto(L, p->p[i], d->children[i], p->source);
  }
  p->lineinfo = luaM_newvectorchecked(L, d->sizelineinfo, ls_byte);
  p->sizelineinfo = d->sizelineinfo;
  if (d->sizelineinfo) memcpy(p->lineinfo, d->lineinfo, d->sizelineinfo);
  p->abslineinfo = luaM_newvectorchecked(L, d->sizeabslineinfo, AbsLineInfo);
  p->sizeabslineinfo = d->sizeabslineinfo;
  if (d->sizeabslineinfo)
    memcpy(p->abslineinfo, d->abslineinfo, d->sizeabslineinfo * sizeof(AbsLineInfo));
  p->locvars = luaM_newvectorchecked(L, d->sizelocvars, LocVar);
  p->sizelocvars = d->sizelocvars;
  for (i = 0; i < p->sizelocvars; ++i) p->locvars[i].varname = NULL;
  for (i = 0; i < p->sizelocvars; ++i) {
    p->locvars[i].varname = newstring(L, p, d->locals[i].name);
    p->locvars[i].startpc = d->locals[i].startpc;
    p->locvars[i].endpc = d->locals[i].endpc;
  }
}

void lua2c_pushchunk(lua_State *L, const Lua2cProto *d) {
  LClosure *cl;
  luaL_checkstack(L, 1, "lua2c chunk");
  lua_lock(L);
  cl = luaF_newLclosure(L, d->sizeupvalues);
  setclLvalue2s(L, L->top.p, cl);
  L->top.p++;
  cl->p = luaF_newproto(L);
  luaC_objbarrier(L, cl, cl->p);
  fillproto(L, cl->p, d, NULL);
  luaF_initupvals(L, cl);
  if (cl->nupvalues) {
    const TValue *globals = &hvalue(&G(L)->l_registry)->array[LUA_RIDX_GLOBALS - 1];
    setobj(L, cl->upvals[0]->v.p, globals);
    luaC_barrier(L, cl->upvals[0], globals);
  }
  lua_unlock(L);
}

int lua2c_finishmain(lua_State *L, int status, lua_KContext context) {
  (void)status;
  (void)context;
  return lua_gettop(L);
}

static int traceback(lua_State *L) {
  const char *message = lua_tostring(L, 1);
  if (message) luaL_traceback(L, L, message, 1);
  else if (!luaL_callmeta(L, 1, "__tostring"))
    lua_pushliteral(L, "(error object is not a string)");
  return 1;
}

static int prepare(lua_State *L) {
  const Lua2cProto *d = (const Lua2cProto *)lua_touserdata(L, 1);
  int argc = (int)lua_tointeger(L, 2);
  char **argv = (char **)lua_touserdata(L, 3);
  int i;
  luaL_openlibs(L);
  lua_createtable(L, argc - 1, 1);
  for (i = 0; i < argc; ++i) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "arg");
  lua2c_pushchunk(L, d);
  return 1;
}

int lua2c_cli(lua_State *L, const Lua2cProto *d, int argc, char **argv) {
  int status, i;
  lua_pushcfunction(L, traceback);
  lua_pushcfunction(L, prepare);
  lua_pushlightuserdata(L, (void *)d);
  lua_pushinteger(L, argc);
  lua_pushlightuserdata(L, argv);
  status = lua_pcall(L, 3, 1, 1);
  if (status == LUA_OK) {
    if (!lua_checkstack(L, argc)) {
      fputs("lua2c: too many command line arguments\n", stderr);
      return 1;
    }
    for (i = 1; i < argc; ++i) lua_pushstring(L, argv[i]);
    status = lua_pcall(L, argc - 1, LUA_MULTRET, 1);
  }
  if (status != LUA_OK) {
    const char *message = lua_tostring(L, -1);
    fprintf(stderr, "%s\n", message ? message : "lua2c: unknown Lua error");
  }
  return status == LUA_OK ? 0 : 1;
}
