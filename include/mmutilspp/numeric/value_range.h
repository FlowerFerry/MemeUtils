
#ifndef MMUPP_NUMERIC_VALUE_RANGE_H_INCLUDED
#define MMUPP_NUMERIC_VALUE_RANGE_H_INCLUDED

#include <limits>
#include <type_traits>

#ifdef min
#define MIN_ORIGINAL min
#undef min
#endif

#ifdef max
#define MAX_ORIGINAL max
#undef max
#endif

namespace mmupp {
namespace numeric {

template <typename T>
class value_range {
public:
    constexpr value_range() noexcept : min_(0), max_(0) {}
    value_range(T min, T max) noexcept : min_(min), max_(max) 
    {
        if (min_ > max_) {
            std::swap(min_, max_);
        }
    }

    constexpr T min() const noexcept { return min_; }
    constexpr T max() const noexcept { return max_; }

    bool invalid() const noexcept { return min_ == max_ && min_ == 0; }

    bool contains(T value) const noexcept { 
        if (invalid()) 
            return true;
        return value >= min_ && value <= max_; 
    }

    bool contains_exclusive(T value) const noexcept 
    { 
        if (invalid()) 
            return true;
        return value > min_ && value < max_; 
    }

    void set_min(T min) noexcept { 
        if (invalid()) {
            max_ = std::numeric_limits<T>::max();
        }
        min_ = min; 
        if (min_ > max_) {
            std::swap(min_, max_);
        }
    }

    void set_max(T max) noexcept { 
        if (invalid()) {
            min_ = std::numeric_limits<T>::min();
        }
        max_ = max; 
        if (min_ > max_) {
            std::swap(min_, max_);
        }
    }

    void set(T min, T max) noexcept { 
        min_ = min; max_ = max; 
        if (min_ > max_) {
            std::swap(min_, max_);
        }
    }

    template <typename U>
    void set(const value_range<U>& range) noexcept { 
        min_ = static_cast<T>(range.min()); 
        max_ = static_cast<T>(range.max()); 
        if (min_ > max_) {
            std::swap(min_, max_);
        }
    }

    constexpr void reset() noexcept { min_ = 0; max_ = 0; }

    value_range<T>& operator=(const value_range<T>& range) noexcept { 
        min_ = range.min(); 
        max_ = range.max(); 
        return *this; 
    }

    bool operator==(const value_range<T>& range) const noexcept { return min() == range.min() && max() == range.max(); }
    bool operator!=(const value_range<T>& range) const noexcept { return !(*this == range); }

    bool operator<(const value_range<T>& range) const noexcept { return range_size() < range.range_size(); }
    bool operator>(const value_range<T>& range) const noexcept { return range_size() > range.range_size(); }

    bool operator<=(const value_range<T>& range) const noexcept { return range_size() <= range.range_size(); }
    bool operator>=(const value_range<T>& range) const noexcept { return range_size() >= range.range_size(); }

    value_range<T> operator+(const value_range<T>& range) const noexcept { return value_range<T>(min() + range.min(), max() + range.max()); }
    value_range<T> operator-(const value_range<T>& range) const noexcept { return value_range<T>(min() - range.min(), max() - range.max()); }
    value_range<T> operator*(const value_range<T>& range) const noexcept { 
        auto minv = (std::min)({min() * range.min(), max() * range.min(), min() * range.max(), max() * range.max()});
        auto maxv = (std::max)({min() * range.min(), max() * range.min(), min() * range.max(), max() * range.max()});
        return value_range<T>(minv, maxv);
    }
    value_range<T> operator/(const value_range<T>& range) const noexcept { 
        if (range.invalid()) {
            return value_range<T>(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
        }

        auto minv = (std::min)({min() / range.min(), max() / range.min(), min() / range.max(), max() / range.max()});
        auto maxv = (std::max)({min() / range.min(), max() / range.min(), min() / range.max(), max() / range.max()});

        return value_range<T>(minv, maxv);
    }

private:
    template<typename U = T>
    static constexpr auto range_diff(T _max, T _min) noexcept {
        if constexpr (std::is_integral_v<U>) {
            return std::make_unsigned_t<U>(_max) - std::make_unsigned_t<U>(_min);
        } else {
            return _max - _min;
        }
    }

    constexpr auto range_size() const noexcept { return range_diff(max_, min_); }

    T min_;
    T max_;
};

}
}

#ifdef MAX_ORIGINAL
#define max MAX_ORIGINAL
#undef MAX_ORIGINAL
#endif

#ifdef MIN_ORIGINAL
#define min MIN_ORIGINAL
#undef MIN_ORIGINAL
#endif

#endif // !MMUPP_NUMERIC_VALUE_RANGE_H_INCLUDED
