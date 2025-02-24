#include <lppm/template_info.h>

#include <cctype>
#include <format>
#include <fstream>
#include <map>
#include <optional>
#include <variant>
#include <vector>

#include <lppm/common.h>
#include <lppm/os.h>
#include <lppm/substitutor.h>
#include <lppm/utils.h>

enum class parser_state {
    outside,
    inside_command,
    after_command,
};

namespace lppm {

template_info::template_info(std::vector<std::string> commands) : m_commands(std::move(commands)) {}

std::variant<std::string, template_info> template_info::parse_from_file(const std::string& path) {
    // structure of the file is very simple:
    // LPPM TEMPLATE V1
    // "<command1>";"<command2>";"<command3>"

    // open file
    std::ifstream file { path };
    if (!file)
        return std::format("cannot open file `{}` for reading", path);

    // read header line and check it
    std::string header_line {};
    if (!std::getline(file, header_line))
        return std::format("cannot read header from file `{}` - file is empty", path);
    trim_string_in_place(header_line);
    if (header_line != header_string_v1)
        return std::format("header contained in file `{}` is invalid for current lppm version", path);

    // read next line containing commands to execute (if exists)
    std::string commands_line {};
    std::vector<std::string> commands {};
    std::getline(file, commands_line);
    if (!commands_line.empty()) {
        trim_string_in_place(commands_line);
        if (commands_line.empty())
            goto end;

        // separate individual commands from the read line
        c8 last_character = 0;
        parser_state state { parser_state::outside };
        std::string current_command {};
        for (c8 current : commands_line) {
            // depending on parser state, do stuff
            switch (state) {
                case parser_state::outside: {
                    if (current != '"') {
                        return std::format(
                            "commands line in file `{}` is malformed - expected command list, but \" was not found",
                            path);
                    }

                    // switch state
                    state = parser_state::inside_command;
                    break;
                }

                case parser_state::inside_command: {
                    // make escapes
                    if (last_character == '\\') {
                        current_command += current;
                        break;
                    }

                    // ignore backslashes
                    if (current == '\\')
                        break;

                    // treat unescaped quotes specially
                    if (current == '"' && last_character != '\\') {
                        // add command to commands list
                        commands.push_back(current_command);
                        current_command.clear();

                        // switch state
                        state = parser_state::after_command;
                        break;
                    }

                    // otherwise, just append to current command
                    current_command += current;
                    break;
                }

                case parser_state::after_command: {
                    // skip all whitespaces
                    if (std::isspace(current))
                        break;

                    // if the next character is not semicolon, something is wrong
                    if (current != ';') {
                        return std::format(
                            "commands line in file `{}` is malformed - expected semicolon after `{}` command", path,
                            current_command);
                    }

                    // switch state
                    state = parser_state::outside;
                    break;
                }
            }

            // assign last character to be used in the next iteration
            last_character = current;
        }

        // check parses state
        if (state == parser_state::inside_command) {
            return std::format("commands line in file `{}` is malformed - unfinished command `{}`", path,
                               current_command);
        }
    }

end:
    return template_info { std::move(commands) };
}

std::optional<std::string> template_info::save_to_file(const std::string& path) const {
    // open file
    std::ofstream file { path };
    if (!file)
        return std::format("cannot open file `{}` for writing", path);

    // write a header and commands (if there are any)
    file << header_string_v1 << '\n';
    if (!m_commands.empty()) {
        for (auto& command : m_commands)
            file << '"' << command << "\";";
        file << "\n";
    }

    // signal success
    return {};
}

std::vector<std::string>& template_info::commands() const { return m_commands; }

std::optional<std::string> template_info::run_commands_at(std::string directory_path,
                                                          std::map<std::string, std::string>& mappings) const {
    // save current working directory and switch to the new one
    auto saved_wd = os::get_working_directory();
    if (!os::set_working_directory(directory_path))
        return std::format("could not change working directory to `{}`", directory_path);

    // execute commands one-by-one
    for (auto& command : m_commands) {
        std::string substituted_command = do_the_substitutions(command, mappings);
        int result = !os::run_command(substituted_command);
        if (!result) {
            os::set_working_directory(saved_wd);
            return std::format("executed command `{}` returned non-zero ({}) exit code", substituted_command, result);
        }
    }

    // restore state and return
    os::set_working_directory(saved_wd);
    return {};
}

} // namespace lppm
