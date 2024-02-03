
#ifndef MMUPP_FS_RELATIVE_EXEC_PATH_HPP_INCLUDED
#define MMUPP_FS_RELATIVE_EXEC_PATH_HPP_INCLUDED

#include <meme/native.h>
#include <mego/predef/os/linux.h>
#include <mego/predef/os/windows.h>
#include <mego/util/get_exec_path.h>
#include <memepp/string.hpp>
#include <memepp/variable_buffer.hpp>

namespace mmupp {
namespace fs {

    inline memepp::string program_directory_path()
    {
        int pos = 0;
        int len = MegoUtil_GetExecutablePath(NULL, 0, NULL);
        if (len <= 0)
        {
            return {};
        }
        memepp::variable_buffer vb{ len, 0 };
        len = MegoUtil_GetExecutablePath(
            reinterpret_cast<char*>(vb.data()), len, &pos);
        if (len <= 0)
        {
            return {};
        }

        return memepp::string{ vb.data(), pos };
    }

    inline memepp::string program_file_path()
    {
        int pos = 0;
        int len = MegoUtil_GetExecutablePath(NULL, 0, NULL);
        if (len <= 0)
        {
            return {};
        }
        memepp::variable_buffer vb{ len, 0 };
        len = MegoUtil_GetExecutablePath(
            reinterpret_cast<char*>(vb.data()), len, &pos);
        if (len <= 0)
        {
            return {};
        }

        return memepp::string{ vb.data(), len };
    }

    inline memepp::string relative_with_program_path(const memepp::string_view& _path)
    {
        auto path = _path.trim_space();
        if (path.empty())
            return {};
        
        if (path.starts_with("/"))
            return path.to_string();

#if MG_OS__WIN_AVAIL
        if (path.starts_with("\\"))
            return path.to_string();

        auto pos = path.find(":/");
        if (pos != memepp::string_view::npos)
            return path.to_string();
        
        pos = path.find(":\\");
        if (pos != memepp::string_view::npos)
            return path.to_string();
#endif

        auto dir = program_directory_path();
        
        memepp::variable_buffer vb{ dir.bytes(), dir.size() };
        vb.push_back(uint8_t(MMN_PATH_SEP_CH));
        vb.append(path);

        memepp::string s;
        vb.release(s);
        return s;
    }
} // namespace fs
} // namespace mmupp


#endif // MMUPP_FS_RELATIVE_EXEC_PATH_HPP_INCLUDED
