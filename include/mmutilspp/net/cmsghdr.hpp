
#ifndef MMUPP_NET_CMSGHDR_HPP_INCLUDED
#define MMUPP_NET_CMSGHDR_HPP_INCLUDED

#include <mego/predef/os/linux.h>
#include <mego/predef/os/windows.h>

#if MG_OS__WINDOWS_AVAIL
#include <ws2def.h>
#endif

namespace mmupp {
namespace net {

    struct cmsghdr_ptr
    {
        constexpr cmsghdr_ptr(struct cmsghdr * ptr) noexcept : ptr_(ptr) {}
        
        constexpr struct cmsghdr * get() const noexcept { return ptr_; }
        constexpr struct cmsghdr * operator->() const noexcept { return ptr_; }
        constexpr struct cmsghdr & operator*() const noexcept { return *ptr_; }

        constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }

        constexpr bool operator==(const cmsghdr_ptr & other) const noexcept { return ptr_ == other.ptr_; }

        constexpr bool is_null() const noexcept { return ptr_ == nullptr; }

#if MG_OS__LINUX_AVAIL
        void self_next(struct msghdr * _msg) noexcept { ptr_ = CMSG_NXTHDR(msg, ptr_); }
        cmsghdr_ptr next(struct msghdr * _msg) const noexcept { return CMSG_NXTHDR(msg, ptr_); }
        
        void* data() const noexcept { return CMSG_DATA(ptr_); }
#endif
#if MG_OS__WINDOWS_AVAIL
        void self_next(WSAMSG * _msg) noexcept { ptr_ = CMSG_NXTHDR(msg, ptr_); }
        cmsghdr_ptr next(WSAMSG * _msg) const noexcept { return CMSG_NXTHDR(msg, ptr_); }

        void* data() const noexcept { return WSA_CMSG_DATA(ptr_); }
#endif


    private:
        struct cmsghdr * ptr_;
    };

}
}

#endif // !MMUPP_NET_CMSGHDR_HPP_INCLUDED
