#pragma once
#ifndef TIMED_GUARD
#define TIMED_GUARD
	#include <mutex>

	template<bool wait = true, size_t timeout = 100>
	class _NODISCARD_LOCK timed_guard {
	private:
		bool signal;
		std::timed_mutex& lock;

	public:
		bool Acquired() { return signal; }

		void Unlock() { if (signal) lock.unlock(); }

		~timed_guard() noexcept { Unlock(); }

		explicit timed_guard(std::timed_mutex& lock) : lock(lock) { signal = (wait)? lock.try_lock_for(std::chrono::milliseconds(timeout)) : lock.try_lock(); }

		timed_guard(const timed_guard&) = delete;

		timed_guard& operator=(const timed_guard&) = delete;
	};

#endif