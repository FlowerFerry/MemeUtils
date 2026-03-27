#include <catch2/catch_test_macros.hpp>
#include <mmutilspp/openext/exprtk/extract.h>
#include <string>

using mmupp::openext::exprtk::extract_formula_ids;
using mmupp::openext::exprtk::formula_id_result;
using mmupp::openext::exprtk::func_id_config;

// ── Basic cases ───────────────────────────────────────────────────────────────

TEST_CASE("empty formula returns empty result", "[exprtk_extract]")
{
    auto r = extract_formula_ids("", func_id_config{"getValue", 0});
    CHECK(r.empty());
}

TEST_CASE("no configs returns empty result", "[exprtk_extract]")
{
    auto r = extract_formula_ids("getValue('abc1')");
    CHECK(r.empty());
}

TEST_CASE("runtime vector configs extract correctly", "[exprtk_extract]")
{
    std::vector<func_id_config> cfgs = {
        {"getValue",      0},
        {"getDailyValue", 0},
    };
    auto r = extract_formula_ids(
        "getValue('abc1') + getDailyValue('abc3', 1)",
        cfgs);

    REQUIRE(r.size() == 2);
    CHECK(r[0].func_name == "getValue");
    CHECK(r[0].id == "abc1");
    CHECK(r[1].func_name == "getDailyValue");
    CHECK(r[1].id == "abc3");
}

TEST_CASE("runtime vector with empty configs returns empty result", "[exprtk_extract]")
{
    std::vector<func_id_config> cfgs;
    auto r = extract_formula_ids("getValue('abc1')", cfgs);
    CHECK(r.empty());
}

TEST_CASE("single call extracts id and func_name", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "getValue('abc1')",
        func_id_config{"getValue", 0});

    REQUIRE(r.size() == 1);
    CHECK(r[0].func_name == "getValue");
    CHECK(r[0].id == "abc1");
}

TEST_CASE("multiple calls of same function extract all ids in order", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "getValue('abc1') + getValue('abc2')",
        func_id_config{"getValue", 0});

    REQUIRE(r.size() == 2);
    CHECK(r[0].func_name == "getValue");
    CHECK(r[0].id == "abc1");
    CHECK(r[1].func_name == "getValue");
    CHECK(r[1].id == "abc2");
}

TEST_CASE("duplicate ids are preserved", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "getValue('abc1') + getValue('abc1')",
        func_id_config{"getValue", 0});

    REQUIRE(r.size() == 2);
    CHECK(r[0].id == "abc1");
    CHECK(r[1].id == "abc1");
}

// ── param_index ───────────────────────────────────────────────────────────────

TEST_CASE("param_index 0 with trailing args: getDailyValue('abc3', 1)", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "getDailyValue('abc3', 1)",
        func_id_config{"getDailyValue", 0});

    REQUIRE(r.size() == 1);
    CHECK(r[0].func_name == "getDailyValue");
    CHECK(r[0].id == "abc3");
}

TEST_CASE("param_index 1 extracts the second argument", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "getTaggedValue(123, 'abc4')",
        func_id_config{"getTaggedValue", 1});

    REQUIRE(r.size() == 1);
    CHECK(r[0].func_name == "getTaggedValue");
    CHECK(r[0].id == "abc4");
}

// ── Mixed configs ─────────────────────────────────────────────────────────────

TEST_CASE("mixed configs extract from multiple function types", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "getValue('abc1') + getDailyValue('abc3', 1)",
        func_id_config{"getValue",      0},
        func_id_config{"getDailyValue", 0});

    REQUIRE(r.size() == 2);
    CHECK(r[0].func_name == "getValue");
    CHECK(r[0].id == "abc1");
    CHECK(r[1].func_name == "getDailyValue");
    CHECK(r[1].id == "abc3");
}

TEST_CASE("complex formula with repeated and mixed calls", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "getValue('abc1') * 2 + getDailyValue('abc3', 1) / getDailyValue('abc4', 2)",
        func_id_config{"getValue",      0},
        func_id_config{"getDailyValue", 0});

    REQUIRE(r.size() == 3);
    CHECK(r[0].func_name == "getValue");
    CHECK(r[0].id == "abc1");
    CHECK(r[1].func_name == "getDailyValue");
    CHECK(r[1].id == "abc3");
    CHECK(r[2].func_name == "getDailyValue");
    CHECK(r[2].id == "abc4");
}

// ── Comment skipping ──────────────────────────────────────────────────────────

TEST_CASE("// line comment is skipped", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "// getValue('commented')\n"
        "getValue('actual')",
        func_id_config{"getValue", 0});

    REQUIRE(r.size() == 1);
    CHECK(r[0].id == "actual");
}

TEST_CASE("# line comment is skipped", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "# getValue('commented')\n"
        "getValue('actual')",
        func_id_config{"getValue", 0});

    REQUIRE(r.size() == 1);
    CHECK(r[0].id == "actual");
}

TEST_CASE("/* */ block comment is skipped", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "/* getValue('commented') */ getValue('actual')",
        func_id_config{"getValue", 0});

    REQUIRE(r.size() == 1);
    CHECK(r[0].id == "actual");
}

TEST_CASE("multi-line block comment is skipped", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "/*\n"
        "  getValue('commented')\n"
        "*/\n"
        "getValue('actual')",
        func_id_config{"getValue", 0});

    REQUIRE(r.size() == 1);
    CHECK(r[0].id == "actual");
}

// ── Silently ignored cases ────────────────────────────────────────────────────

TEST_CASE("non-string at target param_index is silently skipped", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "getValue(42)",
        func_id_config{"getValue", 0});

    CHECK(r.empty());
}

TEST_CASE("function not in config is not extracted", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "otherFunc('abc1') + getValue('abc2')",
        func_id_config{"getValue", 0});

    REQUIRE(r.size() == 1);
    CHECK(r[0].id == "abc2");
}

TEST_CASE("call without opening paren is skipped gracefully", "[exprtk_extract]")
{
    // "getValue" used as a plain symbol, not a call
    auto r = extract_formula_ids(
        "getValue + 1",
        func_id_config{"getValue", 0});

    CHECK(r.empty());
}

// ── String interior ───────────────────────────────────────────────────────────

TEST_CASE("function name inside a string literal is not extracted", "[exprtk_extract]")
{
    // The text 'getValue(x)' is a string parameter of wrap(), not a real call.
    // Our scanner consumes the string atomically so the inner text is invisible.
    auto r = extract_formula_ids(
        "wrap('getValue(x)')",
        func_id_config{"getValue", 0});

    CHECK(r.empty());
}

TEST_CASE("id string may contain special characters", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "getValue('tag/group:item-01_v2')",
        func_id_config{"getValue", 0});

    REQUIRE(r.size() == 1);
    CHECK(r[0].id == "tag/group:item-01_v2");
}

// ── Nesting ───────────────────────────────────────────────────────────────────

TEST_CASE("configured call nested inside unknown function is still extracted", "[exprtk_extract]")
{
    // The outer loop sees all symbol tokens regardless of nesting context,
    // so getValue inside outer(...) is still found.
    auto r = extract_formula_ids(
        "outer(getValue('abc1'))",
        func_id_config{"getValue", 0});

    REQUIRE(r.size() == 1);
    CHECK(r[0].id == "abc1");
}

TEST_CASE("nested parens inside argument are tracked correctly", "[exprtk_extract]")
{
    // The comma after inner(x) is at nest_depth>0, so it does not advance
    // the argument index for getDailyValue.
    auto r = extract_formula_ids(
        "getDailyValue(inner(x, y), 'should_not_match')",
        func_id_config{"getDailyValue", 0});

    // param_index=0: first arg is inner(x,y), not a string → no capture
    CHECK(r.empty());
}

TEST_CASE("param_index 1 with nested parens in first arg", "[exprtk_extract]")
{
    auto r = extract_formula_ids(
        "getDailyValue(inner(x, y), 'abc5')",
        func_id_config{"getDailyValue", 1});

    REQUIRE(r.size() == 1);
    CHECK(r[0].id == "abc5");
}
