
#ifndef MMUPP_THRD_FUTURE_WATCHER_HPP_INCLUDED
#define MMUPP_THRD_FUTURE_WATCHER_HPP_INCLUDED

#include <mutex>
#include <thread>
#include <future>
#include <chrono>

namespace mmupp { namespace thrd {

// ============================================================
// shared_status — holds progress + cancellation state
// ============================================================
struct shared_status
{
    template <typename T>
    friend struct shared_future_watcher;

    // ---- Progress ----
    double progress_percent() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto range = maximum_progress_ - minimum_progress_;
        if (range == 0)
            return 0.0;
        return (current_progress_ - minimum_progress_) / double(range);
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

    // ---- Cancellation ----
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

    // Returns true on first trigger, false on subsequent
    bool request_cancel() noexcept
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (should_cancel_) return false;
        should_cancel_ = true;
        return true;
    }

private:
    mutable std::mutex mtx_;
    bool should_cancel_    = false;
    int  minimum_progress_ = 0;
    int  maximum_progress_ = 0;
    int  current_progress_ = 0;
};

using shared_status_ptr = std::shared_ptr<shared_status>;

// ============================================================
// cancel_token — read-only handle for worker threads
// ============================================================
class cancel_token
{
public:
    cancel_token() noexcept = default;

    [[nodiscard]] bool is_cancelled() const noexcept
    {
        return status_ && status_->should_cancel();
    }

    [[nodiscard]] bool valid() const noexcept { return status_ != nullptr; }
    explicit operator bool() const noexcept   { return valid(); }

private:
    template <typename T> friend struct shared_future_watcher;
    friend class cancel_source;

    explicit cancel_token(shared_status_ptr s) noexcept
        : status_(std::move(s)) {}

    shared_status_ptr status_;
};

// ============================================================
// cancel_source — write handle for the controller; can request
// cancellation and issue read-only tokens
// ============================================================
class cancel_source
{
public:
    cancel_source(shared_status_ptr s) noexcept
        : status_(std::move(s)) {}

    cancel_source(const cancel_source&) = delete;
    cancel_source& operator=(const cancel_source&) = delete;
    cancel_source(cancel_source&&) noexcept = default;
    cancel_source& operator=(cancel_source&&) noexcept = default;

    // Request cancellation; returns true on first trigger
    bool request_cancel()
    {
        return status_ && status_->request_cancel();
    }

    // Query current cancellation state
    [[nodiscard]] bool is_cancelled() const noexcept
    {
        return status_ && status_->should_cancel();
    }

    // Issue a read-only token to workers
    [[nodiscard]] cancel_token token() const noexcept
    {
        return cancel_token(status_);
    }

private:
    shared_status_ptr status_;
};

// ============================================================
// shared_future_watcher<T>
// ============================================================
template <typename T>
struct shared_future_watcher
{
public:
    shared_future_watcher()
        : status_(std::make_shared<shared_status>())
    {}

    // ---- Future operations ----
    void set_future(const std::shared_future<T>& _f) { f_ = _f; }

    // Block until the future is ready
    void wait() const
    {
        if (f_.valid())
            f_.wait();
    }

    // Timed wait; returns future_status for caller to check ready/timeout
    template <typename Rep, typename Period>
    std::future_status wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) const
    {
        if (!f_.valid())
            return std::future_status::ready;
        return f_.wait_for(timeout_duration);
    }

    // Wait until the given time point
    template <typename Clock, typename Duration>
    std::future_status wait_until(
        const std::chrono::time_point<Clock, Duration>& timeout_time) const
    {
        if (!f_.valid())
            return std::future_status::ready;
        return f_.wait_until(timeout_time);
    }

    // Check whether a future has been associated (i.e. set_future was called)
    bool valid() const noexcept
    {
        return f_.valid();
    }

    // Safe version: validates first, then checks readiness
    bool is_ready() const
    {
        return f_.valid()
            && f_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    // Safe version: validates first (std::future::get() is UB otherwise)
    // Throws std::future_error(std::future_errc::no_state) if future is invalid
    T get_result()
    {
        if (!f_.valid())
            throw std::future_error(std::future_errc::no_state);
        return f_.get();
    }

    // ---- Cancel (privilege separation) ----
    cancel_source  get_cancel_source() { return cancel_source(status_); }
    cancel_token   get_cancel_token() const
    {
        return cancel_token(status_);
    }

    // ---- Backward-compatible shortcuts ----
    void set_cancel()          { status_->request_cancel(); }
    bool is_cancelled() const  { return get_cancel_token().is_cancelled(); }

    // Consistent naming with shared_status::should_cancel
    bool should_cancel() const
    {
        return status_->should_cancel();
    }

    // ---- Progress delegation ----
    double progress_percent() const
    {
        return status_->progress_percent();
    }

    void set_progress_range(int _min, int _max)
    {
        status_->set_progress_range(_min, _max);
    }

    void set_progress(int _progress)
    {
        status_->set_progress(_progress);
    }

    void increment_progress(int _step = 1)
    {
        status_->increment_progress(_step);
    }

    shared_status_ptr get_status() { return status_; }

private:
    std::shared_future<T> f_;
    shared_status_ptr     status_;
};

}}

#endif // !MMUPP_THRD_FUTURE_WATCHER_HPP_INCLUDED
