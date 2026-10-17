#pragma once
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace lua2c {
enum class InputKind { Auto, Source, Bytecode };

struct Options {
    InputKind input_kind = InputKind::Auto;
    std::string chunk_name = "=(lua2c)";
    std::string module_name = "lua2c_module";
    std::string main_name = "lua2c_main";
};

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Binary-safe. Compilation does not execute the input program.
std::string convert(std::string_view input, const Options& options = {});
void convert_to_file(std::string_view input, const std::filesystem::path& output,
                     const Options& options = {}, bool generate_header = false);
std::string convert_file(const std::filesystem::path& input, const Options& options = {});
void convert_file_to_file(const std::filesystem::path& input,
                          const std::filesystem::path& output, const Options& options = {},
                          bool generate_header = false);
// When generate_header is true, replace the output extension with .h and
// emit C/C++ declarations for Options::main_name and the optional module initializer.
}
