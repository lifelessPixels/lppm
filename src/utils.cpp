#include <iterator>
#include <lppm/utils.h>

#include <algorithm>
#include <fstream>
#include <streambuf>
#include <string>

#include <lppm/common.h>

namespace lppm {

std::string trim_string(std::string input) {
    trim_string_in_place(input);
    return input;
}

std::string trim_string_left(std::string input) {
    trim_string_left_in_place(input);
    return input;
}

std::string trim_string_right(std::string input) {
    trim_string_right_in_place(input);
    return input;
}

void trim_string_in_place(std::string& input) {
    trim_string_left_in_place(input);
    trim_string_right_in_place(input);
}

void trim_string_left_in_place(std::string& input) {
    input.erase(input.begin(), std::find_if(input.begin(), input.end(), [](c8 c) { return !std::isspace(c); }));
}

void trim_string_right_in_place(std::string& input) {
    input.erase(std::find_if(input.rbegin(), input.rend(), [](c8 c) { return !std::isspace(c); }).base(), input.end());
}

std::optional<std::string> read_all_text(const std::string& path) {
    std::ifstream file { path };
    if (!file)
        return {};

    // read text and return it
    return std::string { std::istreambuf_iterator<char> { file }, std::istreambuf_iterator<char> {} };
}

} // namespace lppm
