#include "lua2c/lua2c.hpp"
#include <iostream>
#include <string>

namespace {
void print_help() {
    std::cout << "lua2c - Convert Lua 5.4 source or bytecode to C\n"
                 "\n"
                 "Usage:\n"
                 "  lua2c [options] INPUT\n"
                 "\n"
                 "Input:\n"
                 "  INPUT                Path to a Lua source or Lua 5.4 bytecode file.\n"
                 "                       Input format is detected automatically by default.\n"
                 "\n"
                 "Options:\n"
                 "  -h, --help           Show this help and exit.\n"
                 "  --source             Force INPUT to be treated as Lua source.\n"
                 "  --bytecode           Force INPUT to be treated as Lua 5.4 bytecode.\n"
                 "  -s, --strip          Remove Lua debug information (default: retain it).\n"
                 "                       Applies to source and bytecode, like luac -s.\n"
                 "  --no-bytecode        Omit bytecode arrays (default: retain them).\n"
                 "                       Disables dumping native functions; limits debug names.\n"
                 "  -o FILE              Write generated C to FILE (default: stdout).\n"
                 "  --header             Also generate a matching .h file beside FILE.\n"
                 "                       Requires -o; replaces the output extension with .h.\n"
                 "  --main-name NAME     Set the C chunk entry function name.\n"
                 "                       Default: lua2c_main.\n"
                 "  --module NAME        Set the module name used in luaopen_NAME.\n"
                 "                       Default: lua2c_module.\n"
                 "  --chunk-name NAME    Set the source debug name (default: @INPUT).\n"
                 "                       Does not override debug names in bytecode input.\n"
                 "\n"
                 "Names:\n"
                 "  Module and entry names must be valid C identifiers. Entry names must\n"
                 "  also avoid C/C++ keywords and existing runtime or generated symbols.\n"
                 "\n"
                 "Examples:\n"
                 "  lua2c -o program.c script.lua\n"
                 "  lua2c --bytecode -o program.c script.luac\n"
                 "  lua2c --strip -o program.c script.lua\n"
                 "  lua2c --header --main-name run_demo --module demo -o demo.c demo.lua\n"
                 "\n"
                 "Generated C (compile-time options, both disabled by default):\n"
                 "  LUA2C_MAIN=1         Enable a standalone main() function.\n"
                 "  LUA2C_API=1          Export luaopen_NAME for Lua require().\n"
                 "  Link against the matching lua2c runtime; see README.md for details.\n";
}
}

int main(int argc, char** argv) {
    lua2c::Options options;
    std::filesystem::path input, output;
    bool generate_header = false;
    try {
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--help" || argument == "-h") {
                print_help();
                return 0;
            }
            if (argument == "--source") options.input_kind = lua2c::InputKind::Source;
            else if (argument == "--bytecode") options.input_kind = lua2c::InputKind::Bytecode;
            else if (argument == "--strip" || argument == "-s") options.strip = true;
            else if (argument == "--no-bytecode") options.no_bytecode = true;
            else if (argument == "--header") generate_header = true;
            else if (argument == "-o" || argument == "--module" || argument == "--main-name" || argument == "--chunk-name") {
                if (++i >= argc) throw lua2c::Error("missing value after " + argument);
                if (argument == "-o") output = argv[i];
                else if (argument == "--module") options.module_name = argv[i];
                else if (argument == "--main-name") options.main_name = argv[i];
                else options.chunk_name = argv[i];
            } else if (!argument.empty() && argument[0] == '-') throw lua2c::Error("unknown option: " + argument);
            else if (!input.empty()) throw lua2c::Error("only one input file is accepted");
            else input = argument;
        }
        if (input.empty()) throw lua2c::Error("input file required (use --help)");
        if (generate_header && output.empty()) throw lua2c::Error("--header requires -o FILE.c");
        if (output.empty()) std::cout << lua2c::convert_file(input, options);
        else lua2c::convert_file_to_file(input, output, options, generate_header);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "lua2c: " << error.what() << '\n';
        return 1;
    }
}
