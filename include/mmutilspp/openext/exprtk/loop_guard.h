#ifndef MMUPP_OPENEXT_EXPRTK_LOOP_GUARD_H_INCLUDED
#define MMUPP_OPENEXT_EXPRTK_LOOP_GUARD_H_INCLUDED

// loop_guard.h — Inject iteration-count guards into exprtk formula loops.
//
// Rewrites every `for`, `while`, and `repeat...until` loop in an exprtk
// formula string so that each loop terminates after at most `max_iterations`
// iterations, preventing user-supplied formulas from containing infinite loops.
//
// Does NOT depend on exprtk.hpp.  Uses the same lightweight lexer approach as
// extract.h: a single forward-scan that correctly skips
//   - `//` and `#` single-line comments
//   - `/* ... */` block comments
//   - single-quoted string literals (with backslash-escape sequences)
//
// Guard variables injected into the formula are named
//   __mmutils_loop_guard_0__, __mmutils_loop_guard_1__, ...
// They are declared directly inside the returned formula string using exprtk's
// `var` keyword, so no extra symbol_table registration is required.  The
// `loop_count` field of the returned struct tells the caller how many guard
// variables were declared.
//
// Usage:
//   auto res = mmupp::openext::exprtk::guard_formula_loops(
//       "while (x < 100) { x := x + 1; }");
//   // res.loop_count == 1
//   // res.formula begins with: var __mmutils_loop_guard_0__ := 0;
//   // No sym_table.add_variable() calls needed.
//
// Guard injection strategy:
//   while (COND)        → while (((__mmutils_loop_guard_N__ := __mmutils_loop_guard_N__ + 1) <= MAX) and (COND))
//   for (I; COND; S)    → for (I; ((__mmutils_loop_guard_N__ := __mmutils_loop_guard_N__ + 1) <= MAX) and (COND); S)
//   for (I; ; S)        → for (I; (__mmutils_loop_guard_N__ := __mmutils_loop_guard_N__ + 1) <= MAX; S)
//   repeat B until (C)  → repeat B until ((C) or ((__mmutils_loop_guard_N__ := __mmutils_loop_guard_N__ + 1) >= MAX))

#include "detail/formula_scanner.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mmupp {
namespace openext {
namespace exprtk {

/// Result of guard_formula_loops().
struct loop_guard_result
{
    std::string formula;      ///< Rewritten formula with iteration guards injected.
                              ///< Prepended with `var __mmutils_loop_guard_N__ := 0;`
                              ///< declarations for each guarded loop.
    std::size_t loop_count;   ///< Number of loops guarded; equals the number of
                              ///< `var __mmutils_loop_guard_N__` declarations
                              ///< prepended to the formula.
};

// ─────────────────────────────────────────────────────────────────────────────
// Implementation detail — not part of the public API
// ─────────────────────────────────────────────────────────────────────────────
namespace detail {

/// A text splice: insert `text` immediately before position `offset` in the
/// original formula.  Multiple splices are applied right-to-left so that
/// earlier offsets remain valid.
struct splice
{
    std::size_t offset;
    std::string text;
};


// Build the guard variable name for the N-th loop (0-based).
inline std::string guard_var_name(std::size_t n)
{
    return std::string("__mmutils_loop_guard_") + std::to_string(n) + "__";
}

// Apply a sorted-ascending list of splices to `original`, producing a new
// string.  Splices must not overlap.  Applied right-to-left so that earlier
// offsets are unaffected by insertions at later positions.
inline std::string apply_splices(std::string_view                  original,
                                 std::vector<splice>&              splices)
{
    // Sort descending by offset so we apply from right to left.
    std::sort(splices.begin(), splices.end(),
              [](const splice& a, const splice& b){ return a.offset > b.offset; });

    std::string result(original);
    for (const splice& s : splices)
        result.insert(s.offset, s.text);
    return result;
}

// Core rewriter.
inline loop_guard_result do_guard(std::string_view formula,
                                  std::size_t      max_iterations)
{
    std::vector<splice> splices;
    std::size_t         loop_count = 0;

    pos_scanner sc{formula};

    while (!sc.at_end())
    {
        sc.skip_noise();
        if (sc.at_end()) break;

        char c = sc.peek();

        // Skip single-quoted string literals atomically.
        if (c == '\'') { sc.skip_string(); continue; }

        // Only identifiers can be loop keywords.
        if (!( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ))
        {
            sc.advance();
            continue;
        }

        std::size_t      sym_start = sc.pos();
        std::string_view sym       = sc.read_symbol();
        (void)sym_start;

        const std::string gvar  = guard_var_name(loop_count);
        const std::string max_s = std::to_string(max_iterations);

        // ── while (COND) { body } ────────────────────────────────────────────
        if (sym == "while")
        {
            sc.skip_noise();
            if (sc.at_end() || sc.peek() != '(') continue;

            std::size_t lparen = sc.pos(); // position of '('
            sc.advance();                  // consume '('

            std::size_t rparen = sc.find_matching_rparen();
            if (rparen == std::string_view::npos) continue;

            // Insert after '(': ((__mmutils_loop_guard_N__ := ... + 1) <= MAX) and (
            splices.push_back({lparen + 1,
                "((" + gvar + " := " + gvar + " + 1) <= " + max_s + ") and ("});
            // Insert before ')': )
            splices.push_back({rparen, ")"});

            sc.seek(rparen + 1); // advance past ')'
            ++loop_count;
            continue;
        }

        // ── for (INIT; COND; STEP) { body } ─────────────────────────────────
        if (sym == "for")
        {
            sc.skip_noise();
            if (sc.at_end() || sc.peek() != '(') continue;

            sc.advance(); // consume '('

            // Find first ';' (end of INIT)
            std::size_t semi1 = sc.find_semicolon_at_depth0();
            if (semi1 == std::string_view::npos) continue;
            sc.seek(semi1 + 1); // position at start of COND

            // Remember where COND starts (after semi1, before any noise)
            std::size_t cond_start_raw = sc.pos();

            // Find second ';' (end of COND)
            std::size_t semi2 = sc.find_semicolon_at_depth0();
            if (semi2 == std::string_view::npos) continue;

            // Check whether the COND section is empty (only whitespace/comments)
            std::string_view between = formula.substr(cond_start_raw,
                                                      semi2 - cond_start_raw);
            bool cond_empty = true;
            for (char ch : between)
            {
                if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
                {
                    cond_empty = false;
                    break;
                }
            }

            if (cond_empty)
            {
                // for (I; ; S) → for (I; (guard := guard + 1) <= MAX; S)
                splices.push_back({semi1 + 1,
                    std::string(" (") + gvar + " := " + gvar + " + 1) <= " + max_s});
            }
            else
            {
                // Advance to first non-noise char of COND
                pos_scanner tmp{formula};
                tmp.seek(cond_start_raw);
                tmp.skip_noise();
                std::size_t cond_start = tmp.pos();

                // for (I; COND; S) → for (I; ((guard := guard + 1) <= MAX) and (COND); S)
                splices.push_back({cond_start,
                    "((" + gvar + " := " + gvar + " + 1) <= " + max_s + ") and ("});
                splices.push_back({semi2, ")"});
            }

            sc.seek(semi2 + 1);
            ++loop_count;
            continue;
        }

        // ── repeat { body } until (COND) ────────────────────────────────────
        if (sym == "repeat")
        {
            // Scan forward from current position to find `until` at depth 0.
            std::size_t until_pos = sc.find_keyword_at_depth0("until");
            if (until_pos == std::string_view::npos) continue;
            // sc.pos() is now just past "until"

            sc.skip_noise();
            if (sc.at_end() || sc.peek() != '(') continue;

            std::size_t lparen = sc.pos(); // position of 'until' '('
            sc.advance();                  // consume '('

            std::size_t rparen = sc.find_matching_rparen();
            if (rparen == std::string_view::npos) continue;

            // repeat B until ((C) or ((guard := guard + 1) >= MAX))
            splices.push_back({lparen + 1, "("});
            splices.push_back({rparen,
                ") or ((" + gvar + " := " + gvar + " + 1) >= " + max_s + ")"});

            sc.seek(rparen + 1);
            ++loop_count;
            continue;
        }

        // Any other identifier — already consumed, just continue.
    }

    // Prepend `var __mmutils_loop_guard_N__ := 0;` declarations so that the
    // returned formula is self-contained and requires no symbol_table setup.
    if (loop_count > 0)
    {
        std::string preamble;
        for (std::size_t n = 0; n < loop_count; ++n)
            preamble += "var " + guard_var_name(n) + " := 0; ";
        splices.push_back({0, preamble});
    }

    return {apply_splices(formula, splices), loop_count};
}

} // namespace detail

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

/// Rewrites every `for`, `while`, and `repeat...until` loop in `formula` to
/// terminate after at most `max_iterations` iterations.
///
/// For each loop N (0-based, in textual order) a guard variable named
///   __mmutils_loop_guard_N__
/// is injected into the loop's controlling condition.  A corresponding
/// `var __mmutils_loop_guard_N__ := 0;` declaration is prepended to the
/// returned formula so no symbol_table registration is required.
///
/// Comments (`//`, `#`, `/* */`) and single-quoted string literals are
/// correctly skipped; keywords appearing inside them are not modified.
inline loop_guard_result guard_formula_loops(
    std::string_view formula,
    std::size_t      max_iterations = 1'000'000)
{
    return detail::do_guard(formula, max_iterations);
}

} // namespace exprtk
} // namespace openext
} // namespace mmupp

#endif // MMUPP_OPENEXT_EXPRTK_LOOP_GUARD_H_INCLUDED
