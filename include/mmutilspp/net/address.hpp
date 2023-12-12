
#ifndef MMUPP_NET_ADDRESS_HPP_INCLUDED
#define MMUPP_NET_ADDRESS_HPP_INCLUDED

#include <mego/predef/os/linux.h>
#include <mego/predef/os/windows.h>

#include <memepp/string.hpp>
#include <megopp/util/scope_cleanup.h>

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

        inline bool to_sockaddr_storage(uint16_t _port, sockaddr_storage* _out) const noexcept
        {
            if (type_ == address_type::ipv4) 
            {
                sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(_out);
                addr->sin_family = AF_INET;
                addr->sin_port   = htons(_port);
                if (inet_pton(AF_INET, data_.c_str(), &addr->sin_addr) != 1)
                    return false;
                return true;
            }
            else if (type_ == address_type::ipv6 || type_ == address_type::ipv6_with_ipv4) 
            {
                sockaddr_in6* addr = reinterpret_cast<sockaddr_in6*>(_out);
                addr->sin6_family = AF_INET6;
                addr->sin6_port   = htons(_port);
                if (inet_pton(AF_INET6, data_.c_str(), &addr->sin6_addr) != 1)
                    return false;
                return true;
            }

            return false;
        }

        template<typename _Container>
        inline void split_domain(std::back_insert_iterator<_Container> _out) const
        {
            if (type_ != address_type::domain) 
                return;

            data_.split(".", memepp::split_behavior_t::keep_empty_parts, _out);
        }

        //inline bool block_compare(const address& _other) const noexcept
        //{
        //    switch (type()) {
        //    case address_type::ipv6_with_ipv4:
        //    {
        //        if (_other.type() == address_type::ipv4)
        //            return data_.contains(_other.data_);
        //    } break;
        //    case address_type::ipv4:
        //    {
        //        if (_other.type() == address_type::ipv6_with_ipv4)
        //            return _other.data_.contains(data_);
        //    } break;
        //    default:
        //        break;
        //    }
        //    
        //    addrinfo hints;
        //    memset(&hints, 0, sizeof(hints));
        //    hints.ai_family   = PF_UNSPEC;
        //    hints.ai_socktype = SOCK_STREAM;

        //    addrinfo* lhs_ai = nullptr;
        //    if (type() == address_type::domain)
        //    {
        //        if (getaddrinfo(data_.c_str(), nullptr, &hints, &lhs_ai) != 0)
        //            return false;
        //    }
        //    MEGOPP_UTIL__ON_SCOPE_CLEANUP([lhs_ai] { if (lhs_ai) freeaddrinfo(lhs_ai); });
        //    
        //    memset(&hints, 0, sizeof(hints));
        //    hints.ai_family = PF_UNSPEC;
        //    hints.ai_socktype = SOCK_STREAM;

        //    addrinfo* rhs_ai = nullptr;
        //    if (_other.type() == address_type::domain)
        //    {
        //        if (getaddrinfo(_other.data_.c_str(), nullptr, &hints, &rhs_ai) != 0)
        //            return false;
        //    }
        //    MEGOPP_UTIL__ON_SCOPE_CLEANUP([rhs_ai] { if (rhs_ai) freeaddrinfo(rhs_ai); });
        //    
        //    // TO_DO
        //    
        //    return data_ == _other.data_;
        //}
        
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
