#include "lua2c/lua2c.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
extern "C" {
#include "lua.h"
#include "lobject.h"
#include "lopcodes.h"
}

namespace {
void variable(std::string& data, std::size_t value) {
    std::vector<unsigned char> bytes;
    do { bytes.push_back(static_cast<unsigned char>(value & 127)); value >>= 7; } while (value);
    bytes[0] |= 128;
    for (auto i = bytes.rbegin(); i != bytes.rend(); ++i) data.push_back(static_cast<char>(*i));
}
template<class T> void scalar(std::string& data, T value) {
    data.append(reinterpret_cast<const char*>(&value), sizeof(T));
}
void string(std::string& data, const std::string& value) {
    variable(data, value.size() + 1);
    data += value;
}
void save(const std::filesystem::path& path, const std::string& bytes) {
    std::ofstream output(path, std::ios::binary);
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!output) throw std::runtime_error("fixture write failed");
}
}
int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("fixture output directory required");
        const std::filesystem::path directory(argv[1]);
        std::string binary("\x1bLua\x54\0\x19\x93\r\n\x1a\n", 12);
        binary.push_back(sizeof(Instruction));
        binary.push_back(sizeof(lua_Integer));
        binary.push_back(sizeof(lua_Number));
        scalar(binary, static_cast<lua_Integer>(0x5678));
        scalar(binary, static_cast<lua_Number>(370.5));
        binary.push_back(1); // Root closure upvalue count.
        string(binary, "=(loadkx)");
        variable(binary, 0); variable(binary, 0); // Lines.
        binary.push_back(0); binary.push_back(1); binary.push_back(2); // Params, vararg, stack.
        const Instruction code[] = {
            CREATE_ABCk(OP_VARARGPREP, 0, 0, 0, 0),
            CREATE_ABCk(OP_GETTABUP, 0, 0, 0, 0),
            CREATE_ABx(OP_LOADKX, 1, 0),
            CREATE_Ax(OP_EXTRAARG, 1),
            CREATE_ABCk(OP_CALL, 0, 2, 1, 0),
            CREATE_ABCk(OP_RETURN, 0, 1, 1, 0)
        };
        variable(binary, sizeof(code) / sizeof(code[0]));
        for (auto instruction : code) scalar(binary, instruction);
        variable(binary, 2);
        binary.push_back(LUA_VSHRSTR); string(binary, "print");
        binary.push_back(LUA_VNUMINT); scalar(binary, static_cast<lua_Integer>(12345));
        variable(binary, 1);
        binary.push_back(1); binary.push_back(0); binary.push_back(0); // _ENV descriptor.
        variable(binary, 0); // Children.
        variable(binary, 0); variable(binary, 0); variable(binary, 0); variable(binary, 0); // Debug.
        save(directory / "loadkx.luac", binary);
        lua2c::convert_to_file(binary, directory / "loadkx.c");
        lua2c::Options native_options;
        native_options.no_bytecode = true;
        lua2c::convert_to_file(binary, directory / "loadkx_no_bytecode.c", native_options);

        std::ostringstream stress;
        stress << "local array = {";
        for (int i = 1; i <= 600; ++i) stress << i << ',';
        stress << "}\nlocal fields = {";
        for (int i = 1; i <= 600; ++i) stress << "k" << i << "='value" << i << "',";
        stress << "}\nfunction fields:method(n) return self.k600, n end\n"
                  "print('large', #array, array[600], fields:method(9))\n"
                  "local long = '" << std::string(5000, 'x') << "'\n"
                  "print('long', #long, long:sub(4998))\n";
        save(directory / "large_operands.lua", stress.str());
        lua2c::convert_file_to_file(directory / "large_operands.lua", directory / "large_operands.c");
        lua2c::convert_file_to_file(directory / "large_operands.lua", directory / "large_operands_no_bytecode.c", native_options);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
