# lua2c

将 Lua 5.4 程序的 bytecode 转换为原生 C 代码的 C++17 库与命令行工具。
源码输入先由附带的 Lua 5.4.9 编译器编译成 bytecode，再走相同的转换流程；转换阶段不会执行输入程序。

## 项目结构

```text
lua/                 Lua 5.4.9 依赖及其构建配置
  lua/               Lua 内核、标准库与头文件（包含 lua2c 扩展）
  exec/              lua / luac 的入口源码
runtime/             lua2c 的 C 运行时与 VM 辅助模板
include/lua2c/       C++ 转换库公共接口
src/                 字节码解析、校验、C 代码生成及 CLI
cmake/               VM 模板生成与安装包配置
tests/               单元、差分及 C/C++ 宿主测试
examples/            库调用示例
```

Lua 内核扩展包括 `Proto::lua2c_entry`、新原型初始化、VM 原生调度入口与共享库内部符号导出。
5.4.9 的上游修复保留不变。运行时 ABI 已更新为 `0x00010009`：升级后应重新生成、编译 C 文件及 DLL，
不要混用旧版本生成代码、头文件和运行时。若复用旧构建目录，需重新运行 CMake 配置；安装接口路径不变。

## 构建

需要 CMake 3.20+、C99/C++17 编译器。

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
ctest --test-dir build --output-on-failure -j 8
```

`vcvars64.bat` 路径按实际安装位置调整。多配置生成器使用 `--config Release` 和 `ctest -C Release`。

输出：

- `build/bin/lua2c.exe`：转换工具。
- `build/lib/lua2c.lib`：C++ 转换库。
- `build/bin/lualib.dll`、`build/lib/lualib.lib`：配套 C 运行时及导入库。
- `build/bin/lua.exe`、`luac.exe`：配套 Lua 主程序和字节码编译器。

可选配置：

| 选项 | 默认值 | 用途 |
|---|---|---|
| `LUA2C_SHARED_RUNTIME` | ON | 配套 Lua 运行时构建为共享库；DLL 模块必须使用共享运行时 |
| `BUILD_SHARED_LIBS` | OFF | C++ 转换库构建为共享库 |
| `LUA2C_BUILD_CLI` | ON | 构建 CLI |
| `LUA2C_BUILD_TESTS` | 跟随 BUILD_TESTING | 构建并运行生成代码测试 |
| `LUA2C_RUNTIME_ASSERTIONS` | OFF | 启用 Lua 内部断言及 API 检查 |

静态运行时的断言测试配置：

```bat
cmake -S . -B build-static -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLUA2C_SHARED_RUNTIME=OFF -DLUA2C_RUNTIME_ASSERTIONS=ON
cmake --build build-static -j 8
ctest --test-dir build-static --output-on-failure -j 8
```

## 转换

运行 `lua2c --help` 或 `lua2c -h` 可查看完整参数说明、默认值和使用示例。

```bat
build\bin\lua2c.exe -o program.c script.lua
build\bin\lua2c.exe --bytecode -o program.c script.luac
build\bin\lua2c.exe --module mymodule -o mymodule.c module.lua
build\bin\lua2c.exe --header --module mymodule -o mymodule.c module.lua
build\bin\lua2c.exe --main-name run_script --header -o program.c script.lua
build\bin\lua2c.exe --strip -o program.c script.lua
build\bin\lua2c.exe --strip --no-bytecode -o program.c script.lua
```

输入格式默认自动识别；可使用 `--source` / `--bytecode` 强制指定。
`--chunk-name` 设置源码调试名称；文件输入默认使用 `@文件路径`。
`--strip`（或 `-s`）去除源码或字节码输入的 Lua 调试信息，默认保留。
行为与 `luac -s` 一致：递归移除源码名称、行号映射、局部变量元数据和 upvalue 名称，
保留函数定义起止行、指令、常量及执行所需元数据。traceback、行号 hook、debug API 和名称恢复
可用的信息会减少；这不是代码混淆。`--chunk-name` 仍用于源码编译错误，但不会保留在 strip 后的输出中。
公开入口名、模块名和生成的头文件不变；运行时通过 `load` / `loadfile` 编译的代码不会自动 strip。

`--no-bytecode` 不在生成的 C 中保存指令数组，运行时也不分配或重建指令数组。
此选项递归作用于源码和字节码输入，与 `--strip` 独立；默认仍保留数组。
原生执行和协程/元方法恢复使用整数位置与生成的恢复函数；调用、返回、计数/行号 hook
（包括 C hook 挂起）仍受支持，源码行号、局部变量和 upvalue 元数据在未 strip 时保留。
依赖指令分析的调用名称推断、错误信息中的符号化变量描述会受限。
对这些原生函数调用 `string.dump` 会报 `unable to dump given function`；
C API `lua_dump` 返回失败，且不会调用 writer。运行时通过 `load` / `loadfile` 编译的函数仍有普通字节码，可正常 dump。
常量、原生指令操作数及未 strip 的调试名称仍然存在，因此不保证无法逆向。
本次运行时 ABI 已升级，已有的生成 C 文件及模块需要重新生成、编译。

不指定 `-o` 时将生成代码写入 stdout。模块名必须是合法 C 标识符，默认 `lua2c_module`。
`--header` 必须配合 `-o`，额外生成同目录、同名的 `.h` 文件（替换输出扩展名），默认不生成。
例如 `-o mymodule.c --header` 输出 `mymodule.c` 和 `mymodule.h`。
`--main-name NAME` 设置 chunk 入口函数的实际名称，默认 `lua2c_main`；支持文件输出和 stdout 输出。
名称必须是合法且非关键字的 C/C++ 标识符，不能与 `main`、模块初始化入口或生成的内部符号冲突，
也应避免使用已有的 Lua/C API 名称。

生成文件提供：

```c
int lua2c_main(lua_State *L);
/* 每个 Lua 函数原型对应一个文件内函数： */
static int lua2c_gen_<name>_p<id>(lua_State *L, int trap);
```

有调试信息时尽量恢复局部函数、全局函数及字段名称；匿名函数和 stripped bytecode 使用稳定编号。
这些 `lua2c_gen_*` 是运行时执行入口，需要真实 Lua 闭包及调用帧，**不是**可以随意 `lua_pushcfunction` 注册的普通 C 函数。

`lua2c_main` 是普通 Lua C 入口：栈上的值作为顶层 chunk 的 `...`，返回值留在栈上，返回其数量。
指定 `--main-name run_script` 后，实际定义变为 `int run_script(lua_State *L)`，不再额外定义 `lua2c_main` 别名；
调用约定、错误和协程行为不变。此选项不修改 `luaopen_<module_name>` 或 `lua2c_gen_*` 的名称。
宿主负责创建 `lua_State`、开放标准库和设置环境。通常通过 `lua_pushcfunction` + `lua_pcall` 调用，
错误使用 Lua 的正常错误机制；协程中可经 `lua_resume` 调用并挂起/恢复。参见 `tests/host.c`。

### 生成 C API 头文件

使用 `--header` 后，C 或 C++ 宿主可直接包含生成的头文件：

```c
#include "mymodule.h"

/* 初始化 Lua 状态、标准库后，在受保护的调用中执行脚本。 */
lua_pushcfunction(L, lua2c_main);
if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
    /* 处理 Lua 错误。 */
}
```

头文件包含公开的 `lua.h`、include guard 和 C++ `extern "C"` 保护。
头文件声明配置后的入口名称（默认 `lua2c_main`）；在 `LUA2C_API=1` 时还声明 `luaopen_<module_name>`。
调用模块初始化入口时，生成 C 与调用方均应启用 `LUA2C_API=1`。
文件内的 `lua2c_gen_*` 不属于公开 C API，因此不会出现在头文件中。
头文件不会改变生成 C 的内容，也不会自动启用 `LUA2C_MAIN` 或 `LUA2C_API`。
宿主仍需编译/链接生成的 C 文件与配套运行时，仅包含头文件并不足以执行程序。
例如使用 `--main-name run_script --header` 后，宿主应调用 `lua_pushcfunction(L, run_script)`。
多个生成 C 文件可以通过不同的入口名一起链接，其头文件的 include guard 也会区分入口名称；
若同时开启 `LUA2C_API`，还需要通过 `--module` 为各个 DLL/模块入口指定不同名称。

### 编译为可执行程序

两个宏均默认是 `0`：

- `LUA2C_MAIN=1`：启用 `main`，初始化标准库、`arg`、chunk 参数及错误处理。
- `LUA2C_API=1`：启用并导出 `luaopen_<module_name>`。
- 两者允许同时启用；两者都关闭时适合嵌入宿主。

安装头文件和库：

```bat
cmake --install build --prefix install
```

使用 MSVC 编译生成文件（共享运行时）：

```bat
cl /nologo /O2 /MD /std:c11 /DLUA2C_MAIN=1 /DLUA_BUILD_AS_DLL /DLUA2C_RUNTIME_SHARED /Iinstall\include\lua2c\runtime program.c install\lib\lualib.lib /Fe:install\bin\program.exe
install\bin\program.exe arg1 arg2
```

生成代码只依赖 C 运行时和配套头文件，不链接 C++ 转换库。
可执行程序的 `arg[0]` 是其自身路径，`arg[1..]` 同时作为 chunk 的 `...`，与执行 Lua 脚本时的参数约定一致。

### 编译为 Lua 模块

```bat
build\bin\lua2c.exe --module mymodule -o mymodule.c module.lua
cl /nologo /O2 /MD /LD /DLUA2C_API=1 /DLUA_BUILD_AS_DLL /DLUA2C_RUNTIME_SHARED /Iinstall\include\lua2c\runtime mymodule.c install\lib\lualib.lib /Fe:install\bin\mymodule.dll
install\bin\lua.exe -e "package.cpath='install/bin/?.dll;'..package.cpath; local m=require('mymodule')"
```

模块执行脚本并保留其返回行为：通常脚本返回一个表，但并不强制如此。
宿主和模块必须链接**同一份配套运行时**，不能各自静态链接一份 Lua，不能用任意原版 Lua DLL 加载。
生成代码包含运行时版本、结构大小及数值表示的 ABI 检查。

### GCC / Clang / TinyCC

生成代码使用 C99 的普通分支、标签和函数，不使用 computed goto、嵌套函数、VLA 或必需的编译器 builtin。
下面是使用已安装共享运行时的示例，**未进行这些编译器的实测**：

```sh
cc -std=c99 -O2 -DLUA2C_MAIN=1 -DLUA2C_RUNTIME_SHARED \
  -Iinstall/include/lua2c/runtime program.c -Linstall/lib -llualib -lm -ldl -o program
tcc -DLUA2C_MAIN=1 -DLUA2C_RUNTIME_SHARED \
  -Iinstall/include/lua2c/runtime program.c -Linstall/lib -llualib -lm -ldl -o program
```

Linux 需设置 `LD_LIBRARY_PATH` 或 rpath；Windows 的 TinyCC 还需 `LUA_BUILD_AS_DLL`、
同位数运行时和 TinyCC 可识别的导入库。MSVC `.lib` 不保证可直接用于 TinyCC，必要时使用 DLL 的导出列表生成对应导入库。

## C++ 库接口

公共头文件：`include/lua2c/lua2c.hpp`。

```cpp
#include <lua2c/lua2c.hpp>

lua2c::Options options;
options.module_name = "example";
// 可选；省略时依然生成 lua2c_main。
options.main_name = "run_example";
// 可选；去除源码或字节码输入的 Lua 调试信息，默认 false。
options.strip = true;
// 可选；与 strip 独立，不保留字节码数组，原生函数不再支持 dump，默认 false。
options.no_bytecode = true;

std::string c = lua2c::convert("print(1 + 2)", options);
lua2c::convert_to_file("print(1 + 2)", "example.c", options);
// 第四个参数 generate_header 默认 false；true 时同时生成 example.h。
lua2c::convert_to_file("print(1 + 2)", "example.c", options, true);

// 文件输入：支持 Lua 源码及 .luac。
std::string from_file = lua2c::convert_file("script.lua", options);
lua2c::convert_file_to_file("script.luac", "script.c", options);
lua2c::convert_file_to_file("script.luac", "script.c", options, true); // 额外生成 script.h
```

字符串输入使用 `std::string_view` 的显式长度，能够包含二进制零字节。
转换、解析及文件错误抛出 `lua2c::Error`；分配错误遵循 C++ 标准异常约定。
转换完成前不打开输出文件，非法输入不会覆盖已有输出文件；写入失败不保证文件原子性。
启用头文件输出时，C 与头文件均在转换成功后写入，但两文件写入不是原子事务；I/O 错误可能只留下其中一份。
此时 C 输出路径不能以 `.h` / `.H` 结尾，以免和头文件路径冲突。
各次转换使用独立 Lua 状态，可在不同线程中转换不同输入；同一个 `lua_State` 的执行仍遵循 Lua 的宿主同步规则。

使用安装后的 CMake 包：

```cmake
find_package(lua2c CONFIG REQUIRED)
target_link_libraries(your_cpp_target PRIVATE lua2c::lua2c)

# 生成的 C 文件：
target_link_libraries(your_c_target PRIVATE lua2c::runtime)
target_compile_definitions(your_c_target PRIVATE LUA2C_MAIN=1)
```

`examples/library/` 提供可通过 `-DCMAKE_PREFIX_PATH=安装目录` 构建的完整库使用示例。

## 执行模型与语义

- 有界解析 Lua 5.4 bytecode，校验头、计数、常量、寄存器、upvalue、跳转及指令配对。
- 每个 `Proto` 生成独立的 C 执行函数。指令和操作数成为编译期常量；普通控制流直接跳转，入口 `switch` 仅处理进入/恢复位置。
- 直接访问 Lua 栈、`TValue`、表及 upvalue；整数回绕、浮点运算、比较、位运算等使用 Lua 语义的 C 快路径。
- 保留真实 `LClosure`、Lua 调用帧、GC 根与屏障，复用内核的元方法、字符串、关闭变量及错误展开机制。
- 已转换函数通过原生 C 路径执行，不是嵌入 `.luac` 后调用 `lua_load` 的解释器包装。
- 默认保留指令元数据以支持恢复元方法操作和 `string.dump`；`no_bytecode` 改用生成的恢复函数，不支持 dump 原生函数。未启用 `strip` 时保留调试元数据。dump 后重新 load 的函数默认由原 VM 执行，不自动重新编译。
- 运行时 `load` / `loadfile` / 动态模块可与生成函数互相调用。

完整语义是设计目标，不是已对 Lua 所有行为进行形式化证明。C 模块入口和 chunk 入口（默认 `lua2c_main`）本身是额外的宿主 C 边界，
`debug` 可观察到这些边界；生成的 Lua 函数本身仍报告为 `what="Lua"`。
`collectgarbage("count")`、时间和内存占用等与原解释器不保证相同。

限制：

- bytecode 的指令大小、整数/浮点配置和字节序必须与转换器兼容；不接受 Lua 5.1/5.2/5.3/5.5 格式。
- 不恢复已经被 stripped 删除的函数名、局部名和行号。
- 内核 ABI 已扩展，不保证与原版 Lua 的内部结构兼容。
- 解析器有结构校验和 200 层原型嵌套上限，但并非恶意 bytecode 的完整类型/数据流验证器；不要把转换执行环境当作不可信程序的安全沙箱。
- 大型程序生成的 C 文件可能较大；当前没有函数拆分或跨模块链接优化。

## 测试

MSVC x64 已测试：

- Release、共享运行时：243 项 CTest。
- Debug、静态运行时、Lua 内部断言/API 检查开启：241 项 CTest（不包含 DLL require 测试）。

包括原有 24 个脚本的源码、bytecode、stripped 路径，以及协程/元方法 yield、尾调用、`<close>`、
GC、动态加载、dump/load、精确 debug hooks、数值边界、C 宿主入口与两个宏的组合。
差分参照为配套 Lua 的普通解释执行路径。
无字节码模式覆盖源码/字节码输入、与 strip 组合、运行时无指令数组、协程/元方法恢复、
可变参数及开放结果调用的精确 hook、C hook 挂起、GC、调用 writer 前拒绝 dump、
解释函数的正常 dump/load、大操作数与无字节码共享模块。
另有手工构造的 `LOADKX` bytecode 与 600 元素/字段的大操作数测试。
头文件输出覆盖源码/bytecode、CLI、默认关闭、路径冲突与 C/C++ 宿主链接测试。
自定义入口覆盖源码/bytecode、C/C++ 调用、协程恢复、模块加载、双宏启用，以及多个生成文件同时链接。
strip 测试对照 Lua 自带的 stripped dump，覆盖嵌套闭包和绝对行号信息。
所有差分脚本还会分别对源码使用 `--strip`、未 strip 字节码使用 `-s`，以 `luac -s` 输出为参照；
另有 strip 后的 debug API、dump/load、头文件不变、非法输入，以及自定义入口宿主/模块的测试。

速度取决于工作负载及 C 编译器优化。优先使用 Release；Debug 和 TinyCC 缺少优化时不承诺加速。
历史测量（升级至 5.4.9 前）：本机 MSVC x64 Release 对 `24_arith_bench.lua` 的 7 次进程级运行中位数为：普通 Lua VM **106.82 ms**，
生成 C **58.63 ms**，约 **1.82 倍**。该数字包含进程启动，仅代表此算术工作负载。

## 许可证

附带 Lua 及从其 `lvm.c` 派生的指令处理代码遵循 Lua MIT 许可证，版权声明见 `lua/lua/lua.h`。
本项目新增代码遵循 `LICENSE` 中的 MIT 许可证。分发生成代码时应保留 Lua 许可证。
