#pragma once
#ifndef MINIVK_MINIVKOBJECT
#define MINIVK_MINIVKOBJECT
	#include "./MiniVulkan.hpp"

	namespace MINIVULKAN_NS {
		class MvkObject {
		protected:
			bool disposed = false;
		public:
			std::invokable<> onDispose;
			~MvkObject() { Dispose(); }
			void Dispose() {
				if (disposed) return;
				onDispose.invoke();
				disposed = true;
			}
			bool IsDisposed() { return disposed; }
			MvkObject() { /* Default Constructor, does nothing. */ }

			static void DisposeOrdered(const std::vector<MvkObject*> objects, bool descending = true) {
				if (descending) {
					for(size_t i = 0; i < objects.size(); i++) objects[i]->Dispose();
				} else {
					for(size_t i = objects.size()-1; i > 0; i--) objects[i]->Dispose();
				}
			}
		};
	}
#endif