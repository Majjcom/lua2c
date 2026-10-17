#include <stdio.h>
#include <string.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "entry.h"

#ifndef LUA2C_TEST_ENTRY
#define LUA2C_TEST_ENTRY lua2c_main
#endif

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "host check failed at line %d\n", __LINE__); lua_close(L); return 1; } } while (0)
int main(void) {
  int iteration;
  for (iteration = 0; iteration < 8; ++iteration) {
    int results, status;
    lua_State *co;
    lua_State *L = luaL_newstate();
    if (!L) return 1;
    luaL_openlibs(L);
    lua_gc(L, iteration % 2 ? LUA_GCGEN : LUA_GCINC, 0, 0, 0);
    lua_pushcfunction(L, LUA2C_TEST_ENTRY);
    lua_pushliteral(L, "normal");
    lua_pushinteger(L, 7);
    CHECK(lua_pcall(L, 2, 2, 0) == LUA_OK);
    CHECK(lua_isfunction(L, -2));
    CHECK(strcmp(lua_tostring(L, -1), "normal") == 0);
    lua_pop(L, 1);
    lua_gc(L, LUA_GCCOLLECT);
    lua_pushinteger(L, 5);
    CHECK(lua_pcall(L, 1, 1, 0) == LUA_OK);
    CHECK(lua_tointeger(L, -1) == 12);
    lua_settop(L, 0);
    lua_pushcfunction(L, LUA2C_TEST_ENTRY);
    lua_pushliteral(L, "error");
    lua_pushinteger(L, 0);
    CHECK(lua_pcall(L, 2, 0, 0) == LUA_ERRRUN);
    CHECK(strstr(lua_tostring(L, -1), "entry expected") != NULL);
    lua_settop(L, 0);
    co = lua_newthread(L);
    lua_pushcfunction(co, LUA2C_TEST_ENTRY);
    lua_pushliteral(co, "yield");
    lua_pushinteger(co, 9);
    status = lua_resume(co, L, 2, &results);
    CHECK(status == LUA_YIELD && results == 2);
    CHECK(strcmp(lua_tostring(co, -2), "paused") == 0 && lua_tointeger(co, -1) == 9);
    lua_settop(co, 0);
    lua_pushinteger(co, 123);
    status = lua_resume(co, L, 1, &results);
    CHECK(status == LUA_OK && results == 2);
    CHECK(lua_tointeger(co, -2) == 123 && lua_tointeger(co, -1) == 9);
    lua_close(L);
  }
  puts("host entry tests passed");
  return 0;
}
