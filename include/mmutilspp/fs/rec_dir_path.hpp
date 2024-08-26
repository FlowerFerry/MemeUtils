
#ifndef MMUPP_FS_REC_LOG_DIR_PATH_HPP_INCLUDED
#define MMUPP_FS_REC_LOG_DIR_PATH_HPP_INCLUDED

#include <mego/util/get_exec_path.h>
#include <mego/fs/dir.h>
#include <mego/util/os/windows/windows_simplify.h>

#if MG_OS__WIN_AVAIL
#  include <shlobj.h>
#  pragma comment(lib, "Shell32.lib")
#  pragma comment(lib, "Ole32.lib")
#endif

#include <memepp/string.hpp>
#include <memepp/native.hpp>
#include <memepp/variable_buffer.hpp>
#include <memepp/convert/std/string.hpp>
#include <memepp/convert/std/wstring.hpp>

namespace mmupp {
namespace fs {

inline memepp::string rec_writable_dir_path()
{
#if MG_OS__WIN_AVAIL
    wchar_t exec_path[MAX_PATH];
    int dir_pos = -1;
    int plen = mgu_get_exec_w_path(exec_path, MAX_PATH, &dir_pos);
    if (plen < 0)
        return {};
    exec_path[dir_pos] = L'\0';
    if (mgfs__is_w_dir_writable(exec_path, dir_pos))
    {
        std::wstring wpath{ exec_path, static_cast<size_t>(dir_pos) };
        return memepp::from(wpath);
    }

    memepp::string str;
    PWSTR local_path = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &local_path);
    while (SUCCEEDED(hr)) {
        std::wstring wpath{ local_path };
        auto name_pos = wcsstr(exec_path + dir_pos + 1, L".exe");
        if (!name_pos) {
            break;
        }

        auto letter_pos = wcsstr(exec_path, L":\\");
        if (!letter_pos) {
            break;
        }

        *name_pos = L'\0';
        wpath += L"\\";
        wpath += (exec_path + dir_pos + 1);
        wpath += L"\\";
        wpath += letter_pos + 2;

        str = memepp::from(wpath);
        break;
    }

    if (local_path)
        CoTaskMemFree(local_path);

    return str;
#else
    char exec_path[PATH_MAX];
    int dir_pos = -1;
    int plen = mgu_get_exec_path(exec_path, PATH_MAX, &dir_pos);
    if (plen < 0)
        return {};
    
    exec_path[dir_pos] = '\0';
    if (mgfs__is_dir_writable(exec_path, dir_pos))
        return memepp::string{ exec_path, dir_pos };
    
    memepp::variable_buffer buffer;
    buffer.append("~/.local/share/");
    buffer.append(exec_path + dir_pos + 1);
    buffer.append("/");
    buffer.append(exec_path);

    memepp::string str;
    buffer.release(str);
    return str;
#endif
}

}
}

#endif // !MMUPP_FS_REC_LOG_DIR_PATH_HPP_INCLUDED
