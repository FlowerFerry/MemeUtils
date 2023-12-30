
#ifndef MMUPP_THRD_FUTURE_WATCHER_HPP_INCLUDED
#define MMUPP_THRD_FUTURE_WATCHER_HPP_INCLUDED

#include <mutex>
#include <thread>
#include <future>
#include <chrono>

namespace mmupp { namespace thrd {

struct shared_status
{
    friend struct shared_future_watcher;

    double progress_percent() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto range = maximum_progress_ - minimum_progress_;
        if (range == 0)
            return 0.0;
        return current_progress_ - minimum_progress_ / double(range);
    }

    bool should_cancel() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return should_cancel_;
    }

    void set_cancel()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        should_cancel_ = true;
    }

    void set_progress_range(int _min, int _max)
    {
        if (_min > _max)
            std::swap(_min, _max);
        std::lock_guard<std::mutex> lock(mtx_);
        minimum_progress_ = _min;
        maximum_progress_ = _max;
    }

    void set_progress(int _progress)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        current_progress_ = _progress;
    }

    void increment_progress(int _step = 1)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        current_progress_ += _step;
    }

private:

    mutable std::mutex mtx_;
    bool should_cancel_ = false;
    int minimum_progress_ = 0;
    int maximum_progress_ = 0;
    int current_progress_ = 0;
};
using shared_status_ptr = std::shared_ptr<shared_status>;

template <typename T>
struct shared_future_watcher
{
public:
    shared_future_watcher() 
        : status_(std::make_shared<shared_status>())
    {}

    void set_future(const std::shared_future<T>& _f)
    {
        f_ = _f;
    }

    bool is_ready() const
    {
        return f_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    bool is_cancelled() const
    {
        return status_->should_cancel();
    }

    void set_cancel()
    {
        status_->set_cancel();
    }

    shared_status_ptr get_status()
    {
        return status_;
    }

    T get_result()
    {
        return f_.get();
    }

private:
    std::shared_future<T> f_;
    shared_status_ptr status_;
};

}} 

#endif // !MMUPP_THRD_FUTURE_WATCHER_HPP_INCLUDED
