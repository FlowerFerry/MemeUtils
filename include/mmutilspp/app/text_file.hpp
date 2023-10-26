
#ifndef MMUPP_APP_TXTFILE_HPP_INCLUDED
#define MMUPP_APP_TXTFILE_HPP_INCLUDED

#include <mego/predef/os/linux.h>

#if MEGO_OS__LINUX__AVAILABLE
#   include <unistd.h>
#   include <fcntl.h>
#endif

#include <megopp/util/scope_cleanup.h>
#include <megopp/err/err.hpp>
#include <memepp/string.hpp>
#include <memepp/convert/std/string.hpp>
#include <ghc/filesystem.hpp>
#include <outcome/result.hpp>

#include <fstream>

namespace outcome = OUTCOME_V2_NAMESPACE;

namespace mmupp::app {

    template<typename _Object>
    class txtfile_handler
    {
        using object_t = _Object;
        using object_ptr_t = std::shared_ptr<object_t>;

        inline object_ptr_t default_object()
        {
            return std::make_shared<object_t>();
        }

        inline outcome::checked<object_ptr_t, mgpp::err> deserializer(const memepp::string& _str)
        {
            return outcome::failure(mgpp::err{ MGEC__ERR });
        }

        inline outcome::checked<memepp::string, mgpp::err> serializer(const object_t& _obj)
        {
            return outcome::failure(mgpp::err{ MGEC__ERR });
        }
    };

    template<typename _Object>
    class txtfile
    {
    public:
        using object_t = _Object;
        using object_ptr_t = std::shared_ptr<object_t>;
        
        txtfile():
            utf8_bom_(true),
            auto_create_(true),
            max_file_size_(10 * 1024 * 1024)
        {}
        
        virtual ~txtfile()
        {}
        
        inline void set_path(const ghc::filesystem::path& _path)
        {
            path_ = _path;
        }

        inline void set_utf8_bom_flag(bool _check)
        {
            utf8_bom_ = _check;
        }

        inline void set_auto_create_flag(bool _flag)
        {
            auto_create_ = _flag;
        }

        inline outcome::checked<object_ptr_t, mgpp::err> load();
        inline mgpp::err save(const object_t& _obj);

        inline static txtfile& instance()
        {
            static txtfile inst;
            return inst;
        }

    protected:
        ghc::filesystem::path path_;
        bool utf8_bom_;
        bool auto_create_;
        int  max_file_size_;
    };
    
    template<typename _Object>
    inline outcome::checked<typename txtfile<_Object>::object_ptr_t, mgpp::err> txtfile<_Object>::load()
    {
        if (path_.empty()) {
            return outcome::failure(mgpp::err{ MGEC__ERR });
        }

        auto hdr = txtfile_handler<_Object>{};
        if (!ghc::filesystem::exists(path_))
        {
            if (auto_create_) {
                auto obj = hdr.default_object();
                
                auto res = hdr.serializer(obj);
                if (!res)
                    return res.error();

                auto err = save(res.value());
                if (!err)
                    return outcome::failure(err);
                
                return obj;
            }
            else {
                return outcome::failure(mgpp::err{ MGEC__ERR });
            }
        }

        auto file_path = path_.native();
        std::ifstream ifs(file_path, std::ios::binary);
        if (!ifs.is_open()) {

            if (auto_create_) {
                auto obj = hdr.default_object();

                auto res = hdr.serializer(obj);
                if (!res)
                    return res.error();

                auto err = save(res.value());
                if (!err)
                    return outcome::failure(err);

                return obj;
            }
            else {
                return outcome::failure(mgpp::err{ MGEC__ERR });
            }
        }

        ifs.seekg(0, std::ios::end);
        auto size = static_cast<std::size_t>(ifs.tellg());
        ifs.seekg(0, std::ios::beg);

        if (size > max_file_size_)
            return outcome::failure(mgpp::err{ MGEC__ERR });

        if (utf8_bom_) {
            char bom[3] = { 0 };
            ifs.read(bom, 3);

            if (bom[0] != '\xEF' || bom[1] != '\xBB' || bom[2] != '\xBF')
            {
                ifs.seekg(0, std::ios::beg);
            }
            
        }

        memepp::string str = mm_from(
            std::string{ std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() });
        
        return hdr.deserializer(str);
    }
    
    template<typename _Object>
    inline mgpp::err txtfile<_Object>::save(const object_t& _obj)
    {
        auto file_path = path_.native();
        std::ofstream ofs{ file_path, std::ios::trunc | std::ios::binary };
        if (!ofs.is_open()) {
            return mgpp::err{ MGEC__ERR };
        }

        auto res = txtfile_handler<_Object>{}.serializer(_obj);
        if (!res) {
            return res.error();
        }

        if (utf8_bom_) {
            char bom[] = { 0xEF, 0xBB, 0xBF, '\0' };
            ofs.write(bom, strlen(bom));
        }

        ofs.write(res.value().data(), res.value().size());
        if (!ofs.good()) {
            return mgpp::err{ MGEC__ERR };
        }

        ofs.close();

#if MEGO_OS__LINUX__AVAILABLE
        auto fd = open(file_path.data(), O_RDONLY);
        if (fd == -1)
            return mgpp::err{ MGEC__ERR };
        MEGOPP_UTIL__ON_SCOPE_CLEANUP([&fd] { close(fd); });

        fsync(fd);
#endif
        return mgpp::err{ };
    }
};

#endif // !MMUPP_APP_TXTFILE_HPP_INCLUDED
