
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
    span2d() noexcept
        : x_size_(0)
        , y_size_(0)
    {
    }

    span2d(const nonstd::span<_Ty>& _data, std::size_t _x_size, std::size_t _y_size) noexcept
        : data_(_data)
        , x_size_(_x_size)
        , y_size_(_y_size)
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
    {
        if (_data.size() != _x_size * _y_size)
            throw std::invalid_argument("span2d: data size does not match dimensions");
    }

    inline std::size_t x_size() const noexcept { return x_size_; }
    inline std::size_t y_size() const noexcept { return y_size_; }

    inline const axis_info& x_axis() const noexcept { return x_axis_; }
    inline const axis_info& y_axis() const noexcept { return y_axis_; }

    inline const _Ty& at(std::size_t _x, std::size_t _y) const
    {
        if (_x >= x_size_ || _y >= y_size_)
            throw std::out_of_range("span2d: index out of range");
        return data_[_y * x_size_ + _x];
    }

    nonstd::span<_Ty> data_;
    std::size_t x_size_;
    std::size_t y_size_;
    axis_info x_axis_;
    axis_info y_axis_;
};

}
}

#endif // !MMUPP_UTIL_SPAN2D_H_INCLUDED
