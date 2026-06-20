#include <catch2/catch_test_macros.hpp>
#include <mmutilspp/locale/tag.h>
#include <memepp/string_view.hpp>
#include <string>
#include <cstring>

using mmupp::locale::simple_tag;

static simple_tag parse(const char* s) {
    return simple_tag::parse_from(memepp::string_view(s, std::strlen(s)));
}

// Returns std::string from a memepp::rune (empty rune → "")
static std::string to_str(const memepp::rune& r) {
    if (r.size() == 0) return {};
    return std::string(reinterpret_cast<const char*>(r.data()), r.size());
}

// ---------------------------------------------------------------------------
// Basic valid tags
// ---------------------------------------------------------------------------

TEST_CASE("simple_tag parse language-only 2-char code", "[locale][tag]") {
    auto tag = parse("en");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.language()) == "en");
    CHECK(tag.script().size() == 0);
    CHECK(tag.region().size() == 0);
}

TEST_CASE("simple_tag parse language-only 3-char code", "[locale][tag]") {
    auto tag = parse("zho");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.language()) == "zho");
    CHECK(tag.script().size() == 0);
    CHECK(tag.region().size() == 0);
}

TEST_CASE("simple_tag parse full lang-Script-REGION with hyphens", "[locale][tag]") {
    auto tag = parse("zh-Hans-CN");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.language()) == "zh");
    CHECK(to_str(tag.script())   == "Hans");
    CHECK(to_str(tag.region())   == "CN");
}

TEST_CASE("simple_tag parse full tag with underscore separators", "[locale][tag]") {
    auto tag = parse("zh_Hans_CN");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.language()) == "zh");
    CHECK(to_str(tag.script())   == "Hans");
    CHECK(to_str(tag.region())   == "CN");
}

TEST_CASE("simple_tag parse lang-Script only (no region)", "[locale][tag]") {
    auto tag = parse("zh-Hans");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.language()) == "zh");
    CHECK(to_str(tag.script())   == "Hans");
    CHECK(tag.region().size() == 0);
}

TEST_CASE("simple_tag lang-REGION without script: region is captured at part_index==1", "[locale][tag]") {
    auto tag = parse("en-US");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.language()) == "en");
    CHECK(tag.script().size() == 0);
    CHECK(to_str(tag.region()) == "US");
}

// ---------------------------------------------------------------------------
// Invalid tags
// ---------------------------------------------------------------------------

TEST_CASE("simple_tag parse empty string is invalid", "[locale][tag]") {
    auto tag = parse("");
    CHECK_FALSE(tag.valid());
}

TEST_CASE("simple_tag parse single-character language is invalid (< 2 chars)", "[locale][tag]") {
    auto tag = parse("e");
    CHECK_FALSE(tag.valid());
}

TEST_CASE("simple_tag parse language longer than 3 chars is invalid", "[locale][tag]") {
    auto tag = parse("english");
    CHECK_FALSE(tag.valid());
}

TEST_CASE("simple_tag parse language with digits is invalid", "[locale][tag]") {
    auto tag = parse("e1");
    CHECK_FALSE(tag.valid());
}

// ---------------------------------------------------------------------------
// Case normalization
// ---------------------------------------------------------------------------

TEST_CASE("simple_tag language is normalized to lowercase", "[locale][tag]") {
    auto tag = parse("ZH-Hans-CN");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.language()) == "zh");
}

TEST_CASE("simple_tag script is normalized to title-case (first letter upper, rest lower)", "[locale][tag]") {
    auto tag = parse("zh-hans-CN");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.script()) == "Hans");
}

TEST_CASE("simple_tag script all-uppercase is normalized to title-case", "[locale][tag]") {
    auto tag = parse("zh-HANS-CN");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.script()) == "Hans");
}

TEST_CASE("simple_tag region is normalized to uppercase", "[locale][tag]") {
    auto tag = parse("zh-Hans-cn");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.region()) == "CN");
}

// ---------------------------------------------------------------------------
// Projection helpers
// ---------------------------------------------------------------------------

TEST_CASE("simple_tag to_lang_only strips script and region", "[locale][tag]") {
    auto tag = parse("zh-Hans-CN");
    REQUIRE(tag.valid());
    auto lang = tag.to_lang_only();
    CHECK(lang.valid());
    CHECK(to_str(lang.language()) == "zh");
    CHECK(lang.script().size() == 0);
    CHECK(lang.region().size() == 0);
}

TEST_CASE("simple_tag to_lang_region_only strips script, preserves region", "[locale][tag]") {
    auto tag = parse("zh-Hans-CN");
    REQUIRE(tag.valid());
    auto lr = tag.to_lang_region_only();
    CHECK(lr.valid());
    CHECK(to_str(lr.language()) == "zh");
    CHECK(lr.script().size() == 0);
    CHECK(to_str(lr.region()) == "CN");
}

TEST_CASE("simple_tag to_lang_only preserves validity from original tag", "[locale][tag]") {
    auto tag = parse("en");
    REQUIRE(tag.valid());
    auto lang = tag.to_lang_only();
    CHECK(lang.valid());
    CHECK(to_str(lang.language()) == "en");
}
