
#ifndef MMUPP_UTIL_OS_WIN_COM_H_INCLUDED
#define MMUPP_UTIL_OS_WIN_COM_H_INCLUDED

#include <mego/predef/os/windows.h>

#if MG_OS__WIN_AVAIL
#  include <oleauto.h>
#endif

namespace mmupp {
namespace util {
namespace os {
namespace win {

struct com_env
{
    inline com_env() noexcept
    {
#if MG_OS__WIN_AVAIL
        result_ = CoInitializeEx(
            0,
            COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE
        );
#endif
    }

    inline com_env(int _coinit) noexcept
    {
#if MG_OS__WIN_AVAIL
        result_ = CoInitializeEx(
            0,
            _coinit
        );
#endif
    }

    inline ~com_env() noexcept
    {
#if MG_OS__WIN_AVAIL
        if (SUCCEEDED(result_))
        {
            CoUninitialize();
        }
#endif
    }

private:
    HRESULT result_;
};

}
}
}
}

#endif // !MMUPP_UTIL_OS_WIN_COM_H_INCLUDED
