#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lua2c::detail {
struct Constant {
    int tag = 0;
    std::string bytes;
};
struct Upvalue {
    int instack, idx, kind;
    std::optional<std::string> name;
};
struct Local {
    std::optional<std::string> name;
    int startpc, endpc;
};
struct Proto {
    std::optional<std::string> source;
    int linedefined = 0, lastlinedefined = 0;
    int numparams = 0, is_vararg = 0, maxstacksize = 0;
    std::vector<std::uint32_t> code;
    std::vector<Constant> constants;
    std::vector<Upvalue> upvalues;
    std::vector<Proto> children;
    std::vector<int> lineinfo;
    std::vector<std::pair<int, int>> abslineinfo;
    std::vector<Local> locals;
    std::size_t id = 0;
    std::string name;
};
Proto parse_bytecode(std::string_view input);
void validate(const Proto& proto, const Proto* parent = nullptr);
std::string emit(Proto proto, const std::string& module_name, const std::string& main_name, bool no_bytecode = false);
}
