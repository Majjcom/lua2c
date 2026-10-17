#include "entry.h"
#include "renamed/entry.h"
#include <cstdio>
extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

static bool call_entry(lua_State* L, lua_CFunction entry, lua_Integer value) {
    lua_settop(L, 0);
    lua_pushcfunction(L, entry);
    lua_pushliteral(L, "normal");
    lua_pushinteger(L, value);
    if (lua_pcall(L, 2, 2, 0) != LUA_OK || !lua_isfunction(L, -2)) return false;
    lua_pop(L, 1);
    lua_pushinteger(L, 5);
    return lua_pcall(L, 1, 1, 0) == LUA_OK && lua_tointeger(L, -1) == value + 5;
}

int main() {
    lua_State* L = luaL_newstate();
    if (!L) return 1;
    luaL_openlibs(L);
    const bool valid = call_entry(L, lua2c_main, 7) && call_entry(L, run_custom_chunk, 11);
    if (!valid) std::fputs("multiple generated entry points failed\n", stderr);
    lua_close(L);
    return valid ? 0 : 1;
}
