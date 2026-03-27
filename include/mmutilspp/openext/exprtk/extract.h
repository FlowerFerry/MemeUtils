#ifndef MMUPP_OPENEXT_EXPRTK_EXTRACT_H_INCLUDED
#define MMUPP_OPENEXT_EXPRTK_EXTRACT_H_INCLUDED

// extract.h — Parse IDs out of exprtk-style formula strings.
//
// Scans a formula for calls to specified functions and extracts the value of
// a nominated argument (which must be a single-quoted string literal) as an ID.
//
// Does NOT depend on exprtk.hpp. Implements its own lightweight forward-only
// lexer that correctly skips // line comments, # line comments, /* block
// comments */, single-quoted string literals (with \\ escapes), and all other
// tokens irrelevant to ID extraction.
//
// Usage (variadic / compile-time):
//   auto ids = mmupp::openext::exprtk::extract_formula_ids(
//       "getValue('abc1') + getDailyValue('abc2', 1)",
//       mmupp::openext::exprtk::func_id_config{"getValue",      0},
//       mmupp::openext::exprtk::func_id_config{"getDailyValue", 0});
//
// Usage (runtime / std::vector):
//   std::vector<mmupp::openext::exprtk::func_id_config> cfgs = buildConfigs();
//   auto ids = mmupp::openext::exprtk::extract_formula_ids(formula, cfgs);

#include "detail/formula_scanner.h"

#include <cstddef>
#include <string_view>
#include <vector>

namespace mmupp {
namespace openext {
namespace exprtk {

/// Describes which function to look for and which argument (0-based index) is
/// the ID.  The argument must be a single-quoted string literal at the call
/// site; if it is not, the call is silently ignored.
struct func_id_config
{
    std::string_view func_name;   ///< Name of the function to match
    std::size_t      param_index; ///< 0-based index of the argument that is the ID
};

/// One extracted result.  Both views point into the original data:
///   - func_name  → func_id_config::func_name supplied by the caller
///   - id         → the content of the single-quoted string in the formula
///                  (without surrounding quotes)
///
/// The caller must ensure that the original formula string and the
/// func_id_config objects outlive every formula_id_result.
struct formula_id_result
{
    std::string_view func_name;
    std::string_view id;
};

// ─────────────────────────────────────────────────────────────────────────────
// Implementation detail — not part of the public API
// ─────────────────────────────────────────────────────────────────────────────
namespace detail {

enum class tok_type : unsigned char
{
    symbol,
    string_lit,
    lparen,
    rparen,
    comma,
    eof,
    other
};

struct tok
{
    tok_type         type;
    std::string_view value;
};

/// Tokenising scanner built on top of pos_scanner.
/// Produces a stream of tok values via next().
class scanner
{
public:
    explicit scanner(std::string_view input) noexcept
        : sc_(input)
    {}

    tok next() noexcept
    {
        sc_.skip_noise();

        if (sc_.at_end())
            return {tok_type::eof, {}};

        const char c = sc_.peek();

        if (c == '(') { std::size_t p = sc_.pos(); sc_.advance(); return {tok_type::lparen, sc_.input().substr(p, 1)}; }
        if (c == ')') { std::size_t p = sc_.pos(); sc_.advance(); return {tok_type::rparen, sc_.input().substr(p, 1)}; }
        if (c == ',') { std::size_t p = sc_.pos(); sc_.advance(); return {tok_type::comma,  sc_.input().substr(p, 1)}; }

        if (c == '\'')                        return {tok_type::string_lit, sc_.read_string()};
        if (pos_scanner::is_ident_start(c))   return {tok_type::symbol,     sc_.read_symbol()};

        std::size_t p = sc_.pos(); sc_.advance();
        return {tok_type::other, sc_.input().substr(p, 1)};
    }

private:
    pos_scanner sc_;
};

// Core extraction logic operating on a plain C array of configs so that it can
// live in the .h without being a template (avoids code bloat for every
// param-pack size).
inline std::vector<formula_id_result> do_extract(
    std::string_view      formula,
    const func_id_config* configs,
    std::size_t           config_count)
{
    std::vector<formula_id_result> results;
    scanner sc{formula};

    for (;;)
    {
        tok t = sc.next();
        if (t.type == tok_type::eof) break;
        if (t.type != tok_type::symbol) continue;

        // Look for a config whose func_name matches this symbol.
        const func_id_config* matched = nullptr;
        for (std::size_t i = 0; i < config_count; ++i)
        {
            if (configs[i].func_name == t.value)
            {
                matched = &configs[i];
                break;
            }
        }
        if (!matched) continue;

        // The symbol must be followed immediately by '('
        tok lp = sc.next();
        if (lp.type != tok_type::lparen) continue;

        // Walk the argument list.
        //   current_arg  — 0-based index of the argument we are currently in
        //   nest_depth   — parenthesis nesting depth *inside* this call
        //                  (0 = top level of the call's argument list)
        //   first_of_arg — true until the first significant token of an argument
        //                  has been consumed; used to capture only the leading
        //                  string literal of the target argument
        std::size_t current_arg  = 0;
        int         nest_depth   = 0;
        bool        first_of_arg = true;
        bool        captured     = false;
        bool        done         = false;

        while (!done)
        {
            tok a = sc.next();
            switch (a.type)
            {
            case tok_type::eof:
                done = true;
                break;

            case tok_type::lparen:
                ++nest_depth;
                first_of_arg = false;
                break;

            case tok_type::rparen:
                if (nest_depth == 0)
                    done = true;
                else
                {
                    --nest_depth;
                    first_of_arg = false;
                }
                break;

            case tok_type::comma:
                if (nest_depth == 0)
                {
                    ++current_arg;
                    first_of_arg = true;
                }
                else
                {
                    first_of_arg = false;
                }
                break;

            case tok_type::string_lit:
                if (!captured
                    && first_of_arg
                    && nest_depth == 0
                    && current_arg == matched->param_index)
                {
                    results.push_back({matched->func_name, a.value});
                    captured = true;
                }
                first_of_arg = false;
                break;

            default:
                first_of_arg = false;
                break;
            }
        }
    }

    return results;
}

} // namespace detail

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

/// Extract IDs from an exprtk-style formula string.
///
/// The function scans `formula` token by token, correctly skipping:
///   - `//` and `#` single-line comments
///   - `/* ... */` block comments
///   - String literals that are not the target argument
///
/// Duplicate occurrences of the same ID are preserved in order of appearance.
///
/// Both `func_name` and `id` in each result are non-owning views whose
/// lifetimes are bounded by `formula` and the supplied `func_id_config` objects.

// ── Runtime overload: std::vector ────────────────────────────────────────────
inline std::vector<formula_id_result> extract_formula_ids(
    std::string_view                   formula,
    const std::vector<func_id_config>& configs)
{
    return detail::do_extract(formula, configs.data(), configs.size());
}

// ── Compile-time variadic overload ───────────────────────────────────────────

// Zero-argument fallback — nothing to search for.
inline std::vector<formula_id_result> extract_formula_ids(
    std::string_view /*formula*/)
{
    return {};
}

// One-or-more configs.  The pack is expanded into a fixed-size array so the
// compiler can verify every element is convertible to func_id_config and the
// array size is always >= 1 (avoids the MSVC C2466 zero-size array error).
template <typename Cfg0, typename... Cfgs>
std::vector<formula_id_result> extract_formula_ids(
    std::string_view formula,
    Cfg0             cfg0,
    Cfgs...          cfgs)
{
    const func_id_config configs[] = {
        static_cast<func_id_config>(cfg0),
        static_cast<func_id_config>(cfgs)...
    };
    return detail::do_extract(formula, configs, 1 + sizeof...(Cfgs));
}

} // namespace exprtk
} // namespace openext
} // namespace mmupp

#endif // MMUPP_OPENEXT_EXPRTK_EXTRACT_H_INCLUDED
