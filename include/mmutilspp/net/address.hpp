
#ifndef MMUPP_NET_ADDRESS_HPP_INCLUDED
#define MMUPP_NET_ADDRESS_HPP_INCLUDED

#include <memepp/string.hpp>
#include <mego/predef/os/linux.h>
#include <mego/predef/os/windows.h>

#if MG_OS__WIN_AVAIL
#include <ws2tcpip.h>
#endif

#if MG_OS__LINUX_AVAIL
#include <arpa/inet.h>
#endif

namespace mmupp {
namespace net {

    enum class address_type: uint8_t
    {
        none           = 0x00,
        ipv4           = 0x01,
        ipv6           = 0x02,
        ipv6_with_ipv4 = 0x03,
        domain         = 0x04
    };

    struct address 
    {
        using string_type = memepp::string;

        address() noexcept: 
            data_("127.0.0.1"),
            type_(address_type::ipv4) 
        {}
        
        address(string_type _data):
            data_(_data.trim_space())
        {
            identify_and_set_type(data_);
        }

        address(const address& _other):
            data_(_other.data_),
            type_(_other.type_)
        {}

        address(address&& _other):
            data_(std::move(_other.data_)),
            type_(std::move(_other.type_))
        {}

        inline address& operator=(const address& _other)
        {
            data_ = _other.data_;
            type_ = _other.type_;
            return *this;
        }

        inline address& operator=(address&& _other)
        {
            data_ = std::move(_other.data_);
            type_ = std::move(_other.type_);
            return *this;
        }

        inline const string_type& str() const noexcept
        {
            return data_;
        }

        inline constexpr address_type type() const noexcept
        {
            return type_;
        }

        inline constexpr bool is_ip_format() const noexcept
        {
            return !!(static_cast<uint8_t>(type_) & static_cast<uint8_t>(address_type::ipv6_with_ipv4));
        }

        inline constexpr bool is_domain_format() const noexcept
        {
            return type_ == address_type::domain;
        }

        template<typename _Container>
        inline void split_domain(std::back_insert_iterator<_Container> _out) const
        {
            if (type_ != address_type::domain) 
                return;

            data_.split(".", memepp::split_behavior_t::keep_empty_parts, _out);
        }

        inline bool operator==(const address& _other) const noexcept
        {
            switch (type()) {
            case address_type::ipv6_with_ipv4:
            {
                if (_other.type() == address_type::ipv4)
                    return data_.contains(_other.data_);
            } break;
            case address_type::ipv4:
            {
                if (_other.type() == address_type::ipv6_with_ipv4)
                    return _other.data_.contains(data_);
            } break;
            default: 
                break;
            }
            
            return data_ == _other.data_;
        }

        inline bool operator!=(const address& _other) const noexcept
        {
            return !(*this == _other);
        }

        inline void set_data(string_type _data)
        {
            data_ = _data.trim_space(); 
            identify_and_set_type(data_);
        }

    private:
        inline void identify_and_set_type(const string_type& _data) noexcept
        {
            unsigned char buf[sizeof(struct in6_addr)] = { 0 };
            if (inet_pton(AF_INET, _data.data(), buf) == 1)
            {
                type_ = address_type::ipv4;
            }
            else if (inet_pton(AF_INET6, _data.data(), buf) == 1)
            {
                if (_data.starts_with("::ffff:") || _data.starts_with("::FFFF:"))
                    type_ = address_type::ipv6_with_ipv4;
                else
                    type_ = address_type::ipv6;
            }
            else {
                type_ = address_type::domain;
            }
        }


        string_type  data_;
        address_type type_;
        
    };

}
} // namespace mmupp

#endif // !MMUPP_NET_ADDRESS_HPP_INCLUDED
