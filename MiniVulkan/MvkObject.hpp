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
		};
	}
#endif