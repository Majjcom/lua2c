#ifndef LUA2C_RUNTIME_H
#define LUA2C_RUNTIME_H
#include <stddef.h>
#include "lua.h"
#include "lobject.h"

#define LUA2C_ABI_VERSION 0x0001000AUL
#define LUA2C_BOUNDARY_RETURN 0
#define LUA2C_NEW_FRAME 1
#define LUA2C_RETURN_TO_CALLER 2

#if defined(_WIN32)
#define LUA2C_MODULE_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define LUA2C_MODULE_EXPORT __attribute__((visibility("default")))
#else
#define LUA2C_MODULE_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif
typedef struct Lua2cBytes {
  const unsigned char *data;  /* NULL means a missing string */
  size_t size;
} Lua2cBytes;
typedef struct Lua2cConstant {
  int tag;
  Lua2cBytes bytes;
} Lua2cConstant;
typedef struct Lua2cUpvalue {
  unsigned char instack, idx, kind;
  Lua2cBytes name;
} Lua2cUpvalue;
typedef struct Lua2cLocal {
  Lua2cBytes name;
  int startpc, endpc;
} Lua2cLocal;
typedef struct Lua2cProto {
  int numparams, is_vararg, maxstacksize;
  int linedefined, lastlinedefined;
  Lua2cBytes source;
  int sizecode;
  const Instruction *code;  /* NULL for no-bytecode native prototypes */
  int sizek;
  const Lua2cConstant *constants;
  int sizeupvalues;
  const Lua2cUpvalue *upvalues;
  int sizep;
  const struct Lua2cProto *const *children;
  int sizelineinfo;
  const signed char *lineinfo;
  int sizeabslineinfo;
  const AbsLineInfo *abslineinfo;
  int sizelocvars;
  const Lua2cLocal *locals;
  int (*entry)(lua_State *L, int trap);
  lua_CFunction resume;
} Lua2cProto;

LUA_API void lua2c_checkabi(lua_State *L, unsigned long version,
                          size_t proto_size, size_t state_size,
                          size_t integer_size, size_t number_size,
                          const unsigned char *integer_probe,
                          const unsigned char *number_probe);
/* Push a real Lua closure. Allocations/errors follow normal Lua conventions. */
LUA_API void lua2c_pushchunk(lua_State *L, const Lua2cProto *desc);
LUA_API int lua2c_finishmain(lua_State *L, int status, lua_KContext context);
LUA_API int lua2c_cli(lua_State *L, const Lua2cProto *desc, int argc, char **argv);
#ifdef __cplusplus
}
#endif
#endif
