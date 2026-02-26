#pragma once

#include "../metadata/schema.hpp"
#include <map>
#include <string>

namespace scaffolder {

class ConditionEvaluator {
public:
    using Variables = std::map<std::string, std::string>;
    bool evaluate(const Condition& cond, const Variables& vars) const;
    std::string to_cmake_if(const Condition& cond) const;

private:
    bool eval_atom(const Condition& cond, const Variables& vars) const;
    std::string to_cmake_atom(const Condition& cond) const;
};

}  // namespace scaffolder
