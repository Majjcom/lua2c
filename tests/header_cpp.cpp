#include "entry.h"
#include "entry.h" // Verify the generated include guard.
#include <cstdio>
extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}
#ifndef LUA2C_TEST_ENTRY
#define LUA2C_TEST_ENTRY lua2c_main
#endif

int main() {
    lua_State* L = luaL_newstate();
    if (!L) return 1;
    luaL_openlibs(L);
#if defined(LUA2C_API) && LUA2C_API
    lua_pushcfunction(L, luaopen_entry);
#else
    lua_pushcfunction(L, LUA2C_TEST_ENTRY);
#endif
    lua_pushliteral(L, "normal");
    lua_pushinteger(L, 7);
    int status = lua_pcall(L, 2, 2, 0);
    bool valid = status == LUA_OK && lua_isfunction(L, -2);
    if (valid) {
        lua_pop(L, 1);
        lua_pushinteger(L, 3);
        status = lua_pcall(L, 1, 1, 0);
        valid = status == LUA_OK && lua_tointeger(L, -1) == 10;
    }
    if (!valid) std::fputs("generated header C++ linkage test failed\n", stderr);
    lua_close(L);
    return valid ? 0 : 1;
}
