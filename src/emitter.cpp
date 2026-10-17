#include "ir.hpp"
#include "lua2c/lua2c.hpp"
#include "vm_templates.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <locale>
#include <sstream>
extern "C" {
#include "lua.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lopnames.h"
}

namespace lua2c::detail {
namespace {
void replace(std::string& text, const std::string& from, const std::string& to) {
    std::size_t at = 0;
    while ((at = text.find(from, at)) != std::string::npos) {
        text.replace(at, from.size(), to);
        at += to.size();
    }
}
bool identifier(const std::string& s) {
    const auto alpha = [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; };
    if (s.empty() || !alpha(static_cast<unsigned char>(s[0]))) return false;
    return std::all_of(s.begin(), s.end(), [&](unsigned char c) { return alpha(c) || (c >= '0' && c <= '9'); });
}
void validate_main_name(const std::string& name, const std::string& module_name) {
    if (!identifier(name)) throw Error("main_name must be a C identifier");
    // Headers support both C and C++; do not emit a keyword in either language.
    constexpr std::string_view keywords[] = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
        "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept",
        "const", "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return", "co_yield",
        "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern",
        "false", "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new",
        "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public",
        "register", "reinterpret_cast", "requires", "restrict", "return", "short", "signed", "sizeof", "static",
        "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local", "throw", "true",
        "try", "typedef", "typeid", "typename", "typeof", "typeof_unqual", "union", "unsigned", "using",
        "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq", "_Alignas", "_Alignof", "_Atomic",
        "_BitInt", "_Bool", "_Complex", "_Decimal32", "_Decimal64", "_Decimal128", "_Generic", "_Imaginary",
        "_Noreturn", "_Pragma", "_Static_assert", "_Thread_local"
    };
    for (const auto keyword : keywords)
        if (name == keyword) throw Error("main_name must not be a C/C++ keyword");
    constexpr std::string_view reserved[] = {
        "main", "lua2c_check_runtime", "lua2c_verify", "lua2c_integer_probe", "lua2c_number_probe",
        "lua2c_checkabi", "lua2c_pushchunk", "lua2c_finishmain", "lua2c_cli",
        "forlimit", "forprep", "floatforloop", "pushclosure", "LTintfloat", "LEintfloat", "LTfloatint", "LEfloatint", "LTnum", "LEnum"
    };
    for (const auto symbol : reserved)
        if (name == symbol) throw Error("main_name conflicts with an internal or program entry symbol");
    constexpr std::string_view prefixes[] = {
        "lua2c_gen_", "lua2c_proto_", "lua2c_bytes_", "lua2c_code_", "lua2c_constants_", "lua2c_upvalues_",
        "lua2c_children_", "lua2c_lines_", "lua2c_abslines_", "lua2c_locals_", "LUA2C_"
    };
    for (const auto prefix : prefixes)
        if (name.compare(0, prefix.size(), prefix) == 0) throw Error("main_name uses a reserved generated-symbol prefix");
    if (name == "luaopen_" + module_name) throw Error("main_name conflicts with the module initializer");
}
std::string clean(std::string s) {
    for (auto& c : s) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) c = '_';
    }
    if (s.empty()) s = "anonymous";
    if (s[0] >= '0' && s[0] <= '9') s.insert(s.begin(), '_');
    if (s.size() > 80) s.resize(80);
    return s;
}
void assign_names(Proto& p, std::size_t& id, std::vector<Proto*>& protos) {
    p.id = id++;
    if (p.name.empty()) p.name = "anonymous";
    p.name = "lua2c_gen_" + clean(p.name) + "_p" + std::to_string(p.id);
    protos.push_back(&p);
    for (std::size_t pc = 0; pc < p.code.size(); ++pc) {
        const Instruction i = p.code[pc];
        if (GET_OPCODE(i) != OP_CLOSURE) continue;
        auto& child = p.children[GETARG_Bx(i)];
        int slot = 0;
        for (const auto& local : p.locals) {
            if (local.startpc <= static_cast<int>(pc + 1) && local.endpc > static_cast<int>(pc + 1)) {
                if (slot++ == GETARG_A(i) && local.name) child.name = *local.name;
            }
        }
        if (child.name.empty() && pc + 1 < p.code.size()) {
            const Instruction next = p.code[pc + 1];
            if ((GET_OPCODE(next) == OP_SETTABUP || GET_OPCODE(next) == OP_SETFIELD) &&
                !TESTARG_k(next) && GETARG_C(next) == GETARG_A(i)) {
                const auto& k = p.constants[GETARG_B(next)];
                child.name = k.bytes;
            }
        }
    }
    for (auto& child : p.children) assign_names(child, id, protos);
}

std::array<std::string, NUM_OPCODES> handlers() {
    std::array<std::string, NUM_OPCODES> result;
    const std::string source = vm_source;
    for (int op = 0; op < NUM_OPCODES; ++op) {
        const std::string marker = "      vmcase(OP_" + std::string(opnames[op]) + ")";
        const auto begin = source.find(marker);
        if (begin == std::string::npos) throw Error("missing bundled VM handler: " + marker);
        auto end = source.find("      vmcase(", begin + marker.size());
        if (end == std::string::npos) end = source.find("\n    }", begin);
        result[op] = source.substr(begin + marker.size(), end - begin - marker.size());
        replace(result[op], "lessthanothers", "luaV_lessthan");
        replace(result[op], "lessequalothers", "luaV_lessequal");
        replace(result[op], "l_tforloop:", "");
    }
    return result;
}

std::string jump(int target) { return "goto lua2c_pc_" + std::to_string(target) + ";"; }
std::string next_jump(const Proto& p, int pc) {
    const Instruction i = p.code[pc];
    const int op = GET_OPCODE(i);
    int next = pc + 1;
    if (op == OP_LOADKX || op == OP_NEWTABLE || op == OP_LFALSESKIP || (op == OP_SETLIST && TESTARG_k(i))) next++;
    if (op == OP_JMP) return jump(pc + 1 + GETARG_sJ(i));
    if ((op >= OP_ADDI && op <= OP_SHR) || (op >= OP_EQ && op <= OP_TESTSET) ||
        op == OP_FORLOOP || op == OP_FORPREP || op == OP_TFORLOOP) {
        int first = pc + 1, second;
        if (op >= OP_ADDI && op <= OP_SHR) second = pc + 2;
        else if (op >= OP_EQ && op <= OP_TESTSET) {
            first = pc + 2;
            second = pc + 2 + GETARG_sJ(p.code[pc + 1]);
        } else if (op == OP_FORPREP) second = pc + GETARG_Bx(i) + 2;
        else second = pc + 1 - GETARG_Bx(i);
        return "if (pc == cl->p->code + " + std::to_string(first) + ") { " + jump(first) + " } else { " + jump(second) + " }";
    }
    return jump(next);
}

void emit_function(std::ostream& out, const Proto& p, const std::array<std::string, NUM_OPCODES>& bodies) {
    out << "\nstatic int " << p.name << "(lua_State *L) {\n"
           "  CallInfo *ci = L->ci;\n"
           "  LClosure *cl = ci_func(ci);\n"
           "  TValue *k = cl->p->k;\n"
           "  StkId base = ci->func.p + 1;\n"
           "  const Instruction *pc = ci->u.l.savedpc;\n"
           "  int trap = ci->u.l.trap;\n"
           "  (void)k; (void)base;\n"
           "  /* Dispatch only when entering/resuming a frame, never per opcode. */\n"
           "  switch (pc - cl->p->code) {\n";
    for (int pc = 0; pc < static_cast<int>(p.code.size()); ++pc)
        if (GET_OPCODE(p.code[pc]) != OP_EXTRAARG) out << "    case " << pc << ": " << jump(pc) << "\n";
    out << "    default: luaG_runerror(L, \"invalid native resume position\");\n  }\n";
    const std::string ret = "return (ci->callstatus & CIST_FRESH) ? LUA2C_BOUNDARY_RETURN : LUA2C_RETURN_TO_CALLER;";
    for (int index = 0; index < static_cast<int>(p.code.size()); ++index) {
        const Instruction i = p.code[index];
        const int op = GET_OPCODE(i);
        if (op == OP_EXTRAARG) continue;
        out << "lua2c_pc_" << index << ":\n"
               "  pc = cl->p->code + " << index << ";\n"
               "  if (l_unlikely(trap)) {\n"
               "    trap = luaG_traceexec(L, pc);\n"
               "    updatebase(ci);\n"
               "  }\n"
               "  pc = cl->p->code + " << index + 1 << ";\n"
               "lua2c_body_" << index << ": {\n"
               "  const Instruction i = " << i << "u; /* " << opnames[op] << " */\n"
               "  (void)i;\n";
        if (!isIT(i)) out << "  L->top.p = base;\n";
        if (op == OP_TFORPREP) {
            const int target = index + 1 + GETARG_Bx(i);
            out << "  StkId ra = RA(i);\n  halfProtect(luaF_newtbcupval(L, ra + 3));\n"
                   "  pc = cl->p->code + " << target + 1 << ";\n"
                   "  goto lua2c_body_" << target << ";\n";
        } else if (op == OP_TFORCALL) {
            out << "  StkId ra = RA(i);\n"
                   "  memcpy(ra + 4, ra, 3 * sizeof(*ra));\n"
                   "  L->top.p = ra + 7;\n"
                   "  ProtectNT(luaD_call(L, ra + 4, GETARG_C(i)));\n"
                   "  updatestack(ci);\n"
                   "  pc = cl->p->code + " << index + 2 << ";\n"
                   "  goto lua2c_body_" << index + 1 << ";\n";
        } else {
            std::string body = bodies[op];
            if (op == OP_RETURN1) {
                const auto label = body.find("       ret:");
                if (label == std::string::npos) throw Error("bundled RETURN1 handler changed");
                body.erase(label);
                body += "\n" + ret + "\n}\n";
            }
            replace(body, "goto startfunc;", "return LUA2C_NEW_FRAME;");
            replace(body, "goto ret;", ret);
            replace(body, "vmbreak;", next_jump(p, index));
            replace(body, "last += GETARG_Ax(*pc)", "last += (unsigned int)GETARG_Ax(*pc)");
            // Make metadata reads for EXTRAARG and metamethod operands constant.
            if (body.find("GETARG_Ax(*pc)") != std::string::npos && index + 1 < static_cast<int>(p.code.size()))
                replace(body, "GETARG_Ax(*pc)", std::to_string(getarg(p.code[index + 1], POS_Ax, SIZE_Ax)));
            if (index > 0) replace(body, "*(pc - 2)", std::to_string(p.code[index - 1]) + "u");
            out << body;
        }
        out << "}\n";
    }
    out << "}\n";
}

class Metadata {
    std::ostream& out_;
    std::size_t serial_ = 0;
public:
    explicit Metadata(std::ostream& out) : out_(out) {}
    std::string bytes(const std::optional<std::string>& value) {
        if (!value) return "{NULL, 0}";
        const std::string name = "lua2c_bytes_" + std::to_string(serial_++);
        out_ << "static const unsigned char " << name << "[] = {";
        if (value->empty()) out_ << '0';
        for (std::size_t i = 0; i < value->size(); ++i) {
            if (i % 24 == 0) out_ << '\n';
            out_ << static_cast<unsigned>(static_cast<unsigned char>((*value)[i])) << ',';
        }
        out_ << "\n};\n";
        return "{" + name + ", " + std::to_string(value->size()) + "}";
    }
    void proto(const Proto& p) {
        const auto suffix = std::to_string(p.id);
        const auto source = bytes(p.source);
        std::vector<std::string> constants, upvalues, locals;
        for (const auto& k : p.constants)
            constants.push_back("{" + std::to_string(k.tag) + ", " +
                bytes(k.tag == LUA_VNUMINT || k.tag == LUA_VNUMFLT || k.tag == LUA_VSHRSTR || k.tag == LUA_VLNGSTR
                    ? std::optional<std::string>(k.bytes) : std::nullopt) + "}");
        for (const auto& u : p.upvalues)
            upvalues.push_back("{" + std::to_string(u.instack) + ", " + std::to_string(u.idx) + ", " +
                std::to_string(u.kind) + ", " + bytes(u.name) + "}");
        for (const auto& l : p.locals)
            locals.push_back("{" + bytes(l.name) + ", " + std::to_string(l.startpc) + ", " + std::to_string(l.endpc) + "}");
        out_ << "static const Instruction lua2c_code_" << suffix << "[] = {\n";
        for (std::size_t i = 0; i < p.code.size(); ++i) {
            out_ << p.code[i] << "u,";
            if (i % 12 == 11) out_ << '\n';
        }
        out_ << "\n};\n";
        const auto array = [&](const std::string& type, const std::string& prefix, const std::vector<std::string>& elements) {
            if (elements.empty()) return;
            out_ << "static const " << type << " " << prefix << suffix << "[] = {\n";
            for (const auto& element : elements) out_ << "  " << element << ",\n";
            out_ << "};\n";
        };
        array("Lua2cConstant", "lua2c_constants_", constants);
        array("Lua2cUpvalue", "lua2c_upvalues_", upvalues);
        array("Lua2cLocal", "lua2c_locals_", locals);
        if (!p.children.empty()) {
            out_ << "static const Lua2cProto *const lua2c_children_" << suffix << "[] = {";
            for (const auto& child : p.children) out_ << "&lua2c_proto_" << child.id << ',';
            out_ << "};\n";
        }
        if (!p.lineinfo.empty()) {
            out_ << "static const signed char lua2c_lines_" << suffix << "[] = {";
            for (const auto line : p.lineinfo) out_ << line << ',';
            out_ << "};\n";
        }
        if (!p.abslineinfo.empty()) {
            out_ << "static const AbsLineInfo lua2c_abslines_" << suffix << "[] = {";
            for (const auto& line : p.abslineinfo) out_ << '{' << line.first << ',' << line.second << "},";
            out_ << "};\n";
        }
        const auto pointer = [&](std::size_t count, const std::string& prefix) { return count ? prefix + suffix : "NULL"; };
        out_ << "static const Lua2cProto lua2c_proto_" << suffix << " = {\n"
             << p.numparams << ',' << p.is_vararg << ',' << p.maxstacksize << ','
             << p.linedefined << ',' << p.lastlinedefined << ',' << source << ",\n"
             << p.code.size() << ",lua2c_code_" << suffix << ",\n"
             << p.constants.size() << ',' << pointer(p.constants.size(), "lua2c_constants_") << ",\n"
             << p.upvalues.size() << ',' << pointer(p.upvalues.size(), "lua2c_upvalues_") << ",\n"
             << p.children.size() << ',' << pointer(p.children.size(), "lua2c_children_") << ",\n"
             << p.lineinfo.size() << ',' << pointer(p.lineinfo.size(), "lua2c_lines_") << ",\n"
             << p.abslineinfo.size() << ',' << pointer(p.abslineinfo.size(), "lua2c_abslines_") << ",\n"
             << p.locals.size() << ',' << pointer(p.locals.size(), "lua2c_locals_") << ",\n"
             << p.name << "\n};\n";
    }
};
}

std::string emit(Proto p, const std::string& module_name, const std::string& main_name) {
    if (!identifier(module_name)) throw Error("module_name must be a C identifier");
    validate_main_name(main_name, module_name);
    std::vector<Proto*> protos;
    std::size_t id = 0;
    p.name = "chunk";
    assign_names(p, id, protos);
    const auto bodies = handlers();
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "/* Generated by lua2c. Native C execution; bytecode is retained only\n"
           "   as metadata for Lua debug hooks, yield completion and string.dump.\n"
           "   Instruction semantics derived from Lua 5.4.9; see lua.h license. */\n"
           "#include <stdio.h>\n#include \"lua2c_vm.h\"\n#include \"lauxlib.h\"\n"
           "#if LUA2C_ABI_VERSION != 0x00010009UL\n#error incompatible lua2c headers\n#endif\n"
           "#ifndef LUA2C_MAIN\n#define LUA2C_MAIN 0\n#endif\n"
           "#ifndef LUA2C_API\n#define LUA2C_API 0\n#endif\n";
    for (const auto* proto : protos)
        out << "static int " << proto->name << "(lua_State *L);\nstatic const Lua2cProto lua2c_proto_" << proto->id << ";\n";
    Metadata metadata(out);
    const lua_Integer integer = 0x5678;
    const lua_Number number = 370.5;
    const auto integer_probe = metadata.bytes(std::string(reinterpret_cast<const char*>(&integer), sizeof(integer)));
    const auto number_probe = metadata.bytes(std::string(reinterpret_cast<const char*>(&number), sizeof(number)));
    out << "static const Lua2cBytes lua2c_integer_probe = " << integer_probe << ";\n"
        << "static const Lua2cBytes lua2c_number_probe = " << number_probe << ";\n";
    for (const auto* proto : protos) metadata.proto(*proto);
    for (const auto* proto : protos) emit_function(out, *proto, bodies);
    const char* module_state = main_name == "L" ? "lua2c_state" : "L";
    out << "\nstatic void lua2c_check_runtime(lua_State *L) {\n"
           "  lua2c_checkabi(L, 0x00010009UL, sizeof(Proto), sizeof(lua_State), "
        << sizeof(lua_Integer) << ", " << sizeof(lua_Number) << ",\n"
           "    lua2c_integer_probe.data, lua2c_number_probe.data);\n}\n"
           "int " << main_name << "(lua_State *L) {\n"
           "  int nargs = lua_gettop(L);\n"
           "  lua2c_check_runtime(L);\n"
           "  lua2c_pushchunk(L, &lua2c_proto_0);\n"
           "  lua_insert(L, 1);\n"
           "  lua_callk(L, nargs, LUA_MULTRET, 0, lua2c_finishmain);\n"
           "  return lua_gettop(L);\n}\n"
           "#if LUA2C_API\nLUA2C_MODULE_EXPORT int luaopen_" << module_name << "(lua_State *" << module_state << ") {\n"
           "  return " << main_name << "(" << module_state << ");\n}\n#endif\n"
           "#if LUA2C_MAIN\n"
           "static int lua2c_verify(lua_State *L) { lua2c_check_runtime(L); return 0; }\n"
           "int main(int argc, char **argv) {\n"
           "  int result;\n"
           "  lua_State *L = luaL_newstate();\n"
           "  if (!L) { fputs(\"cannot create Lua state\\n\", stderr); return 1; }\n"
           "  lua_pushcfunction(L, lua2c_verify);\n"
           "  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {\n"
           "    fprintf(stderr, \"%s\\n\", lua_tostring(L, -1));\n"
           "    lua_close(L); return 1;\n  }\n"
           "  result = lua2c_cli(L, &lua2c_proto_0, argc, argv);\n"
           "  lua_close(L);\n  return result;\n}\n#endif\n";
    return out.str();
}
}
