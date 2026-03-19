
#ifndef MMUTILSPP_THRD_DISPATCHER_HPP_INCLUDED
#define MMUTILSPP_THRD_DISPATCHER_HPP_INCLUDED

#include <thread>
#include <atomic>
#include <functional>
#include <type_traits>
#include <condition_variable>

#include <concurrentqueue.h>

namespace mmupp {
namespace thrd  {

template <typename _Ty>
struct dispatcher
{
public:
    using callback_t    = std::function<void(_Ty&&)>;
    using concurrency_t = size_t;

    dispatcher(concurrency_t _thrd_count, const callback_t& _callback)
        : workers_running_(false)
        , waiting_(false)
        , paused_(false)
        , threads_count_(determine_thread_count(_thrd_count))
        , tasks_running_(0)
        , callback_(_callback)
    {
        threads_ = std::make_unique<std::thread[]>(threads_count_);
        create_threads();
    }

    ~dispatcher()
    {
        wait_for_tasks ();
        destroy_threads();
    }

    inline void submit(const _Ty& item)
    {
        queue_.enqueue(item);
        task_avail_cv_.notify_one();
    }

    inline void submit(_Ty&& item)
    {
        queue_.enqueue(std::forward<_Ty>(item));
        task_avail_cv_.notify_one();
    }

    inline void reset(concurrency_t _thread_count = 0)
    {
        std::unique_lock locker(mutex_);
        bool was_paused = paused_;
        paused_ = true;
        locker.unlock();
        wait_for_tasks();
        destroy_threads();
        threads_count_ = determine_thread_count(_thread_count);
        threads_ = std::make_unique<std::thread[]>(threads_count_);
        
        paused_ = was_paused;
        create_threads();
    }

    inline void wait_for_tasks()
    {
        std::unique_lock locker(mutex_);
        waiting_ = true;
        task_done_cv_.wait(locker, 
            [this] { return !tasks_running_ && (paused_ || queue_.size_approx() == 0); });
        waiting_ = false;
    }

    inline static concurrency_t determine_thread_count(concurrency_t _thread_count)
    {        
        if (_thread_count > 0)
            return _thread_count;
        else
        {
            if (std::thread::hardware_concurrency() > 0)
                return std::thread::hardware_concurrency();
            else
                return 1;
        }
    }
private:

    inline void create_threads()
    {
        std::unique_lock locker(mutex_);
        workers_running_ = true;
        locker.unlock();
        for (size_t i = 0; i < threads_count_; ++i) {
            threads_[i] = std::thread([this] { worker(); });
        }
    }

    inline void destroy_threads()
    {
        std::unique_lock locker(mutex_);
        workers_running_ = false;
        locker.unlock();
        task_avail_cv_.notify_all();
        for (size_t i = 0; i < threads_count_; ++i) {
            threads_[i].join();
        }
    }

    inline void worker()
    {
        _Ty item;
        while (true) {
            std::unique_lock locker(mutex_);
            task_avail_cv_.wait(locker, [this] {
                return !workers_running_ || (!paused_ && queue_.size_approx() > 0);
            });
            if (!workers_running_) {
                break;
            }
            if (paused_) {
                continue;
            }
            locker.unlock();

            if (queue_.try_dequeue(item)) {
                ++tasks_running_;
                try {
                    callback_(std::move(item));
                }
                catch (...) {
                    --tasks_running_;
                    throw;
                }
                --tasks_running_;
            } 

            locker.lock();
            if (waiting_ && !tasks_running_ && (paused_ || queue_.size_approx() == 0))
                task_done_cv_.notify_all();
        }
    }

    std::condition_variable task_avail_cv_;
    std::condition_variable task_done_cv_;

    std::mutex mutex_;
    bool workers_running_;
    bool waiting_;
    bool paused_;
    concurrency_t threads_count_;
    std::unique_ptr<std::thread[]> threads_;

    std::atomic<size_t> tasks_running_;
    callback_t callback_;
    moodycamel::ConcurrentQueue<_Ty> queue_;
};

}
}

#endif // !MMUTILSPP_THRD_DISPATCHER_HPP_INCLUDED
