#include <lppm/globals.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>

#include <lppm/cli.h>
#include <lppm/common.h>
#include <lppm/os.h>
#include <lppm/utils.h>

namespace lppm {

globals::globals() {
    // read the globals file (if any exists)
    std::ifstream globals_file { get_globals_file_path() };
    if (globals_file) {
        // read lines one by line
        for (std::string line {}; std::getline(globals_file, line);) {
            // trim the line from the left to remove all leading whitespaces
            trim_string_left_in_place(line);

            // skip the line if it is empty or a comment
            if (line.empty() || line[0] == '#')
                continue;

            // extract entry name from the read line
            std::string entry_name { line };
            auto colon_iterator = std::find_if(entry_name.begin(), entry_name.end(), [](c8 c) { return c == ':'; });
            usz colon_position = colon_iterator - entry_name.begin();
            if (colon_iterator == entry_name.end()) {
                print_warning(std::format(
                    "malformed replacement variable entry in globals config file (path: `{}`) - `{}` - was the "
                    "file modified by hand?",
                    get_globals_file_path(), line));
                m_was_file_malformed_on_read = true;
                continue;
            }
            entry_name.erase(colon_iterator, entry_name.end());
            trim_string_in_place(entry_name);

            // everything after the colon is treated as global value (trimmed)
            std::string value { line.begin() + colon_position + 1, line.end() };
            trim_string_in_place(value);

            // insert a value into known values map
            if (m_values.contains(entry_name)) {
                print_warning(std::format(
                    "duplicated replacement variable entry `{}` found in globals config file (path: `{}`) - "
                    "using the first value found: `{}` instead of newly found value `{}` - was the "
                    "file incorrectly modified by hand?",
                    entry_name, get_globals_file_path(), m_values[entry_name], value));
                m_was_file_malformed_on_read = true;
                continue;
            }
            m_values[entry_name] = value;
        }
    } else {
        print_warning(std::format(
            "globals config file (path: `{}`) could not be read (missing or permissions error) - consider "
            "running `lppm globals init` command to initialize user data or check your configuration if you "
            "think this should not be the case",
            get_globals_file_path()));
    }
}

globals& globals::the() {
    if (!s_the)
        s_the = new globals {};
    return *s_the;
}

std::map<std::string, std::string> globals::mappings() const { return m_values; }

std::vector<std::string> globals::key_set() const {
    std::vector<std::string> keys {};
    for (auto& [key, _] : m_values)
        keys.push_back(key);
    return keys;
}
bool globals::contains_key(const std::string& key) const { return m_values.contains(key); }

std::optional<std::string> globals::get_value(const std::string& key) const {
    if (contains_key(key))
        return m_values.at(key);
    return {};
}
void globals::set_value(const std::string& key, const std::string& value, bool should_fail_on_override,
                        bool should_save_file) {
    // check if entry already exists
    auto maybe_element = m_values.find(key);
    if (maybe_element != m_values.end()) {
        // prompt user for confirmation
        auto& old_value = maybe_element->second;
        print_warning(
            std::format("as the result of this operation, the global replacement variable `{}` with the old value "
                        "`{}` will be overriten with a new value of `{}`",
                        key, old_value, value));
        bool should_override = prompt_user_boolean("should the variable value be overriten?");
        if (!should_override && should_fail_on_override)
            print_fatal_and_exit(
                std::format("user did not consent to overriding the global replacement variable `{}`", key));
    }

    // set value
    m_values[key] = trim_string(value);

    // save file if necessary
    if (should_save_file)
        save_globals_file();
}

void globals::remove_value(const std::string& key, bool should_save_file) {
    // check if entry exists
    if (!m_values.contains(key))
        print_fatal_and_exit(std::format("globals config file does not contain a value with key `{}`", key));

    // remove entry and save if necessary
    m_values.erase(key);
    if (should_save_file)
        save_globals_file();
}

void globals::save_globals_file() const {
    // check if file was malformed on read
    if (m_was_file_malformed_on_read) {
        print_warning("globals config file is about to be modified, but it was malformed on read");
        bool should_override = prompt_user_boolean("should the file be overritten?");
        if (!should_override)
            print_fatal_and_exit(std::format("user did not consent to overriding globals config file (path: `{}`)",
                                             get_globals_file_path()));

        // do not prompt this on subsequent writes
        m_was_file_malformed_on_read = false;
    }

    // ensure config directory exists
    os::ensure_directory_exists(os::get_lppm_config_directory());

    // open file for writing
    std::ofstream globals_file { get_globals_file_path() };
    if (!globals_file) {
        print_fatal_and_exit(
            std::format("cannot open globals config file for writing (path: `{}`)", get_globals_file_path()));
    }

    // write a header
    globals_file << "# this file contains global replacement variable definitions\n";
    globals_file << "# each entry has a form <key>:<value>\n";
    globals_file << "# it is strongly encouraged to make all keys UPPERCASE and use llpm commands to manage contents "
                    "of this file\n\n";

    // write all entries
    for (auto& [key, value] : m_values)
        globals_file << std::format("{}:{}\n", key, value);
}

std::string globals::get_globals_file_path() {
    return std::filesystem::path { os::get_lppm_config_directory() } / globals_file_name;
}

} // namespace lppm
