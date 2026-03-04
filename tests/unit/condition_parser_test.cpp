#include <gtest/gtest.h>
#include <type_traits>
#include "interactive/condition_parser.hpp"

using scaffolder::ConditionData;
using scaffolder::parse_condition_text;
using scaffolder::condition_to_text;
using scaffolder::condition_data_to_schema;

TEST(ConditionParserTest, SimpleEquals) {
    std::string err;
    auto cond = parse_condition_text("SOC equals stm32h7", &err);
    ASSERT_NE(cond, nullptr);
    EXPECT_TRUE(err.empty());
    ASSERT_TRUE(cond->var.has_value());
    ASSERT_TRUE(cond->op.has_value());
    ASSERT_TRUE(cond->value.has_value());
    EXPECT_EQ(*cond->var, "SOC");
    EXPECT_EQ(*cond->op, "equals");
    std::string value;
    std::visit([&](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) {
            value = v;
        } else {
            value = v.empty() ? "" : v.front();
        }
    }, *cond->value);
    EXPECT_EQ(value, "stm32h7");

    auto text = condition_to_text(cond);
    EXPECT_NE(text.find("SOC"), std::string::npos);
    EXPECT_NE(text.find("equals"), std::string::npos);
    EXPECT_NE(text.find("stm32h7"), std::string::npos);
}

TEST(ConditionParserTest, InListAndOrNot) {
    std::string err;
    auto cond = parse_condition_text(
        "SOC in (stm32h7, stm32g4) AND NOT BUILD_VARIANT equals release",
        &err);
    ASSERT_NE(cond, nullptr);
    EXPECT_TRUE(err.empty());
    ASSERT_TRUE(cond->and_.has_value());
    ASSERT_EQ(cond->and_->size(), 2u);

    auto first = (*cond->and_)[0];
    ASSERT_NE(first, nullptr);
    ASSERT_TRUE(first->var.has_value());
    ASSERT_TRUE(first->op.has_value());
    ASSERT_TRUE(first->value.has_value());
    EXPECT_EQ(*first->var, "SOC");
    EXPECT_EQ(*first->op, "in");

    std::vector<std::string> list;
    std::visit([&](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) {
            list = {v};
        } else {
            list = v;
        }
    }, *first->value);
    ASSERT_EQ(list.size(), 2u);
    EXPECT_EQ(list[0], "stm32h7");
    EXPECT_EQ(list[1], "stm32g4");

    auto second = (*cond->and_)[1];
    ASSERT_NE(second, nullptr);
    ASSERT_TRUE(second->not_.has_value());
    auto inner = *second->not_;
    ASSERT_NE(inner, nullptr);
    ASSERT_TRUE(inner->var.has_value());
    ASSERT_TRUE(inner->op.has_value());
    ASSERT_TRUE(inner->value.has_value());
    EXPECT_EQ(*inner->var, "BUILD_VARIANT");
    EXPECT_EQ(*inner->op, "equals");

    auto text = condition_to_text(cond);
    EXPECT_NE(text.find("SOC"), std::string::npos);
    EXPECT_NE(text.find("in"), std::string::npos);
    EXPECT_NE(text.find("BUILD_VARIANT"), std::string::npos);
    EXPECT_NE(text.find("NOT"), std::string::npos);
}

TEST(ConditionParserTest, DefaultKeyword) {
    std::string err;
    auto cond = parse_condition_text("default", &err);
    ASSERT_NE(cond, nullptr);
    EXPECT_TRUE(err.empty());
    ASSERT_TRUE(cond->default_.has_value());
    EXPECT_TRUE(*cond->default_);
}

TEST(ConditionParserTest, InvalidExpressionSetsError) {
    std::string err;
    auto cond = parse_condition_text("SOC equals", &err);
    EXPECT_EQ(cond, nullptr);
    EXPECT_FALSE(err.empty());
}

// Verify compound condition (AND) maps to schema correctly so JSON has "and": [ {...}, {...} ].
TEST(ConditionParserTest, CompoundConditionToSchemaForJson) {
    std::string err;
    auto cd = parse_condition_text("SOC equals stm32h7 AND BOARD equals nucleo", &err);
    ASSERT_NE(cd, nullptr) << err;
    EXPECT_TRUE(err.empty());

    auto c = condition_data_to_schema(cd);
    ASSERT_TRUE(c.and_.has_value());
    ASSERT_EQ(c.and_->size(), 2u);

    const auto& first = *c.and_->at(0);
    EXPECT_TRUE(first.var.has_value());
    EXPECT_EQ(*first.var, "SOC");
    EXPECT_TRUE(first.op.has_value());
    EXPECT_EQ(*first.op, "equals");
    ASSERT_TRUE(first.value.has_value());
    EXPECT_TRUE(std::holds_alternative<std::string>(*first.value));
    EXPECT_EQ(std::get<std::string>(*first.value), "stm32h7");

    const auto& second = *c.and_->at(1);
    EXPECT_TRUE(second.var.has_value());
    EXPECT_EQ(*second.var, "BOARD");
    EXPECT_TRUE(second.op.has_value());
    EXPECT_EQ(*second.op, "equals");
    ASSERT_TRUE(second.value.has_value());
    EXPECT_TRUE(std::holds_alternative<std::string>(*second.value));
    EXPECT_EQ(std::get<std::string>(*second.value), "nucleo");
}

