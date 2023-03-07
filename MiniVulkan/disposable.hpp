#pragma once
#ifndef DISPOSABLE_OBJECT
#define DISPOSABLE_OBJECT
	#include <mutex>
	#include "invokable.hpp"
	
	namespace std {
		class disposable {
		protected:
			bool disposed = false;

		public:
			static std::mutex global_lock;

			std::invokable<bool> onDispose;
			
			void Dispose() {
				if (disposed) return;
				onDispose.invoke(true);
				disposed = true;
			}

			bool IsDisposed() { return disposed; }

			~disposable() { Dispose(); }
			disposable() {}

			static void DisposeOrdered(std::vector<std::disposable*> objects, bool descending = false) {
				std::lock_guard g(global_lock);

				if (descending)
					std::reverse(objects.begin(), objects.end());

				for(std::disposable* obj : objects)
					obj->Dispose();
			}
		};

		std::mutex disposable::global_lock;
	}

#endif