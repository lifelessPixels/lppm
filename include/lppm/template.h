#pragma once
#include <map>
#include <optional>
#include <string>
#include <variant>

#include <lppm/template_info.h>

namespace lppm {

class project_template {
public:
    static inline std::string template_info_file_name = ".lppm_template";
    static inline std::string templates_directory_name = "templates";

    static std::map<std::string, project_template> get_all_templates();
    static std::variant<std::string, project_template> template_from_directory(const std::string& directory_path);
    static std::variant<std::string, project_template>
    create_new_template(std::string template_name, std::optional<std::string> maybe_source_directory = {},
                        bool should_write_empty_info = true);
    static std::variant<std::string, project_template> import_template(const std::string& template_name,
                                                                       const std::string& source_directory);

    const std::string& base_directory() const;
    const template_info& info() const;

    std::optional<std::string> save_info() const;

private:
    project_template(std::string base_directory, template_info info);

    std::string m_base_directory {};
    template_info m_info;
};

} // namespace lppm
