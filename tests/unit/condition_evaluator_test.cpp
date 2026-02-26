#include <gtest/gtest.h>
#include "generator/condition_evaluator.hpp"
#include "metadata/schema.hpp"

TEST(ConditionEvaluatorTest, Equals) {
    scaffolder::Condition c;
    c.var = "SOC";
    c.op = "equals";
    c.value = std::string("stm32h7");
    scaffolder::ConditionEvaluator eval;
    scaffolder::ConditionEvaluator::Variables vars;

    vars["SOC"] = "stm32h7";
    EXPECT_TRUE(eval.evaluate(c, vars));

    vars["SOC"] = "stm32g4";
    EXPECT_FALSE(eval.evaluate(c, vars));
}

TEST(ConditionEvaluatorTest, In) {
    scaffolder::Condition c;
    c.var = "SOC";
    c.op = "in";
    c.value = std::vector<std::string>{"stm32h7", "stm32g4"};
    scaffolder::ConditionEvaluator eval;
    scaffolder::ConditionEvaluator::Variables vars;

    vars["SOC"] = "stm32h7";
    EXPECT_TRUE(eval.evaluate(c, vars));

    vars["SOC"] = "stm32g4";
    EXPECT_TRUE(eval.evaluate(c, vars));

    vars["SOC"] = "stm32f4";
    EXPECT_FALSE(eval.evaluate(c, vars));
}

TEST(ConditionEvaluatorTest, Default) {
    scaffolder::Condition c;
    c.default_ = true;
    scaffolder::ConditionEvaluator eval;
    scaffolder::ConditionEvaluator::Variables vars;
    EXPECT_TRUE(eval.evaluate(c, vars));
}

TEST(ConditionEvaluatorTest, ToCmakeEquals) {
    scaffolder::Condition c;
    c.var = "SOC";
    c.op = "equals";
    c.value = std::string("stm32h7");
    scaffolder::ConditionEvaluator eval;
    std::string cmake = eval.to_cmake_if(c);
    EXPECT_TRUE(cmake.find("STREQUAL") != std::string::npos);
    EXPECT_TRUE(cmake.find("stm32h7") != std::string::npos);
}
