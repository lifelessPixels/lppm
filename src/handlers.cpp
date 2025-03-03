#include <lppm/handlers.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>
#include <variant>
#include <vector>

#include <lppm/cli.h>
#include <lppm/common.h>
#include <lppm/globals.h>
#include <lppm/os.h>
#include <lppm/substitutor.h>
#include <lppm/template.h>
#include <lppm/utils.h>

namespace lppm::handlers {

static void print_global_value(const std::string& key, const std::string& value) {
    print_unformatted_line(std::format(STYLE_BLUE "{}" STYLE_RESET ": " STYLE_YELLOW "{}" STYLE_RESET, key, value));
}

bool globals_get_handler(const std::vector<std::string>& arguments) {
    // get a key
    auto key = trim_string(arguments[0]);
    if (key.empty())
        print_fatal_and_exit("provided key is empty");

    // check if entry exists in globals
    if (!globals::the().contains_key(key))
        print_fatal_and_exit(std::format("globals config file does not contain an entry with provided key `{}`", key));

    auto value = globals::the().get_value(key).value();
    print_global_value(key, value);
    return true;
}

bool globals_set_handler(const std::vector<std::string>& arguments) {
    // get arguments
    auto key = trim_string(arguments[0]);
    auto value = trim_string(arguments[1]);

    // set value
    globals::the().set_value(key, value, true);
    return true;
}

bool globals_unset_handler(const std::vector<std::string>& arguments) {
    // get key
    auto key = trim_string(arguments[0]);
    if (key.empty())
        print_fatal_and_exit("provided key is empty");

    // check if entry exists in globals
    if (!globals::the().contains_key(key))
        print_fatal_and_exit(std::format("globals config file does not contain an entry with provided key `{}`", key));

    // remove entry
    globals::the().remove_value(key);
    return true;
}

bool globals_list_handler(const std::vector<std::string>& arguments) {
    UNUSED(arguments);

    // get available keys
    auto keys = globals::the().key_set();
    if (keys.empty()) {
        print_info("globals config file is empty!");
        return true;
    }

    // print all saved entries
    for (auto& key : keys)
        print_global_value(key, globals::the().get_value(key).value());
    return true;
}

bool globals_init_handler(const std::vector<std::string>& arguments) {
    UNUSED(arguments);
    auto ask_for_input = [](const std::string& enter_string, const std::vector<std::string>& globals,
                            const std::string& empty_string, const std::string& default_value = {}) {
        std::string globals_string {};
        usz globals_size = globals.size();
        if (globals_size != 0) {
            globals_string += "(sets ";
            for (usz index = 0; index < globals_size; index++) {
                globals_string += STYLE_BLUE;
                globals_string += globals[index];
                globals_string += STYLE_RESET;
                if (globals_size > 1 && index < globals_size - 2) {
                    globals_string += ", ";
                } else if (globals_size > 1 && index == globals_size - 2) {
                    globals_string += " and ";
                }
            }

            globals_string += globals.size() == 1 ? " global" : " globals";
            globals_string += ")";
        }

        auto input = prompt_user_input(std::format("enter {} {}", enter_string, globals_string), default_value);
        if (!input.empty()) {
            for (usz index = 0; index < globals_size; index++)
                globals::the().set_value(globals[index], input, false, index == globals_size - 1);
            return input;
        } else {
            print_warning(std::format("empty {} provided - skipping", empty_string));
            return std::string { "" };
        }
    };

    // ask for data
    ask_for_input("your name", { "NAME", "AUTHOR" }, "name");
    ask_for_input("your e-mail address", { "EMAIL", "MAIL" }, "e-mail");
    ask_for_input("your website address", { "WEBSITE", "WWW", "SITE" }, "website address");
    ask_for_input("your github profile address", { "GITHUB" }, "github profile address");
    ask_for_input("default license", { "LICENSE" }, "license", "All rights reserved.");

    return true;
}

bool project_create_handler(const std::vector<std::string>& arguments) {
    // get arguments
    auto const& template_name = arguments[0];
    auto const& target_path = arguments.size() == 2 ? arguments[1] : template_name;
    std::vector<std::string> new_arguments { template_name, target_path };

    // ensure that the target does not exist
    if (std::error_code code; std::filesystem::exists(target_path, code) && !code) {
        print_error(std::format("target directory `{}` already exists", target_path));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }

    // create a target directory
    std::error_code code {};
    std::filesystem::create_directories(target_path, code);
    if (code) {
        print_error(std::format("cannot create target directory `{}`", target_path));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }

    // do the same as in the init command
    return project_init_handler(new_arguments);
}

bool project_init_handler(const std::vector<std::string>& arguments) {
    // get arguments
    auto const& template_name = arguments[0];
    std::string target_path = std::filesystem::absolute(arguments[1]);

    // ensure the target directory exists...
    if (std::error_code code; !std::filesystem::is_directory(target_path, code) || code) {
        print_error(std::format("cannot find target directory `{}`", target_path));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }

    // ...and is empty
    if (std::error_code code; !std::filesystem::is_empty(target_path, code) || code) {
        print_error(std::format("target directory `{}` is not empty", target_path));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }

    // get template by name
    std::string template_path = std::filesystem::path { os::get_lppm_config_directory() } /
                                project_template::templates_directory_name / template_name;
    auto maybe_template = project_template::template_from_directory(template_path);
    if (std::holds_alternative<std::string>(maybe_template)) {
        print_error(std::get<std::string>(maybe_template));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }
    auto& the_template = std::get<project_template>(maybe_template);

    // create substitutions set and try to substitute all of the template variables
    auto mappings = globals::the().mappings();
    mappings.insert_or_assign("PROJECT_NAME", std::filesystem::path { target_path }.filename());
    for (auto& directory_entry : std::filesystem::recursive_directory_iterator { the_template.base_directory() }) {
        // do the substitutions in file name if necessary
        auto relative_path = std::filesystem::relative(directory_entry.path(), the_template.base_directory());
        std::string unsubstituted_name = target_path / relative_path;
        std::string path_in_target = do_the_substitutions(unsubstituted_name, mappings);

        // if the entry refers to the directory, create it in target directory
        if (std::error_code code; directory_entry.is_directory(code) || code) {
            std::filesystem::create_directories(path_in_target, code);
            if (code) {
                print_error(std::format("could not create a directory `{}`", path_in_target));
                print_fatal_and_exit("could not successfuly perform the operation, aborting...");
            }

            continue;
        }

        // if the entry refers to the .lppm_template file, continue
        std::string template_file_path = std::filesystem::absolute(
            std::filesystem::path { the_template.base_directory() } / project_template::template_info_file_name);
        if (std::filesystem::absolute(directory_entry.path()) == template_file_path)
            continue;

        // create target directory
        std::error_code code {};
        std::string directory_path = std::filesystem::path { path_in_target }.parent_path();
        if (!std::filesystem::exists(directory_path)) {
            std::filesystem::create_directories(directory_path, code);
            if (code) {
                print_error(std::format("could not create a directory `{}`", path_in_target));
                print_fatal_and_exit("could not successfuly perform the operation, aborting...");
            }
        }

        // read file contents, do the substitutions and write a file
        auto file_contents = read_all_text(directory_entry.path());
        if (!file_contents.has_value()) {
            print_error(
                std::format("could not read contents of file `{}`", static_cast<std::string>(directory_entry.path())));
            print_fatal_and_exit("could not successfuly perform the operation, aborting...");
        }

        // do the substitutions
        auto after_substitutions = do_the_substitutions(file_contents.value(), mappings);

        // write a substituted file
        std::ofstream resulting_file { path_in_target };
        if (!resulting_file) {
            print_error(std::format("could not create a file `{}` - {}", path_in_target, std::strerror(errno)));
            print_fatal_and_exit("could not successfuly perform the operation, aborting...");
        }
        resulting_file.write(after_substitutions.c_str(), after_substitutions.size());
    }

    // execute commands and log info
    auto result = the_template.info().run_commands_at(target_path, mappings);
    if (result.has_value()) {
        print_error(result.value());
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }
    print_info(std::format("successfuly created a project at `" STYLE_BLUE "{}" STYLE_RESET
                           "` from template `" STYLE_BLUE "{}" STYLE_RESET "`",
                           target_path, template_name));
    return true;
}

bool template_import_handler(const std::vector<std::string>& arguments) {
    // get arguments
    auto template_name = arguments[0];
    auto source_directory = arguments[1];

    // try to import
    auto result = project_template::import_template(template_name, source_directory);
    if (std::holds_alternative<std::string>(result)) {
        print_error(std::get<std::string>(result));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }

    // show info
    auto imported = std::get<project_template>(result);
    print_info(std::format("successfully imported project template `{}` from `{}` to `{}`", template_name,
                           source_directory, imported.base_directory()));

    return true;
}

bool template_create_handler(const std::vector<std::string>& arguments) {
    // get arguments
    auto template_name = arguments[0];
    std::optional<std::string> maybe_source_directory =
        arguments.size() == 2 ? arguments[1] : std::optional<std::string> {};

    // try to import
    auto result = project_template::create_new_template(template_name, maybe_source_directory);
    if (std::holds_alternative<std::string>(result)) {
        print_error(std::get<std::string>(result));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }

    // show info
    auto imported = std::get<project_template>(result);
    print_info(std::format(
        "successfully created new project template `{}`{}", template_name,
        maybe_source_directory.has_value() ? std::format(" from `{}`", maybe_source_directory.value()) : ""));
    return true;
}

bool template_list_handler(const std::vector<std::string>& arguments) {
    UNUSED(arguments);

    // get a list of available project templates
    auto templates = project_template::get_all_templates();
    if (templates.size() == 0) {
        print_info("no project templates found");
        return true;
    }

    // print the templates
    for (auto& current : templates) {
        print_unformatted_line(std::format(STYLE_BLUE "{}" STYLE_RESET ": " STYLE_YELLOW "{}" STYLE_RESET,
                                           current.first, current.second.base_directory()));
    }

    return true;
}

bool template_show_handler(const std::vector<std::string>& arguments) {
    // get template by name
    std::string template_path = std::filesystem::path { os::get_lppm_config_directory() } /
                                project_template::templates_directory_name / arguments[0];
    auto maybe_template = project_template::template_from_directory(template_path);
    if (std::holds_alternative<std::string>(maybe_template)) {
        print_error(std::get<std::string>(maybe_template));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }

    // count files in the template
    int file_count = 0;
    for (auto& directory_entry : std::filesystem::recursive_directory_iterator { template_path }) {
        if (std::error_code code; directory_entry.is_regular_file(code) && !code &&
                                  directory_entry.path().filename() != project_template::template_info_file_name) {
            file_count++;
        }
    }

    // print info about the template
    auto const& the_template = std::get<project_template>(maybe_template);
    auto& info = the_template.info();
    auto& commands = info.commands();
    print_unformatted_line(std::format("project template `" STYLE_BLUE "{}" STYLE_RESET "`", arguments[0]));
    print_unformatted_line(
        std::format(STYLE_BLUE "path" STYLE_RESET ": " STYLE_YELLOW "{}",
                    static_cast<std::string>(std::filesystem::absolute(the_template.base_directory()))));
    print_unformatted_line(std::format(STYLE_BLUE "file count" STYLE_RESET ": " STYLE_YELLOW "{}", file_count));

    // print commands to be run
    if (commands.empty()) {
        print_unformatted_line(
            std::format(STYLE_BLUE "template does not contain commands to be run on project creation" STYLE_RESET));
    } else {
        print_unformatted_line(std::format(STYLE_BLUE "commands to be run on project creation: " STYLE_RESET));
        for (usz index = 0; index < commands.size(); index++)
            print_unformatted_line(std::format(" {} - " STYLE_YELLOW "{}" STYLE_RESET, index, commands[index]));
    }

    return true;
}

bool template_remove_handler(const std::vector<std::string>& arguments) {
    // get template by name
    std::string template_path = std::filesystem::path { os::get_lppm_config_directory() } /
                                project_template::templates_directory_name / arguments[0];
    auto maybe_template = project_template::template_from_directory(template_path);
    if (std::holds_alternative<std::string>(maybe_template)) {
        print_error(std::format("cannot find a template named `{}` to remove", arguments[0]));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }

    // if the template exists, ask user for confirmation
    if (prompt_user_boolean(std::format("do you really want to remove project named `" STYLE_BLUE "{}" STYLE_RESET "`",
                                        arguments[0]))) {
        std::error_code code {};
        std::filesystem::remove_all(template_path, code);
        if (code) {
            print_error(std::format("cannot remove files from template named `{}`", arguments[0]));
            print_fatal_and_exit("could not successfuly perform the operation, aborting...");
        }
    }

    return true;
}

bool template_cmd_add_handler(const std::vector<std::string>& arguments) {
    // get arguments
    auto template_name = arguments[0];
    auto command = arguments[1];

    // get template by name
    std::string template_path = std::filesystem::path { os::get_lppm_config_directory() } /
                                project_template::templates_directory_name / arguments[0];
    auto maybe_template = project_template::template_from_directory(template_path);
    if (std::holds_alternative<std::string>(maybe_template)) {
        print_error(std::format("cannot find a template named `{}` to remove", arguments[0]));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }
    auto const& the_template = std::get<project_template>(maybe_template);
    auto& info = the_template.info();

    // add command to the info and resave it
    info.commands().push_back(command);
    auto result = the_template.save_info();
    if (result.has_value()) {
        print_error(result.value());
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }

    return true;
}

bool template_cmd_remove_handler(const std::vector<std::string>& arguments) {
    // get arguments
    auto template_name = arguments[0];
    auto command_index = arguments[1];

    // parse command index
    usz index = {};
    try {
        index = std::stoll(command_index);
    } catch (...) {
        print_error(std::format("`{}` is not a valid integral index", command_index));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }

    // get template by name
    std::string template_path = std::filesystem::path { os::get_lppm_config_directory() } /
                                project_template::templates_directory_name / arguments[0];
    auto maybe_template = project_template::template_from_directory(template_path);
    if (std::holds_alternative<std::string>(maybe_template)) {
        print_error(std::format("cannot find a template named `{}` to remove", arguments[0]));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }
    auto const& the_template = std::get<project_template>(maybe_template);
    auto& info = the_template.info();
    auto& commands = info.commands();

    // check command index
    if (index >= commands.size()) {
        print_error(std::format("cannot find a command with index {} in template named `{}`", index, arguments[0]));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }

    // remove command at index
    auto const& command = commands[index];
    if (prompt_user_boolean(std::format("do you really want to remove command `" STYLE_BLUE "{}" STYLE_RESET
                                        "` from template named `" STYLE_BLUE "{}" STYLE_RESET "`",
                                        command, template_name))) {
        commands.erase(commands.begin() + index);
        auto result = the_template.save_info();
        if (result.has_value()) {
            print_error(result.value());
            print_fatal_and_exit("could not successfuly perform the operation, aborting...");
        }
    }

    return true;
}

bool template_cmd_list_handler(const std::vector<std::string>& arguments) {
    // get template by name
    std::string template_path = std::filesystem::path { os::get_lppm_config_directory() } /
                                project_template::templates_directory_name / arguments[0];
    auto maybe_template = project_template::template_from_directory(template_path);
    if (std::holds_alternative<std::string>(maybe_template)) {
        print_error(std::get<std::string>(maybe_template));
        print_fatal_and_exit("could not successfuly perform the operation, aborting...");
    }

    // print commands to be run
    auto const& the_template = std::get<project_template>(maybe_template);
    auto& info = the_template.info();
    auto& commands = info.commands();
    if (commands.empty()) {
        print_unformatted_line(
            std::format(STYLE_BLUE "template does not contain commands to be run on project creation" STYLE_RESET));
    } else {
        print_unformatted_line(std::format(STYLE_BLUE "commands to be run on project creation: " STYLE_RESET));
        for (usz index = 0; index < commands.size(); index++)
            print_unformatted_line(std::format(" {} - " STYLE_YELLOW "{}" STYLE_RESET, index, commands[index]));
    }
    return true;
}

} // namespace lppm::handlers
