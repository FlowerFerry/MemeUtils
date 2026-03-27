#include <catch2/catch_test_macros.hpp>
#include <mmutilspp/openext/exprtk/noncoded.h>
#include <string>
#include <string_view>

using mmupp::openext::exprtk::find_noncoded_spans;
using mmupp::openext::exprtk::noncoded_kind;
using mmupp::openext::exprtk::noncoded_span;

// Returns the raw text covered by a span (including delimiters).
static std::string raw(std::string_view formula, const noncoded_span& s)
{
    return std::string(formula.substr(s.begin, s.end - s.begin));
}

// ── Empty / trivial ───────────────────────────────────────────────────────────

TEST_CASE("empty formula yields no spans", "[exprtk_noncoded]")
{
    auto r = find_noncoded_spans("");
    CHECK(r.empty());
}

TEST_CASE("pure code with no strings or comments yields no spans", "[exprtk_noncoded]")
{
    auto r = find_noncoded_spans("x := (a + b) * 3 / c;");
    CHECK(r.empty());
}

// ── String literals ───────────────────────────────────────────────────────────

TEST_CASE("single string literal is found", "[exprtk_noncoded]")
{
    const std::string_view f = "'hello'";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind  == noncoded_kind::string_literal);
    CHECK(r[0].begin == 0);
    CHECK(r[0].end   == f.size());
    CHECK(raw(f, r[0]) == "'hello'");
}

TEST_CASE("string literal embedded in code", "[exprtk_noncoded]")
{
    const std::string_view f = "getValue('abc1') + 1";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind == noncoded_kind::string_literal);
    CHECK(raw(f, r[0]) == "'abc1'");
}

TEST_CASE("multiple string literals", "[exprtk_noncoded]")
{
    const std::string_view f = "f('x') + g('y', 'z')";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 3);
    CHECK(r[0].kind == noncoded_kind::string_literal);
    CHECK(raw(f, r[0]) == "'x'");
    CHECK(r[1].kind == noncoded_kind::string_literal);
    CHECK(raw(f, r[1]) == "'y'");
    CHECK(r[2].kind == noncoded_kind::string_literal);
    CHECK(raw(f, r[2]) == "'z'");
}

TEST_CASE("string with backslash-escaped quote", "[exprtk_noncoded]")
{
    // exprtk formula: 'it\'s'   (C++ literal: "'it\\'s'")
    const std::string_view f = "'it\\'s'";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind  == noncoded_kind::string_literal);
    CHECK(r[0].begin == 0);
    CHECK(r[0].end   == f.size());
    CHECK(raw(f, r[0]) == "'it\\'s'");
}

TEST_CASE("string containing comment markers is not split", "[exprtk_noncoded]")
{
    // The '//' and '/* */' inside the string must NOT produce comment spans.
    const std::string_view f = "x + 'http://example.com' + 'a /* b */ c' + y";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 2);
    CHECK(r[0].kind == noncoded_kind::string_literal);
    CHECK(raw(f, r[0]) == "'http://example.com'");
    CHECK(r[1].kind == noncoded_kind::string_literal);
    CHECK(raw(f, r[1]) == "'a /* b */ c'");
}

// ── Line comments  // ─────────────────────────────────────────────────────────

TEST_CASE("// comment with no trailing newline", "[exprtk_noncoded]")
{
    const std::string_view f = "// a comment";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind  == noncoded_kind::line_comment);
    CHECK(r[0].begin == 0);
    CHECK(r[0].end   == f.size()); // consumed to EOS
    CHECK(raw(f, r[0]) == "// a comment");
}

TEST_CASE("// comment stops before trailing newline", "[exprtk_noncoded]")
{
    // The '\n' must NOT be included in the span.
    const std::string_view f = "// a comment\nx := 1";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind  == noncoded_kind::line_comment);
    CHECK(r[0].begin == 0);
    CHECK(raw(f, r[0]) == "// a comment"); // no '\n'
    CHECK(r[0].end == std::string_view("// a comment").size());
}

TEST_CASE("// comment after code", "[exprtk_noncoded]")
{
    const std::string_view f = "x := 1; // set x\ny := 2;";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind == noncoded_kind::line_comment);
    CHECK(raw(f, r[0]) == "// set x");
}

// ── Line comments  # ──────────────────────────────────────────────────────────

TEST_CASE("# comment with no trailing newline", "[exprtk_noncoded]")
{
    const std::string_view f = "# shell comment";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind  == noncoded_kind::line_comment);
    CHECK(r[0].begin == 0);
    CHECK(r[0].end   == f.size());
    CHECK(raw(f, r[0]) == "# shell comment");
}

TEST_CASE("# comment stops before trailing newline", "[exprtk_noncoded]")
{
    const std::string_view f = "# comment\nx := 1";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind  == noncoded_kind::line_comment);
    CHECK(raw(f, r[0]) == "# comment");
    CHECK(r[0].end == std::string_view("# comment").size());
}

// ── Block comments ────────────────────────────────────────────────────────────

TEST_CASE("/* block comment */ is found", "[exprtk_noncoded]")
{
    const std::string_view f = "/* this is a block comment */";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind  == noncoded_kind::block_comment);
    CHECK(r[0].begin == 0);
    CHECK(r[0].end   == f.size());
    CHECK(raw(f, r[0]) == "/* this is a block comment */");
}

TEST_CASE("block comment embedded in code", "[exprtk_noncoded]")
{
    const std::string_view f = "x := /* scale */ 2.0 * y;";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind == noncoded_kind::block_comment);
    CHECK(raw(f, r[0]) == "/* scale */");
}

TEST_CASE("multi-line block comment", "[exprtk_noncoded]")
{
    const std::string_view f =
        "x :=\n"
        "/* line one\n"
        "   line two */\n"
        "1 + 2;";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind == noncoded_kind::block_comment);
    CHECK(raw(f, r[0]) == "/* line one\n   line two */");
}

TEST_CASE("comment containing single-quote is not a string", "[exprtk_noncoded]")
{
    // The "'" inside the comment must NOT produce a string_literal span.
    const std::string_view f = "/* it's fine */ x";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind == noncoded_kind::block_comment);
    CHECK(raw(f, r[0]) == "/* it's fine */");
}

// ── Unterminated tokens ───────────────────────────────────────────────────────

TEST_CASE("unterminated string is consumed to end of input", "[exprtk_noncoded]")
{
    const std::string_view f = "getValue('unclosed";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind  == noncoded_kind::string_literal);
    CHECK(r[0].begin == 9);
    CHECK(r[0].end   == f.size());
}

TEST_CASE("unterminated block comment is consumed to end of input", "[exprtk_noncoded]")
{
    const std::string_view f = "x + /* unclosed comment";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    CHECK(r[0].kind  == noncoded_kind::block_comment);
    CHECK(r[0].begin == 4);
    CHECK(r[0].end   == f.size());
}

// ── Mixed / integration ───────────────────────────────────────────────────────

TEST_CASE("mixed formula returns all spans in source order", "[exprtk_noncoded]")
{
    // Formula: getValue('k1') + /* mid */ getDailyValue('k2') // tail
    const std::string_view f =
        "getValue('k1') + /* mid */ getDailyValue('k2') // tail";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 4);

    CHECK(r[0].kind == noncoded_kind::string_literal);
    CHECK(raw(f, r[0]) == "'k1'");

    CHECK(r[1].kind == noncoded_kind::block_comment);
    CHECK(raw(f, r[1]) == "/* mid */");

    CHECK(r[2].kind == noncoded_kind::string_literal);
    CHECK(raw(f, r[2]) == "'k2'");

    CHECK(r[3].kind == noncoded_kind::line_comment);
    CHECK(raw(f, r[3]) == "// tail");
}

TEST_CASE("spans are ordered by begin position", "[exprtk_noncoded]")
{
    const std::string_view f = "# c1\n's1' /* c2 */ 'S2' // c3";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 5);
    for (std::size_t i = 1; i < r.size(); ++i)
        CHECK(r[i].begin > r[i - 1].begin);
}

TEST_CASE("begin and end produce the correct raw substring", "[exprtk_noncoded]")
{
    const std::string_view f = "a + 'hello' + 1";
    auto r = find_noncoded_spans(f);
    REQUIRE(r.size() == 1);
    // Verify half-open semantics: end - begin == length of raw token.
    CHECK(r[0].end - r[0].begin == std::string_view("'hello'").size());
}
