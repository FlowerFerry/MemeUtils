#include <catch2/catch_test_macros.hpp>
#include <mmutilspp/locale/tag.h>
#include <memepp/string_view.hpp>
#include <string>
#include <cstring>
#include <unordered_set>
#include <functional>

using mmupp::locale::simple_tag;

static simple_tag parse(const char* s) {
    return simple_tag::parse_from(memepp::string_view(s, std::strlen(s)));
}

// Returns std::string from a subtag (empty subtag → "")
static std::string to_str(const mmupp::locale::subtag& r) {
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

// ---------------------------------------------------------------------------
// to_string serialization
// ---------------------------------------------------------------------------

TEST_CASE("simple_tag to_string language-only", "[locale][tag][to_string]") {
    auto tag = parse("en");
    auto s = tag.to_string();
    CHECK(std::string(s.data(), s.size()) == "en");
}

TEST_CASE("simple_tag to_string lang-region skips empty script", "[locale][tag][to_string]") {
    auto tag = parse("en-US");
    auto s = tag.to_string();
    CHECK(std::string(s.data(), s.size()) == "en-US");
}

TEST_CASE("simple_tag to_string full lang-Script-REGION", "[locale][tag][to_string]") {
    auto tag = parse("zh-Hans-CN");
    auto s = tag.to_string();
    CHECK(std::string(s.data(), s.size()) == "zh-Hans-CN");
}

TEST_CASE("simple_tag to_string on invalid tag yields empty string", "[locale][tag][to_string]") {
    auto tag = parse("e1");
    CHECK_FALSE(tag.valid());
    auto s = tag.to_string();
    CHECK(s.empty());
}

TEST_CASE("simple_tag to_string round-trips through parse_from", "[locale][tag][to_string]") {
    const char* inputs[] = { "en", "zho", "en-US", "zh-Hans-CN", "srp-Cyrl-RS" };
    for (const char* in : inputs) {
        auto t1 = parse(in);
        REQUIRE(t1.valid());
        auto s = t1.to_string();
        auto t2 = simple_tag::parse_from(memepp::string_view(s.data(), s.size()));
        REQUIRE(t2.valid());
        CHECK(t1 == t2);
    }
}

// ---------------------------------------------------------------------------
// three-argument direct construction
// ---------------------------------------------------------------------------

TEST_CASE("simple_tag direct construction from string_views", "[locale][tag][ctor]") {
    simple_tag tag(memepp::string_view("zh", 2),
                   memepp::string_view("Hans", 4),
                   memepp::string_view("CN", 2));
    CHECK(tag.valid());
    CHECK(to_str(tag.language()) == "zh");
    CHECK(to_str(tag.script()) == "Hans");
    CHECK(to_str(tag.region()) == "CN");
}

TEST_CASE("simple_tag direct construction with language only", "[locale][tag][ctor]") {
    simple_tag tag(memepp::string_view("en", 2));
    CHECK(tag.valid());
    CHECK(tag.script().empty());
    CHECK(tag.region().empty());
}

TEST_CASE("simple_tag direct construction with invalid language is invalid", "[locale][tag][ctor]") {
    simple_tag tag(memepp::string_view("toolong", 7));
    CHECK_FALSE(tag.valid());
}

TEST_CASE("simple_tag default construction is invalid and empty", "[locale][tag][ctor]") {
    simple_tag tag;
    CHECK_FALSE(tag.valid());
    CHECK(tag.language().empty());
    CHECK(tag.script().empty());
    CHECK(tag.region().empty());
}

// ---------------------------------------------------------------------------
// equality / inequality
// ---------------------------------------------------------------------------

TEST_CASE("simple_tag equal tags compare equal", "[locale][tag][eq]") {
    CHECK(parse("zh-Hans-CN") == parse("ZH-hans-cn"));  // normalization makes them equal
    CHECK(parse("en") == parse("EN"));
}

TEST_CASE("simple_tag different tags compare not-equal", "[locale][tag][eq]") {
    CHECK(parse("en") != parse("en-US"));
    CHECK(parse("en-US") != parse("en-GB"));
    CHECK(parse("zh-Hans-CN") != parse("zh-Hant-CN"));
    CHECK(parse("en") != parse("e1"));            // valid vs invalid
    CHECK(parse("en") != simple_tag{});           // valid vs default
}

// ---------------------------------------------------------------------------
// numeric region and malformed part handling
// ---------------------------------------------------------------------------

TEST_CASE("simple_tag parse 3-digit numeric region", "[locale][tag][region]") {
    auto tag = parse("en-419");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.language()) == "en");
    CHECK(tag.script().size() == 0);
    CHECK(to_str(tag.region()) == "419");
}

TEST_CASE("simple_tag parse region after script with numeric code", "[locale][tag][region]") {
    auto tag = parse("es-419");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.region()) == "419");
}

TEST_CASE("simple_tag invalid middle part is skipped, region still parsed", "[locale][tag][region]") {
    // "Hansx" is 5 chars: neither script (4 alpha) nor region (2 alpha/3 digit).
    // Parsing continues to part_index==1... region at part 2 position.
    auto tag = parse("en-Hansx-US");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.language()) == "en");
    CHECK(tag.script().size() == 0);
    CHECK(to_str(tag.region()) == "US");
}

TEST_CASE("simple_tag duplicate region keeps the first", "[locale][tag][region]") {
    auto tag = parse("en-US-GB");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.region()) == "US");
}

TEST_CASE("simple_tag trailing separator does not invalidate tag", "[locale][tag][region]") {
    auto tag = parse("en-");
    REQUIRE(tag.valid());
    CHECK(to_str(tag.language()) == "en");
    CHECK(tag.region().size() == 0);
}

// ---------------------------------------------------------------------------
// std::hash specialization
// ---------------------------------------------------------------------------

TEST_CASE("std::hash simple_tag equal tags hash equal", "[locale][tag][hash]") {
    std::hash<simple_tag> h;
    CHECK(h(parse("zh-Hans-CN")) == h(parse("ZH-hans-cn")));
    CHECK(h(parse("en")) == h(parse("en")));
}

TEST_CASE("std::hash simple_tag distinguishes field boundaries", "[locale][tag][hash]") {
    std::hash<simple_tag> h;
    // different tags should (with overwhelming probability) differ
    CHECK(h(parse("en-US")) != h(parse("en-GB")));
    CHECK(h(parse("en")) != h(parse("zh")));
    CHECK(h(parse("zh-Hans-CN")) != h(parse("zh-Hant-CN")));
}

TEST_CASE("std::hash simple_tag usable in unordered_set", "[locale][tag][hash]") {
    std::unordered_set<simple_tag> set;
    set.insert(parse("en"));
    set.insert(parse("EN"));          // same after normalization
    set.insert(parse("zh-Hans-CN"));
    set.insert(parse("zh-hans-cn"));  // duplicate
    CHECK(set.size() == 2);
    CHECK(set.count(parse("en-US")) == 0);
    CHECK(set.count(parse("en")) == 1);
}
