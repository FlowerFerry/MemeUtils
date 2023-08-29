
#ifndef MMUPP_TRANSLATION_MAPPER_HPP_INCLUDED
#define MMUPP_TRANSLATION_MAPPER_HPP_INCLUDED

//! \file translation_mapper.hpp
//! 
//! 该类用于将一个字符串映射到另一个字符串，
//! 例如将一个英文字符串映射到一个中文字符串。
//! 基本原理是 根据国际标识码找到对应的表，
//! 然后根据表中的映射关系进行查找，如果找不到则到对应Sqlite数据库中查找，还是找不到则返回原字符串。

#include <shared_mutex>
#include <unordered_map>
#include <memory>
#include <tuple>

#include <stdio.h>

#include <memepp/string.hpp>
#include <memepp/convert/fmt.hpp>
#include <memepp/hash/std_hash.hpp>
#include <megopp/util/scope_cleanup.h>
#include <memepp/convert/fmt.hpp>
#include <mmutilspp/paths/program_path.hpp>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>
#include <fmt/format.h>

namespace mmupp {
namespace app {

    struct translation_loader
    {
        using table_t = std::unordered_map<memepp::string_view, memepp::string>;

        virtual ~translation_loader() = default;

        virtual std::tuple<int, memepp::string> load(const memepp::string& _code, const memepp::string& _path) = 0;
        virtual memepp::string try_get(const memepp::string_view& _origin_text) const = 0;
        virtual std::shared_ptr<table_t> get_table() const = 0;
    };

    //! {
    //!     "code": "zh-CN",
    //!     "name": "简体中文",
    //!     "version": "1.0.0",
    //!     "table": [
    //!         {
    //!             "origin": "hello",
    //!             "trans": "你好"
    //!         },
    //!         {
    //!            "origin": "world",
    //!            "trans" : "世界"
    //!         }
    //!     ]
    //! }
    struct json_translation_loader : public translation_loader
    {
        virtual ~json_translation_loader() = default;

        std::tuple<int, memepp::string> load(const memepp::string& _code, const memepp::string& _path);
        memepp::string try_get(const memepp::string_view& _origin_text) const;
        std::shared_ptr<table_t> get_table() const;

    protected:
        memepp::string code_;
        memepp::string path_;
        std::shared_ptr<table_t> table_;
    };

    struct translation_mapper
    {
        using code_t = memepp::string;
        using origin_text_view_t = memepp::string_view;
        using trans_text_t = memepp::string;
        using table_t = std::unordered_map<origin_text_view_t, trans_text_t>;

        void set_origin_code(const code_t& _code);
        void set_default_code(const code_t& _code);
        void set_translate_directory(
            const memepp::string& _library_type, const memepp::string& _path);
        void init_default_table();

        memepp::string_view translate(
            code_t _dst_code,
            code_t _src_code,
            const memepp::string_view&  _context,
            const origin_text_view_t&   _origin_text);

        memepp::string_view translate(
            code_t _dst_code,
            const memepp::string_view&  _context,
            const origin_text_view_t&   _origin_text);

        memepp::string_view translate(
            const memepp::string_view&  _context,
            const origin_text_view_t&   _origin_text);
        
        static translation_mapper& instance();

    private:
        mutable std::shared_mutex smtx_;
        code_t origin_code_;
        code_t default_code_;
        memepp::string library_type_;
        memepp::string library_path_;
        std::unordered_map<code_t, std::shared_ptr<translation_loader>> loader_group_;
        std::unordered_map<code_t, std::shared_ptr<table_t>> table_group_;
    };

    inline void translation_mapper::set_origin_code(const code_t& _code)
    {
        std::unique_lock<std::shared_mutex> locker(smtx_);
        origin_code_ = _code;
    }

    inline void translation_mapper::set_default_code(const code_t& _code)
    {
        std::unique_lock<std::shared_mutex> locker(smtx_);
        default_code_ = _code;
    }

    inline void translation_mapper::set_translate_directory(
        const memepp::string& _library_type, const memepp::string& _path)
    {
        std::unique_lock<std::shared_mutex> locker(smtx_);
        library_type_ = _library_type;
        library_path_ = mmupp::paths::relative_with_program_path(_path);
    }

    inline void translation_mapper::init_default_table()
    {
        auto loader = std::make_shared<json_translation_loader>();
        auto [code, msg] = loader->load(default_code_, library_path_);
        if (code != 0) {
            // TO_OD
            return;
        }

        std::unique_lock<std::shared_mutex> locker(smtx_);
        loader_group_.insert({ default_code_, loader });
        auto table = loader->get_table();
        if (table)
            table_group_.insert({ default_code_, table });
    }

    inline memepp::string_view translation_mapper::translate(
        code_t _dst_code,
        code_t _src_code,
        const memepp::string_view&  _context,
        const origin_text_view_t&   _origin_text)
    {
        std::shared_lock<std::shared_mutex> slocker(smtx_);
        if (_dst_code.empty()) {
            _dst_code = default_code_;
        }
        
        if (_src_code.empty()) {
            _src_code = origin_code_;
        }

        if (_dst_code == _src_code) {
            slocker.unlock();
            return _origin_text;
        }

        do {
            auto g_it = table_group_.find(_dst_code);
            if (g_it != table_group_.end())
            {
                auto table = g_it->second;
                slocker.unlock();
                auto it = table->find(_origin_text);
                if (it == table->end()) {
                    break;
                }
                return it->second;
            }

            auto l_it = loader_group_.find(_dst_code);
            if (l_it != loader_group_.end()) {
                auto loader = l_it->second;
                slocker.unlock();

                auto table = loader->get_table();
                if (!table) {
                    break;
                }

                std::unique_lock<std::shared_mutex> ulocker(smtx_);
                table_group_.insert({ _dst_code, table });
                ulocker.unlock();

                auto it = table->find(_origin_text);
                if (it == table->end()) {
                    break;
                }
                return it->second;
            }
            
            slocker.unlock();
            auto loader = std::make_shared<json_translation_loader>();
            auto [code, msg] = loader->load(_dst_code, library_path_);
            if (code != 0) {
                break;
            }
            auto table = loader->get_table();
            if (!table) {
                break;
            }
            std::unique_lock<std::shared_mutex> ulocker(smtx_);
            loader_group_.insert({ _dst_code, loader });
            table_group_.insert({ _dst_code, table });
            ulocker.unlock();

            auto it = table->find(_origin_text);
            if (it == table->end()) {
                break;
            }
            return it->second;
        } while (0);
        
        if (slocker.owns_lock())
            slocker.unlock();
        return _origin_text;
    }

    inline memepp::string_view translation_mapper::translate(
        const memepp::string_view&  _context,
        const origin_text_view_t&   _origin_text)
    {
        std::shared_lock<std::shared_mutex> slocker(smtx_);
        auto dst_code = default_code_;
        auto src_code = origin_code_;
        slocker.unlock();
        return translate(dst_code, src_code, _context, _origin_text);
    }
    
    inline memepp::string_view translation_mapper::translate(
        code_t _dst_code,
        const memepp::string_view&  _context,
        const origin_text_view_t&   _origin_text)
    {
        std::shared_lock<std::shared_mutex> slocker(smtx_);
        auto src_code = origin_code_;
        slocker.unlock();
        return translate(_dst_code, src_code, _context, _origin_text);
    }
    
    inline translation_mapper& translation_mapper::instance()
    {
        static translation_mapper m;
        return m;
    }

    inline std::tuple<int, memepp::string> json_translation_loader::load(const memepp::string& _code, const memepp::string& _path)
    {
        code_ = _code;
        path_ = _path;

        auto path = fmt::format("{}/{}.json", path_, code_);
        FILE* fp = fopen(path.data(), "rb");
        if (!fp) {
            return std::make_tuple(-1, mm_from(fmt::format("open file(path:{}) failed", path)));
        }
        auto fileCleanup = megopp::util::scope_cleanup__create([fp] {
            fclose(fp);
        });

        fseek(fp, 0, SEEK_END);
        auto fileLen = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        std::unique_ptr<char[]> fileContent(new char[fileLen + 1]);
        uint8_t buffer[1024];
        for (int index = 0; index < fileLen; index += 1024) {
            auto readLen = fread(buffer, 1, 1024, fp);
            if (readLen <= 0) {
                return std::make_tuple(-1, mm_from(fmt::format("read file(path:{}) failed", path)));
            }
            memcpy(fileContent.get() + index, buffer, readLen);
        }
        fileContent[fileLen] = '\0';

        rapidjson::Document doc;
        if (doc.Parse(fileContent.get()).HasParseError()) {
            return std::make_tuple(-1, mm_from(fmt::format("parse file(path:{}) failed", path)));
        }
        
        auto table_jvalue = rapidjson::Pointer("/table").Get(doc);
        if (!table_jvalue) {
            return std::make_tuple(-1, mm_from(fmt::format("parse file(path:{}) failed, no table field", path)));
        }
        if (!table_jvalue->IsArray()) {
            return std::make_tuple(-1, mm_from(fmt::format("parse file(path:{}) failed, table field is not array", path)));
        }

        auto table = std::make_shared<table_t>();
        for (auto item = table_jvalue->Begin(); item != table_jvalue->End(); ++item)
        {
            if (!item->IsObject()) {
                continue;
            }

            auto origin_jval = rapidjson::Pointer("/origin").Get(*item);
            if (!origin_jval || !origin_jval->IsString()) {
                continue;
            }

            auto trans_jval = rapidjson::Pointer("/trans").Get(*item);
            if (!trans_jval || !trans_jval->IsString()) {
                continue;
            }

            table->insert({
                memepp::string { origin_jval->GetString(), origin_jval->GetStringLength(), memepp::string_storage_t::large }, 
                memepp::string { trans_jval->GetString(), trans_jval->GetStringLength() } 
            });
        }
        table_ = table;

        return std::make_tuple(0, memepp::string{});
    }
    
    inline memepp::string json_translation_loader::try_get(const memepp::string_view& _origin_text) const
    {
        if (!table_) {
            return _origin_text.to_string();
        }
        auto it = table_->find(_origin_text);
        if (it == table_->end()) {
            return _origin_text.to_string();
        }
        return it->second;
    }

    inline std::shared_ptr<translation_loader::table_t> json_translation_loader::get_table() const
    {
        return table_;
    }
};
};

inline memepp::string_view mmu_tr(
    const memepp::string_view& _context,
    const memepp::string_view& _origin_text)
{
    return mmupp::app::translation_mapper::instance().translate(
        _context,
        _origin_text);
}

inline memepp::string_view mmu_tr1(
    const memepp::string& _code,
    const memepp::string_view& _context,
    const memepp::string_view& _origin_text)
{
    return mmupp::app::translation_mapper::instance().translate(
        _code,
        _context,
        _origin_text);
}

inline memepp::string_view mmu_tr2(
    const memepp::string& _dst_code,
    const memepp::string& _src_code,
    const memepp::string_view& _context,
    const memepp::string_view& _origin_text)
{
    return mmupp::app::translation_mapper::instance().translate(
        _dst_code,
        _src_code,
        _context,
        _origin_text);
}

#endif // !MMUPP_TRANSLATION_MAPPER_HPP_INCLUDED
