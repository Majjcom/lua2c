#include "ir.hpp"
#include "lua2c/lua2c.hpp"
#include <climits>
#include <cstring>
#include <limits>
extern "C" {
#include "lua.h"
#include "lobject.h"
#include "lopcodes.h"
}

namespace lua2c::detail {
namespace {
class Reader {
    std::string_view data_;
    std::size_t pos_ = 0;
public:
    explicit Reader(std::string_view data) : data_(data) {}
    [[noreturn]] void fail(const std::string& message) const {
        throw Error("bytecode offset " + std::to_string(pos_) + ": " + message);
    }
    std::string_view bytes(std::size_t size) {
        if (size > data_.size() - pos_) fail("truncated chunk");
        auto result = data_.substr(pos_, size);
        pos_ += size;
        return result;
    }
    unsigned byte() { return static_cast<unsigned char>(bytes(1)[0]); }
    std::size_t size(std::size_t limit = std::numeric_limits<std::size_t>::max()) {
        std::size_t value = 0;
        for (;;) {
            const unsigned b = byte();
            if (value >= (limit >> 7)) fail("size overflow");
            value = (value << 7) | (b & 127);
            if (b & 128) return value;
        }
    }
    int integer() { return static_cast<int>(size(INT_MAX)); }
    int count(std::size_t minimum = 1) {
        int n = integer();
        if (static_cast<std::size_t>(n) > (data_.size() - pos_) / minimum)
            fail("array count exceeds remaining input");
        return n;
    }
    template<class T> T scalar() {
        T result;
        const auto raw = bytes(sizeof(T));
        std::memcpy(&result, raw.data(), sizeof(T));
        return result;
    }
    std::optional<std::string> string() {
        auto n = size();
        if (n == 0) return std::nullopt;
        return std::string(bytes(n - 1));
    }
    Proto proto(int depth) {
        if (depth > 200) fail("prototype nesting limit exceeded");
        Proto p;
        p.source = string();
        p.linedefined = integer();
        p.lastlinedefined = integer();
        p.numparams = static_cast<int>(byte());
        p.is_vararg = static_cast<int>(byte());
        p.maxstacksize = static_cast<int>(byte());
        const int codes = count(sizeof(std::uint32_t));
        p.code.reserve(codes);
        for (int i = 0; i < codes; ++i) p.code.push_back(scalar<std::uint32_t>());
        const int constants = count();
        p.constants.reserve(constants);
        for (int i = 0; i < constants; ++i) {
            Constant c;
            c.tag = static_cast<int>(byte());
            switch (c.tag) {
            case LUA_VNIL: case LUA_VFALSE: case LUA_VTRUE: break;
            case LUA_VNUMINT: c.bytes = std::string(bytes(sizeof(lua_Integer))); break;
            case LUA_VNUMFLT: c.bytes = std::string(bytes(sizeof(lua_Number))); break;
            case LUA_VSHRSTR: case LUA_VLNGSTR: {
                auto s = string();
                if (!s) fail("null string constant");
                c.bytes = std::move(*s);
                if ((c.tag == LUA_VSHRSTR) != (c.bytes.size() <= LUAI_MAXSHORTLEN))
                    fail("string constant tag/length mismatch");
                break;
            }
            default: fail("unknown constant tag");
            }
            p.constants.push_back(std::move(c));
        }
        const int upvalues = count(3);
        if (upvalues > 255) fail("too many upvalues");
        for (int i = 0; i < upvalues; ++i) {
            Upvalue u;
            u.instack = static_cast<int>(byte());
            u.idx = static_cast<int>(byte());
            u.kind = static_cast<int>(byte());
            p.upvalues.push_back(std::move(u));
        }
        const int children = count();
        p.children.reserve(children);
        for (int i = 0; i < children; ++i) p.children.push_back(proto(depth + 1));
        const int lines = count();
        for (int i = 0; i < lines; ++i) {
            const int b = static_cast<int>(byte());
            p.lineinfo.push_back(b < 128 ? b : b - 256);
        }
        const int abs = count(2);
        for (int i = 0; i < abs; ++i) {
            const int pc = integer();
            const int line = integer();
            p.abslineinfo.emplace_back(pc, line);
        }
        const int locals = count(3);
        for (int i = 0; i < locals; ++i) {
            Local l;
            l.name = string();
            l.startpc = integer();
            l.endpc = integer();
            p.locals.push_back(std::move(l));
        }
        const int names = count();
        if (names != 0 && names != upvalues) fail("invalid upvalue name count");
        for (int i = 0; i < names; ++i) p.upvalues[i].name = string();
        return p;
    }
    bool done() const { return pos_ == data_.size(); }
};
}

Proto parse_bytecode(std::string_view input) {
    Reader r(input);
    if (r.bytes(4) != std::string_view("\x1bLua", 4)) r.fail("not Lua bytecode");
    if (r.byte() != 0x54 || r.byte() != 0) r.fail("expected Lua 5.4 format 0");
    if (r.bytes(6) != std::string_view("\x19\x93\r\n\x1a\n", 6)) r.fail("bad header");
    if (r.byte() != sizeof(Instruction) || r.byte() != sizeof(lua_Integer) ||
        r.byte() != sizeof(lua_Number)) r.fail("incompatible numeric sizes");
    if (r.scalar<lua_Integer>() != 0x5678 || r.scalar<lua_Number>() != 370.5)
        r.fail("incompatible endianness or numeric representation");
    const int upvalues = static_cast<int>(r.byte());
    Proto p = r.proto(0);
    if (upvalues != static_cast<int>(p.upvalues.size())) r.fail("root upvalue count mismatch");
    if (!r.done()) r.fail("trailing data");
    validate(p);
    return p;
}

void validate(const Proto& p, const Proto* parent) {
    const auto fail = [&](const std::string& message) {
        throw Error("invalid prototype at line " + std::to_string(p.linedefined) + ": " + message);
    };
    if (p.code.empty() || p.maxstacksize < 2 || p.numparams > p.maxstacksize ||
        p.is_vararg > 1 || p.lastlinedefined < p.linedefined)
        fail("invalid frame/header");
    for (const auto& u : p.upvalues) {
        if (u.instack > 1 || u.kind > 3) fail("invalid upvalue descriptor");
        if (parent && u.idx >= (u.instack ? parent->maxstacksize : static_cast<int>(parent->upvalues.size())))
            fail("upvalue index out of bounds");
    }
    const int n = static_cast<int>(p.code.size());
    if (!p.lineinfo.empty() && p.lineinfo.size() != p.code.size()) fail("invalid line info size");
    int previous = -1;
    std::size_t absindex = 0;
    for (const auto& a : p.abslineinfo) {
        if (a.first <= previous || a.first >= n || p.lineinfo.empty() || p.lineinfo[a.first] != -128)
            fail("invalid absolute line info");
        previous = a.first;
    }
    for (int pc = 0; pc < static_cast<int>(p.lineinfo.size()); ++pc) {
        if (p.lineinfo[pc] == -128) {
            if (absindex >= p.abslineinfo.size() || p.abslineinfo[absindex++].first != pc)
                fail("missing absolute line info");
        }
    }
    for (const auto& l : p.locals)
        if (!l.name || l.startpc > l.endpc || l.endpc > n) fail("invalid local debug range");

    const auto reg = [&](int r) { if (r < 0 || r >= p.maxstacksize) fail("register out of bounds"); };
    const auto constant = [&](int k) {
        if (k < 0 || k >= static_cast<int>(p.constants.size())) fail("constant out of bounds");
    };
    const auto stringk = [&](int k, bool shortonly = true) {
        constant(k);
        if (p.constants[k].tag != LUA_VSHRSTR &&
            (shortonly || p.constants[k].tag != LUA_VLNGSTR)) fail("expected string constant");
    };
    const auto up = [&](int u) { if (u >= static_cast<int>(p.upvalues.size())) fail("upvalue out of bounds"); };
    const auto target = [&](long long t) {
        if (t < 0 || t >= n || GET_OPCODE(p.code[static_cast<std::size_t>(t)]) == OP_EXTRAARG)
            fail("invalid control-flow target");
    };
    const auto extra = [&](int pc) {
        if (pc + 1 >= n || GET_OPCODE(p.code[pc + 1]) != OP_EXTRAARG) fail("missing EXTRAARG");
    };
    for (int pc = 0; pc < n; ++pc) {
        const Instruction i = p.code[pc];
        const int op = GET_OPCODE(i), a = GETARG_A(i);
        const int b = getarg(i, POS_B, SIZE_B), c = getarg(i, POS_C, SIZE_C);
        if (op >= NUM_OPCODES) fail("unknown opcode");
        const auto rk = [&](int k) { if (TESTARG_k(i)) constant(k); else reg(k); };
        switch (op) {
        case OP_MOVE: reg(a); reg(b); break;
        case OP_LOADI: case OP_LOADF: case OP_LOADFALSE: case OP_LFALSESKIP: case OP_LOADTRUE: reg(a); break;
        case OP_LOADK: reg(a); constant(GETARG_Bx(i)); break;
        case OP_LOADKX: reg(a); extra(pc); constant(GETARG_Ax(p.code[pc + 1])); break;
        case OP_LOADNIL: reg(a); reg(a + b); break;
        case OP_GETUPVAL: reg(a); up(b); break;
        case OP_SETUPVAL: reg(a); up(b); break;
        case OP_GETTABUP: reg(a); up(b); stringk(c); break;
        case OP_GETTABLE: reg(a); reg(b); reg(c); break;
        case OP_GETI: reg(a); reg(b); break;
        case OP_GETFIELD: reg(a); reg(b); stringk(c); break;
        case OP_SETTABUP: up(a); stringk(b); rk(c); break;
        case OP_SETTABLE: reg(a); reg(b); rk(c); break;
        case OP_SETI: reg(a); rk(c); break;
        case OP_SETFIELD: reg(a); stringk(b); rk(c); break;
        case OP_NEWTABLE:
            reg(a); extra(pc);
            if (b > 31 || static_cast<unsigned long long>(c) +
                (TESTARG_k(i) ? static_cast<unsigned long long>(GETARG_Ax(p.code[pc + 1])) * 256 : 0) > INT_MAX)
                fail("table size overflow");
            if ((!TESTARG_k(i)) != (GETARG_Ax(p.code[pc + 1]) == 0)) fail("invalid table extra argument");
            break;
        case OP_SELF:
            reg(a); reg(a + 1); reg(b);
            if (TESTARG_k(i)) stringk(c, false); else reg(c);
            break;
        case OP_ADDI: case OP_SHRI: case OP_SHLI: reg(a); reg(b); break;
        case OP_ADDK: case OP_SUBK: case OP_MULK: case OP_MODK: case OP_POWK: case OP_DIVK: case OP_IDIVK:
            reg(a); reg(b); constant(c);
            if (p.constants[c].tag != LUA_VNUMINT && p.constants[c].tag != LUA_VNUMFLT) fail("non-numeric arithmetic constant");
            break;
        case OP_BANDK: case OP_BORK: case OP_BXORK:
            reg(a); reg(b); constant(c);
            if (p.constants[c].tag != LUA_VNUMINT) fail("non-integer bitwise constant");
            break;
        case OP_ADD: case OP_SUB: case OP_MUL: case OP_MOD: case OP_POW: case OP_DIV: case OP_IDIV:
        case OP_BAND: case OP_BOR: case OP_BXOR: case OP_SHL: case OP_SHR: reg(a); reg(b); reg(c); break;
        case OP_MMBIN: case OP_MMBINI: case OP_MMBINK:
            reg(a);
            if (op == OP_MMBIN) reg(b);
            if (op == OP_MMBINK) constant(b);
            if (c < 6 || c > 17 || pc == 0 || GET_OPCODE(p.code[pc - 1]) < OP_ADDI || GET_OPCODE(p.code[pc - 1]) > OP_SHR)
                fail("invalid arithmetic metamethod companion");
            break;
        case OP_UNM: case OP_BNOT: case OP_NOT: case OP_LEN: reg(a); reg(b); break;
        case OP_CONCAT: reg(a); if (b < 2) fail("invalid concat count"); reg(a + b - 1); break;
        case OP_CLOSE: if (a > p.maxstacksize) fail("close level out of bounds"); break;
        case OP_TBC: reg(a); break;
        case OP_JMP: target(static_cast<long long>(pc) + 1 + GETARG_sJ(i)); break;
        case OP_EQ: case OP_LT: case OP_LE: reg(a); reg(b); break;
        case OP_EQK: reg(a); constant(b); break;
        case OP_EQI: case OP_LTI: case OP_LEI: case OP_GTI: case OP_GEI: case OP_TEST: reg(a); break;
        case OP_TESTSET: reg(a); reg(b); break;
        case OP_CALL: case OP_TAILCALL:
            reg(a); if (b) reg(a + b - 1);
            if (op == OP_CALL && c > 1) reg(a + c - 2);
            if (op == OP_TAILCALL && c != (p.is_vararg ? p.numparams + 1 : 0)) fail("invalid tailcall vararg adjustment");
            break;
        case OP_RETURN:
            if (b == 1) { if (a > p.maxstacksize) fail("return level out of bounds"); }
            else reg(a);
            if (b > 1) reg(a + b - 2);
            if (c != (p.is_vararg ? p.numparams + 1 : 0)) fail("invalid return vararg adjustment");
            break;
        case OP_RETURN0: break;
        case OP_RETURN1: reg(a); break;
        case OP_FORLOOP: case OP_FORPREP: reg(a); reg(a + 3); break;
        case OP_TFORPREP: reg(a); reg(a + 3); break;
        case OP_TFORCALL: reg(a); reg(a + 6); if (c < 1) fail("invalid iterator result count"); reg(a + 3 + c); break;
        case OP_TFORLOOP: reg(a); reg(a + 4); break;
        case OP_SETLIST:
            reg(a); if (b) reg(a + b); if (TESTARG_k(i)) extra(pc);
            if (static_cast<unsigned long long>(c) + b +
                (TESTARG_k(i) ? static_cast<unsigned long long>(GETARG_Ax(p.code[pc + 1])) * 256 : 0) > UINT_MAX)
                fail("SETLIST size overflow");
            break;
        case OP_CLOSURE:
            reg(a); if (GETARG_Bx(i) >= static_cast<int>(p.children.size())) fail("child prototype out of bounds"); break;
        case OP_VARARG:
            if (!p.is_vararg) fail("VARARG in non-vararg function");
            reg(a); if (c > 1) reg(a + c - 2); break;
        case OP_VARARGPREP:
            if (!p.is_vararg || pc != 0 || a != p.numparams) fail("invalid VARARGPREP");
            break;
        case OP_EXTRAARG:
            if (pc == 0 || !(GET_OPCODE(p.code[pc - 1]) == OP_LOADKX || GET_OPCODE(p.code[pc - 1]) == OP_NEWTABLE ||
                (GET_OPCODE(p.code[pc - 1]) == OP_SETLIST && TESTARG_k(p.code[pc - 1]))))
                fail("orphan EXTRAARG");
            continue;
        default: fail("unsupported opcode");
        }
        if (op >= OP_ADDI && op <= OP_SHR) {
            if (pc + 1 >= n) fail("missing arithmetic companion");
            const int expected = op >= OP_ADD ? OP_MMBIN :
                (op == OP_ADDI || op == OP_SHRI || op == OP_SHLI ? OP_MMBINI : OP_MMBINK);
            if (GET_OPCODE(p.code[pc + 1]) != expected) fail("wrong arithmetic companion");
            target(pc + 1); target(pc + 2);
        } else if (op >= OP_EQ && op <= OP_TESTSET) {
            if (pc + 1 >= n || GET_OPCODE(p.code[pc + 1]) != OP_JMP) fail("comparison without JMP");
            target(pc + 2);
        } else if (op == OP_FORPREP) {
            target(static_cast<long long>(pc) + GETARG_Bx(i) + 2); target(pc + 1);
            const long long end = static_cast<long long>(pc) + GETARG_Bx(i) + 1;
            if (end >= n || GET_OPCODE(p.code[static_cast<std::size_t>(end)]) != OP_FORLOOP ||
                GETARG_A(p.code[static_cast<std::size_t>(end)]) != a) fail("FORPREP without matching FORLOOP");
        } else if (op == OP_FORLOOP || op == OP_TFORLOOP) {
            target(static_cast<long long>(pc) + 1 - GETARG_Bx(i)); target(pc + 1);
        } else if (op == OP_TFORPREP) {
            const long long t = static_cast<long long>(pc) + 1 + GETARG_Bx(i);
            target(t);
            if (GET_OPCODE(p.code[static_cast<std::size_t>(t)]) != OP_TFORCALL ||
                GETARG_A(p.code[static_cast<std::size_t>(t)]) != a) fail("TFORPREP without TFORCALL");
        } else if (op == OP_TFORCALL) {
            target(pc + 1);
            if (GET_OPCODE(p.code[pc + 1]) != OP_TFORLOOP || GETARG_A(p.code[pc + 1]) != a)
                fail("TFORCALL without TFORLOOP");
        } else if (op == OP_LOADKX || op == OP_NEWTABLE || op == OP_LFALSESKIP ||
                   (op == OP_SETLIST && TESTARG_k(i))) target(pc + 2);
        else if (op != OP_JMP && op != OP_TAILCALL && op != OP_RETURN && op != OP_RETURN0 && op != OP_RETURN1)
            target(pc + 1);
    }
    if (p.is_vararg && GET_OPCODE(p.code[0]) != OP_VARARGPREP) fail("missing VARARGPREP");
    for (const auto& child : p.children) validate(child, &p);
}
}
