
#ifndef MMUTILSPP_LOCALE_TAG_H_INCLUDED
#define MMUTILSPP_LOCALE_TAG_H_INCLUDED

#include <cctype>
#include <cstdint>
#include <cstring>
#include <algorithm>

#include <memepp/string_view.hpp>

namespace mmupp {
namespace locale {

// Compact inline storage for a single BCP 47 subtag (language / script / region).
// The longest standard subtag is 4 bytes (ISO 15924 script codes), so 7 bytes of
// payload is more than enough. Total size: 8 bytes (1 byte size + 7 bytes data).
struct subtag
{
    static constexpr size_t max_size = 4;

    subtag() noexcept = default;

    subtag(const uint8_t* _data, size_t _size) noexcept
        : size_{ (_size > max_size) ? static_cast<uint8_t>(max_size)
                                    : static_cast<uint8_t>(_size) }
    {
        for (size_t i = 0; i < size_; ++i)
            data_[i] = _data[i];
    }

    const uint8_t* data() const noexcept { return data_; }
    uint8_t* data() noexcept { return data_; }
    size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    const uint8_t* begin() const noexcept { return data_; }
    const uint8_t* end() const noexcept { return data_ + size_; }
    uint8_t* begin() noexcept { return data_; }
    uint8_t* end() noexcept { return data_ + size_; }

    bool operator==(const subtag& _other) const noexcept
    {
        return size_ == _other.size_ &&
            (size_ == 0 || std::memcmp(data_, _other.data_, size_) == 0);
    }

    bool operator!=(const subtag& _other) const noexcept { return !(*this == _other); }

private:
    uint8_t size_ = 0;
    uint8_t data_[7] = {};
};

struct simple_tag
{
    simple_tag() = default;

    simple_tag(
        const memepp::string_view& _language,
        const memepp::string_view& _script = {},
        const memepp::string_view& _region = {})
        : language_{ _language.bytes(), static_cast<size_t>(_language.size()) },
          script_{ _script.bytes(), static_cast<size_t>(_script.size()) },
          region_{ _region.bytes(), static_cast<size_t>(_region.size()) },
          valid_{ _language.size() >= 2 && _language.size() <= 3 && is_alpha(_language) }
    {}

    simple_tag(const simple_tag& _other) = default;
    simple_tag(simple_tag&& _other) noexcept = default;
    ~simple_tag() = default;
    
    simple_tag& operator=(const simple_tag& _other) = default;
    simple_tag& operator=(simple_tag&& _other) noexcept = default;

    inline constexpr bool valid() const noexcept { return valid_; }

    inline constexpr const subtag& language() const noexcept { return language_; }
    inline constexpr const subtag& script() const noexcept { return script_; }
    inline constexpr const subtag& region() const noexcept { return region_; }

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
        bool has_region = false;

        for (; end <= len; ++end) {
            if (end == len || _sv.at(end) == '-' || _sv.at(end) == '_')
            {
                memepp::string_view part = _sv.substr(start, end - start);
                start = end + 1;  // Always update start

                if (part_index == 0) {
                    // Language: 2-3 alpha chars
                    if (part.size() < 2 || part.size() > 3 || !is_alpha(part))
                        break;
                    tag.language_ = subtag{ part.bytes(), static_cast<size_t>(part.size()) };
                    std::transform(
                        tag.language_.begin(), tag.language_.end(),
                        tag.language_.begin(),
                        [](unsigned char c){ return std::tolower(c); });
                    part_index++;
                } else if (part_index == 1) {
                    // Script: 4 alpha chars
                    if (part.size() == 4 && is_alpha(part)) {
                        tag.script_ = subtag{ part.bytes(), static_cast<size_t>(part.size()) };
                        std::transform(
                            tag.script_.begin(), tag.script_.end(),
                            tag.script_.begin(),
                            [](unsigned char c){ return std::tolower(c); });
                        tag.script_.data()[0] = std::toupper(tag.script_.data()[0]);
                        part_index++;
                    }
                    // Region: 2 alpha or 3 digit
                    else if (!has_region &&
                             ((part.size() == 2 && is_alpha(part)) ||
                              (part.size() == 3 && is_digit(part)))) {
                        tag.region_ = subtag{ part.bytes(), static_cast<size_t>(part.size()) };
                        std::transform(
                            tag.region_.begin(), tag.region_.end(),
                            tag.region_.begin(),
                            [](unsigned char c){ return std::toupper(c); });
                        has_region = true;
                        part_index++;
                    }
                    // Invalid part: skip but continue parsing
                } else if (part_index == 2) {
                    // Region: 2 alpha or 3 digit (only if not already set)
                    if (!has_region &&
                        ((part.size() == 2 && is_alpha(part)) ||
                         (part.size() == 3 && is_digit(part)))) {
                        tag.region_ = subtag{ part.bytes(), static_cast<size_t>(part.size()) };
                        std::transform(
                            tag.region_.begin(), tag.region_.end(),
                            tag.region_.begin(),
                            [](unsigned char c){ return std::toupper(c); });
                        has_region = true;
                        part_index++;
                    }
                    // Invalid or duplicate region: skip
                }
            }
        }

        tag.valid_ = (part_index > 0);
        return tag;
    }
    
    bool operator==(const simple_tag& _other) const noexcept
    {
        return language_ == _other.language_
            && script_ == _other.script_
            && region_ == _other.region_
            && valid_ == _other.valid_;
    }

    bool operator!=(const simple_tag& _other) const noexcept
    {
        return !(*this == _other);
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

    subtag language_;
    subtag script_;
    subtag region_;
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
        // Single-pass FNV-1a over language/script/region (a 0xFF byte separates
        // fields so that e.g. {"en","US"} and {"enU","S"} cannot collide), then
        // fold in the validity flag.
        constexpr size_t fnv_offset_basis =
            (sizeof(size_t) == 8) ? static_cast<size_t>(0xcbf29ce484222325ull)
                                  : static_cast<size_t>(0x811c9dc5u);
        constexpr size_t fnv_prime =
            (sizeof(size_t) == 8) ? static_cast<size_t>(0x100000001b3ull)
                                  : static_cast<size_t>(0x01000193u);

        size_t h = fnv_offset_basis;
        auto feed = [&h](const mmupp::locale::subtag& _s) noexcept {
            const uint8_t* p = _s.data();
            for (size_t i = 0; i < _s.size(); ++i) {
                h ^= p[i];
                h *= fnv_prime;
            }
            h ^= 0xffu;
            h *= fnv_prime;
        };
        feed(_tag.language());
        feed(_tag.script());
        feed(_tag.region());
        h ^= static_cast<size_t>(_tag.valid());
        h *= fnv_prime;
        return h;
    }
};

}

#endif // !MMUTILSPP_LOCALE_TAG_H_INCLUDED
