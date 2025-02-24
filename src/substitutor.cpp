#include <lppm/substitutor.h>

#include <format>
#include <string>

#include <lppm/cli.h>
#include <lppm/common.h>

namespace lppm {

std::string do_the_substitutions(const std::string& text, std::map<std::string, std::string>& mappings) {
    usz current_index = 0;
    std::string result {};
    usz found_index = 0;
    while ((found_index = text.find("@@", current_index)) != std::string::npos) {
        // if we are here, the double "at" symbol was found at found_index
        // firstly, copy the text up to this point to the result string
        result += text.substr(current_index, found_index - current_index);
        current_index = found_index;

        // find next occurence marking the end of substitution
        usz start_index = found_index + 2;
        usz end_index = text.find("@@", found_index + 2);
        if (end_index == std::string::npos)
            break;

        // extract the substitution variable's name
        std::string variable_name = text.substr(start_index, end_index - start_index);

        // if variable does not exist in current mappings, ask user for the substitution
        if (!mappings.contains(variable_name)) {
            auto value = prompt_user_input(
                std::format("enter substitution value for variable " STYLE_BLUE "@@{}@@" STYLE_RESET, variable_name));
            mappings.insert_or_assign(variable_name, value);
        }

        // substitute the value and continue
        result += mappings.at(variable_name);
        current_index = end_index + 2;
    }

    // no further occurences of "@@" were found, copy all text from current_index to the end of string to the result
    result += text.substr(current_index);

    // return fully substituted string
    return result;
}

} // namespace lppm
