
#ifndef MMUPP_UTIL_OS_WIN_SERVICE_H_INCLUDED
#define MMUPP_UTIL_OS_WIN_SERVICE_H_INCLUDED

#include <mego/predef/os/windows.h>
#include <mego/util/os/windows/windows_simplify.h>
#include <mego/err/ec_impl.h>
#include <mego/util/std/time.h>

#include <memepp/string_view.hpp>
#include <memepp/native.hpp>
#include <memepp/convert/std/wstring.hpp>
#include <memepp/convert/self.hpp>
#include <megopp/err/err.h>
#include <megopp/util/scope_cleanup.h>
#include <mmutilspp/fs/program_path.hpp>

#include <mutex>
#include <atomic>
#include <functional>

#if MG_OS__WIN_AVAIL
#	include <DbgHelp.h>
#	pragma comment(lib, "DbgHelp.lib")
#endif

namespace mmupp {
namespace util {
namespace os {
namespace win {

struct service
{
    enum class process_rate_e 
    {
        none = 0,
        query_status,
        wait_for_stop,
        wait_for_dependents_stop,
        wait_for_start,
    };

    using executor_t = std::function<mgpp::err()>;
    using process_rate_cb_t = std::function<void(process_rate_e, int _count)>;

    enum class event_level_e : uint32_t
    {
        error = 0xC0020001,
        info  = 0x40020001,
        warn  = 0x80020001
    };

    struct install_options
    {
        memepp::string desc;
    };

    struct start_options
    {
        int timeout_ms = 30000; // Default timeout for service start
    };

    struct stop_options
    {
        int timeout_ms = 30000; // Default timeout for service stop
    };

    void set_service_name(const memepp::string_view& _name)
    {
        std::unique_lock locker{ mutex_ };
        service_name_ = mm_into<memepp::native_string>(_name);
    }

    void on_init (const executor_t& _fn)
    {
        init_fn_ = _fn;
    }

    void on_start_req(const executor_t& _fn)
    {
        start_req_fn_ = _fn;
    }

    void on_stop_req (const executor_t& _fn)
    {
        stop_req_fn_ = _fn;
    }
    void on_loop (const executor_t& _fn)
    {
        loop_fn_ = _fn;
    }
    void on_exit (const executor_t& _fn)
    {
        exit_fn_ = _fn;
    }

    void on_started(const executor_t& _fn)
    {
        started_fn_ = _fn;
    }

    void on_stopped(const executor_t& _fn)
    {
        stopped_fn_ = _fn;
    }

    mgpp::err install(const install_options& _opts);
    mgpp::err uninstall();
    mgpp::err start(const start_options& _opts, const process_rate_cb_t& _process_rate_cb = nullptr);
    mgpp::err stop (const stop_options&  _opts, const process_rate_cb_t& _process_rate_cb = nullptr);

    mgpp::err run();

    mgpp::err report_event(event_level_e _level, const memepp::string_view& _message) const
    {
#if MG_OS__WIN_AVAIL
        std::unique_lock locker{ mutex_ };
        auto service_name = service_name_;
        locker.unlock();
        return report_event(service_name, _level, mm_into<memepp::native_string>(_message).c_str());
#else
        return { MGEC__OPNOTSUPP, "Service event reporting is not supported on this OS" };
#endif
    }

#if MG_OS__WIN_AVAIL
    static mgpp::err report_event(
        const memepp::native_string& _service_name,
        event_level_e _level,
        const wchar_t* _message);
#endif

    static service& instance()
    {
        static service instance_;
        return instance_;
    }

private:
    service();

#if MG_OS__WIN_AVAIL

	void  __on_win_svc_main(DWORD _dwArgc, LPWSTR *_lpszArgv);
	DWORD __on_win_svc_ctrl_handler(
        DWORD _dwCtrl, DWORD _dwEventType, LPVOID _lpEventData);
	LONG  __on_unhandled_exception_handler(EXCEPTION_POINTERS *_lpExceptionInfo);

	static VOID  __win_svc_report_status(
        SERVICE_STATUS_HANDLE _handle, 
        DWORD _dwCurrentState, DWORD _dwWin32ExitCode, DWORD _dwWaitHint,
        SERVICE_STATUS& _status);
        
	static VOID  WINAPI __win_svc_main(DWORD _dwArgc, LPWSTR *_lpszArgv);
	static DWORD WINAPI __win_svc_ctrl_handler(
        DWORD _dwCtrl, DWORD _dwEventType, LPVOID _lpEventData, LPVOID _lpContext);
	static LONG  WINAPI __unhandled_exception_handler(
        EXCEPTION_POINTERS *_lpExceptionInfo);
    
    static mgpp::err __wait_service_status(
        SC_HANDLE _scService, DWORD _desiredStatus, DWORD64 _timeout = 30000);

	static mgpp::err __stop_dependent_services(
        SC_HANDLE _scManager, SC_HANDLE _scService, const process_rate_cb_t& _process_rate_cb);

#endif

    mutable std::mutex mutex_;
    executor_t init_fn_;
    executor_t start_req_fn_;
    executor_t stop_req_fn_;
    executor_t loop_fn_;
    executor_t exit_fn_;
    executor_t started_fn_;
    executor_t stopped_fn_;

#if MG_OS__WIN_AVAIL
    memepp::native_string service_name_;
#else
    memepp::string service_name_;
#endif

#if MG_OS__WIN_AVAIL
	SERVICE_STATUS_HANDLE service_status_handle_ = nullptr;
	SERVICE_STATUS service_status_;
#endif

    std::atomic_bool is_stopping_ = false;
};

service::service()
{
#if MG_OS__WIN_AVAIL
    memset(&service_status_, 0, sizeof(service_status_));
    service_status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    service_status_.dwControlsAccepted = 
        SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_TIMECHANGE;
    service_status_.dwServiceSpecificExitCode = 0;
#endif
}

inline mgpp::err service::install(const install_options& _opts)
{		
#if MG_OS__WIN_AVAIL
    auto progPath = mmupp::fs::program_file_path();
    if (progPath.empty()) {
        return { MGEC__INVAL, "Failed to get program file path" };
    }
    
    auto schSCManager = OpenSCManagerW(
        NULL,                    // local computer
        NULL,                    // servicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights
    if (NULL == schSCManager) {
        return { mgec__from_sys_err(GetLastError()), "OpenSCManager failed" };
    }
    MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] { CloseServiceHandle(schSCManager); });

    std::unique_lock locker{ mutex_ };
    if (service_name_.empty())
        return { MGEC__INVAL, "Service name is not set" };
    auto service_name = service_name_;
    locker.unlock();

    memepp::native_string exePath;
    if (progPath.starts_with('\"')) {
        exePath = mm_into<memepp::native_string>(progPath);
    } else {
        exePath = MMN_TEXT('\"') + mm_into<memepp::native_string>(progPath) + MMN_TEXT('\"');
    }
    
    auto schService = CreateServiceW(
        schSCManager,
        service_name.c_str(),
        service_name.c_str(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        exePath.c_str(),
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);
    if (NULL == schService) {
        return { mgec__from_sys_err(GetLastError()), "CreateService failed" };
    }
    MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] { CloseServiceHandle(schService); });
    			
    SERVICE_FAILURE_ACTIONSW servFailActions;
	SC_ACTION failActions[3];

	failActions[0].Type = SC_ACTION_RESTART; //Failure action: Restart Service
	failActions[0].Delay = 180000; //number of milliseconds to wait before performing failure action, in milliseconds = 3minutes
	failActions[1].Type = SC_ACTION_RESTART;
	failActions[1].Delay = 180000;
	failActions[2].Type = SC_ACTION_RESTART;
	failActions[2].Delay = 180000;

	servFailActions.dwResetPeriod = 172800; // Reset Failures Counter, in Seconds = 2days
	servFailActions.lpCommand = NULL; //Command to perform due to service failure, not used
	servFailActions.lpRebootMsg = NULL; //Message during rebooting computer due to service failure, not used
	servFailActions.cActions = 3; // Number of failure action to manage
	servFailActions.lpsaActions = failActions;

	ChangeServiceConfig2W(schService, SERVICE_CONFIG_FAILURE_ACTIONS, &servFailActions);

    auto descStr = mm_into<memepp::native_string>(_opts.desc);

	SERVICE_DESCRIPTIONW srvDesc;
	srvDesc.lpDescription = const_cast<wchar_t*>(descStr.data());
	ChangeServiceConfig2W(schService, SERVICE_CONFIG_DESCRIPTION, &srvDesc);

	SERVICE_DELAYED_AUTO_START_INFO srvDelayAutoStartInfo;
	srvDelayAutoStartInfo.fDelayedAutostart = TRUE;
	ChangeServiceConfig2W(schService, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, &srvDelayAutoStartInfo);

    return {};
#else
    return { MGEC__OPNOTSUPP, "Service installation is not supported on this OS" };
#endif
}

inline mgpp::err service::uninstall()
{
#if MG_OS__WIN_AVAIL
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    schSCManager = OpenSCManagerW(
        NULL,
        NULL,
        SC_MANAGER_CONNECT);
    if (NULL == schSCManager) {
        return { mgec__from_sys_err(GetLastError()), "OpenSCManager failed" };
    }
    MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] { CloseServiceHandle(schSCManager); });

    std::unique_lock locker{ mutex_ };
    if (service_name_.empty())
        return { MGEC__INVAL, "Service name is not set" };
    auto service_name = service_name_;
    locker.unlock();

    schService = OpenServiceW(
        schSCManager,
        service_name.c_str(),
        SERVICE_STOP | DELETE);
    if (NULL == schService) {
        return { mgec__from_sys_err(GetLastError()), "OpenService failed" };
    }
    MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] { CloseServiceHandle(schService); });

    if (!DeleteService(schService)) {
        auto ec = GetLastError();
        if (ec == ERROR_SERVICE_MARKED_FOR_DELETE) {
            return { MGEC__OK, "Service is marked for deletion" };
        }
        return { mgec__from_sys_err(GetLastError()), "DeleteService failed" };
    }

    return {};
#else
    return { MGEC__OPNOTSUPP, "Service uninstallation is not supported on this OS" };
#endif
}

inline mgpp::err service::start(const start_options& _opts, const process_rate_cb_t& _process_rate_cb)
{
#if MG_OS__WIN_AVAIL
    mgpp::err err;
    auto schSCManager = OpenSCManagerW(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_CONNECT);
    if (NULL == schSCManager) {
        return { mgec__from_sys_err(GetLastError()), "OpenSCManager failed" };
    }
    MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] { CloseServiceHandle(schSCManager); });

    std::unique_lock locker{ mutex_ };
    if (service_name_.empty())
        return { MGEC__INVAL, "Service name is not set" };
    auto service_name = service_name_;
    locker.unlock();

    auto schService = OpenServiceW(
        schSCManager,         // SCM database 
        service_name.c_str(),      // name of service 
        SERVICE_START | SERVICE_QUERY_STATUS);
    if (NULL == schService) {
        return { mgec__from_sys_err(GetLastError()), "OpenService failed" };
    }
    MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] { CloseServiceHandle(schService); });

    if (_process_rate_cb) {
        _process_rate_cb(process_rate_e::query_status, 0);
    }

	SERVICE_STATUS_PROCESS serviceStatus;
    if (!QueryServiceStatusEx(
        schService,
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&serviceStatus,
        sizeof(serviceStatus),
        NULL))
    {
        return { mgec__from_sys_err(GetLastError()), "QueryServiceStatusEx failed" };
    }

    if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        return {};
    }

    if (serviceStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        if (_process_rate_cb) {
            _process_rate_cb(process_rate_e::wait_for_start, 0);
        }

        err = __wait_service_status(
            schService, SERVICE_RUNNING, _opts.timeout_ms); // Wait for service to start
        if (err)
            return err;

        return {};
    }

    if (serviceStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        if (_process_rate_cb) {
            _process_rate_cb(process_rate_e::wait_for_stop, 0);
        }

        err = __wait_service_status(schService, SERVICE_STOPPED, _opts.timeout_ms);
        if (err)
            return err;
    }

    if (!StartServiceW(
        schService,  // handle to service 
        0,           // number of arguments 
        NULL))       // no arguments 
    {
        if (ERROR_SERVICE_DISABLED == GetLastError()) {
            return { mgec__from_sys_err(GetLastError()), "Service is disabled" };
        }
        return { mgec__from_sys_err(GetLastError()), "StartService failed" };
    }

    if (_process_rate_cb) {
        _process_rate_cb(process_rate_e::wait_for_start, 0);
    }

    err = __wait_service_status(schService, SERVICE_RUNNING, _opts.timeout_ms); // Wait for service to start
    if (err)
        return err;

    return {};
#else
    return { MGEC__OPNOTSUPP, "Service start is not supported on this OS" };
#endif
}

inline mgpp::err service::stop(const stop_options& _opts, const process_rate_cb_t& _process_rate_cb)
{
#if MG_OS__WIN_AVAIL
    mgpp::err err;
    auto schSCManager = OpenSCManagerW(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (NULL == schSCManager) {
        return { mgec__from_sys_err(GetLastError()), "OpenSCManager failed" };
    }
    MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] { CloseServiceHandle(schSCManager); });

    std::unique_lock locker{ mutex_ };
    if (service_name_.empty())
        return { MGEC__INVAL, "Service name is not set" };
    auto service_name = service_name_;
    locker.unlock();

    auto schService = OpenServiceW(
        schSCManager,         // SCM database 
        service_name.c_str(),      // name of service 
        SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS);
    if (NULL == schService) {
        return { mgec__from_sys_err(GetLastError()), "OpenService failed" };
    }
    MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] { CloseServiceHandle(schService); });

    if (_process_rate_cb) {
        _process_rate_cb(process_rate_e::query_status, 0);
    }

    SERVICE_STATUS_PROCESS serviceStatus;
    if (!QueryServiceStatusEx(
        schService,
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&serviceStatus,
        sizeof(serviceStatus),
        NULL))
    {
        return { mgec__from_sys_err(GetLastError()), "QueryServiceStatusEx failed" };
    }

    if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
    {
        return { MGEC__OK, "Service is already stopped" };
    }

    if (serviceStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        if (_process_rate_cb) {
            _process_rate_cb(process_rate_e::wait_for_stop, 0);
        }
        
        err = __wait_service_status(
            schService, SERVICE_STOPPED, _opts.timeout_ms); // Wait for service to stop
        if (err)
            return err;

        return {};
    }

    if (serviceStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        if (_process_rate_cb) {
            _process_rate_cb(process_rate_e::wait_for_start, 0);
        }

        err = __wait_service_status(
            schService, SERVICE_RUNNING, _opts.timeout_ms); // Wait for service to start
        if (err)
            return err;
    }

    err = __stop_dependent_services(schSCManager, schService, _process_rate_cb);
    if (err)
        return err;

    if (!ControlService(
        schService,
        SERVICE_CONTROL_STOP,
        (LPSERVICE_STATUS)&serviceStatus))
    {
        if (ERROR_SERVICE_NOT_ACTIVE == GetLastError()) {
            return {};
        }
        return { mgec__from_sys_err(GetLastError()), "ControlService failed" };
    }

    if (_process_rate_cb) {
        _process_rate_cb(process_rate_e::wait_for_stop, 0);
    }

    err = __wait_service_status(
        schService, SERVICE_STOPPED, _opts.timeout_ms); // Wait for service to stop
    if (err)
        return err;

    return {};
#else
    return { MGEC__OPNOTSUPP, "Service stop is not supported on this OS" };
#endif 
}

inline mgpp::err service::run()
{
#if MG_OS__WIN_AVAIL
    std::unique_lock locker{ mutex_ };
    if (service_name_.empty())
        return { MGEC__INVAL, "Service name is not set" };
    auto service_name = service_name_;
    locker.unlock();

    is_stopping_ = false;

    SetUnhandledExceptionFilter(__unhandled_exception_handler);

    SERVICE_TABLE_ENTRYW serviceTable[] = {
        { const_cast<wchar_t*>(service_name.c_str()), __win_svc_main },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcherW(serviceTable)) {
        return { mgec__from_sys_err(GetLastError()), "StartServiceCtrlDispatcher failed" };
    }

    return exit_fn_ ? exit_fn_() : mgpp::err{};
#else
    return { MGEC__OPNOTSUPP, "Service run is not supported on this OS" };
#endif
}

#if MG_OS__WIN_AVAIL
inline mgpp::err service::report_event(
    const memepp::native_string& _service_name,
    event_level_e _level, 
    const wchar_t* _message)
{

    if (_service_name.empty())
        return { MGEC__INVAL, "Service name is not set" };

    auto hEventSource = RegisterEventSourceW(NULL, _service_name.data());
    if (hEventSource == NULL) {
        return { mgec__from_sys_err(GetLastError()), "RegisterEventSource failed" };
    }
    MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] { DeregisterEventSource(hEventSource); });
    
	LPCTSTR lpszStrings[2] = { 0 };
    lpszStrings[0] = _service_name.c_str();
    lpszStrings[1] = _message;

    WORD evType;
    switch (_level) {
    case event_level_e::info:
        evType = EVENTLOG_INFORMATION_TYPE;
        break;
    case event_level_e::warn:
        evType = EVENTLOG_WARNING_TYPE;
        break;
    default:
        evType = EVENTLOG_ERROR_TYPE;
        break;
    }

    ReportEventW(
        hEventSource, 			
        evType,              // event type
		0,                   // event category
		static_cast<DWORD>(_level),// event identifier
		NULL,                // no security identifier
		2,                   // size of lpszStrings array
		0,                   // no binary data
		lpszStrings,         // array of strings
		NULL);               // no binary data
    return {};
}
#endif 

#if MG_OS__WIN_AVAIL
inline void service::__on_win_svc_main(DWORD _dwArgc, LPWSTR *_lpszArgv)
{
    std::unique_lock locker{ mutex_ };
    if (service_name_.empty())
    {
        // TO_DO
        return;
    }
    auto service_name = service_name_;
    locker.unlock();

	service_status_handle_ = RegisterServiceCtrlHandlerExW(
			service_name.c_str(), __win_svc_ctrl_handler, this);
	if (service_status_handle_ == NULL)
	{
		report_event(service_name, event_level_e::error, 
            L"In service::__on_win_svc_main, RegisterServiceCtrlHandlerExW failed");
		return;
	}

    std::unique_lock locker{ mutex_ };
	__win_svc_report_status(
        service_status_handle_, SERVICE_START_PENDING, NO_ERROR, 30000, service_status_);
    locker.unlock();

	mgpp::err result;
	try {
        if (init_fn_)
			result = init_fn_();
        else
			result = {};
	}
	catch (...) {
		report_event(service_name, event_level_e::error, 
            L"In service::__on_win_svc_main, init callback throw exception");
        locker.lock();
		__win_svc_report_status(
            service_status_handle_, SERVICE_STOPPED, ERROR_INVALID_FUNCTION, 0, service_status_);
		return;
	}
	if (result) {
        report_event(service_name, event_level_e::error, 
            L"In service::__on_win_svc_main, init callback returned error");
        locker.lock();
        __win_svc_report_status(
            service_status_handle_, SERVICE_STOPPED, ERROR_INVALID_FUNCTION, 0, service_status_);
		return;
	}

	try {
        if (start_req_fn_)
		    result = start_req_fn_();
        else {
            report_event(service_name, event_level_e::info, 
                L"In service::__on_win_svc_main, start_req callback is not set");
            result = {};
        }
	}
	catch (...) {
		report_event(service_name, event_level_e::error, 
            L"In service::__on_win_svc_main, start_req callback throw exception");
        locker.lock();
		__win_svc_report_status(
            service_status_handle_, SERVICE_STOPPED, ERROR_INVALID_FUNCTION, 0, service_status_);
		return;
	}
	if (result) {
		report_event(service_name, event_level_e::error, 
            L"In service::__on_win_svc_main, start_req callback returned error");
        locker.lock();
		__win_svc_report_status(
            service_status_handle_, SERVICE_STOPPED, ERROR_INVALID_FUNCTION, 0, service_status_);
		return;
	}

	try {
		if (started_fn_)
			result = started_fn_();
        else
            result = {};
	}
	catch (...) {
		report_event(service_name, event_level_e::error, 
            L"In service::__on_win_svc_main, started callback throw exception");
        locker.lock();
		__win_svc_report_status(
            service_status_handle_, SERVICE_STOPPED, ERROR_INVALID_FUNCTION, 0, service_status_);
		return;
	}
	if (result) {
        report_event(service_name, event_level_e::error, 
            L"In service::__on_win_svc_main, started callback returned error");
        locker.lock();
        __win_svc_report_status(
            service_status_handle_, SERVICE_STOPPED, ERROR_INVALID_FUNCTION, 0, service_status_);
		return;
	}

    locker.lock();
	__win_svc_report_status(
            service_status_handle_, SERVICE_RUNNING, NO_ERROR, 0, service_status_);
    locker.unlock();

	try {
        if(loop_fn_)
		    result = loop_fn_();
        else {
            report_event(service_name, event_level_e::info, 
                L"In ServiceRegistrar::__onWinSvcMain, loop callback is not set");
            result = {};
        }
	}
	catch (...) {
		report_event(service_name, event_level_e::error, 
            L"In ServiceRegistrar::__onWinSvcMain, loop callback throw exception");
        locker.lock();
		__win_svc_report_status(
            service_status_handle_, SERVICE_STOPPED, ERROR_INVALID_FUNCTION, 0, service_status_);
		return;
	}
	if (result) {
        report_event(service_name, event_level_e::error, 
            L"In ServiceRegistrar::__onWinSvcMain, loop callback returned error");
        locker.lock();
        __win_svc_report_status(
            service_status_handle_, SERVICE_STOPPED, ERROR_INVALID_FUNCTION, 0, service_status_);
		return;
	}

	try {
		if (stopped_fn_)
			result = stopped_fn_();
        else
            result = {};
	}
	catch (...) {
		report_event(service_name, event_level_e::error, 
            L"In ServiceRegistrar::__onWinSvcMain, stopped callback throw exception");
        locker.lock();
		__win_svc_report_status(
            service_status_handle_, SERVICE_STOPPED, ERROR_INVALID_FUNCTION, 0, service_status_);
		return;
	}
        
	if (result) {
        report_event(service_name, event_level_e::error, 
            L"In ServiceRegistrar::__onWinSvcMain, stopped callback returned error");
        locker.lock();
        __win_svc_report_status(
            service_status_handle_, SERVICE_STOPPED, ERROR_INVALID_FUNCTION, 0, service_status_);
		return;
	}

    locker.lock();
	__win_svc_report_status(service_status_handle_, SERVICE_STOPPED, NO_ERROR, 0, service_status_);
    locker.unlock();
}
#endif

#if MG_OS__WIN_AVAIL
inline DWORD service::__on_win_svc_ctrl_handler(
        DWORD _dwCtrl, DWORD _dwEventType, LPVOID _lpEventData)
{
	switch (_dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN: {

        std::unique_lock locker{ mutex_ };
        auto service_name = service_name_;
		__win_svc_report_status(
            service_status_handle_, SERVICE_STOP_PENDING, NO_ERROR, 30000, service_status_);
        locker.unlock();
			// Signal the service to stop.

		is_stopping_ = true;
		try {
            mgpp::err result;
            if (stop_req_fn_)
			    result = stop_req_fn_();
            else {
                report_event(service_name, event_level_e::info, 
                    L"In service::__on_win_svc_ctrl_handler, stop_req callback is not set");
            }
            if (result) {
                report_event(service_name, event_level_e::error, 
                    L"In service::__on_win_svc_ctrl_handler, stop_req callback returned error");
                locker.lock();
                __win_svc_report_status(
                    service_status_handle_, SERVICE_STOPPED, ERROR_INVALID_FUNCTION, 0, service_status_);
                return NO_ERROR;
            }
		}
		catch (...) {
			report_event(service_name, event_level_e::error, 
                L"In service::__on_win_svc_ctrl_handler, stop_req callback throw exception");
            locker.lock();
			__win_svc_report_status(
                service_status_handle_, SERVICE_STOPPED, ERROR_INVALID_FUNCTION, 0, service_status_);
			return NO_ERROR;
		}

        // locker.lock();
		// __win_svc_report_status(
        //     service_status_handle_, SERVICE_STOPPED, NO_ERROR, 0, service_status_);
        // locker.unlock();

		return NO_ERROR;

	}
	case SERVICE_CONTROL_INTERROGATE:
		return NO_ERROR;

	default:
		break;
	}
	return ERROR_CALL_NOT_IMPLEMENTED;
}
#endif

#if MG_OS__WIN_AVAIL
inline LONG service::__on_unhandled_exception_handler(EXCEPTION_POINTERS *_lpExceptionInfo)
{
    if (is_stopping_) {
        // If the service is stopping, we don't want to handle the exception.
        return EXCEPTION_EXECUTE_HANDLER;
    }

     auto dmp_path = 
        mm_into<memepp::native_string>(
            mmupp::fs::relative_with_program_path(
                memepp::c_format(256, "%s.%lld.dmp", 
                    mm_from(service_name_).data(), (mgu_time_t)time(0))));

    HANDLE hFile = CreateFileW(
        dmp_path.data(),
        FILE_SHARE_READ | GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return EXCEPTION_EXECUTE_HANDLER; // Continue searching for another handler
    }
    MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] { CloseHandle(hFile); });

    MINIDUMP_EXCEPTION_INFORMATION mdei;
    memset(&mdei, 0, sizeof(mdei));
    mdei.ThreadId = GetCurrentThreadId();
    mdei.ExceptionPointers = _lpExceptionInfo;
    mdei.ClientPointers = FALSE;

    MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        MiniDumpNormal,
        _lpExceptionInfo ? &mdei : NULL,
        NULL,
        NULL);

    return EXCEPTION_EXECUTE_HANDLER; // Indicate that the exception has been handled
}
#endif 

#if MG_OS__WIN_AVAIL
inline VOID service::__win_svc_report_status(
    SERVICE_STATUS_HANDLE _handle, 
    DWORD _dwCurrentState, DWORD _dwWin32ExitCode, DWORD _dwWaitHint,
    SERVICE_STATUS& _status)
{		
    _status.dwCurrentState  = _dwCurrentState;
	_status.dwWin32ExitCode = _dwWin32ExitCode;
	_status.dwWaitHint      = _dwWaitHint;

	if (_dwCurrentState == SERVICE_START_PENDING)
		_status.dwControlsAccepted = 0;
	else 
		_status.dwControlsAccepted = 
            SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_TIMECHANGE;

	if ((_dwCurrentState == SERVICE_RUNNING) ||
		(_dwCurrentState == SERVICE_STOPPED))
		_status.dwCheckPoint = 0;
	else 
		_status.dwCheckPoint += 1;

	// Report the status of the service to the SCM.
	SetServiceStatus(_handle, &_status);
}
#endif 
        
#if MG_OS__WIN_AVAIL
inline VOID WINAPI service::__win_svc_main(DWORD _dwArgc, LPWSTR *_lpszArgv)
{
    service::instance().__on_win_svc_main(_dwArgc, _lpszArgv);
}
#endif 
    
#if MG_OS__WIN_AVAIL
inline DWORD WINAPI service::__win_svc_ctrl_handler(
        DWORD _dwCtrl, DWORD _dwEventType, LPVOID _lpEventData, LPVOID _lpContext)
{
    return reinterpret_cast<service*>(_lpContext)->
        __on_win_svc_ctrl_handler(_dwCtrl, _dwEventType, _lpEventData);
}
#endif

#if MG_OS__WIN_AVAIL
inline LONG WINAPI service::__unhandled_exception_handler(EXCEPTION_POINTERS *_lpExceptionInfo)
{
    return service::instance().__on_unhandled_exception_handler(_lpExceptionInfo);
}
#endif

#if MG_OS__WIN_AVAIL
inline mgpp::err service::__wait_service_status(
    SC_HANDLE _scService, DWORD _desiredStatus, DWORD64 _timeout)
{
	DWORD64 dwStartTime = GetTickCount64();
    DWORD dwOldCheckPoint = 0;
    SERVICE_STATUS_PROCESS serviceStatus;
    while (true) {
        if (!QueryServiceStatusEx(
            _scService,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&serviceStatus,
            sizeof(serviceStatus),
            NULL))
        {
            return { mgec__from_sys_err(GetLastError()), "QueryServiceStatusEx failed" };
        }

        if (serviceStatus.dwCurrentState == _desiredStatus)
            return {};

        if (serviceStatus.dwCheckPoint > dwOldCheckPoint) {
            dwStartTime = GetTickCount64();
            dwOldCheckPoint = serviceStatus.dwCheckPoint;
        } else if (GetTickCount64() - dwStartTime > _timeout) {
            return { MGEC__TIMEDOUT, "Timeout waiting for service status change" };
        }

        auto dwWaitTime = serviceStatus.dwWaitHint / 10;
        if (dwWaitTime < 100)
            dwWaitTime = 100;
        else if (dwWaitTime > 5000)
            dwWaitTime = 5000;
        Sleep(dwWaitTime);
    }
}
#endif // MG_OS__WIN_AVAIL


#if MG_OS__WIN_AVAIL
inline mgpp::err service::__stop_dependent_services(
    SC_HANDLE _scManager, SC_HANDLE _scService, const process_rate_cb_t& _process_rate_cb)
{
	DWORD dwBytesNeeded;
	DWORD dwCount;
	LPENUM_SERVICE_STATUSW lpDependencies = NULL;

    if (EnumDependentServicesW(
        _scService, SERVICE_ACTIVE, lpDependencies, 0, &dwBytesNeeded, &dwCount))
    {
        // Successfully enumerated dependent services.
        return {};
    }
    else if (GetLastError() != ERROR_MORE_DATA) {
        // An unexpected error occurred.
        return { mgec__from_sys_err(GetLastError()), "EnumDependentServicesW failed" };
    }
    
    // Allocate a buffer for the dependencies.
    lpDependencies = (LPENUM_SERVICE_STATUSW)HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded);
    if (!lpDependencies) {
        return { MGEC__NOMEM, "Memory allocation failed" };
    }
    MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] { HeapFree(GetProcessHeap(), 0, lpDependencies); });

    if (!EnumDependentServicesW(
        _scService, SERVICE_ACTIVE, lpDependencies, dwBytesNeeded, &dwBytesNeeded, &dwCount))
    {
        // Failed to enumerate dependent services.
        return { mgec__from_sys_err(GetLastError()), "EnumDependentServicesW failed" };
    }

	SERVICE_STATUS_PROCESS ssp;
    for (DWORD index = 0; index < dwCount; ++index)
    {
        // Stop each dependent service.
        SC_HANDLE hDependentService = OpenServiceW(
            _scManager, lpDependencies[index].lpServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (!hDependentService) {
            // Failed to open the dependent service.
            continue;
        }
        MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] { CloseServiceHandle(hDependentService); });

        // Stop the dependent service.
        if (!ControlService(
            hDependentService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp)) 
        {
            // Failed to stop the dependent service.
            continue;
        }
        
        if (_process_rate_cb) {
            _process_rate_cb(process_rate_e::wait_for_dependents_stop, index);
        }

        auto err = __wait_service_status(
            hDependentService, SERVICE_STOPPED, 30000); // Wait for dependent service to stop
        if (err) {
            // Handle the error if needed.
            continue;
        }
    }
    return {};
}
#endif // MG_OS__WIN_AVAIL

}
}
}
}

#endif // !MMUPP_UTIL_OS_WIN_SERVICE_H_INCLUDED
