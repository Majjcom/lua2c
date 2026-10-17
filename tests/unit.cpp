#include "lua2c/lua2c.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <locale>
#include <string>
extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

namespace {
struct GroupedNumbers : std::numpunct<char> {
    char do_thousands_sep() const override { return ','; }
    std::string do_grouping() const override { return "\3"; }
};
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot read test output");
    return std::string(std::istreambuf_iterator<char>(input), {});
}
int dump(lua_State*, const void* p, std::size_t n, void* context) {
    static_cast<std::string*>(context)->append(static_cast<const char*>(p), n);
    return 0;
}
void rejects(const std::string& input, const lua2c::Options& options = {}) {
    try { (void)lua2c::convert(input, options); }
    catch (const lua2c::Error&) { return; }
    throw std::runtime_error("invalid input accepted");
}
}
int main() {
    try {
        const std::string source = "local function add(a,b) return a+b end; return add(2,3)";
        auto code = lua2c::convert(source);
        check(code.find("int lua2c_main(lua_State *L)") != std::string::npos, "missing main entry");
        check(code.find("lua2c_gen_add_") != std::string::npos, "missing named function");
        check(code.find("luaL_loadbuffer") == std::string::npos, "interpreter wrapper emitted");
        check(code.find("Lua 5.4.9") != std::string::npos, "generated code names the wrong Lua revision");
        check(code.find("0x00010009UL") != std::string::npos, "generated code uses the old runtime ABI");
        check(lua2c::convert(source) == code, "output is not deterministic");
        lua2c::Options named_options;
        check(named_options.main_name == "lua2c_main", "entry name default changed");
        named_options.main_name = "run_custom_chunk";
        const auto named_code = lua2c::convert(source, named_options);
        check(named_code.find("int run_custom_chunk(lua_State *L)") != std::string::npos, "custom entry definition missing");
        check(named_code.find("int lua2c_main(lua_State *L)") == std::string::npos, "default entry emitted after renaming");
        check(named_code.find("return run_custom_chunk(L);") != std::string::npos, "module wrapper calls the wrong entry");
        check(named_code.find("int luaopen_lua2c_module(lua_State *L)") != std::string::npos, "entry rename changed module name");
        named_options.main_name = "L";
        check(lua2c::convert(source, named_options).find("return L(lua2c_state);") != std::string::npos,
            "module parameter shadows the custom entry name");
        for (const std::string invalid : {"", "1entry", "bad-name", "for", "class", "_Thread_local", "main",
                 "luaopen_lua2c_module", "lua2c_check_runtime", "lua2c_gen_chunk_p0", "lua2c_proto_0", "LUA2C_API"}) {
            named_options.main_name = invalid;
            rejects(source, named_options);
        }
        named_options.main_name = "run_custom_chunk";
        const std::locale previous = std::locale();
        std::locale::global(std::locale(previous, new GroupedNumbers));
        const auto localized = lua2c::convert(source);
        std::locale::global(previous);
        check(localized == code, "output depends on host locale");
        check(!lua2c::convert("").empty(), "empty chunk unsupported");
        check(!lua2c::convert("\xef\xbb\xbf#!/bin/lua\nreturn 2").empty(), "BOM/shebang unsupported");
        rejects("local = broken");
        lua2c::Options options;
        options.module_name = "bad-name";
        rejects(source, options);
        options.module_name = "test";
        options.input_kind = lua2c::InputKind::Bytecode;
        rejects(source, options);
        lua_State* L = luaL_newstate();
        check(L != nullptr, "state allocation failed");
        check(luaL_loadbuffer(L, source.data(), source.size(), "=(lua2c)") == LUA_OK, "load failed");
        std::string bytecode;
        check(lua_dump(L, dump, &bytecode, 0) == 0, "dump failed");
        lua_close(L);
        check(lua2c::convert(bytecode) == code, "source/bytecode output mismatch");
        check(lua2c::convert(bytecode, named_options) == named_code, "bytecode custom entry differs from source");
        named_options.main_name = "for";
        rejects(bytecode, named_options);
        named_options.main_name = "run_custom_chunk";
        for (std::size_t n = 0; n < bytecode.size(); ++n) rejects(bytecode.substr(0, n), options);
        rejects(bytecode + "extra", options);
        auto damaged = bytecode;
        damaged[4] = '\x53';
        rejects(damaged, options);
        // Conversion never executes input, even if the input immediately errors.
        check(!lua2c::convert("error('must not execute')").empty(), "input was executed");
        const auto output = std::filesystem::current_path() / "lua2c_unit_output.c";
        auto header = output;
        header.replace_extension(".h");
        lua2c::convert_to_file(source, output);
        check(std::filesystem::file_size(output) == code.size(), "file output mismatch");
        check(!std::filesystem::exists(header), "header generated by default");
        try { lua2c::convert_to_file("local = broken", output); }
        catch (const lua2c::Error&) {}
        check(std::filesystem::file_size(output) == code.size(), "failed conversion destroyed output");
        lua2c::Options header_options;
        header_options.module_name = "public_module";
        lua2c::convert_to_file(source, output, header_options, true);
        const auto declarations = read_file(header);
        const auto generated = read_file(output);
        check(generated == lua2c::convert(source, header_options), "header flag changed generated C");
        check(declarations.find("#include \"lua.h\"") != std::string::npos, "header missing Lua types");
        check(declarations.find("extern \"C\"") != std::string::npos, "header missing C++ linkage");
        check(declarations.find("int lua2c_main(lua_State *L);") != std::string::npos, "header missing main declaration");
        check(declarations.find("int luaopen_public_module(lua_State *L);") != std::string::npos, "header module name mismatch");
        check(declarations.find("lua2c_gen_") == std::string::npos, "header exposed internal executors");
        header_options.input_kind = lua2c::InputKind::Bytecode;
        lua2c::convert_to_file(bytecode, output, header_options, true);
        check(read_file(output) == generated && read_file(header) == declarations, "bytecode/header output mismatch");
        header_options.input_kind = lua2c::InputKind::Auto;
        bool rejected = false;
        try { lua2c::convert_to_file("local = broken", output, header_options, true); }
        catch (const lua2c::Error&) { rejected = true; }
        check(rejected && read_file(output) == generated && read_file(header) == declarations,
            "invalid input overwrote C/header outputs");
        rejected = false;
        try { lua2c::convert_to_file(source, header, header_options, true); }
        catch (const lua2c::Error&) { rejected = true; }
        check(rejected && read_file(header) == declarations, "C/header path collision not rejected");
        auto uppercase_header = output;
        uppercase_header.replace_extension(".H");
        rejected = false;
        try { lua2c::convert_to_file(source, uppercase_header, header_options, true); }
        catch (const lua2c::Error&) { rejected = true; }
        check(rejected, "case-insensitive header collision not rejected");

        const auto input = std::filesystem::current_path() / "lua2c_unit_input.lua";
        { std::ofstream file(input, std::ios::binary); file << source; check(static_cast<bool>(file), "input write failed"); }
        lua2c::convert_file_to_file(input, output, header_options, true);
        check(read_file(output) == lua2c::convert_file(input, header_options), "file-to-file C output mismatch");
        check(read_file(header) == declarations, "file-to-file header mismatch");
        named_options.module_name = header_options.module_name;
        lua2c::convert_to_file(source, output, named_options, true);
        const auto named_header = read_file(header);
        const auto named_output = read_file(output);
        check(named_header.find("int run_custom_chunk(lua_State *L);") != std::string::npos, "header has no custom entry");
        check(named_header.find("int lua2c_main(lua_State *L);") == std::string::npos, "header retains default entry");
        check(named_header.find("int luaopen_public_module(lua_State *L);") != std::string::npos, "header module name changed");
        check(named_header.find("#ifndef LUA2C_GENERATED_public_module_run_custom_chunk_H") != std::string::npos,
            "header guard does not distinguish custom entries");
        named_options.input_kind = lua2c::InputKind::Bytecode;
        lua2c::convert_to_file(bytecode, output, named_options, true);
        check(read_file(output) == named_output && read_file(header) == named_header, "bytecode custom header mismatch");
        named_options.input_kind = lua2c::InputKind::Auto;
        lua2c::convert_file_to_file(input, output, named_options, true);
        check(read_file(output) == lua2c::convert_file(input, named_options), "file custom entry mismatch");
        check(read_file(header) == named_header, "file custom header mismatch");
        const auto named_file_output = read_file(output);
        named_options.main_name = "for";
        rejected = false;
        try { lua2c::convert_to_file(source, output, named_options, true); }
        catch (const lua2c::Error&) { rejected = true; }
        check(rejected && read_file(output) == named_file_output && read_file(header) == named_header,
            "invalid entry name overwrote existing C/header files");
        std::filesystem::remove(header);
        lua2c::convert_file_to_file(input, output, header_options);
        check(!std::filesystem::exists(header), "file-to-file header generated by default");
        std::filesystem::remove(input);

        const auto extensionless = std::filesystem::current_path() / "lua2c_unit_extensionless";
        lua2c::convert_to_file(source, extensionless, {}, true);
        auto extensionless_header = extensionless;
        extensionless_header.replace_extension(".h");
        check(std::filesystem::exists(extensionless_header), "extensionless output missing header");
        std::filesystem::remove(extensionless);
        std::filesystem::remove(extensionless_header);
        std::filesystem::remove(output);
        std::cout << "library tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
