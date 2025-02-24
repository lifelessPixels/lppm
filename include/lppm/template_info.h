#pragma once
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace lppm {

class template_info {
public:
    explicit template_info(std::vector<std::string> commands);

    static std::variant<std::string, template_info> parse_from_file(const std::string& path);
    std::optional<std::string> save_to_file(const std::string& path) const;

    std::vector<std::string>& commands() const;
    std::optional<std::string> run_commands_at(std::string directory_path,
                                               std::map<std::string, std::string>& mappings) const;

private:
    static inline std::string header_string_v1 = std::string { "LPPM TEMPLATE V1" };

    mutable std::vector<std::string> m_commands {};
};

} // namespace lppm
