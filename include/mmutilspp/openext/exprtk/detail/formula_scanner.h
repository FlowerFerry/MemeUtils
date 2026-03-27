#ifndef MMUPP_OPENEXT_EXPRTK_DETAIL_FORMULA_SCANNER_H_INCLUDED
#define MMUPP_OPENEXT_EXPRTK_DETAIL_FORMULA_SCANNER_H_INCLUDED

// detail/formula_scanner.h — Shared position-aware lexer for exprtk formula strings.
//
// Provides pos_scanner: a forward-only, seekable scanner that correctly skips
//   - `//` and `#` single-line comments
//   - `/* ... */` block comments
//   - single-quoted string literals (with backslash-escape sequences)
//
// Used internally by extract.h (to build a tokenising scanner) and by
// loop_guard.h (for splice-offset tracking).

#include <cstddef>
#include <string_view>

namespace mmupp {
namespace openext {
namespace exprtk {
namespace detail {

/// Position-aware, forward-only scanner for exprtk formula strings.
/// Tracks the current byte offset (`pos()`) so that callers can record
/// insertion points for the splice mechanism, or tokenise the input.
class pos_scanner
{
public:
    explicit pos_scanner(std::string_view input) noexcept
        : input_(input), p_(0)
    {}

    std::size_t pos() const noexcept { return p_; }
    bool        at_end() const noexcept { return p_ >= input_.size(); }

    char peek() const noexcept
    {
        return p_ < input_.size() ? input_[p_] : '\0';
    }

    void advance(std::size_t n = 1) noexcept
    {
        if (p_ + n <= input_.size()) p_ += n;
        else p_ = input_.size();
    }

    void seek(std::size_t pos) noexcept
    {
        p_ = pos <= input_.size() ? pos : input_.size();
    }

    // Skips ASCII whitespace.
    void skip_whitespace() noexcept
    {
        while (p_ < input_.size())
        {
            char c = input_[p_];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break;
            ++p_;
        }
    }

    // Consumes one comment if present at the current position.
    // Returns true if a comment was consumed.
    bool skip_comment() noexcept
    {
        if (p_ >= input_.size()) return false;

        // # single-line comment
        if (input_[p_] == '#')
        {
            while (p_ < input_.size() && input_[p_] != '\n') ++p_;
            return true;
        }

        if (p_ + 1 >= input_.size()) return false;

        // // single-line comment
        if (input_[p_] == '/' && input_[p_ + 1] == '/')
        {
            while (p_ < input_.size() && input_[p_] != '\n') ++p_;
            return true;
        }

        // /* ... */ block comment
        if (input_[p_] == '/' && input_[p_ + 1] == '*')
        {
            p_ += 2;
            while (p_ + 1 < input_.size())
            {
                if (input_[p_] == '*' && input_[p_ + 1] == '/')
                {
                    p_ += 2;
                    return true;
                }
                ++p_;
            }
            p_ = input_.size(); // unterminated — consume to end
            return true;
        }

        return false;
    }

    // Skips all whitespace and comments.
    void skip_noise() noexcept
    {
        for (;;)
        {
            skip_whitespace();
            if (!skip_comment()) break;
        }
    }

    // Consumes a single-quoted string literal (including surrounding quotes).
    // Must be called when peek() == '\''.
    void skip_string() noexcept
    {
        ++p_; // opening quote
        while (p_ < input_.size())
        {
            char c = input_[p_];
            if (c == '\\') { p_ += 2; continue; }
            if (c == '\'') { ++p_; return; }
            ++p_;
        }
        // unterminated string — consumed to end
    }

    // Consumes a single-quoted string literal and returns a view of its
    // contents (without the surrounding quotes).
    // Must be called when peek() == '\''.
    std::string_view read_string() noexcept
    {
        ++p_; // opening quote
        std::size_t start = p_;
        while (p_ < input_.size())
        {
            char c = input_[p_];
            if (c == '\\') { p_ += 2; continue; }
            if (c == '\'') break;
            ++p_;
        }
        std::string_view content = input_.substr(start, p_ - start);
        if (p_ < input_.size()) ++p_; // closing quote
        return content;
    }

    // Reads an identifier and returns a view into the original input.
    // Must be called when is_ident_start(peek()) is true.
    std::string_view read_symbol() noexcept
    {
        std::size_t start = p_;
        while (p_ < input_.size() && is_ident_cont(input_[p_])) ++p_;
        return input_.substr(start, p_ - start);
    }

    // Finds the matching ')' for a '(' that has already been consumed.
    // Scans forward tracking nesting depth, skipping strings and comments.
    // Returns the absolute offset of the matching ')' or npos if not found.
    std::size_t find_matching_rparen() noexcept
    {
        int depth = 1;
        while (p_ < input_.size() && depth > 0)
        {
            skip_noise();
            if (p_ >= input_.size()) break;
            char c = input_[p_];
            if (c == '\'')        { skip_string(); continue; }
            if (c == '(')         { ++depth; ++p_; continue; }
            if (c == ')')
            {
                --depth;
                if (depth == 0) return p_; // do NOT advance; caller handles it
                ++p_;
                continue;
            }
            ++p_;
        }
        return std::string_view::npos;
    }

    // Finds the next ';' at paren/bracket depth 0, skipping strings and comments.
    // Returns the absolute offset of the ';', or npos.
    std::size_t find_semicolon_at_depth0() noexcept
    {
        int depth = 0;
        while (p_ < input_.size())
        {
            skip_noise();
            if (p_ >= input_.size()) break;
            char c = input_[p_];
            if (c == '\'')                 { skip_string(); continue; }
            if (c == '(' || c == '[')      { ++depth; ++p_; continue; }
            if (c == ')' || c == ']')      { --depth; ++p_; continue; }
            if (c == ';' && depth == 0)    { return p_; }
            ++p_;
        }
        return std::string_view::npos;
    }

    // Finds the next occurrence of keyword `kw` (as a complete identifier) at
    // depth 0 for '()', '{}' and '[]', skipping strings and comments.
    // Returns the absolute offset of the first character of `kw`, or npos.
    std::size_t find_keyword_at_depth0(std::string_view kw) noexcept
    {
        int depth = 0;
        while (p_ < input_.size())
        {
            skip_noise();
            if (p_ >= input_.size()) break;

            char c = input_[p_];
            if (c == '\'')                           { skip_string(); continue; }
            if (c == '(' || c == '{' || c == '[')    { ++depth; ++p_; continue; }
            if (c == ')' || c == '}' || c == ']')    { --depth; ++p_; continue; }

            if (depth == 0 && is_ident_start(c))
            {
                std::size_t sym_start = p_;
                std::string_view sym  = read_symbol();
                if (sym == kw)
                    return sym_start;
                continue; // p_ already advanced past symbol
            }
            ++p_;
        }
        return std::string_view::npos;
    }

    std::string_view input() const noexcept { return input_; }

    static bool is_ident_start(char c) noexcept
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }
    static bool is_ident_cont(char c) noexcept
    {
        return is_ident_start(c) || (c >= '0' && c <= '9');
    }

private:
    std::string_view input_;
    std::size_t      p_;
};

} // namespace detail
} // namespace exprtk
} // namespace openext
} // namespace mmupp

#endif // MMUPP_OPENEXT_EXPRTK_DETAIL_FORMULA_SCANNER_H_INCLUDED
