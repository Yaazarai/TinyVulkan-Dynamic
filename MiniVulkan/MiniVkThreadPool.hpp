#pragma once
#ifndef MINIVK_MINIVKTHREADPOOL
#define MINIVK_MINIVKTHREADPOOL
	#include "./MiniVK.hpp"
	#include <chrono>

	namespace MINIVULKAN_NAMESPACE {
		class MiniVkThreadPool : public std::disposable {
		private:
			void slave() {
				while (working)
					if (safety_lock.try_lock()) {
						callback<std::atomic_bool&> callback = queue.front();
						queue.pop();
						safety_lock.unlock();
						callback.invoke(working);
					} else { std::this_thread::yield(); }
			}
		public:
			std::mutex safety_lock;
			std::atomic_bool working = true;
			std::queue<callback<std::atomic_bool&>> queue;
			std::vector<std::thread> pool;

			MiniVkThreadPool(const uint32_t potentialThreads = 2, bool startWorking = true) : working(startWorking) {
				onDispose.hook(callback<bool>(this, &MiniVkThreadPool::AwaitClosePool));
				
				if (startWorking)
					TrySpawnThreads(potentialThreads);
			}

			size_t GetSize() { return pool.size(); }

			void ClearCallbacks(size_t timeout = 100) {
				std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
				std::chrono::milliseconds startTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
				std::chrono::milliseconds endTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

				while (!safety_lock.try_lock()) {
					endTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
					if (endTime.count() - startTime.count() > static_cast<long long>(timeout))
						return;
				}

				std::queue<callback<std::atomic_bool&>>().swap(queue);
			}

			void AwaitClosePool(size_t timeout = 100) {
				std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
				std::chrono::milliseconds startTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
				std::chrono::milliseconds endTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

				while (!safety_lock.try_lock()) {
					endTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
					if (endTime.count() - startTime.count() >= static_cast<long long>(timeout))
						return;
				}
				
				working = false;
				while (pool.size() > 0)
					for (std::thread& t : pool)
						if (t.joinable()) {
							t.join();
							std::erase_if(pool, [&](const std::thread& tt) { return tt.get_id() == t.get_id(); });
						}
				pool.clear();
				safety_lock.unlock();
			}

			size_t TrySpawnThreads(const uint32_t potentialThreads = 2, size_t timeout = 100) {
				std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
				std::chrono::milliseconds startTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
				std::chrono::milliseconds endTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

				while (!safety_lock.try_lock()) {
					endTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
					if (endTime.count() - startTime.count() >= static_cast<long long>(timeout))
						return 0;
				}

				working = true;
				size_t threadCount = 0;
				try {
					const size_t hw = std::thread::hardware_concurrency();
					const size_t threads = (hw == 0) ? potentialThreads : hw;
					threadCount = threads - pool.size();

					for (size_t t = 0; t < threadCount; t++)
						pool.push_back(std::thread(&MiniVkThreadPool::slave, this));
				} catch (std::runtime_error err) { working = false; std::cout << err.what() << std::endl; }
				return threadCount;
			}

			bool TryPushCallback(callback<std::atomic_bool&> task, size_t timeout = 100) {
				std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
				std::chrono::milliseconds startTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
				std::chrono::milliseconds endTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

				while (!safety_lock.try_lock()) {
					endTime = duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
					if (endTime.count() - startTime.count() >= static_cast<long long>(timeout))
						return;
				}

				queue.push(task);
				safety_lock.unlock();
				return true;
			}
		};
	}
#endif