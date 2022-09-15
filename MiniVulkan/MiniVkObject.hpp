#pragma once
#ifndef MINIVK_MINIVKOBJECT
#define MINIVK_MINIVKOBJECT
	#include "./MiniVk.hpp"

	namespace MINIVULKAN_NS {
		class MiniVkObject {
		protected:
			bool disposed = false;
		public:
			std::invokable<> onDispose;
			~MiniVkObject() { Dispose(); }
			void Dispose() {
				if (disposed) return;
				onDispose.invoke();
				disposed = true;
			}
			bool IsDisposed() { return disposed; }
			MiniVkObject() { /* Default Constructor, does nothing. */ }
		};
	}
#endif