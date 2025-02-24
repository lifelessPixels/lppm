#pragma once
#include <string>

#include <lppm/common.h>

namespace lppm {

void print_unformatted_line(const std::string& message);
void print_info(const std::string& message);
void print_warning(const std::string& message);
void print_error(const std::string& message);
void print_fatal(const std::string& message);
[[noreturn]] void print_fatal_and_exit(const std::string& message);
[[noreturn]] void print_internal_error_and_exit(const std::string& message);

std::string prompt_user_input(const std::string& prompt, const std::string& default_value = {},
                              bool skip_default_text = false);
bool prompt_user_boolean(const std::string& prompt);

} // namespace lppm
