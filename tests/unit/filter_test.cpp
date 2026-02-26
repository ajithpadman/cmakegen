#include <gtest/gtest.h>
#include "copy/filter.hpp"
#include <filesystem>

TEST(FilterTest, MatchesExtension) {
    scaffolder::PathFilters pf;
    scaffolder::Filter filter(pf);
    EXPECT_TRUE(filter.matches_extension(std::filesystem::path("foo.c"), {"*.c", "*.cpp"}));
    EXPECT_TRUE(filter.matches_extension(std::filesystem::path("foo.cpp"), {"*.c", "*.cpp"}));
    EXPECT_FALSE(filter.matches_extension(std::filesystem::path("foo.c"), {"*.h"}));
}

TEST(FilterTest, ShouldIncludeEmptyFilters) {
    scaffolder::PathFilters pf;
    scaffolder::Filter filter(pf);
    std::filesystem::path p("/a/b/c");
    std::filesystem::path base("/a");
    EXPECT_TRUE(filter.should_include(p, base));
}

TEST(FilterTest, ExcludePath) {
    scaffolder::PathFilters pf;
    pf.exclude_paths.push_back("test");
    scaffolder::Filter filter(pf);
    std::filesystem::path p("/a/b/test/c");
    std::filesystem::path base("/a");
    EXPECT_FALSE(filter.should_include(p, base));
}
