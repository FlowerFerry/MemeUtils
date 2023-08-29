
#ifndef MMUPPAPP_TEXT_FILE_MONITOR_HPP_INCLUDED
#define MMUPPAPP_TEXT_FILE_MONITOR_HPP_INCLUDED

#include <memory>
#include <functional>
#include <mutex>
#include <thread>

#include <memepp/string.hpp>
#include <memepp/string_view.hpp>
#include <memepp/hash/std_hash.hpp>
#include <memepp/convert/std/string.hpp>
#include <uvw/loop.h>
#include <uvw/async.h>
#include "uvw/fs.h"
#include "uvw/fs_event.h"
#include "uvw/fs_poll.h"
#include <ghc/filesystem.hpp>

namespace mmupp::app {

    template<typename _Object>
    struct txtfile_monitor
    {
        using object_t   = _Object;
        using change_cb  = std::function<void(const std::shared_ptr<object_t>&)>;
        using parse_cb   = 
            std::function<int(const memepp::string_view&, std::shared_ptr<object_t>&)>;
        using default_cb = std::function<memepp::string()>;

        txtfile_monitor();
        txtfile_monitor(const txtfile_monitor&) = delete;
        txtfile_monitor& operator=(const txtfile_monitor&) = delete;

        ~txtfile_monitor();

        void set_filepath(const memepp::string& _filepath);
        void set_change_callback(const memepp::string_view& _key, const change_cb& _cb);
        void set_parse_callback(const parse_cb& _cb);
        void set_default_callback(const default_cb& _cb);

        int start(std::shared_ptr<uvw::Loop> _loop);
        int start();
        int stop ();

    private:
        void __on_file_changed(const memepp::string_view& _data);
        void __on_filepath_changed();
        void __on_stop(uvw::AsyncHandle& _handle);
        void __on_fs_event(const uvw::FsEventEvent& _event, uvw::FsEventHandle& _handle);
        
        void __check_exist_and_create(const memepp::string_view& _filepath);
        void __read_file_once();

        std::mutex mutex_;
        std::thread thread_;
        std::shared_ptr<uvw::Loop> loop_;
        bool internal_loop_;
        uint32_t max_file_size_;
        memepp::string filepath_;
        std::shared_ptr<object_t>  curr_object_;
        std::unordered_map<memepp::string_view, std::shared_ptr<change_cb>> change_callbacks_;
        std::shared_ptr<parse_cb>  parse_callback_;
        std::shared_ptr<default_cb> default_callback_;
		std::shared_ptr<uvw::AsyncHandle> async_stop_;
		std::shared_ptr<uvw::FileReq> file_;
		std::shared_ptr<uvw::FsEventHandle> fs_ev_;
    };

    template<typename _Object>
    txtfile_monitor<_Object>::txtfile_monitor():
        internal_loop_(false),
        max_file_size_(1024 * 1024 * 10)
    {
    }

    template<typename _Object>
    txtfile_monitor<_Object>::~txtfile_monitor()
    {
    }

    template<typename _Object>
    void txtfile_monitor<_Object>::set_filepath(const memepp::string& _filepath)
    {
        std::unique_lock<std::mutex> locker(mutex_);
        if (filepath_ == _filepath)
            return;
        filepath_ = _filepath;
        locker.unlock();
        __on_filepath_changed();
    }

    template<typename _Object>
    void txtfile_monitor<_Object>::set_change_callback(const memepp::string_view& _key, const change_cb& _cb)
    {
        std::unique_lock<std::mutex> locker(mutex_);
        change_callbacks_[_key.to_string()] = std::make_shared<change_cb>(_cb);
    }

    template<typename _Object>
    void txtfile_monitor<_Object>::set_parse_callback(const parse_cb& _cb)
    {
        std::unique_lock<std::mutex> locker(mutex_);
        parse_callback_ = std::make_shared<parse_cb>(_cb);
    }

    template<typename _Object>
    void txtfile_monitor<_Object>::set_default_callback(const default_cb& _cb)
    {
        std::unique_lock<std::mutex> locker(mutex_);
        default_callback_ = std::make_shared<default_cb>(_cb);
    }

    template<typename _Object>
    int txtfile_monitor<_Object>::start(std::shared_ptr<uvw::Loop> _loop)
    {
        std::unique_lock<std::mutex> locker(mutex_);
        if (filepath_.empty())
            return -1;

        if (loop_ != nullptr)
            return -1;

        if (_loop == nullptr) {
            loop_ = std::make_shared<uvw::Loop>();
            internal_loop_ = true;
        }
        else {
            loop_ = _loop;
            internal_loop_ = false;
        }

        auto filepath = filepath_;
        locker.unlock();

        auto path = mm_to<std::string>(filepath);
        if (ghc::filesystem::is_directory(path))
            return -1;
        __check_exist_and_create(filepath);

        async_stop_ = loop_->resource<uvw::AsyncHandle>();
        async_stop_->on<uvw::AsyncEvent>([this](const uvw::AsyncEvent&, uvw::AsyncHandle& _handle) 
        {
            __on_stop(_handle);
        });

        fs_ev_ = loop_->resource<uvw::FsEventHandle>();
        fs_ev_->on<uvw::FsEventEvent>([this](const uvw::FsEventEvent& _ev, uvw::FsEventHandle& _h) 
        {
            __on_fs_event(_ev, _h);
        });
        fs_ev_->on<uvw::ErrorEvent>([this](const uvw::ErrorEvent& _ev, uvw::FsEventHandle&) 
        {
        });

		fsEv_->start(path, uvw::FsEventHandle::Event::STAT);

        locker.lock();
        if (internal_loop_) {
            thread_ = std::thread([this]() 
            {
                loop_->run();
            });
        }
        
        __read_file_once();
        return 0;
    }

    template<typename _Object>
    int txtfile_monitor<_Object>::start()
    {
        return start(nullptr);
    }

    template<typename _Object>
    int txtfile_monitor<_Object>::stop()
    {
        std::unique_lock<std::mutex> locker(mutex_);
        if (loop_ == nullptr)
            return -1;

        if (async_stop_ != nullptr) {
            async_stop_->send();
        }

        if (internal_loop_) {
            locker.unlock();
            thread_.join();
        }
        loop_ = nullptr;
        return 0;
    }

    template<typename _Object>
    void txtfile_monitor<_Object>::__on_file_changed(const memepp::string& _data)
    {
        std::unique_lock<std::mutex> locker(mutex_);
        if (parse_callback_ == nullptr)
            return;
        auto parse_cb = parse_callback_;
        locker.unlock();
        auto object = std::make_shared<object_t>();
        if (!(*parse_cb)(_data, *object))
            return;
        locker.lock();
        curr_object_ = object;
        auto change_callbacks = change_callbacks_;
        locker.unlock();
        for (auto& it : change_callbacks) {
            if (it.second == nullptr)
                continue;
            (*it.second)(object);
        }
    }
    
    template<typename _Object>
    void txtfile_monitor<_Object>::__on_filepath_changed()
    {
        
    }

    template<typename _Object>
    void txtfile_monitor<_Object>::__on_stop(uvw::AsyncHandle& _handle)
    {
        std::lock_guard<std::mutex> locker(mutex_);
        if (loop_ == nullptr)
            return;

        if (fs_ev_ != nullptr) {
            fs_ev_->stop();
            fs_ev_->close();
            fs_ev_.reset();
        }

        if (file_ != nullptr) {
            file_->close();
            file_.reset();
        }

        if (async_stop_ != nullptr) {
            async_stop_->close();
            async_stop_.reset();
        }

    }

    template<typename _Object>
    void txtfile_monitor<_Object>::__on_fs_event(const uvw::FsEventEvent& _event, uvw::FsEventHandle& _handle)
    {
        auto path = _handle.path();

        if (_event.flags & uvw::Flags<uvw::FsEventHandle::Event>(uvw::FsEventHandle::Event::STAT))
        {
            if (file_) {
                return;
            }

            file_ = _handle.loop().resource<uvw::FileReq>();
            file_->on<uvw::ErrorEvent>([this](const uvw::ErrorEvent& _ev, uvw::FileReq&) 
            {
				file_->close();
				file_.reset();
            });

            file_->on< uvw::FsEvent<uvw::FileReq::Type::OPEN> >([this](const uvw::FsEvent<uvw::FileReq::Type::OPEN>&, uvw::FileReq& _handle) 
            {
				_handle.stat();
            });

            file_->on< uvw::FsEvent<uvw::FileReq::Type::FSTAT> >([this](const uvw::FsEvent<uvw::FileReq::Type::STAT>& _ev, uvw::FileReq& _handle) 
            {
                auto size = _ev.stat.st_size;
                if (size >= 0 && size < max_file_size_) {
                    _handle.read(0, (uint32_t)bSzie);
                }
                else {
                    _handle.close();
                    file_.reset();
                }
            });

            file_->on< uvw::FsEvent<uvw::FileReq::Type::READ> >([this](const uvw::FsEvent<uvw::FileReq::Type::READ>& _ev, uvw::FileReq& _handle) 
            {
                auto data = mm_view(_ev.data.get(), _ev.size);
                __on_file_changed(data);

                _handle.close();
                file_.reset();
            });

			file_->open(path, uvw::FileReq::FileOpen::RDONLY, 0);
        }
    }

    template<typename _Object>
    void txtfile_monitor<_Object>::__check_exist_and_create(const memepp::string_view& _filepath)
    {
        auto filepath = mm_to<std::string>(_filepath);
        if (!ghc::filesystem::exists(filepath)) {
            std::unique_lock<std::mutex> locker(mutex_);
            auto default_cb = default_callback_;
            locker.unlock();
            if (default_cb != nullptr) {
                auto default_str = default_cb();
                std::ofstream ofs(filepath);
                ofs << default_str;
                ofs.close();
            }
        }
    }
    
};

#endif // !MMUPPAPP_TEXT_FILE_MONITOR_HPP_INCLUDED
