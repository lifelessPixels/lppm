#pragma once

#include <map>
#include <string>

namespace lppm {

std::string do_the_substitutions(const std::string& text, std::map<std::string, std::string>& mappings);

} // namespace lppm
