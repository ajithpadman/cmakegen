#include "generator/condition_evaluator.hpp"
#include <algorithm>
#include <sstream>

namespace scaffolder {

bool ConditionEvaluator::evaluate(const Condition& cond, const Variables& vars) const {
    if (cond.default_) return true;
    return eval_atom(cond, vars);
}

bool ConditionEvaluator::eval_atom(const Condition& cond, const Variables& vars) const {
    if (cond.and_) {
        for (const auto& c : *cond.and_) {
            if (!evaluate(*c, vars)) return false;
        }
        return true;
    }
    if (cond.or_) {
        for (const auto& c : *cond.or_) {
            if (evaluate(*c, vars)) return true;
        }
        return false;
    }
    if (cond.not_) {
        return !evaluate(**cond.not_, vars);
    }
    if (cond.var && cond.op) {
        auto it = vars.find(*cond.var);
        std::string val = (it != vars.end()) ? it->second : "";
        if (cond.op == "equals" && cond.value) {
            if (std::holds_alternative<std::string>(*cond.value))
                return val == std::get<std::string>(*cond.value);
        }
        if (cond.op == "in" && cond.value) {
            if (auto* v = std::get_if<std::vector<std::string>>(&*cond.value)) {
                return std::find(v->begin(), v->end(), val) != v->end();
            }
        }
        if (cond.op == "not_in" && cond.value) {
            if (auto* v = std::get_if<std::vector<std::string>>(&*cond.value)) {
                return std::find(v->begin(), v->end(), val) == v->end();
            }
        }
    }
    return false;
}

std::string ConditionEvaluator::to_cmake_if(const Condition& cond) const {
    return to_cmake_atom(cond);
}

std::string ConditionEvaluator::to_cmake_atom(const Condition& cond) const {
    if (cond.default_) return "TRUE";
    if (cond.and_) {
        std::string r;
        for (size_t i = 0; i < cond.and_->size(); ++i) {
            if (i) r += " AND ";
            r += "(" + to_cmake_if(*cond.and_->at(i)) + ")";
        }
        return r.empty() ? "TRUE" : r;
    }
    if (cond.or_) {
        std::string r;
        for (size_t i = 0; i < cond.or_->size(); ++i) {
            if (i) r += " OR ";
            r += "(" + to_cmake_if(*cond.or_->at(i)) + ")";
        }
        return r.empty() ? "FALSE" : r;
    }
    if (cond.not_) {
        return "NOT (" + to_cmake_if(**cond.not_) + ")";
    }
    if (cond.var && cond.op && cond.value) {
        std::string var = *cond.var;
        if (cond.op == "equals" && std::holds_alternative<std::string>(*cond.value)) {
            return var + " STREQUAL \"" + std::get<std::string>(*cond.value) + "\"";
        }
        if (cond.op == "in" && std::holds_alternative<std::vector<std::string>>(*cond.value)) {
            const auto& v = std::get<std::vector<std::string>>(*cond.value);
            if (v.empty()) return "FALSE";
            std::string r = "(";
            for (size_t i = 0; i < v.size(); ++i) {
                if (i) r += " OR ";
                r += var + " STREQUAL \"" + v[i] + "\"";
            }
            return r + ")";
        }
        if (cond.op == "not_in" && std::holds_alternative<std::vector<std::string>>(*cond.value)) {
            const auto& v = std::get<std::vector<std::string>>(*cond.value);
            if (v.empty()) return "TRUE";
            std::string r = "(";
            for (size_t i = 0; i < v.size(); ++i) {
                if (i) r += " AND ";
                r += "NOT (" + var + " STREQUAL \"" + v[i] + "\")";
            }
            return r + ")";
        }
    }
    return "FALSE";
}

}  // namespace scaffolder
