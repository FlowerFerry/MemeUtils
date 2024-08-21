
#ifndef MMUPP_UTIL_OS_WIN_FIREWALL_H_INCLUDED
#define MMUPP_UTIL_OS_WIN_FIREWALL_H_INCLUDED

#include <mego/predef/os/windows.h>
#include "com.h"

#if MG_OS__WIN_AVAIL
#  include <objbase.h>
#  include <netfw.h>
#endif

#include <megopp/err/err.h>
#include <memepp/convert/std/wstring.hpp>
#include <memepp/string_view.hpp>
#include <memepp/native.hpp>
#include <megopp/util/scope_cleanup.h>

namespace mmupp {
namespace util {
namespace os {
namespace win {

struct firewall
{
    inline firewall() noexcept
    {
        init();
    }

    inline ~firewall() noexcept
    {
        uninit();
    }

    inline mgpp::err init() noexcept
    {
#if MG_OS__WIN_AVAIL
        auto hr = CoCreateInstance(
            __uuidof(NetFwPolicy2),
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(INetFwPolicy2),
            (void**)&pNetFwPolicy2_
        );

        if (FAILED(hr)) {
            return { MGEC__ERR };
        }

        return 0;
#else
        return { MGEC__OPNOTSUPP };
#endif
    }

    inline void uninit() noexcept
    {
#if MG_OS__WIN_AVAIL
        if (pNetFwPolicy2_) {
            pNetFwPolicy2_->Release();
            pNetFwPolicy2_ = NULL;
        }
#endif
    }

    inline mgpp::err add_app_full_allowed(
		const memepp::string_view & _filename, const memepp::string_view & _fwname) noexcept
    {
#if MG_OS__WIN_AVAIL
        if (!pNetFwPolicy2_) {
            return { MGEC__INVALID_HANDLE };
        }

        INetFwRules *pFwRules = NULL;
		INetFwRule* pFwRule = NULL;
		HRESULT hr = S_OK;
		long CurrentProfilesBitMask = 0;
		BSTR bstrRuleApplication = NULL;
		BSTR bstrRuleName = NULL;

        MEGOPP_UTIL__ON_SCOPE_CLEANUP([&]() {
            if (bstrRuleApplication) {
                SysFreeString(bstrRuleApplication);
            }
            if (bstrRuleName) {
                SysFreeString(bstrRuleName);
            }
            if (pFwRule) {
                pFwRule->Release();
            }
            if (pFwRules) {
                pFwRules->Release();
            }
        });

        hr = pNetFwPolicy2_->get_CurrentProfileTypes(&CurrentProfilesBitMask);
        if (FAILED(hr)) {
            return { MGEC__ERR };
        }

        hr = pNetFwPolicy2_->get_Rules(&pFwRules);
        if (FAILED(hr)) {
            return { MGEC__ERR };
        }

        auto name = mm_into<memepp::native_string>(_fwname);
        bstrRuleName = SysAllocString(name.c_str());
        if (bstrRuleName == NULL) {
            return { MGEC__ERR };
        }

        hr = pFwRules->Item(bstrRuleName, &pFwRule);
        if (SUCCEEDED(hr)) {
            return { MGEC__EXIST };
        }
        else if (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) 
        {
            return { MGEC__ERR };
        }

        hr = CoCreateInstance(
            __uuidof(NetFwRule),
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(INetFwRule),
            (void**)&pFwRule
        );
        if (FAILED(hr)) {
            return { MGEC__ERR };
        }

        auto filename = mm_into<memepp::native_string>(_filename);
        bstrRuleApplication = SysAllocString(filename.c_str());

		hr = pFwRule->put_Name(bstrRuleName);
//		hr = pFwRule->put_Description(bstrRuleDescription);
		hr = pFwRule->put_ApplicationName(bstrRuleApplication);
//		hr = pFwRule->put_ServiceName(bstrRuleService);
		hr = pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
//		hr = pFwRule->put_LocalPorts(NULL);
//		hr = pFwRule->put_Grouping(bstrRuleGroup);
		hr = pFwRule->put_Action(NET_FW_ACTION_ALLOW);
		hr = pFwRule->put_Enabled(VARIANT_TRUE);
		hr = pFwRule->put_Profiles(NET_FW_PROFILE2_PUBLIC | NET_FW_PROFILE2_PRIVATE | NET_FW_PROFILE2_DOMAIN);
		if (FAILED(hr)) {
			pFwRule->put_Profiles(CurrentProfilesBitMask);
		}

		hr = pFwRules->Add(pFwRule);
		if (FAILED(hr)) {
            return { MGEC__ERR };
		}
		
		return 0;
#else
        return { MGEC__OPNOTSUPP };
#endif
    }

private:
    com_env comenv_;
    INetFwPolicy2* pNetFwPolicy2_;
};

}
}
}
}

#endif // !MMUPP_UTIL_OS_WIN_FIREWALL_H_INCLUDED
