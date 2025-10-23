
#ifndef MMUPP_UTIL_SPAN2D_H_INCLUDED
#define MMUPP_UTIL_SPAN2D_H_INCLUDED

#include <memepp/string.hpp>
#include <nonstd/span.hpp>

namespace mmupp {
namespace util {

struct axis_info
{
    memepp::string name;
    memepp::string unit;
};

template<typename _Ty>
struct span2d
{
    enum class storage_layout
    {
        x_major,
        y_major
    };

    span2d() noexcept
        : x_size_(0)
        , y_size_(0)
        , x_interval_(1)
        , y_interval_(1)
        , layout_(storage_layout::y_major)
    {
    }

    span2d(const nonstd::span<_Ty>& _data, std::size_t _x_size, std::size_t _y_size) noexcept
        : data_(_data)
        , x_size_(_x_size)
        , y_size_(_y_size)
        , x_interval_(1)
        , y_interval_(1)
        , layout_(storage_layout::y_major)
    {
        if (_data.size() != _x_size * _y_size)
            throw std::invalid_argument("span2d: data size does not match dimensions");
    }

    span2d(const nonstd::span<_Ty>& _data, std::size_t _x_size, std::size_t _y_size,
        axis_info const& _x_axis, axis_info const& _y_axis) noexcept
        : data_(_data)
        , x_size_(_x_size)
        , y_size_(_y_size)
        , x_axis_(_x_axis)
        , y_axis_(_y_axis)
        , x_interval_(1)
        , y_interval_(1)
        , layout_(storage_layout::y_major)
    {
        if (_data.size() != _x_size * _y_size)
            throw std::invalid_argument("span2d: data size does not match dimensions");
    }

    inline std::size_t x_size() const noexcept { return x_size_; }
    inline std::size_t y_size() const noexcept { return y_size_; }

    inline const axis_info& x_axis() const noexcept { return x_axis_; }
    inline const axis_info& y_axis() const noexcept { return y_axis_; }

    inline const _Ty& x_interval() const noexcept { return x_interval_; }
    inline const _Ty& y_interval() const noexcept { return y_interval_; }

    inline storage_layout layout() const noexcept { return layout_; }

    inline const _Ty& at(std::size_t _x, std::size_t _y) const
    {
        if (_x >= x_size_ || _y >= y_size_)
            throw std::out_of_range("span2d: index out of range");
        if (layout_ == storage_layout::x_major)
            return data_[_x * y_size_ + _y];
        else
            return data_[_y * x_size_ + _x];
    }

    inline _Ty x_at(std::size_t _x) const
    {
        if (_x >= x_size_)
            throw std::out_of_range("span2d: x index out of range");
        return x_interval_ * static_cast<_Ty>(_x);
    }

    inline _Ty y_at(std::size_t _y) const
    {
        if (_y >= y_size_)
            throw std::out_of_range("span2d: y index out of range");
        return y_interval_ * static_cast<_Ty>(_y);
    }

    inline void set_x_interval(const _Ty& _interval) noexcept { x_interval_ = _interval; }
    inline void set_y_interval(const _Ty& _interval) noexcept { y_interval_ = _interval; }
    inline void set_layout(storage_layout _layout) noexcept { layout_ = _layout; }

private:
    nonstd::span<_Ty> data_;
    std::size_t x_size_;
    std::size_t y_size_;
    axis_info x_axis_;
    axis_info y_axis_;
    _Ty x_interval_;
    _Ty y_interval_;
    storage_layout layout_;
};

}
}

#endif // !MMUPP_UTIL_SPAN2D_H_INCLUDED
