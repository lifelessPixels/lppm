#pragma once

#include <optional>
#include <string>

namespace lppm {

std::string trim_string(std::string input);
std::string trim_string_left(std::string input);
std::string trim_string_right(std::string input);

void trim_string_in_place(std::string& input);
void trim_string_left_in_place(std::string& input);
void trim_string_right_in_place(std::string& input);

std::optional<std::string> read_all_text(const std::string& path);

} // namespace lppm
