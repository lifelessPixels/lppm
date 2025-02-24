#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <system_error>

#include <lppm/cli.h>
#include <lppm/os.h>

#if defined(__linux__)
#include <linux/limits.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace lppm {

#if defined(__linux__)
std::string os::get_user_directory() {
    // try to fetch home directory from $HOME environment variable
    const char* home_directory = getenv("HOME");
    if (home_directory != nullptr)
        return home_directory;

    // if $HOME is not available, fetch it directly from /etc/passwd
    struct passwd* user_passwd = getpwuid(getuid());
    if (user_passwd != nullptr)
        return user_passwd->pw_dir;

    // if no directory is available, return current working directory
    char buffer[2 * PATH_MAX] {};
    return getcwd(buffer, 2 * PATH_MAX);
}

std::string os::get_lppm_config_directory() {
    // use XDG Base Directory Specification path by default
    // source: https://specifications.freedesktop.org/basedir-spec/latest/
    const char* config_directory = getenv("XDG_CONFIG_HOME");
    if (config_directory != nullptr && strlen(config_directory) != 0)
        return std::filesystem::path { config_directory } / "lppm";

    // otherwise, use .lppm directory in user's home dir
    return std::filesystem::path { get_user_directory() } / ".config" / "lppm";
}
#endif

std::string os::get_working_directory() { return std::filesystem::current_path(); }

bool os::set_working_directory(const std::string& new_wd) {
    std::error_code code {};
    std::filesystem::current_path(new_wd, code);
    return !static_cast<bool>(code);
}

void os::ensure_directory_exists(const std::string& path) {
    // if it is a directory, just resturn
    if (std::error_code code; std::filesystem::is_directory(path, code))
        return;

    // if no such file exists, create it
    if (std::error_code code; !std::filesystem::exists(path, code) || code) {
        std::filesystem::create_directories(path, code);
        if (code)
            print_fatal_and_exit(std::format("cannot create mandatory directory `{}`", path));
        return;
    }

    // otherwise print error
    print_fatal_and_exit(
        std::format("cannot create mandatory directory `{}` - file with that name already exists", path));
}

int os::run_command(const std::string& command) { return std::system(command.c_str()); }

} // namespace lppm
