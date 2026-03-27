#ifndef MMUPP_OPENEXT_EXPRTK_NONCODED_H_INCLUDED
#define MMUPP_OPENEXT_EXPRTK_NONCODED_H_INCLUDED

// noncoded.h — Locate all non-code spans in an exprtk formula string.
//
// A "non-code" span is any substring that is not executable exprtk code:
//   - single-quoted string literals  ('hello world')
//   - single-line comments           (// ...  and  # ...)
//   - block comments                 (/* ... */)
//
// Usage:
//   auto spans = mmupp::openext::exprtk::find_noncoded_spans(formula);
//   for (auto& s : spans)
//       auto raw = formula.substr(s.begin, s.end - s.begin);
//
// Does NOT depend on exprtk.hpp.

#include "detail/formula_scanner.h"

#include <cstddef>
#include <string_view>
#include <vector>

namespace mmupp {
namespace openext {
namespace exprtk {

/// Classifies the type of a non-code span.
enum class noncoded_kind : unsigned char
{
    none = 0,       ///< Not a real kind; used as a default value when no match
    string_literal, ///< Single-quoted string literal, e.g.  'hello'
    line_comment,   ///< Single-line comment:  // ...  or  # ...
    block_comment,  ///< Block comment:  /* ... */
};

/// A single non-code span inside a formula string.
///
/// The range [begin, end) is a half-open byte interval into the original input,
/// so  formula.substr(s.begin, s.end - s.begin)  yields the full raw token
/// including its delimiters (quotes / comment markers).
///
/// For line comments the trailing '\\n' is NOT included in the span (the
/// scanner stops at, but does not consume, the newline).
struct noncoded_span
{
    noncoded_kind kind;
    std::size_t   begin; ///< Offset of the first byte of the token
    std::size_t   end;   ///< One-past-the-end byte offset (half-open)
};

/// Scan `formula` and return every non-code span in source order.
///
/// Correctness guarantees:
///   - String literals embedded inside comments are NOT reported as strings.
///   - Comment markers inside string literals are NOT reported as comments.
///   - Backslash-escape sequences inside strings (e.g. \\'  \\0x30) are handled.
///   - Unterminated tokens (unclosed string / block comment that hits
///     end-of-input) are reported with  end == formula.size(), consistent with
///     the underlying pos_scanner behaviour.
///
/// Never throws.
inline std::vector<noncoded_span> find_noncoded_spans(
    std::string_view formula) noexcept
{
    std::vector<noncoded_span> result;
    detail::pos_scanner sc{formula};

    while (!sc.at_end())
    {
        const std::size_t p = sc.pos();
        const char        c = sc.peek();

        // ── Single-quoted string literal ──────────────────────────────────
        if (c == '\'')
        {
            sc.skip_string();
            result.push_back({noncoded_kind::string_literal, p, sc.pos()});
            continue;
        }

        // ── # single-line comment ─────────────────────────────────────────
        if (c == '#')
        {
            sc.skip_comment();
            result.push_back({noncoded_kind::line_comment, p, sc.pos()});
            continue;
        }

        // ── // single-line comment  or  /* block comment */ ───────────────
        if (c == '/' && p + 1 < formula.size())
        {
            const char c2 = formula[p + 1];

            if (c2 == '/')
            {
                sc.skip_comment();
                result.push_back({noncoded_kind::line_comment, p, sc.pos()});
                continue;
            }

            if (c2 == '*')
            {
                sc.skip_comment();
                result.push_back({noncoded_kind::block_comment, p, sc.pos()});
                continue;
            }
        }

        sc.advance();
    }

    return result;
}

} // namespace exprtk
} // namespace openext
} // namespace mmupp

#endif // MMUPP_OPENEXT_EXPRTK_NONCODED_H_INCLUDED
