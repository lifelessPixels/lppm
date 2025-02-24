#pragma once
#include <string>

namespace lppm {

class os {
public:
    static std::string get_user_directory();
    static std::string get_lppm_config_directory();
    static std::string get_working_directory();
    static bool set_working_directory(const std::string& new_wd);
    static void ensure_directory_exists(const std::string& path);
    static int run_command(const std::string& command);
};

} // namespace lppm
