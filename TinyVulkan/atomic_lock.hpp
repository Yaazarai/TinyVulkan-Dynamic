#pragma once
#ifndef ATOMIC_LOCK
#define ATOMIC_LOCK
	#include <mutex>

	struct atomic_mutex {
	public:
		std::atomic_bool signal;
		std::mutex lock;
	};

	class atomic_lock {
	private:
		std::atomic_bool signal;
	public:
		atomic_mutex& lock;

		atomic_lock(atomic_mutex& lock, bool wait = false, size_t timeout = 100) : lock(lock) {
			signal = static_cast<bool>(lock.signal);

			if (wait) {
				std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
				std::chrono::milliseconds startTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
				std::chrono::milliseconds endTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
				
				while (signal) {
					endTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
					if (endTime.count() - startTime.count() >= static_cast<long long>(timeout)) break;
					signal = static_cast<bool>(lock.signal);
				}
			}

			lock.signal = !signal;
			if (signal == false) lock.lock.lock();
		}

		~atomic_lock() { if (!signal) { lock.signal = false; lock.lock.unlock(); } }
		
		bool AcquiredLock() { return !signal; }

		void ForceUnlock() { if (!signal) { signal = true; lock.signal = false; lock.lock.unlock(); } }
	};

#endif