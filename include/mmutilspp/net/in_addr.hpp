
#ifndef MMUPP_NET_IN_ADDR_HPP_INCLUDED
#define MMUPP_NET_IN_ADDR_HPP_INCLUDED

#include <mego/predef/os/linux.h>
#include <mego/util/os/windows/windows_simplify.h>
#if MG_OS__LINUX_AVAIL
#  include <netinet/in.h>
#  include <arpa/inet.h>
#endif

#include <memepp/string.hpp>
#include <memepp/string_view.hpp>

#include <string>

namespace mmupp {
namespace net {

    struct in04_addr
    {
        in04_addr()
        {
            memset(&addr_, 0, sizeof(addr_));
        }

        in04_addr(const ::in_addr& _addr)
        : addr_(_addr)
        {}

        in04_addr(const char* _addr)
        {
            if (inet_pton(AF_INET, _addr, &addr_) != 1)
            {
                memset(&addr_, 0, sizeof(addr_));
            }
        }

        in04_addr(const std::string& _addr)
        : in04_addr(_addr.c_str())
        {}

        in04_addr(const memepp::string& _addr)
        : in04_addr(_addr.c_str())
        {}

        in04_addr(const memepp::string_view& _addr)
        : in04_addr(_addr.to_string().c_str())
        {}

        template <typename T>
        inline T to() const
        {
            return T{};
        }

        constexpr const ::in_addr& raw() const noexcept
        {
            return addr_;
        }

        constexpr ::in_addr& raw() noexcept
        {
            return addr_;
        }

        template <>
        inline ::in_addr to<::in_addr>() const
        {
            return addr_;
        }

        template <>
        inline ::in6_addr to<::in6_addr>() const
        {
            auto s = to<std::string>();
            ::in6_addr addr;
            if (inet_pton(AF_INET6, s.c_str(), &addr) != 1)
            {
                return ::in6_addr{};
            }

            return addr;
        }

        template <>
        inline std::string to<std::string>() const
        {
            char buffer[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &addr_, buffer, INET_ADDRSTRLEN) == nullptr)
            {
                return std::string{};
            }
            return std::string{ buffer };
        }

        template <>
        inline memepp::string to<memepp::string>() const
        {
            char buffer[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &addr_, buffer, INET_ADDRSTRLEN) == nullptr)
            {
                return memepp::string{};
            }
            return memepp::string{ buffer };
        }

        static in04_addr from(const ::in_addr& _addr)
        {
            in04_addr addr;
            addr.addr_ = _addr;
            return addr;
        }

        static in04_addr from(const char* _addr)
        {
            in04_addr addr;
            if (inet_pton(AF_INET, _addr, &addr.addr_) != 1)
            {
                return in04_addr{};
            }
            return addr;
        }

        static in04_addr from(const std::string& _addr)
        {
            return from(_addr.c_str());
        }

        static in04_addr from(const memepp::string& _addr)
        {
            return from(_addr.c_str());
        }

        static in04_addr from(const memepp::string_view& _addr)
        {
            return from(_addr.to_string().c_str());
        }

    private:
        ::in_addr addr_;
    };

    struct in06_addr
    {
        in06_addr()
        {
            memset(&addr_, 0, sizeof(addr_));
        }

        in06_addr(const ::in6_addr& _addr)
        : addr_(_addr)
        {}

        in06_addr(const char* _addr)
        {
            if (inet_pton(AF_INET6, _addr, &addr_) != 1)
            {
                memset(&addr_, 0, sizeof(addr_));
            }
        }

        in06_addr(const std::string& _addr)
        : in06_addr(_addr.c_str())
        {}

        in06_addr(const memepp::string& _addr)
        : in06_addr(_addr.c_str())
        {}

        in06_addr(const memepp::string_view& _addr)
        : in06_addr(_addr.to_string().c_str())
        {}

        template <typename T>
        inline T to() const
        {
            return T{};
        }

        template <>
        inline ::in6_addr to<::in6_addr>() const
        {
            return addr_;
        }

        template <>
        inline std::string to<std::string>() const
        {
            char buffer[INET6_ADDRSTRLEN];
            if (inet_ntop(AF_INET6, &addr_, buffer, INET6_ADDRSTRLEN) == nullptr)
            {
                return std::string{};
            }
            return std::string{ buffer };
        }

        template <>
        inline memepp::string to<memepp::string>() const
        {
            char buffer[INET6_ADDRSTRLEN];
            if (inet_ntop(AF_INET6, &addr_, buffer, INET6_ADDRSTRLEN) == nullptr)
            {
                return memepp::string{};
            }
            return memepp::string{ buffer };
        }

        static in06_addr from(const ::in6_addr& _addr)
        {
            in06_addr addr;
            addr.addr_ = _addr;
            return addr;
        }

        static in06_addr from(const char* _addr)
        {
            in06_addr addr;
            if (inet_pton(AF_INET6, _addr, &addr.addr_) != 1)
            {
                return in06_addr{};
            }
            return addr;
        }

        static in06_addr from(const std::string& _addr)
        {
            return from(_addr.c_str());
        }

        static in06_addr from(const memepp::string& _addr)
        {
            return from(_addr.c_str());
        }

        static in06_addr from(const memepp::string_view& _addr)
        {
            return from(_addr.to_string().c_str());
        }

    private:
        ::in6_addr addr_;
    };

    struct in_addr
    {
        in_addr()
        : family_(-1)
        {
            memset(&in6_, 0, sizeof(in6_));
        }

        in_addr(const ::in_addr& _addr)
        : family_(AF_INET)
        , in04_(_addr)
        {}

        in_addr(const ::in6_addr& _addr)
        : family_(AF_INET6)
        , in06_(_addr)
        {}

        in_addr(const char* _addr)
        {
            if (inet_pton(AF_INET, _addr, &in04_.raw()) == 1)
            {
                family_ = AF_INET;
            }
            else if (inet_pton(AF_INET6, _addr, &in06_.raw()) == 1)
            {
                family_ = AF_INET6;
            }
            else {
                family_ = -1;
            }
        }

        in_addr(const std::string& _addr)
        : in_addr(_addr.c_str())
        {}

        in_addr(const memepp::string& _addr)
        : in_addr(_addr.c_str())
        {}

        in_addr(const memepp::string_view& _addr)
        : in_addr(_addr.to_string().c_str())
        {}

        constexpr int family() const noexcept
        {
            return family_;
        }

        constexpr const ::in_addr* ipv4raw() const noexcept
        {
            return family_ == AF_INET ? &in04_ : nullptr;
        }

        constexpr const ::in6_addr* ipv6raw() const noexcept
        {
            return family_ == AF_INET6 ? &in06_ : nullptr;
        }

        template <typename T>
        inline T to() const
        {
            return T{};
        }

        template <>
        inline ::in_addr to<::in_addr>() const
        {
            if (family_ == AF_INET)
            {
                return in04_;
            }
            return ::in_addr{};
        }

        template <>
        inline ::in6_addr to<::in6_addr>() const
        {
            if (family_ == AF_INET6)
            {
                return in06_;
            }
            else if (family_ == AF_INET)
            {
                auto s = to<std::string>();
                ::in6_addr addr;
                if (inet_pton(AF_INET6, s.c_str(), &addr) != 1)
                {
                    return ::in6_addr{};
                }

                return addr;
            }
            return ::in6_addr{};
        }

        template <>
        inline std::string to<std::string>() const
        {
            if (family_ == AF_INET)
            {
                char buffer[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &in04_.raw(), buffer, INET_ADDRSTRLEN) != nullptr)
                {
                    return std::string{ buffer };
                }
            }
            else if (family_ == AF_INET6)
            {
                char buffer[INET6_ADDRSTRLEN];
                if (inet_ntop(AF_INET6, &in06_.raw(), buffer, INET6_ADDRSTRLEN) != nullptr)
                {
                    return std::string{ buffer };
                }
            }
            return std::string{};
        }

        template <>
        inline memepp::string to<memepp::string>() const
        {
            if (family_ == AF_INET)
            {
                char buffer[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &in04_.raw(), buffer, INET_ADDRSTRLEN) != nullptr)
                {
                    return memepp::string{ buffer };
                }
            }
            else if (family_ == AF_INET6)
            {
                char buffer[INET6_ADDRSTRLEN];
                if (inet_ntop(AF_INET6, &in06_.raw(), buffer, INET6_ADDRSTRLEN) != nullptr)
                {
                    return memepp::string{ buffer };
                }
            }
            return memepp::string{};
        }

        static in_addr from(const ::in_addr& _addr)
        {
            in_addr addr;
            addr.family_ = AF_INET;
            addr.in04_ = _addr;
            return addr;
        }

        static in_addr from(const ::in6_addr& _addr)
        {
            in_addr addr;
            addr.family_ = AF_INET6;
            addr.in06_ = _addr;
            return addr;
        }

        static in_addr from(const char* _addr)
        {
            in_addr addr;
            if (inet_pton(AF_INET, _addr, &addr.in04_.raw()) == 1)
            {
                addr.family_ = AF_INET;
            }
            else if (inet_pton(AF_INET6, _addr, &addr.in06_.raw()) == 1)
            {
                addr.family_ = AF_INET6;
            }
            else {
                addr.family_ = -1;
            }
            return addr;
        }

        static in_addr from(const std::string& _addr)
        {
            return from(_addr.c_str());
        }

        static in_addr from(const memepp::string& _addr)
        {
            return from(_addr.c_str());
        }

        static in_addr from(const memepp::string_view& _addr)
        {
            return from(_addr.to_string().c_str());
        }

    private:
        int family_;
        union {
            struct ::in_addr  in04_;
            struct ::in6_addr in06_;
        };
    };

} // namespace net
} // namespace mmupp

#endif // !MMUPP_NET_IN_ADDR_HPP_INCLUDED
