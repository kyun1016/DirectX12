#pragma once
#include "ECSEntity.h"
#include "ECSComponent.h"

namespace ECS
{
	template<typename T>
	class ECSRepository {
	public:
		ECSRepository() = default;
		virtual ~ECSRepository() = default;

		bool IsLoaded(RepoHandle handle) const {
			std::lock_guard<std::mutex> lock(mtx);
			return mResourceStorage.find(handle) != mResourceStorage.end();
		}

		T* Get(RepoHandle handle) const {
			std::lock_guard<std::mutex> lock(mtx);
			auto it = mResourceStorage.find(handle);
			return (it != mResourceStorage.end()) ? it->second.resource.get() : nullptr;
		}

		void Release(RepoHandle handle) {
			std::lock_guard<std::mutex> lock(mtx);
			auto it = mResourceStorage.find(handle);
			if (it == mResourceStorage.end()) return;

			if (--it->second.refCount <= 0) {
				UnloadResource(handle, it->second.resource);
				mResourceStorage.erase(it);
			}
		}

		void Shutdown() {
			std::lock_guard<std::mutex> lock(mtx);
			mResourceStorage.clear();
			mPathToHandle.clear();
			mNextHandle = 1;
		}

	protected:
		// User-defined behavior
		virtual std::unique_ptr<T> LoadResourceInternal(const std::string& path) = 0;
		virtual void UnloadResource(RepoHandle handle, std::unique_ptr<T>& resource) = 0;

		RepoHandle Load(const std::string& path) {
			std::lock_guard<std::mutex> lock(mtx);
			auto it = mPathToHandle.find(path);
			if (it != mPathToHandle.end()) {
				mResourceStorage[it->second].refCount++;
				return it->second;
			}

			RepoHandle handle = mNextHandle++;
			auto resource = LoadResourceInternal(path);
			mResourceStorage[handle] = { std::move(resource), 1 };
			mPathToHandle[path] = handle;
			return handle;
		}

		struct ResourceEntry {
			std::unique_ptr<T> resource;
			int refCount = 1;
		};

		mutable std::mutex mtx;
		std::unordered_map<std::string, RepoHandle> mPathToHandle;
		std::unordered_map<HandleType, ResourceEntry> mResourceStorage;
		RepoHandle mNextHandle = 1;
	};
}