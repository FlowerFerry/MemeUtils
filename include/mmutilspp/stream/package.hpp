
#ifndef MMUPP_STREAM_PACKAGE_PREPROC_HPP_INCLUDED
#define MMUPP_STREAM_PACKAGE_PREPROC_HPP_INCLUDED

#include <memepp/buffer.hpp>
#include <memepp/buffer_view.hpp>
#include <memepp/variable_buffer.hpp>

#include "../chrono/passive_timer.hpp"

namespace mmupp {
namespace stream {

    struct package_preproc
    {
        typedef void(recv_fn_t)(const memepp::buffer_view& _buf, package_preproc*, void* _userdata);
        typedef int (calc_len_fn_t)(
            const memepp::buffer_view& _curr, 
            const memepp::buffer_view& _wait, size_t* _length, void* _userdata);
        typedef bool(checksum_succ_fn_t)(const memepp::buffer_view& _buf, void* _userdata);
        
        package_preproc(void* _userdata = nullptr):
            userdata_(_userdata),
            recv_cb_(nullptr),
            calc_len_cb_(nullptr),
            minLimitPackageSize_(0),
            maxLimitPackageSize_(SIZE_MAX),
            headMatched_(false)
        {
            recv_wait_timer_.set_interval(3000);
            recv_wait_timer_.on([](chrono::passive_timer* _timer, void* _userdata) 
            {
                package_preproc* p = static_cast<package_preproc*>(_userdata);
                // TO_DO
            }, _userdata);
        }
        
        virtual int raw_input(const memepp::buffer_view& _buf, mgu_timestamp_t _now);
        
        inline constexpr void set_recv_cb(recv_fn_t* _cb) noexcept
        {
            recv_cb_ = _cb;
        }

        inline constexpr void set_calc_len_cb(calc_len_fn_t* _cb) noexcept
        {
            calc_len_cb_ = _cb;
        }

        inline constexpr void set_min_limit_package_size(size_t _size) noexcept
        {
            minLimitPackageSize_ = _size;
        }

        inline constexpr void set_max_limit_package_size(size_t _size) noexcept
        {
            maxLimitPackageSize_ = _size;
        }
        
        inline constexpr void set_userdata(void* _userdata) noexcept
        {
            userdata_ = _userdata;
        }
        
        inline void set_head_match(const memepp::buffer_view& _buf) noexcept
        {
            headMatch_ = _buf.to_buffer();
        }

        inline constexpr size_t min_limit_package_size() const noexcept
        {
            return minLimitPackageSize_;
        }

        inline constexpr size_t max_limit_package_size() const noexcept
        {
            return maxLimitPackageSize_;
        }
        
        bool pollable(mgu_timestamp_t _now) const;

        void poll(mgu_timestamp_t _now);

    private:

        inline int raw_input_part(
            const memepp::buffer_view& _buf, mgu_timestamp_t _now, mmint_t* _offset);

        void* userdata_;
        recv_fn_t* recv_cb_;
        calc_len_fn_t* calc_len_cb_;
        checksum_succ_fn_t* checksum_succ_cb_;
        chrono::passive_timer recv_wait_timer_;
        memepp::buffer headMatch_;
        size_t minLimitPackageSize_;
        size_t maxLimitPackageSize_;
        memepp::variable_buffer recv_wait_cache_;
        memepp::variable_buffer recv_curr_cache_;
        bool headMatched_;
    };

    inline int package_preproc::raw_input(const memepp::buffer_view& _buf, mgu_timestamp_t _now)
    {
        if (!recv_wait_cache_.empty()) {
            mmint_t offset = 0;
            int ec = raw_input_part(recv_wait_cache_, _now, &offset);
            if (ec < 0 && ec != MMENO__POSIX_OFFSET(EAGAIN))
            {
                return ec;
            }

            if (ec == 0 && offset > 0)
            {
                recv_wait_cache_.remove(0, offset);
                return 0;
            }
        }

        if (!_buf.empty()) {
            mmint_t offset = 0;
            int ec = raw_input_part(_buf, _now, &offset);
            if (ec < 0 && ec != MMENO__POSIX_OFFSET(EAGAIN))
            {
                return ec;
            }

            if (ec == 0 && offset > 0)
            {
                recv_wait_cache_.append(_buf.data() + offset, _buf.size() - offset);
                return 0;
            }
        }

        return 0;
    }

    inline int package_preproc::raw_input_part(
        const memepp::buffer_view& _buf, mgu_timestamp_t _now, mmint_t* _offset)
    {
        *_offset = 0;
        if (recv_curr_cache_.empty()) {
            if (_buf.size() < min_limit_package_size())
            {
                headMatched_ = false;
                recv_curr_cache_.append(_buf);
                *_offset = _buf.size();
                if (!recv_wait_timer_.is_start())
                    recv_wait_timer_.start_once(_now);
                return 0;
            }
            
            if (!headMatch_.empty()) {

                size_t index = 0;
                for (; index < _buf.size(); ++index)
                {
                    if (_buf.at(index) == headMatch_.at(0))
                    {
                        if (index + headMatch_.size() > _buf.size())
                        {
                            *_offset = index;
                            return MMENO__POSIX_OFFSET(EAGAIN);
                        }
                        else
                        {
                            if (memcmp(_buf.data() + index, 
                                headMatch_.data(), headMatch_.size()) == 0)
                            {
                                if (index > 0) {
                                    *_offset = index;
                                    return MMENO__POSIX_OFFSET(EAGAIN);
                                }
                                break;
                            }
                        }
                    }
                }
                if (index == _buf.size()) 
                {
                    *_offset = index;
                    return MMENO__POSIX_OFFSET(EAGAIN);
                }
                headMatched_ = true;
            }
            
            size_t calc_len = 0;
            if (calc_len_cb_(_buf, {}, &calc_len, userdata_))
                return MMENO__POSIX_OFFSET(EPROTO);

            if (_buf.size() < calc_len) {
                recv_curr_cache_.append(_buf);
                *_offset = _buf.size();
                if (!recv_wait_timer_.is_start())
                    recv_wait_timer_.start_once(_now);
                return 0;
            }
            
            if (!checksum_succ_cb_(
                memepp::buffer_view{ _buf.data(), mmint_t(calc_len) }, userdata_))
            {
                *_offset = calc_len;
                return MMENO__POSIX_OFFSET(EPROTO);
            }

            recv_cb_(memepp::buffer_view{ _buf.data(), mmint_t(calc_len) }, this, userdata_);
            *_offset = calc_len;
            if (recv_wait_timer_.is_start())
                recv_wait_timer_.cancel();
            return 0;
        }
        else {
            if (!headMatch_.empty() && !headMatched_) 
            {
                size_t index = 0;
                for (; index < recv_curr_cache_.size(); ++index)
                {
                    if (recv_curr_cache_.at(index) == headMatch_.at(0))
                    {
                        if (index + headMatch_.size() > recv_curr_cache_.size())
                        {
                            *_offset = index;
                            return MMENO__POSIX_OFFSET(EAGAIN);
                        }
                        else
                        {
                            if (memcmp(recv_curr_cache_.data() + index,
                                headMatch_.data(), headMatch_.size()) == 0)
                            {
                                if (index > 0) {
                                    recv_curr_cache_.remove(0, index);
                                }
                                break;
                            }
                        }
                    }
                }
                if (index == recv_curr_cache_.size())
                {
                    recv_curr_cache_.clear();
                    return MMENO__POSIX_OFFSET(EAGAIN);
                }

                size_t calc_len = 0;
                if (calc_len_cb_(recv_curr_cache_, _buf, &calc_len, userdata_))
                    return MMENO__POSIX_OFFSET(EPROTO);

                if (recv_curr_cache_.size() + _buf.size() < calc_len) {
                    recv_curr_cache_.append(_buf);
                    *_offset = _buf.size();
                    if (!recv_wait_timer_.is_start())
                        recv_wait_timer_.start_once(_now);
                    return 0;
                }

                auto diff = mmint_t(calc_len) - recv_curr_cache_.size();
                recv_curr_cache_.append(_buf.data(), diff);

                if (!checksum_succ_cb_(
                    memepp::buffer_view{ recv_curr_cache_.data(), mmint_t(calc_len) }, userdata_))
                {
                    *_offset = diff;
                    return MMENO__POSIX_OFFSET(EPROTO);
                }

                memepp::buffer buf;
                recv_curr_cache_.release(buf);
                recv_cb_(buf, this, userdata_);
                *_offset = diff;
                if (recv_wait_timer_.is_start())
                    recv_wait_timer_.cancel();
                return 0;
            }
        }
        return 0;
    }
    
    inline bool package_preproc::pollable(mgu_timestamp_t _now) const
    {
        return recv_wait_timer_.due_in() || !recv_wait_cache_.empty();
    }

    inline void package_preproc::poll(mgu_timestamp_t _now)
    {
        if (!recv_wait_cache_.empty())
        {
            raw_input({}, _now);
        }
        
        recv_wait_timer_.timing_continue(_now);
    }

};
};

#endif // !MMUPP_STREAM_PACKAGE_PREPROC_HPP_INCLUDED
