#include <catch2/catch_test_macros.hpp>
#include <mmutilspp/openext/exprtk/loop_guard.h>
#include <string>
#include <string_view>

using mmupp::openext::exprtk::guard_formula_loops;
using mmupp::openext::exprtk::loop_guard_result;

// Helper: check that the result string contains a substring.
static bool contains(const std::string& s, std::string_view sub)
{
    return s.find(sub) != std::string::npos;
}

// ── No loops ─────────────────────────────────────────────────────────────────

TEST_CASE("empty formula returns empty, loop_count 0", "[loop_guard]")
{
    auto r = guard_formula_loops("");
    CHECK(r.formula.empty());
    CHECK(r.loop_count == 0);
}

TEST_CASE("formula without loops is unchanged, loop_count 0", "[loop_guard]")
{
    const std::string f = "x := 2 + getValue('abc') / 3;";
    auto r = guard_formula_loops(f);
    CHECK(r.formula == f);
    CHECK(r.loop_count == 0);
}

TEST_CASE("loop_count 0 means no guard variables needed", "[loop_guard]")
{
    auto r = guard_formula_loops("x := 42;");
    CHECK(r.loop_count == 0);
    CHECK(!contains(r.formula, "__mmutils_loop_guard_"));
}

// ── while ────────────────────────────────────────────────────────────────────

TEST_CASE("while loop injects guard into condition", "[loop_guard]")
{
    auto r = guard_formula_loops("while (x < 10) { x := x + 1; }");
    CHECK(r.loop_count == 1);
    // Guard variable 0 must appear
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    // Original condition wrapped: "and (x < 10)"
    CHECK(contains(r.formula, "and (x < 10)"));
    // Guard increment + limit
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__ + 1) <= 1000000"));
}

TEST_CASE("while loop with max_iterations parameter", "[loop_guard]")
{
    auto r = guard_formula_loops("while (x < 10) { x := x + 1; }", 500);
    CHECK(r.loop_count == 1);
    CHECK(contains(r.formula, "<= 500"));
}

TEST_CASE("while: guard variable name is __mmutils_loop_guard_0__", "[loop_guard]")
{
    auto r = guard_formula_loops("while (1) { x := x + 1; }");
    CHECK(r.loop_count == 1);
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    // Must NOT contain a plain _loop_guard_ style name
    CHECK(!contains(r.formula, "_loop_guard_0__mmutils"));
}

TEST_CASE("while: formula structure after injection is syntactically plausible", "[loop_guard]")
{
    // Verify the outer while(...) now starts with the guard expression
    auto r = guard_formula_loops("while (x < 5) { x := x + 1; }");
    // Should start with: while (((__mmutils_loop_guard_0__ := ...
    CHECK(contains(r.formula, "while (((__mmutils_loop_guard_0__"));
    // The original condition should follow as second operand
    CHECK(contains(r.formula, ") and (x < 5)"));
}

// ── for ──────────────────────────────────────────────────────────────────────

TEST_CASE("for loop with condition injects guard into condition", "[loop_guard]")
{
    auto r = guard_formula_loops("for (var i := 0; i < 10; i += 1) { }");
    CHECK(r.loop_count == 1);
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    CHECK(contains(r.formula, "and (i < 10)"));
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__ + 1) <= 1000000"));
}

TEST_CASE("for loop: guard wraps condition with and", "[loop_guard]")
{
    auto r = guard_formula_loops("for (var i := 0; i < 5; i += 1) { x := x + i; }");
    CHECK(r.loop_count == 1);
    // Resulting condition section should look like:
    // ((__mmutils_loop_guard_0__ := __mmutils_loop_guard_0__ + 1) <= 1000000) and (i < 5)
    CHECK(contains(r.formula,
        "(__mmutils_loop_guard_0__ := __mmutils_loop_guard_0__ + 1) <= 1000000) and (i < 5)"));
}

TEST_CASE("for loop with empty condition (for(;;)) gets guard as condition", "[loop_guard]")
{
    auto r = guard_formula_loops("for (var i := 0; ; i += 1) { if (i > 9) break; }");
    CHECK(r.loop_count == 1);
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    // Should NOT contain "and (" since there was no original condition
    // The guard IS the condition — sits between the two semicolons
    CHECK(!contains(r.formula, ") and ("));
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__ + 1) <= 1000000"));
}

TEST_CASE("for loop with max_iterations parameter", "[loop_guard]")
{
    auto r = guard_formula_loops("for (var i := 0; i < 10; i += 1) { }", 99);
    CHECK(r.loop_count == 1);
    CHECK(contains(r.formula, "<= 99"));
}

// ── repeat...until ───────────────────────────────────────────────────────────

TEST_CASE("repeat-until loop injects guard into until condition", "[loop_guard]")
{
    auto r = guard_formula_loops("repeat { x := x + 1; } until (x >= 10)");
    CHECK(r.loop_count == 1);
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    // Original condition preserved as first operand: (x >= 10)
    CHECK(contains(r.formula, "(x >= 10)"));
    // Guard uses >= MAX (stop when guard hits max)
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__ + 1) >= 1000000"));
}

TEST_CASE("repeat-until: guard is or-ed after original condition", "[loop_guard]")
{
    auto r = guard_formula_loops("repeat { x := x + 1; } until (x >= 5)");
    CHECK(r.loop_count == 1);
    // Should look like: until ((x >= 5) or ((...) >= MAX))
    CHECK(contains(r.formula, "until ((x >= 5) or ("));
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
}

TEST_CASE("repeat-until with max_iterations parameter", "[loop_guard]")
{
    auto r = guard_formula_loops("repeat { x := x + 1; } until (x > 0)", 200);
    CHECK(r.loop_count == 1);
    CHECK(contains(r.formula, ">= 200"));
}

// ── Multiple / nested loops ───────────────────────────────────────────────────

TEST_CASE("two sequential while loops get distinct guard variables", "[loop_guard]")
{
    auto r = guard_formula_loops(
        "while (a < 5) { a := a + 1; }; while (b < 5) { b := b + 1; }");
    CHECK(r.loop_count == 2);
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    CHECK(contains(r.formula, "__mmutils_loop_guard_1__"));
}

TEST_CASE("for + while + repeat-until each get own guard variable", "[loop_guard]")
{
    const std::string f =
        "for (var i := 0; i < 3; i += 1) { }; "
        "while (x < 10) { x := x + 1; }; "
        "repeat { y := y + 1; } until (y >= 5)";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 3);
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    CHECK(contains(r.formula, "__mmutils_loop_guard_1__"));
    CHECK(contains(r.formula, "__mmutils_loop_guard_2__"));
}

TEST_CASE("nested while loops each get own guard variable", "[loop_guard]")
{
    auto r = guard_formula_loops(
        "while (i < 3) { while (j < 3) { j := j + 1; }; i := i + 1; }");
    CHECK(r.loop_count == 2);
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    CHECK(contains(r.formula, "__mmutils_loop_guard_1__"));
}

TEST_CASE("loop_count equals number of distinct guard variables injected", "[loop_guard]")
{
    auto r = guard_formula_loops(
        "while(a<1){a:=a+1;} while(b<1){b:=b+1;} while(c<1){c:=c+1;}");
    CHECK(r.loop_count == 3);
    // All three guard var names must appear
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    CHECK(contains(r.formula, "__mmutils_loop_guard_1__"));
    CHECK(contains(r.formula, "__mmutils_loop_guard_2__"));
    // Guard variable 3 must NOT appear (there are only 3 loops)
    CHECK(!contains(r.formula, "__mmutils_loop_guard_3__"));
}

// ── Keywords inside strings must NOT be modified ──────────────────────────────

TEST_CASE("'while' inside string literal is not treated as loop", "[loop_guard]")
{
    const std::string f = "label := 'while (x < 10) { x := x + 1; }'";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}

TEST_CASE("'for' inside string literal is not treated as loop", "[loop_guard]")
{
    const std::string f = "s := 'for (var i := 0; i < 10; i += 1) { }'";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}

TEST_CASE("'repeat' inside string literal is not treated as loop", "[loop_guard]")
{
    const std::string f = "s := 'repeat { x := x+1; } until (x >= 10)'";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}

TEST_CASE("all three keywords in one string: no loops detected", "[loop_guard]")
{
    const std::string f = "x := 'for while repeat'";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}

// ── Keywords inside comments must NOT be modified ─────────────────────────────

TEST_CASE("'while' in // comment is not treated as loop", "[loop_guard]")
{
    const std::string f = "// while (x < 10) { x := x + 1; }\nx := 1;";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}

TEST_CASE("'for' in # comment is not treated as loop", "[loop_guard]")
{
    const std::string f = "# for (var i := 0; i < 10; i += 1) { }\nx := 1;";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}

TEST_CASE("'while' in /* */ block comment is not treated as loop", "[loop_guard]")
{
    const std::string f = "/* while (x<10){x:=x+1;} */ x := 2;";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}

TEST_CASE("loop in comment and real loop: only real loop is guarded", "[loop_guard]")
{
    auto r = guard_formula_loops(
        "// while (a < 5) { a := a + 1; }\n"
        "while (b < 5) { b := b + 1; }");
    CHECK(r.loop_count == 1);
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    // Guard must appear only once (for the real loop)
    auto count_occurrences = [](const std::string& haystack, std::string_view needle) {
        std::size_t n = 0, pos = 0;
        while ((pos = haystack.find(needle, pos)) != std::string::npos) { ++n; pos += needle.size(); }
        return n;
    };
    // Each guard var name appears in two places: the assignment and the comparison
    CHECK(count_occurrences(r.formula, "__mmutils_loop_guard_0__") >= 2);
    CHECK(!contains(r.formula, "__mmutils_loop_guard_1__"));
}

// ── Guard variable naming pattern ────────────────────────────────────────────

TEST_CASE("guard variable names follow __mmutils_loop_guard_N__ pattern", "[loop_guard]")
{
    auto r = guard_formula_loops("while (x < 1) { x := x + 1; }");
    CHECK(r.loop_count == 1);
    // Name must be double-underscore-prefixed and suffixed
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    // Must not be the old single-underscore style
    CHECK(!contains(r.formula, "_loop_guard_0 "));
}

TEST_CASE("guard variable N increments per loop in textual order", "[loop_guard]")
{
    auto r = guard_formula_loops(
        "while (a < 1) { a := a+1; } "
        "for  (var i := 0; i < 1; i += 1) { } "
        "repeat { b := b+1; } until (b >= 1)");
    CHECK(r.loop_count == 3);
    // Textual order: while(0), for(1), repeat(2)
    std::size_t pos0 = r.formula.find("__mmutils_loop_guard_0__");
    std::size_t pos1 = r.formula.find("__mmutils_loop_guard_1__");
    std::size_t pos2 = r.formula.find("__mmutils_loop_guard_2__");
    REQUIRE(pos0 != std::string::npos);
    REQUIRE(pos1 != std::string::npos);
    REQUIRE(pos2 != std::string::npos);
    CHECK(pos0 < pos1);
    CHECK(pos1 < pos2);
}

// ── Identifier false-positives ────────────────────────────────────────────────

TEST_CASE("'while' as prefix of a longer identifier is not a loop", "[loop_guard]")
{
    // 'whileTrue' is a distinct identifier; must not be matched as 'while'.
    const std::string f = "whileTrue := 1;";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}

TEST_CASE("'for' as prefix of a longer identifier is not a loop", "[loop_guard]")
{
    // 'format' starts with 'for' but is a distinct identifier.
    const std::string f = "format := 'csv';";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}

TEST_CASE("'repeat' as prefix of a longer identifier is not a loop", "[loop_guard]")
{
    const std::string f = "repeater := x + 1;";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}

// ── Boundary / edge cases ─────────────────────────────────────────────────────

TEST_CASE("max_iterations = 1 injects boundary limit of 1 into guard", "[loop_guard]")
{
    auto r = guard_formula_loops("while (x < 10) { x := x + 1; }", 1);
    CHECK(r.loop_count == 1);
    CHECK(contains(r.formula, "<= 1"));
    // Must NOT contain the default 1000000 limit.
    CHECK(!contains(r.formula, "1000000"));
}

TEST_CASE("formula with only whitespace is unchanged and has 0 loops", "[loop_guard]")
{
    const std::string f = "   \t\n  ";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}

TEST_CASE("formula with only a block comment is unchanged and has 0 loops", "[loop_guard]")
{
    // A loop keyword inside a block comment must not be guarded.
    const std::string f = "/* while (x < 1) { x := x+1; } */";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}

TEST_CASE("while with nested-paren condition is guarded correctly", "[loop_guard]")
{
    // The condition itself contains nested parens.
    auto r = guard_formula_loops(
        "while ((a > 0) and (b < 5)) { a := a - 1; }");
    CHECK(r.loop_count == 1);
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    // The entire original compound condition must appear as the second operand.
    CHECK(contains(r.formula, "and ((a > 0) and (b < 5))"));
}

TEST_CASE("three-level nested while loops each get own guard variable", "[loop_guard]")
{
    auto r = guard_formula_loops(
        "while (i < 3) { "
        "  while (j < 3) { "
        "    while (k < 3) { k := k + 1; }; "
        "    j := j + 1; "
        "  }; "
        "  i := i + 1; "
        "}");
    CHECK(r.loop_count == 3);
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    CHECK(contains(r.formula, "__mmutils_loop_guard_1__"));
    CHECK(contains(r.formula, "__mmutils_loop_guard_2__"));
}

TEST_CASE("for loop with empty init still injects guard into condition", "[loop_guard]")
{
    // for (; i < 10; i := i + 1) — empty INIT section
    auto r = guard_formula_loops("for (; i < 10; i := i + 1) { }");
    CHECK(r.loop_count == 1);
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    CHECK(contains(r.formula, "and (i < 10)"));
}

TEST_CASE("repeat-until with compound or-condition is guarded correctly", "[loop_guard]")
{
    // Original condition: (x >= 5) or (y > 10)
    // Guard wraps so either the original exits OR the guard counter hits max.
    auto r = guard_formula_loops(
        "repeat { x := x + 1; } until ((x >= 5) or (y > 10))");
    CHECK(r.loop_count == 1);
    CHECK(contains(r.formula, "__mmutils_loop_guard_0__"));
    // Resulting structure: until (((x >= 5) or (y > 10)) or ((...) >= MAX))
    CHECK(contains(r.formula, "until (((x >= 5) or (y > 10)) or ("));
}

TEST_CASE("while missing opening paren after keyword is skipped gracefully", "[loop_guard]")
{
    // 'while' not followed by '(' must not crash or inject a guard.
    const std::string f = "while x < 10;";
    auto r = guard_formula_loops(f);
    CHECK(r.loop_count == 0);
    CHECK(r.formula == f);
}
