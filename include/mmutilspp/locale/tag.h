
#ifndef MMUTILSPP_LOCALE_TAG_H_INCLUDED
#define MMUTILSPP_LOCALE_TAG_H_INCLUDED

#include <cctype>
#include <algorithm>

#include <memepp/rune.hpp>
#include <memepp/string_view.hpp>
#include <memepp/hash/std/hash.hpp>

namespace mmupp {
namespace locale {

struct simple_tag
{
    simple_tag() = default;

    simple_tag(
        const memepp::string_view& _language,
        const memepp::string_view& _script = {},
        const memepp::string_view& _region = {})
        : language_{ _language.bytes(), _language.size() },
          script_{ _script.bytes(), _script.size() },
          region_{ _region.bytes(), _region.size() },
          valid_{ true }
    {}

    simple_tag(const simple_tag& _other) = default;
    simple_tag(simple_tag&& _other) noexcept = default;
    ~simple_tag() = default;
    
    simple_tag& operator=(const simple_tag& _other) = default;
    simple_tag& operator=(simple_tag&& _other) noexcept = default;

    inline constexpr bool valid() const noexcept { return valid_; }

    inline constexpr const memepp::rune& language() const noexcept { return language_; }
    inline constexpr const memepp::rune& script() const noexcept { return script_; }
    inline constexpr const memepp::rune& region() const noexcept { return region_; }

    inline simple_tag to_lang_only() const noexcept
    {
        simple_tag tag;
        tag.language_ = language_;
        tag.valid_ = valid_;
        return tag;
    }

    inline simple_tag to_lang_region_only() const noexcept
    {
        simple_tag tag;
        tag.language_ = language_;
        tag.region_ = region_;
        tag.valid_ = valid_;
        return tag;
    }

    inline static simple_tag parse_from(const memepp::string_view& _sv)
    {
        simple_tag tag;
        size_t start = 0;
        size_t end = 0;
        size_t len = _sv.size();
        size_t part_index = 0;

        for (; end <= len; ++end) {
            if (end == len || _sv.at(end) == '-' || _sv.at(end) == '_')
            {
                memepp::string_view part = _sv.substr(start, end - start);
                if (part_index == 0) {
                    if (part.size() < 2 || part.size() > 3 || !is_alpha(part))
                        break;
                    tag.language_ = memepp::rune{ part.bytes(), part.size() };
                    std::transform(
                        tag.language_.begin(), tag.language_.end(),
                        tag.language_.begin(),
                        [](unsigned char c){ return std::tolower(c); });
                } else if (part_index == 1) {
                    if (part.size() == 4 && is_alpha(part)) {
                        tag.script_ = memepp::rune{ part.bytes(), part.size() };
                        std::transform(
                            tag.script_.begin(), tag.script_.end(),
                            tag.script_.begin(),
                            [](unsigned char c){ return std::tolower(c); });
                        tag.script_.data()[0] = std::toupper(tag.script_.data()[0]);
                    } else if (part.size() == 2 && is_alpha(part)) {
                        tag.region_ = memepp::rune{ part.bytes(), part.size() };
                        std::transform(
                            tag.region_.begin(), tag.region_.end(),
                            tag.region_.begin(),
                            [](unsigned char c){ return std::toupper(c); });
                    } else if (part.size() == 3 && is_digit(part)) {
                        tag.region_ = memepp::rune{ part.bytes(), part.size() };
                        std::transform(
                            tag.region_.begin(), tag.region_.end(),
                            tag.region_.begin(),
                            [](unsigned char c){ return std::toupper(c); });
                    } else {
                        continue;
                    }
                } else if (part_index == 2) {
                    if (part.size() == 2) {
                        if (!is_alpha(part))
                            continue;
                    } else if (part.size() == 3) {
                        if (!is_digit(part))
                            continue;
                    } else {
                        continue;
                    }
                    tag.region_ = memepp::rune{ part.bytes(), part.size() };
                    std::transform(
                        tag.region_.begin(), tag.region_.end(),
                        tag.region_.begin(),
                        [](unsigned char c){ return std::toupper(c); });
                }
                part_index++;
                start = end + 1;
            }
        }

        tag.valid_ = (part_index > 0);
        return tag;
    }
    
private:
    static bool is_alpha(const memepp::string_view& _sv) {
        return !_sv.empty() &&
            std::all_of(_sv.begin(), _sv.end(), [](unsigned char c){ return std::isalpha(c); });
    }

    static bool is_digit(const memepp::string_view& _sv) {
        return !_sv.empty() &&
            std::all_of(_sv.begin(), _sv.end(), [](unsigned char c){ return std::isdigit(c); });
    }

    memepp::rune language_;
    memepp::rune script_;
    memepp::rune region_;
    bool valid_ = false;
}; 

}
}

namespace std {

template<>
struct hash<mmupp::locale::simple_tag>
{
    size_t operator()(const mmupp::locale::simple_tag& _tag) const noexcept
    {
        size_t h1 = memepp::hash::details::hash_value_with_bytes(
            _tag.language().data(), _tag.language().size());
        size_t h2 = memepp::hash::details::hash_value_with_bytes(
            _tag.script().data(), _tag.script().size());
        size_t h3 = memepp::hash::details::hash_value_with_bytes(
            _tag.region().data(), _tag.region().size());
        size_t h4 = static_cast<size_t>(_tag.valid());
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

}

#endif // !MMUTILSPP_LOCALE_TAG_H_INCLUDED
