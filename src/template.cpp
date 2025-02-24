#include <lppm/template.h>

#include <filesystem>
#include <format>
#include <system_error>
#include <variant>

#include <lppm/cli.h>
#include <lppm/os.h>
#include <lppm/template_info.h>

namespace lppm {

project_template::project_template(std::string base_directory, template_info info)
    : m_base_directory(std::move(base_directory)), m_info(std::move(info)) {}

std::map<std::string, project_template> project_template::get_all_templates() {
    std::map<std::string, project_template> result {};

    // check whether the "templates" directory exists
    std::string templates_directory_path =
        std::filesystem::path { os::get_lppm_config_directory() } / templates_directory_name;
    if (std::error_code code; !std::filesystem::is_directory(templates_directory_path, code) || code)
        return result;

    // get all directories inside of templates directory and try to create templates from them
    for (auto& directory_entry : std::filesystem::directory_iterator { templates_directory_path }) {
        if (std::error_code code; !directory_entry.is_directory(code) || code)
            continue;

        // if we have a directory, try to load a template from it
        auto maybe_template = template_from_directory(std::filesystem::absolute(directory_entry.path()));
        if (std::holds_alternative<std::string>(maybe_template)) {
            print_warning(std::format(
                "error occurred while loading a list of available templates at template directory `{}` - {}",
                static_cast<std::string>(directory_entry.path()), std::get<std::string>(maybe_template)));
            continue;
        }

        // add loaded template to the resulting map
        result.insert_or_assign(directory_entry.path().filename(), std::get<project_template>(maybe_template));
    }

    // return collected project templates
    return result;
}

std::variant<std::string, project_template>
project_template::template_from_directory(const std::string& directory_path) {
    // check if directory even exists
    if (std::error_code code; !std::filesystem::is_directory(directory_path, code) || code)
        return std::format("cannot read template directory `{}`", directory_path);

    // check if template info file exists
    std::string template_info_path = std::filesystem::path { directory_path } / template_info_file_name;
    if (std::error_code code; !std::filesystem::is_regular_file(template_info_path, code) || code)
        return std::format("cannot find or read template info file `{}`", template_info_path);

    // try to parse info
    auto maybe_template_info = template_info::parse_from_file(template_info_path);
    if (std::holds_alternative<std::string>(maybe_template_info))
        return std::get<std::string>(maybe_template_info);

    // if info is correct, create a template
    return project_template { std::filesystem::absolute(directory_path), std::get<template_info>(maybe_template_info) };
}

std::variant<std::string, project_template>
project_template::create_new_template(std::string template_name, std::optional<std::string> maybe_source_directory,
                                      bool should_write_empty_info) {
    // ensure projects directory exitss
    std::string projects_directory_path =
        std::filesystem::path { os::get_lppm_config_directory() } / templates_directory_name;
    os::ensure_directory_exists(projects_directory_path);

    // check whether project with the same name exists
    std::string maybe_template_path = std::filesystem::path { projects_directory_path } / template_name;
    if (std::error_code code; std::filesystem::exists(maybe_template_path, code) || code) {
        return std::format(
            "tried to create a project template at path `{}`, but the file with this name already exists",
            maybe_template_path);
    }

    // create the directory for the new project
    std::error_code code {};
    std::filesystem::create_directory(maybe_template_path, code);
    if (code)
        return std::format("cannot create directory `{}` for newly created project template", maybe_template_path);
    std::string template_path { maybe_template_path };

    // if any source directory is provided, copy all the files from it into the newly created project
    if (maybe_source_directory.has_value()) {
        auto source_directory = maybe_source_directory.value();

        // check if source directory exists
        if (std::error_code code; !std::filesystem::is_directory(source_directory, code) || code) {
            std::filesystem::remove_all(template_path, code);
            return std::format("cannot access source directory for newly created project directory", source_directory);
        }

        // recursively copy all the files from the source directory, to the new template directory
        std::filesystem::copy(source_directory, template_path, std::filesystem::copy_options::recursive, code);
        if (code) {
            std::filesystem::remove_all(template_path, code);
            return std::format("cannot copy files from source directory to newly created project directory",
                               source_directory);
        }
    }

    // create a template info for the new project template and write it out
    if (should_write_empty_info) {
        // NOTE: if source directory contains .lppm_template file, it will be overriden here
        template_info info { {} };
        std::string info_path = std::filesystem::path { template_path } / template_info_file_name;
        auto save_result = info.save_to_file(info_path);
        if (save_result.has_value()) {
            std::filesystem::remove_all(template_path, code);
            return std::format("error occurred while writing a template info file `{}` - {}", info_path,
                               save_result.value());
        }
    }

    // read the template in and return it
    auto maybe_template = template_from_directory(template_path);
    if (std::holds_alternative<std::string>(maybe_template)) {
        print_internal_error_and_exit(
            std::format("cannot create a project_template object from newly created template at `{}` - {}",
                        template_path, std::get<std::string>(maybe_template)));
    }
    return std::get<project_template>(maybe_template);
}

std::variant<std::string, project_template> project_template::import_template(const std::string& template_name,
                                                                              const std::string& source_directory) {
    // try to read info
    std::string info_path = std::filesystem::path { source_directory } / template_info_file_name;
    auto maybe_info = template_info::parse_from_file(info_path);
    if (std::holds_alternative<std::string>(maybe_info)) {
        return std::format("error occurred while reading imported template's info from file `{}` - {}", info_path,
                           std::get<std::string>(maybe_info));
    }

    // create template
    return create_new_template(template_name, source_directory, false);
}

const std::string& project_template::base_directory() const { return m_base_directory; }

const template_info& project_template::info() const { return m_info; }

std::optional<std::string> project_template::save_info() const {
    std::string info_path = std::filesystem::path { base_directory() } / template_info_file_name;
    return m_info.save_to_file(info_path);
}

} // namespace lppm
