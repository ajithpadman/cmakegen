#include <gtest/gtest.h>
#include "copy/filter.hpp"
#include "metadata/schema.hpp"
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

TEST(FilterTest, IncludeFirst_IncludeThenExclude) {
    scaffolder::PathFilters pf;
    pf.filter_mode = scaffolder::FilterMode::IncludeFirst;
    pf.include_paths.push_back("src");
    pf.exclude_paths.push_back("src/test");
    scaffolder::Filter filter(pf);
    std::filesystem::path base("/a");
    EXPECT_TRUE(filter.should_include(std::filesystem::path("/a/src/foo.c"), base));
    EXPECT_FALSE(filter.should_include(std::filesystem::path("/a/src/test/bar.c"), base));
    EXPECT_FALSE(filter.should_include(std::filesystem::path("/a/doc/readme.txt"), base));
}

TEST(FilterTest, ExcludeFirst_ExcludeThenIncludeRescues) {
    scaffolder::PathFilters pf;
    pf.filter_mode = scaffolder::FilterMode::ExcludeFirst;
    pf.exclude_paths.push_back("test");
    pf.include_paths.push_back("test/special");
    scaffolder::Filter filter(pf);
    std::filesystem::path base("/a");
    EXPECT_TRUE(filter.should_include(std::filesystem::path("/a/b/c"), base));
    EXPECT_FALSE(filter.should_include(std::filesystem::path("/a/test/unit/foo.c"), base));
    EXPECT_TRUE(filter.should_include(std::filesystem::path("/a/test/special/bar.c"), base));
}

TEST(FilterTest, ExcludeFirst_NoExclude_AllIncluded) {
    scaffolder::PathFilters pf;
    pf.filter_mode = scaffolder::FilterMode::ExcludeFirst;
    pf.include_paths.push_back("src");
    scaffolder::Filter filter(pf);
    std::filesystem::path base("/a");
    EXPECT_TRUE(filter.should_include(std::filesystem::path("/a/src/foo.c"), base));
    EXPECT_TRUE(filter.should_include(std::filesystem::path("/a/doc/readme.txt"), base));
}
