#pragma once

#include <string>
#include <vector>

namespace lppm::handlers {

bool globals_get_handler(const std::vector<std::string>& arguments);
bool globals_set_handler(const std::vector<std::string>& arguments);
bool globals_unset_handler(const std::vector<std::string>& arguments);
bool globals_list_handler(const std::vector<std::string>& arguments);
bool globals_init_handler(const std::vector<std::string>& arguments);
bool project_create_handler(const std::vector<std::string>& arguments);
bool project_init_handler(const std::vector<std::string>& arguments);
bool template_import_handler(const std::vector<std::string>& arguments);
bool template_create_handler(const std::vector<std::string>& arguments);
bool template_list_handler(const std::vector<std::string>& arguments);
bool template_show_handler(const std::vector<std::string>& arguments);
bool template_remove_handler(const std::vector<std::string>& arguments);
bool template_cmd_add_handler(const std::vector<std::string>& arguments);
bool template_cmd_remove_handler(const std::vector<std::string>& arguments);
bool template_cmd_list_handler(const std::vector<std::string>& arguments);

} // namespace lppm::handlers
