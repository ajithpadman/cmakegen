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

TEST(ConditionEvaluatorTest, And) {
    scaffolder::Condition c;
    c.and_ = std::vector<std::shared_ptr<scaffolder::Condition>>();
    auto a = std::make_shared<scaffolder::Condition>();
    a->var = "SOC";
    a->op = "equals";
    a->value = std::string("stm32h7");
    auto b = std::make_shared<scaffolder::Condition>();
    b->var = "BOARD";
    b->op = "equals";
    b->value = std::string("nucleo");
    c.and_->push_back(a);
    c.and_->push_back(b);

    scaffolder::ConditionEvaluator eval;
    scaffolder::ConditionEvaluator::Variables vars;
    vars["SOC"] = "stm32h7";
    vars["BOARD"] = "nucleo";
    EXPECT_TRUE(eval.evaluate(c, vars));

    vars["BOARD"] = "other";
    EXPECT_FALSE(eval.evaluate(c, vars));
}

TEST(ConditionEvaluatorTest, Or) {
    scaffolder::Condition c;
    c.or_ = std::vector<std::shared_ptr<scaffolder::Condition>>();
    auto a = std::make_shared<scaffolder::Condition>();
    a->var = "SOC";
    a->op = "equals";
    a->value = std::string("stm32h7");
    auto b = std::make_shared<scaffolder::Condition>();
    b->var = "SOC";
    b->op = "equals";
    b->value = std::string("stm32g4");
    c.or_->push_back(a);
    c.or_->push_back(b);

    scaffolder::ConditionEvaluator eval;
    scaffolder::ConditionEvaluator::Variables vars;
    vars["SOC"] = "stm32h7";
    EXPECT_TRUE(eval.evaluate(c, vars));
    vars["SOC"] = "stm32g4";
    EXPECT_TRUE(eval.evaluate(c, vars));
    vars["SOC"] = "stm32f4";
    EXPECT_FALSE(eval.evaluate(c, vars));
}

TEST(ConditionEvaluatorTest, Not) {
    scaffolder::Condition c;
    c.not_ = std::make_shared<scaffolder::Condition>();
    (*c.not_)->var = "BUILD_VARIANT";
    (*c.not_)->op = "equals";
    (*c.not_)->value = std::string("release");

    scaffolder::ConditionEvaluator eval;
    scaffolder::ConditionEvaluator::Variables vars;
    vars["BUILD_VARIANT"] = "debug";
    EXPECT_TRUE(eval.evaluate(c, vars));
    vars["BUILD_VARIANT"] = "release";
    EXPECT_FALSE(eval.evaluate(c, vars));
}

TEST(ConditionEvaluatorTest, CompoundAndOrNot) {
    scaffolder::Condition c;
    c.and_ = std::vector<std::shared_ptr<scaffolder::Condition>>();
    auto soc_in = std::make_shared<scaffolder::Condition>();
    soc_in->var = "SOC";
    soc_in->op = "in";
    soc_in->value = std::vector<std::string>{"stm32h7", "stm32g4"};
    auto not_release = std::make_shared<scaffolder::Condition>();
    not_release->not_ = std::make_shared<scaffolder::Condition>();
    (*not_release->not_)->var = "BUILD_VARIANT";
    (*not_release->not_)->op = "equals";
    (*not_release->not_)->value = std::string("release");
    c.and_->push_back(soc_in);
    c.and_->push_back(not_release);

    scaffolder::ConditionEvaluator eval;
    scaffolder::ConditionEvaluator::Variables vars;
    vars["SOC"] = "stm32h7";
    vars["BUILD_VARIANT"] = "debug";
    EXPECT_TRUE(eval.evaluate(c, vars));
    vars["BUILD_VARIANT"] = "release";
    EXPECT_FALSE(eval.evaluate(c, vars));
}
