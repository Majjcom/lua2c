#include <lua2c/lua2c.hpp>
#include <iostream>
int main(int argc, char** argv) {
    try {
        lua2c::Options options;
        options.module_name = "example";
        const std::string input = "local function square(n) return n*n end; print(square(12))";
        if (argc > 1) lua2c::convert_to_file(input, argv[1], options);
        else std::cout << lua2c::convert(input, options);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
