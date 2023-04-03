#pragma once
#ifndef DISPOSABLE_OBJECT
#define DISPOSABLE_OBJECT
	#include "invokable.hpp"
	
	#ifndef DISPOSABLE_INTERFACE_DEFAULT
		#define DISPOSABLE_BOOL_DEFAULT true
	#endif

	class disposable {
	protected:
		std::atomic_bool disposed = false;

	public:

		invokable<bool> onDispose;
			
		void Dispose() {
			if (disposed) return;
			onDispose.invoke(DISPOSABLE_BOOL_DEFAULT);
			disposed = true;
		}

		bool IsDisposed() { return disposed; }

		~disposable() { Dispose(); }
		
		disposable() {}

		static void DisposeOrdered(std::vector<disposable*> objects, bool descending = false) {
			if (descending)
				std::reverse(objects.begin(), objects.end());

			for(disposable* obj : objects)
				obj->Dispose();
		}
	};

#endif