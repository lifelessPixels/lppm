#pragma once
#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include <lppm/common.h>

namespace lppm {

using operation_handler = std::function<bool(const std::vector<std::string>& arguments)>;

struct operation_argument {
public:
    std::string description;
    bool required;
};

struct operation {
public:
    operation(operation_handler _handler, std::vector<operation_argument> _arguments, std::string _description)
        : handler(_handler), arguments(std::move(_arguments)), description(std::move(_description)) {}
    operation(std::map<std::string, operation> _suboperations, std::string _description)
        : handler(nullptr), suboperations(std::move(_suboperations)), description(std::move(_description)) {};

    const std::variant<std::nullptr_t, operation_handler> handler { nullptr };
    const std::vector<operation_argument> arguments {};
    const std::map<std::string, operation> suboperations {};
    const std::string description {};

    bool has_suboperations() const { return suboperations.size() > 0; };
    usz required_argument_count() const {
        return std::count_if(arguments.cbegin(), arguments.cend(),
                             [](const lppm::operation_argument& arg) { return arg.required; });
    }
};

} // namespace lppm
