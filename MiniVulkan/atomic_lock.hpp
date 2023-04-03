#pragma once
#ifndef ATOMIC_LOCK
#define ATOMIC_LOCK
	#include <mutex>

	class atomic_lock {
	private:
		std::atomic_bool isignal;
	public:
		std::atomic_bool& signal;
		std::mutex& lock;

		atomic_lock(std::atomic_bool& signal, std::mutex& lock, bool wait = false, size_t timeout = 100) : signal(signal), lock(lock) {
			isignal = static_cast<bool>(signal);

			if (wait) {
				std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
				std::chrono::milliseconds startTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
				std::chrono::milliseconds endTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

				
				while (isignal) {
					endTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
					if (endTime.count() - startTime.count() >= static_cast<long long>(timeout)) break;
					isignal = static_cast<bool>(signal);
				}
			}

			signal = !isignal;
			if (isignal == false) lock.lock();
		}

		~atomic_lock() { if (isignal == false) { signal = false; lock.unlock(); } }
		
		bool AcquiredLock() { return !isignal; }
	};

#endif