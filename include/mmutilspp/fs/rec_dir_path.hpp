
#ifndef MMUPP_FS_REC_DIR_PATH_HPP_INCLUDED
#define MMUPP_FS_REC_DIR_PATH_HPP_INCLUDED

#include <mego/util/get_exec_path.h>
#include <mego/fs/dir.h>
#include <mego/util/os/windows/windows_simplify.h>

#if MG_OS__WIN_AVAIL
#  include <shlobj.h>
#  pragma comment(lib, "Shell32.lib")
#  pragma comment(lib, "Ole32.lib")
#endif

#include <memepp/string.hpp>
#include <memepp/string_view.hpp>
#include <memepp/native.hpp>
#include <memepp/variable_buffer.hpp>
#include <memepp/convert/std/string.hpp>
#include <memepp/convert/std/wstring.hpp>

namespace mmupp {
namespace fs {

/**
 * @brief 推荐一个可写的目录路径，用于存储应用程序记录、日志或其他持久化数据。
 *
 * 此函数根据当前操作系统确定一个合适的、可写的目录路径。
 * 它优先考虑可执行文件所在的目录，如果该目录可写，则适合便携式或开发场景。
 * 如果可执行目录不可写（例如，由于系统权限限制），则回退到遵循操作系统约定的用户特定数据目录。
 *
 * @details
 * - **跨平台行为**：
 *   - 在Windows上：使用宽字符路径（wchar_t）以兼容Unicode。首先检查可执行文件的目录。如果不可写，则通过SHGetKnownFolderPath获取LocalAppData文件夹（例如，C:\Users\用户名\AppData\Local），并基于可执行文件的名称和驱动器路径构建子目录。构建的路径通过包含可执行文件的名称（不带.exe后缀）和相对于驱动器的完整路径来确保唯一性（例如，LocalAppData\myapp\C\Path\To\Exe）。
 *   - 在非Windows上（例如，Linux/macOS）：使用窄字符路径。首先检查可执行文件的目录。如果不可写，则在用户主目录下按照类似XDG的约定构建路径（例如，~/.local/share/myapp//path/to/exe），其中"myapp"是可执行文件名，并附加完整可执行路径以确保唯一性。
 *
 * - **优先级和回退机制**：
 *   1. 使用mgu_get_exec_[w_]path检索可执行文件的路径并提取目录部分。
 *   2. 使用mgfs__is_[w_]dir_writable检查此目录是否可写。
 *   3. 如果可写，直接返回该路径。
 *   4. 如果不可写，构建并返回位于用户可写位置的回退路径。
 *
 * - **错误处理**：
 *   - 如果检索可执行路径失败（例如，plen < 0），返回空memepp::string。
 *   - 在Windows上，如果SHGetKnownFolderPath失败或路径构建元素缺失（例如，没有".exe"或驱动器字母），可能返回空字符串或部分路径。
 *   - 不抛出异常；错误通过空返回值来指示。
 *
 * - **依赖**：
 *   - 依赖mego库函数（mgu_get_exec_[w_]path, mgfs__is_[w_]dir_writable）用于路径检索和可写性检查。
 *   - 使用memepp库进行高效字符串处理和转换（例如，从std::wstring转换）。
 *   - 在Windows上，链接到Shell32.lib和Ole32.lib用于SHGetKnownFolderPath和CoTaskMemFree。
 *
 * - **限制**：
 *   - 路径长度受MAX_PATH（Windows上为260）或PATH_MAX（Unix上通常为4096）的限制。
 *   - 假设可执行路径包含标准元素（例如，Windows上的".exe"，驱动器字母）。
 *   - 不创建目录；调用者必须确保目录存在且可写（如需要）。
 *   - 线程安全：此函数是线程安全的，因为它不修改共享状态，但底层系统调用可能有限制。
 *
 * - **用例**：
 *   - 存储应用程序日志：将"/app.log"附加到返回的路径。
 *   - 在持久的、用户特定的位置保存用户配置或缓存。
 *
 * - **示例**：
 * @code
 *   auto log_dir = mmupp::fs::rec_writable_dir_path();
 *   if (!log_dir.empty()) {
 *       // 使用 log_dir + "/mylog.txt" 写入文件
 *   } else {
 *       // 处理错误：未找到可写目录
 *   }
 * @endcode
 *
 *   - 在Windows上（可执行文件：C:\Program Files\MyApp\myapp.exe）：可能返回 "C:\Users\Username\AppData\Local\myapp\C\Program Files\MyApp"
 *   - 在Linux上（可执行文件：/usr/bin/myapp）：可能返回 "/home/username/.local/share/myapp//usr/bin"
 *
 * @return 包含推荐可写目录路径的memepp::string，如果失败则为空字符串。
 *         路径不包含尾部斜杠，除非它是根目录。
 */
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
    buffer.append(memepp::string_view{ "~/.local/share/" });
    buffer.append(memepp::string_view{ exec_path + dir_pos + 1 });
    buffer.append(memepp::string_view{ "/" });
    buffer.append(memepp::string_view{ exec_path });

    memepp::string str;
    buffer.release(str);
    return str;
#endif
}

}
}

#endif // !MMUPP_FS_REC_DIR_PATH_HPP_INCLUDED
