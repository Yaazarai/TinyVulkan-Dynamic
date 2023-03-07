#pragma once
#ifndef MINIVK_MINIVKTHREADPOOL
#define MINIVK_MINIVKTHREADPOOL
	#include "./MiniVK.hpp"

	namespace MINIVULKAN_NAMESPACE {
		class MiniVkThreadPool : public std::disposable {
		private:
			void slave() {
				while (working) {
					if (safety_lock.try_lock()) {
						std::callback<std::atomic_bool&> callback = queue.front();
						queue.pop();
						safety_lock.unlock();
						callback.invoke(working);
					} else { std::this_thread::yield(); }
				}
			}
		public:
			std::mutex safety_lock;
			std::atomic_bool working;
			std::queue<std::callback<std::atomic_bool&>> queue;
			std::vector<std::thread> pool;

			MiniVkThreadPool(const uint32_t potentialThreads = 2, bool startWorking = true) : working(startWorking) {
				onDispose += std::callback<bool>(this, &MiniVkThreadPool::awaitClosePool);
				trySpawnThreads(potentialThreads, startWorking);
			}

			size_t getSize() { return pool.size(); }

			void clearTasks() { std::queue<std::callback<std::atomic_bool&>>().swap(queue); }

			void awaitClosePool(bool lwaitIdle) {
				working = false;
				while (pool.size() > 0)
					for (std::thread& t : pool)
						if (t.joinable()) {
							t.join();
							std::erase_if(pool, [&](const std::thread& tt) { return tt.get_id() == t.get_id(); });
						}
				pool.clear();
			}

			size_t trySpawnThreads(const uint32_t potentialThreads = 2, bool startWorking = false) {
				size_t threadCount = 0;
				try {
					const size_t hw = std::thread::hardware_concurrency();
					const size_t threads = (hw == 0) ? potentialThreads : hw;
					threadCount = threads - pool.size();

					for (size_t t = 0; t < threadCount; t++)
						pool.push_back(std::thread(&MiniVkThreadPool::slave, this));
				} catch (...) { working = false; throw; }
				return threadCount;
			}

			bool tryPushCallback(std::callback<std::atomic_bool&> task) {
				if (safety_lock.try_lock()) {
					queue.push(task);
					safety_lock.unlock();
					return true;
				}
				return false;
			}
		};
	}
#endif