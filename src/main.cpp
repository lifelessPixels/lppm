#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <format>
#include <iostream>
#include <map>
#include <utility>
#include <variant>
#include <vector>

#include <lppm/cli.h>
#include <lppm/common.h>
#include <lppm/globals.h>
#include <lppm/handlers.h>
#include <lppm/operation.h>
#include <lppm/os.h>

static std::map<std::string, lppm::operation> lppm_operations = {
    { "globals",
      { { { "get",
            { lppm::handlers::globals_get_handler,
              { { "name", true } },
              "get the current value of global replacement variable given by name" } },
          { "set",
            { lppm::handlers::globals_set_handler,
              { { "name", true }, { "value", true } },
              "sets the value for the given replacement variable, the variable name should be "
              "uppercase ASCII letters with optional underscores" } },
          { "unset",
            { lppm::handlers::globals_unset_handler,
              { { "name", true } },
              "unsets the value for replacement variable given by name" } },
          { "list", { lppm::handlers::globals_list_handler, {}, "lists all currently set replacement variables" } },
          { "init",
            { lppm::handlers::globals_init_handler,
              {},
              "interactively initialize common user replacement variables for " STYLE_GREEN
              "lppm " STYLE_COLOR_RESET } } },
        "allows managing global" STYLE_GREEN " lppm" STYLE_COLOR_RESET " replacement variables" } },
    { "project",
      { {
            { "create",
              { lppm::handlers::project_create_handler,
                { { "template name", true }, { "target directory", false } },
                "create new project using specified template, by default the project will be created "
                "in a directory named the same as template - this might be overriden by providing target "
                "directory" } },
            { "new",
              { lppm::handlers::project_create_handler,
                { { "template name", true }, { "target directory", false } },
                "alias for " STYLE_GREEN "lppm project create" STYLE_COLOR_RESET } },
            { "init",
              { lppm::handlers::project_init_handler,
                { { "template name", true }, { "target directory", true } },
                "make a project in specified target directory, by using a template with given name" } },
        },
        "allows creation of new projects using saved templates" } },
    { "template",
      { { { "list", { lppm::handlers::template_list_handler, {}, "lists all available project templates" } },
          { "create",
            { lppm::handlers::template_create_handler,
              { { "name", true }, { "source directory", false } },
              "creates new project template, if source directory is given, copies all files from "
              "it to newly created template" } },
          { "import",
            { lppm::handlers::template_create_handler,
              { { "name", true }, { "source directory", true } },
              "imports an existing project template from specified directory and names it using provided name - "
              "specified source directory must contain " STYLE_GREEN ".lppm_template" STYLE_COLOR_RESET " file" } },
          { "new",
            { lppm::handlers::template_create_handler,
              { { "name", true }, { "source directory", false } },
              "alias for " STYLE_GREEN "lppm template create" STYLE_COLOR_RESET } },
          { "show",
            { lppm::handlers::template_show_handler,
              { { "name", true } },
              "show information regarding template with given name" } },
          { "remove",
            { lppm::handlers::template_remove_handler, { { "name", true } }, "remove a template with given name" } },
          { "cmd",
            { {
                  { "add",
                    { lppm::handlers::template_cmd_add_handler,
                      { { "template name", true }, { "command to run", true } },
                      "add a command to be run in the newly created project directory, after copying template files "
                      "and doing substitutions, to the template with a given name" } },
                  { "remove",
                    { lppm::handlers::template_cmd_remove_handler,
                      { { "name", true }, { "command index", true } },
                      "removes a command at specified index from the template with given name - index of command can "
                      "be obtained by running" STYLE_GREEN " lppm template cmd list" STYLE_COLOR_RESET } },
                  { "list",
                    { lppm::handlers::template_cmd_list_handler,
                      { { "name", true } },
                      "lists all commands to be run after creating a project using the specified template" } },
              },
              "allows management of template commands that will be run at the location of created project" } } },
        "allows managing saved templates" } },
};

static void print_usage_header() {
    std::cout << STYLE_GREEN "lppm (lifelessPixels' Project Maker) version 1.0\n" STYLE_RESET;
    std::cout << "usage: " STYLE_BLUE "lppm <operation...>" STYLE_YELLOW " [arguments...]\n\n" STYLE_RESET;
}

static void print_usage_for(const std::string& operation_name, const lppm::operation& op,
                            std::string::size_type indent = 0) {
    std::string indent_string(indent, ' ');
    if (indent != 0)
        indent_string += "\u2514 ";
    std::cout << std::format("{}" STYLE_BLUE "{}" STYLE_RESET, indent_string, operation_name);
    for (auto& argument : op.arguments) {
        std::cout << STYLE_YELLOW;
        std::cout << (argument.required ? std::format("<{}> ", argument.description)
                                        : std::format("[{}] ", argument.description));
        std::cout << STYLE_RESET;
    }
    if (!op.description.empty())
        std::cout << std::format(STYLE_ITALIC "- {}" STYLE_RESET, op.description);
    std::cout << "\n";
    if (op.has_suboperations()) {
        for (auto& [suboperation_name, subop] : op.suboperations)
            print_usage_for(operation_name + suboperation_name + " ", subop, indent + 2);
    }
}

static void print_usage(const std::pair<std::string, lppm::operation>* root_operation = nullptr,
                        const std::string* previous_subcommands = nullptr) {
    std::string name { previous_subcommands == nullptr ? "" : *previous_subcommands };
    print_usage_header();
    if (root_operation == nullptr) {
        std::cout << STYLE_GREEN "available operations: " STYLE_RESET;
        std::cout << STYLE_ITALIC "(<...> denotes required argument, [...] denotes an optional one)\n" STYLE_RESET;
        for (auto& [operation_name, op] : lppm_operations) {
            print_usage_for(name + operation_name + " ", op);
            std::cout << "\n";
        }
    } else {
        auto& op = root_operation->second;

        if (op.has_suboperations()) {
            std::cout << STYLE_GREEN "available suboperations: \n" STYLE_RESET;
            print_usage_for(name, op);
        } else {
            std::cout << STYLE_GREEN "full command specification: \n" STYLE_RESET;
            print_usage_for(name, op);
        }
    }
}

static void verify_operation(const std::string& operation_name, const lppm::operation& op) {
    // verify that optional arguments are last ones
    bool found_first_optional = false;
    for (auto& argument : op.arguments) {
        if (argument.required && found_first_optional)
            lppm::print_internal_error_and_exit(std::format("invalid operation definition - optional arguments of "
                                                            "operation `{}` are mixed with non-optional ones\n",
                                                            operation_name));

        if (!argument.required)
            found_first_optional = true;
    }

    // verify suboperations
    for (auto& [suboperation_name, subop] : op.suboperations)
        verify_operation(suboperation_name, subop);
}

static void verifiy_operations() {
    for (auto& [operation_name, op] : lppm_operations)
        verify_operation(operation_name, op);
}

bool run_operation(const std::vector<std::string>& arguments, const std::map<std::string, lppm::operation> operations,
                   const std::string& previous_subcommands,
                   const std::pair<std::string, lppm::operation>* parent_operation = nullptr) {
    auto no_match_error = [&](const std::string* previous_subcommands) {
        print_usage(parent_operation, previous_subcommands);
        lppm::print_fatal("cannot run any operation - no match found");
        return false;
    };

    if (arguments.size() == 0)
        return no_match_error(&previous_subcommands);

    // extract operation name
    std::string operation_to_run { arguments[0] };
    std::transform(operation_to_run.begin(), operation_to_run.end(), operation_to_run.begin(),
                   [](c8 c) { return std::tolower(c); });

    // extract remaining arguments
    std::vector<std::string> remaining_arguments {};
    remaining_arguments.reserve(arguments.size() - 1);
    remaining_arguments.assign(arguments.begin() + 1, arguments.end());

    // iterate through available operations
    for (auto& [operation_name, op] : operations) {
        auto subcommand = previous_subcommands + operation_name + " ";
        if (operation_name == operation_to_run) {
            // if operation has suboperations, match them recursively
            if (op.has_suboperations()) {
                auto current_operation = std::make_pair(operation_name, op);
                return run_operation(remaining_arguments, op.suboperations, subcommand, &current_operation);
            }

            // try to run the operation - match argument count
            auto max_argument_count = op.arguments.size();
            if (remaining_arguments.size() < op.required_argument_count() ||
                remaining_arguments.size() > max_argument_count) {
                auto current_operation = std::make_pair(operation_name, op);
                print_usage(&current_operation, &subcommand);
                lppm::print_fatal("invalid number of arguments for specified operation");
                return false;
            }

            // if argument count mathches, run the operation
            return std::get<1>(op.handler)(remaining_arguments);
        }
    }

    // if no operation matched, just bail out
    return no_match_error(&previous_subcommands);
}

int main(int argc, char** argv) {
    // do the startup things
    verifiy_operations();

    // prepare arguments pack
    std::vector<std::string> arguments {};
    arguments.reserve(argc - 1);
    arguments.assign(argv + 1, argv + argc);

    // try to run operation to known operations
    bool operation_result = run_operation(arguments, lppm_operations, "lppm ");
    return operation_result ? EXIT_SUCCESS : EXIT_FAILURE;
}
