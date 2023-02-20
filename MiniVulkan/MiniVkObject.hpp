#pragma once
#ifndef MINIVULKAN_MINIVKOBJECT
#define MINIVULKAN_MINIVKOBJECT
	#include "./MiniVK.hpp"

	namespace MINIVULKAN_NAMESPACE {
		class MiniVkObject {
		protected:
			bool disposed = false;

		public:
			MiniVkInvokable<> onDispose;
			
			void Dispose() {
				if (disposed) return;
				onDispose.invoke();
				disposed = true;
			}

			bool IsDisposed() { return disposed; }

			~MiniVkObject() { Dispose(); }
			MiniVkObject() {}

			static void DisposeOrdered(const std::vector<MiniVkObject*> objects, bool descending = true) {
				if (descending) { for(size_t i = 0; i < objects.size(); i++) objects[i]->Dispose(); }
				else { for(size_t i = 0; i < objects.size(); i++) objects[objects.size()-1-i]->Dispose(); }
			}
		};
	}
#endif