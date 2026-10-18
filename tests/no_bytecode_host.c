#include <stdio.h>
#include <string.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lobject.h"

int lua2c_main(lua_State *L);

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "no-bytecode check failed: %d\n", __LINE__); lua_close(L); return 1; } } while (0)

static int writes;
static int yields;
static int no_code(const Proto *p) {
  int i;
  if (p->code != NULL || p->sizecode <= 0 || p->lua2c_resume == NULL) return 0;
  for (i = 0; i < p->sizep; ++i) if (!no_code(p->p[i])) return 0;
  return 1;
}
static int writer(lua_State *L, const void *data, size_t size, void *context) {
  (void)L; (void)data; (void)size; (void)context;
  ++writes;
  return 0;
}
static void hook(lua_State *L, lua_Debug *ar) {
  (void)ar;
  ++yields;
  lua_yield(L, 0);
}
int main(void) {
  lua_State *L = luaL_newstate();
  lua_State *co;
  const LClosure *cl;
  int status, results, iteration;
  if (!L) return 1;
  luaL_openlibs(L);
  lua_pushcfunction(L, lua2c_main);
  status = lua_pcall(L, 0, 1, 0);
  if (status != LUA_OK) fprintf(stderr, "%s\n", lua_tostring(L, -1));
  CHECK(status == LUA_OK && lua_isfunction(L, -1));
  cl = (const LClosure *)lua_topointer(L, -1);
  CHECK(cl->p->sizep > 0 && no_code(cl->p));
  CHECK(lua_dump(L, writer, NULL, 0) != 0 && writes == 0);
  CHECK(lua_dump(L, writer, NULL, 1) != 0 && writes == 0);
  for (iteration = 0; iteration < 2; ++iteration) {
    lua_gc(L, iteration ? LUA_GCGEN : LUA_GCINC);
    co = lua_newthread(L);
    lua_pushvalue(L, -2);
    lua_xmove(L, co, 1);
    lua_pushinteger(co, 10);
    lua_sethook(co, hook, iteration ? LUA_MASKLINE : LUA_MASKCOUNT, 2);
    yields = 0;
    status = lua_resume(co, L, 1, &results);
    while (status == LUA_YIELD && yields < 1000) status = lua_resume(co, L, 0, &results);
    CHECK(status == LUA_OK && results == 1 && lua_tointeger(co, -1) == 55);
    /* Stripped functions have no line events, but count hooks still yield. */
    CHECK(iteration || yields > 0);
    lua_pop(L, 1);
    lua_gc(L, LUA_GCCOLLECT);
  }
  lua_close(L);
  puts("no-bytecode runtime tests passed");
  return 0;
}
