
#ifndef MMUPP_UTIL_SPAN2D_H_INCLUDED
#define MMUPP_UTIL_SPAN2D_H_INCLUDED

#include <memepp/string.hpp>
#include <nonstd/span.hpp>

#include <type_traits>

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
    using value_type = _Ty;
    using mutable_value_type = typename std::remove_const<_Ty>::type;

    enum class storage_layout
    {
        x_major,
        y_major
    };

    span2d() noexcept
        : x_size_(0)
        , y_size_(0)
        , coefficient_(1.0)
        , offset_(0)
        , x_interval_(1)
        , y_interval_(1)
        , layout_(storage_layout::y_major)
    {
    }

    span2d(const nonstd::span<_Ty>& _data, std::size_t _x_size, std::size_t _y_size) noexcept
        : data_(_data)
        , x_size_(_x_size)
        , y_size_(_y_size)
        , coefficient_(1.0)
        , offset_(0)
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
        , coefficient_(1.0)
        , offset_(0)
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

    inline double coefficient() const noexcept { return coefficient_; }
    inline const mutable_value_type& offset() const noexcept { return offset_; }

    inline const mutable_value_type& x_interval() const noexcept { return x_interval_; }
    inline const mutable_value_type& y_interval() const noexcept { return y_interval_; }

    inline storage_layout layout() const noexcept { return layout_; }

    inline const _Ty& at(std::size_t _x, std::size_t _y) const
    {
        if (_x >= x_size_ || _y >= y_size_)
            throw std::out_of_range("span2d: index out of range");
        if (layout_ == storage_layout::x_major)
            return data_[_x * y_size_ + _y] * coefficient_ + offset_;
        else
            return data_[_y * x_size_ + _x] * coefficient_ + offset_;
    }

    inline mutable_value_type x_at(std::size_t _x) const
    {
        if (_x >= x_size_)
            throw std::out_of_range("span2d: x index out of range");
        return x_interval_ * static_cast<mutable_value_type>(_x);
    }

    inline mutable_value_type y_at(std::size_t _y) const
    {
        if (_y >= y_size_)
            throw std::out_of_range("span2d: y index out of range");
        return y_interval_ * static_cast<mutable_value_type>(_y);
    }

    inline void set_coefficient(double _coefficient) noexcept { coefficient_ = _coefficient; }
    inline void set_offset(const mutable_value_type& _offset) noexcept { offset_ = _offset; }

    inline void set_x_interval(const mutable_value_type& _interval) noexcept { x_interval_ = _interval; }
    inline void set_y_interval(const mutable_value_type& _interval) noexcept { y_interval_ = _interval; }
    inline void set_layout(storage_layout _layout) noexcept { layout_ = _layout; }

    inline void set_x_axis(const axis_info& _x_axis) noexcept { x_axis_ = _x_axis; }
    inline void set_y_axis(const axis_info& _y_axis) noexcept { y_axis_ = _y_axis; }

private:
    nonstd::span<_Ty> data_;
    std::size_t x_size_;
    std::size_t y_size_;
    axis_info x_axis_;
    axis_info y_axis_;
    double coefficient_;
    mutable_value_type offset_;
    mutable_value_type x_interval_;
    mutable_value_type y_interval_;
    storage_layout layout_;
};

// template<typename _Ty>
// struct span3d_combined
// {

//     nonstd::span<_Ty> data_xy_;
//     nonstd::span<_Ty> data_xz_;
//     std::size_t x_size_;
//     std::size_t y_size_;
//     std::size_t z_size_;
//     axis_info x_axis_;
//     axis_info y_axis_;
//     axis_info z_axis_;
//     _Ty x_interval_;
//     _Ty y_interval_;
//     _Ty z_interval_;

// };

}
}

#endif // !MMUPP_UTIL_SPAN2D_H_INCLUDED
