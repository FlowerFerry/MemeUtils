
#ifndef MMUPP_CHRONO_PASSIVE_TIMER_HPP_INCLUDED
#define MMUPP_CHRONO_PASSIVE_TIMER_HPP_INCLUDED

#include <mego/util/std/time.h>

#include <thread>
#include <memory>
#include <vector>
#include <set>

#include <megopp/util/scope_cleanup.h>

namespace mmupp  {
namespace chrono {


	class ticker;

	class passive_timer
	{
		friend class ticker;
	public:
        typedef bool callback_t(passive_timer*, void*);

		passive_timer() noexcept :
			//ticker_(nullptr),
			isStart_(false),
			isOnce_(false),
			interval_(0),
			count_(0)
		{}

		passive_timer(const std::weak_ptr<ticker>& _ticker) noexcept :
			ticker_(_ticker),
			isStart_(false),
			isOnce_(false),
			interval_(0),
			count_(0)
		{}

		virtual ~passive_timer();

        inline void set_ticker(const std::weak_ptr<ticker>& _ticker) noexcept { ticker_ = _ticker; }

		inline constexpr void on(callback_t* _cb, void* _u) noexcept
		{
			cb_ = _cb; userdata_ = _u;
		};

        inline constexpr size_t interval() const noexcept { return interval_; }
		
		inline constexpr void set_interval(size_t _ms) noexcept { interval_ = _ms; }

		inline void start(mgu_timestamp_t _curr) noexcept;

		inline void start_once(mgu_timestamp_t _curr) noexcept;

		inline constexpr bool is_start() const noexcept { return isStart_; }
		
        inline constexpr bool is_once() const noexcept { return isOnce_; }

		inline void cancel() noexcept;

		inline void restart(mgu_timestamp_t _curr) noexcept;
		
		inline bool timing_notcall(mgu_timestamp_t _curr)
		{
			if (!interval_ || !isStart_)
				return true;
			if (_curr < lastTs_) {
				lastTs_ = _curr;
				return true;
			}
			count_ += (_curr - lastTs_);
            lastTs_ = _curr;
			return false;
		}
			
		inline bool timing(mgu_timestamp_t _curr, bool* _isDie = nullptr)
		{
			if (timing_notcall(_curr))
				return false;
			
			if (count_ >= interval_)
			{
				count_ = 0;
				if (isOnce_) {
					isOnce_  = false;
					isStart_ = false;
				}
				if (cb_) {
					bool isAlive = cb_(this, userdata_);
					if (_isDie)
						*_isDie = !isAlive;
				}
				return true;
			}

			return false;
		}

		inline intptr_t due_in() const noexcept
		{
			if (is_start())
				return interval_ - count_;
			return INTPTR_MAX;
		}

	private:
		
		std::weak_ptr<ticker> ticker_;
		bool isStart_;
		bool isOnce_;
		intptr_t interval_;
		intptr_t count_;
		mgu_timestamp_t lastTs_;
		
        void* userdata_;
        callback_t* cb_;
	};

	class ticker
	{
	public:
		ticker():
            locked_(false)
        {}
		
		inline bool locked() const noexcept { return locked_; }

		void accept(
				passive_timer* _timer, mgu_timestamp_t _curr) noexcept;

		void remove(passive_timer* _timer) noexcept 
		{
            if (locked_) {
                wait_removes_.insert(_timer);
                return;
            }
			
			for (auto it = timers_.begin(); it != timers_.end(); ++it)
			{
				if (*it == _timer) {
					timers_.erase(it);
					break;
				}
			}
        }
		
		inline bool wheel_timing(mgu_timestamp_t _curr);

	private:

		inline bool
			remove_and_iteration(std::vector<passive_timer*>::iterator& _it)
		{
			auto rit = wait_removes_.find(*_it);
			if (rit != wait_removes_.end())
			{
				wait_removes_.erase(rit);
				_it = timers_.erase(_it);
				return true;
			}
			
            return false;
		}

		//inline std::vector<passive_timer*>::iterator
		//	internal_accept(
		//		passive_timer* _timer, mgu_timestamp_t _curr) noexcept;

        bool locked_;
		std::vector<passive_timer*> timers_;
        std::vector<passive_timer*> wait_accepts_;
        std::set<passive_timer*> wait_removes_;
	};
	using ticker_ptr = std::shared_ptr<ticker>;

	inline passive_timer::~passive_timer()
	{
		auto ticker = ticker_.lock();
		if (ticker)
			ticker->remove(this);
	}

	inline void passive_timer::start(mgu_timestamp_t _curr) noexcept
	{
		count_ = 0;
		isStart_ = true;
		isOnce_ = false;
		lastTs_ = _curr;

		auto ticker = ticker_.lock();
		if (ticker)
			ticker->accept(this, _curr);
	}

	inline void passive_timer::start_once(mgu_timestamp_t _curr) noexcept
	{
		count_ = 0;
		isStart_ = true;
		isOnce_ = true;
		lastTs_ = _curr;

		auto ticker = ticker_.lock();
		if (ticker)
			ticker->accept(this, _curr);
	}

	inline void passive_timer::cancel() noexcept
	{
		count_ = 0;
		isStart_ = false;
		isOnce_ = false;

		auto ticker = ticker_.lock();
		if (ticker)
            ticker->remove(this);
	}

	inline void passive_timer::restart(mgu_timestamp_t _curr) noexcept
	{
		count_ = 0; lastTs_ = _curr;

		auto ticker = ticker_.lock();
		if (ticker)
			ticker->accept(this, _curr);
	}

	inline void 
		ticker::accept(
			passive_timer* _timer, mgu_timestamp_t _curr) noexcept
	{
		if (locked()) {
			wait_accepts_.push_back(_timer);
			return;
		}

		if (timers_.empty()) {
			timers_.insert(timers_.end(), _timer);
			return;
		}

		for (auto it = timers_.begin(); it != timers_.end();)
		{
			if (*it == _timer) {
				it = timers_.erase(it);
			}
            else {
				if (remove_and_iteration(it))
					continue;

                (*it)->timing_notcall(_curr);
				++it;
			}
		}

		for (auto it = timers_.begin(); it != timers_.end();)
		{
			if (intptr_t(_timer->interval()) < (*it)->due_in())
			{
				timers_.insert(it, _timer);
				return;
			} 
			
			++it;
		}

		timers_.insert(timers_.end(), _timer);
	}
	
	//inline std::vector<passive_timer*>::iterator 
	//	ticker::internal_accept(
	//		passive_timer* _timer, mgu_timestamp_t _curr) noexcept
	//{
 //       if (timers_.empty())
	//		return timers_.insert(timers_.end(), _timer);

	//	for (auto it = timers_.begin(); it != timers_.end(); ++it)
	//	{
	//		if (intptr_t(_timer->interval()) < (*it)->due_in()) {
	//			return timers_.insert(it, _timer);
	//		}
	//	}
	//	
 //       return timers_.insert(timers_.end(), _timer);
	//}

	inline bool ticker::wheel_timing(mgu_timestamp_t _curr)
	{
		auto cleanup = megopp::util::scope_cleanup__create([&]
		{
			if (!wait_accepts_.empty()) {
				for (auto it = wait_accepts_.begin(); it != wait_accepts_.end(); ++it)
				{
					accept(*it, _curr);
				}
				wait_accepts_.clear();
			}
		});

		bool hasCall = false;
        locked_ = true;
        for (auto it = timers_.begin(); it != timers_.end();)
        {
			if (remove_and_iteration(it))
				continue;

			bool isDie = false;
            if ((*it)->timing(_curr, &isDie))
            {
				//locked_ = false;

				if (remove_and_iteration(it)) 
					return true;

                auto backup = *it;
				it = timers_.erase(it);
				if (!isDie && !backup->is_once()) 
				{
                    wait_accepts_.push_back(backup);
				}

				std::this_thread::yield();
				_curr = mgu_timestamp_get();
				hasCall = true;
            }
            else
                ++it;
        }

        locked_ = false;
		return hasCall;
	}

};
};

#endif // !MMUPP_CHRONO_PASSIVE_TIMER_HPP_INCLUDED
