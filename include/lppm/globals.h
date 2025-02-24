#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace lppm {

class globals {
public:
    static globals& the();

    std::map<std::string, std::string> mappings() const;
    std::vector<std::string> key_set() const;
    bool contains_key(const std::string& key) const;
    std::optional<std::string> get_value(const std::string& key) const;
    void set_value(const std::string& key, const std::string& value, bool should_fail_on_override,
                   bool should_save_file = true);
    void remove_value(const std::string& key, bool should_save_file = true);

private:
    static constexpr std::string globals_file_name = "globals.conf";

    globals();

    void save_globals_file() const;

    static std::string get_globals_file_path();

    static inline globals* s_the { nullptr };

    std::map<std::string, std::string> m_values {};
    mutable bool m_was_file_malformed_on_read { false };
};

} // namespace lppm
