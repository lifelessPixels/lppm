#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>

#include <lppm/cli.h>
#include <lppm/common.h>
#include <lppm/utils.h>

namespace lppm {

void print_unformatted_line(const std::string& message) { std::cout << message << "\n"; }

void print_info(const std::string& message) { std::cout << STYLE_CYAN << "info: " << message << STYLE_RESET << "\n"; }

void print_warning(const std::string& message) {
    std::cerr << STYLE_YELLOW << "warning: " << message << STYLE_RESET << "\n";
}
void print_error(const std::string& message) { std::cerr << STYLE_RED << "error: " << message << STYLE_RESET << "\n"; }

void print_fatal(const std::string& message) { std::cerr << STYLE_RED << "fatal: " << message << STYLE_RESET << "\n"; }

[[noreturn]] void print_fatal_and_exit(const std::string& message) {
    print_fatal(message);
    std::exit(EXIT_FAILURE);
}

[[noreturn]] void print_internal_error_and_exit(const std::string& message) {
    std::cerr << STYLE_RED << "internal error: " << message << STYLE_RESET << "\n";
    std::exit(EXIT_FAILURE);
}

std::string prompt_user_input(const std::string& prompt, const std::string& default_value, bool skip_default_text) {
    std::string result {};

    while (true) {
        // print a prompt and get an input
        std::cout << prompt;
        !default_value.empty()
            ? std::cout << std::string { " [" } + (skip_default_text ? "" : STYLE_BLUE "default: ") + STYLE_YELLOW
                        << default_value << STYLE_RESET "]: "
            : std::cout << ": ";
        std::getline(std::cin, result);

        // trim input from left and right
        trim_string_in_place(result);

        // if input is empty, return default value (if non-empty)
        if (result.empty() && default_value.empty()) {
            lppm::print_warning("please provide a non-empty input");
            continue;
        }

        // if default is non-empty, just return it
        return result.size() == 0 ? default_value : result;
    }
}

bool prompt_user_boolean(const std::string& prompt) {
    static constexpr std::string default_value_string = "Y/N";
    while (true) {
        auto result = prompt_user_input(prompt, default_value_string, true);
        if (result == default_value_string) {
            lppm::print_warning("please enter Y or N to make a choice");
            continue;
        }

        // try to parse Y or N
        std::transform(result.begin(), result.end(), result.begin(), [](c8 c) { return std::tolower(c); });
        if (result == "y")
            return true;
        if (result == "n")
            return false;

        // if not matched, print warning and try again
        lppm::print_warning(std::format("invalid input `{}`, please try again", result));
    }
}

} // namespace lppm
