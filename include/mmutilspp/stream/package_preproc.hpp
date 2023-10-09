
#ifndef MMUPP_STREAM_PACKAGE_HPP_INCLUDED
#define MMUPP_STREAM_PACKAGE_HPP_INCLUDED

#include <mego/err/ec.h>
#include <memepp/buffer.hpp>
#include <memepp/buffer_view.hpp>
#include <memepp/variable_buffer.hpp>

#include "../chrono/passive_timer.hpp"

namespace mmupp {
namespace stream {

    struct package_preproc
    {
        typedef mgec_t(recv_cb_t)(const memepp::buffer_view& _buf, package_preproc*, void* _userdata);
        typedef int (calc_len_cb_t)(
            const memepp::buffer_view& _curr, 
            const memepp::buffer_view& _wait, size_t* _length, void* _userdata);
        typedef bool(checksum_succ_cb_t)(const memepp::buffer_view& _buf, void* _userdata);
        
        package_preproc(void* _userdata = nullptr):
            userdata_(_userdata),
            recv_cb_(nullptr),
            calc_len_cb_(nullptr),
            checksum_succ_cb_(nullptr),
            min_limit_package_size_(0),
            max_limit_package_size_(SIZE_MAX),
            head_matched_(false)
        {
            recv_wait_timer_.set_interval(3000);
            recv_wait_timer_.on([](chrono::passive_timer* _timer, void* _userdata) 
            {
                package_preproc* p = static_cast<package_preproc*>(_userdata);
                // TO_DO
                return true;
            }, this);
        }
        
        virtual mgec_t raw_input(const memepp::buffer_view& _buf, mgu_timestamp_t _now);
        
        inline constexpr void set_recv_cb(recv_cb_t* _cb) noexcept
        {
            recv_cb_ = _cb;
        }

        inline constexpr void set_calc_len_cb(calc_len_cb_t* _cb) noexcept
        {
            calc_len_cb_ = _cb;
        }

        inline constexpr void set_checksum_succ_cb(checksum_succ_cb_t* _cb) noexcept
        {
            checksum_succ_cb_ = _cb;
        }

        inline constexpr void set_min_limit_package_size(size_t _size) noexcept
        {
            min_limit_package_size_ = _size;
        }

        inline constexpr void set_max_limit_package_size(size_t _size) noexcept
        {
            max_limit_package_size_ = _size;
        }
        
        inline constexpr void set_userdata(void* _userdata) noexcept
        {
            userdata_ = _userdata;
        }
        
        inline void set_head_match(const memepp::buffer_view& _buf) noexcept
        {
            head_match_ = _buf.to_buffer();
        }

        inline void set_wait_timeout(int _ms) noexcept
        {
            recv_wait_timer_.set_interval(_ms);
        }

        inline constexpr size_t min_limit_package_size() const noexcept
        {
            return min_limit_package_size_;
        }

        inline constexpr size_t max_limit_package_size() const noexcept
        {
            return max_limit_package_size_;
        }
        
        mmint_t poll_check(mgu_timestamp_t _now) const;
        mgec_t  poll(mgu_timestamp_t _now);

    private:

        inline mgec_t raw_input_part(
            const memepp::buffer_view& _buf, mgu_timestamp_t _now, mmint_t* _offset);

        void* userdata_;
        recv_cb_t* recv_cb_;
        calc_len_cb_t* calc_len_cb_;
        checksum_succ_cb_t* checksum_succ_cb_;
        chrono::passive_timer recv_wait_timer_;
        memepp::buffer head_match_;
        size_t min_limit_package_size_;
        size_t max_limit_package_size_;
        memepp::variable_buffer recv_wait_cache_;
        memepp::variable_buffer recv_curr_cache_;
        bool head_matched_;
    };

    inline mgec_t package_preproc::raw_input(const memepp::buffer_view& _buf, mgu_timestamp_t _now)
    {
        if (!recv_wait_cache_.empty()) {
            mmint_t offset = 0;
            int ec = raw_input_part(recv_wait_cache_, _now, &offset);
            if (ec < 0 && ec != MGEC__AGAIN)
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
            if (ec < 0 && ec != MGEC__AGAIN)
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

    inline mgec_t package_preproc::raw_input_part(
        const memepp::buffer_view& _buf, mgu_timestamp_t _now, mmint_t* _offset)
    {
        *_offset = 0;
        if (recv_curr_cache_.empty()) {
            if (_buf.size() < min_limit_package_size())
            {
                head_matched_ = false;
                recv_curr_cache_.append(_buf);
                *_offset = _buf.size();
                if (!recv_wait_timer_.is_start())
                    recv_wait_timer_.start_once(_now);
                return 0;
            }
            
            if (!head_match_.empty()) {

                size_t index = 0;
                for (; index < _buf.size(); ++index)
                {
                    if (_buf.at(index) == head_match_.at(0))
                    {
                        if (index + head_match_.size() > _buf.size())
                        {
                            *_offset = index;
                            return MGEC__AGAIN;
                        }
                        else
                        {
                            if (memcmp(_buf.data() + index, 
                                head_match_.data(), head_match_.size()) == 0)
                            {
                                if (index > 0) {
                                    *_offset = index;
                                    return MGEC__AGAIN;
                                }
                                break;
                            }
                        }
                    }
                }
                if (index == _buf.size()) 
                {
                    *_offset = index;
                    return MGEC__AGAIN;
                }
                head_matched_ = true;
            }
            
            size_t calc_len = 0;
            if (calc_len_cb_(_buf, {}, &calc_len, userdata_))
                return MGEC__PROTO;

            if (calc_len > max_limit_package_size())
                return MGEC__PROTO;

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
                return MGEC__PROTO;
            }

            recv_cb_(memepp::buffer_view{ _buf.data(), mmint_t(calc_len) }, this, userdata_);
            *_offset = calc_len;
            if (recv_wait_timer_.is_start())
                recv_wait_timer_.cancel();
            return 0;
        }
        else {
            if (!head_match_.empty() && !head_matched_) 
            {
                size_t index = 0;
                for (; index < recv_curr_cache_.size(); ++index)
                {
                    if (recv_curr_cache_.at(index) == head_match_.at(0))
                    {
                        if (index + head_match_.size() > recv_curr_cache_.size())
                        {
                            *_offset = index;
                            return MGEC__AGAIN;
                        }
                        else
                        {
                            if (memcmp(recv_curr_cache_.data() + index,
                                head_match_.data(), head_match_.size()) == 0)
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
                    return MGEC__AGAIN;
                }

                size_t calc_len = 0;
                if (calc_len_cb_(recv_curr_cache_, _buf, &calc_len, userdata_))
                    return MGEC__PROTO;

                if (calc_len > max_limit_package_size())
                    return MGEC__PROTO;

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
                    return MGEC__PROTO;
                }
                
                recv_cb_(recv_curr_cache_, this, userdata_);
                recv_curr_cache_.clear();
                *_offset = diff;
                if (recv_wait_timer_.is_start())
                    recv_wait_timer_.cancel();
                return 0;
            }
        }
        return 0;
    }
    
    inline mmint_t package_preproc::poll_check(mgu_timestamp_t _now) const
    {
        auto iv = recv_wait_timer_.due_in();
        if (!iv)
            return 0;
        if (!recv_wait_cache_.empty())
            return 0;
        return mmint_t(iv);
    }

    inline mgec_t package_preproc::poll(mgu_timestamp_t _now)
    {
        if (!recv_wait_cache_.empty())
        {
            raw_input({}, _now);
        }
        
        recv_wait_timer_.timing(_now);
        return 0;
    }

};
};

#endif // !MMUPP_STREAM_PACKAGE_HPP_INCLUDED
