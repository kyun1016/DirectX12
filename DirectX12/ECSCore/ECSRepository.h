#pragma once
#include "ECSEntity.h"

namespace ECS
{
	template<typename T>
	class IRepository {
	public:
		bool IsLoaded(RepoHandle handle) const {
			std::lock_guard<std::mutex> lock(mtx);
			return mResourceStorage.find(handle) != mResourceStorage.end();
		}

		RepoHandle Load(const std::string& name) {
			std::lock_guard<std::mutex> lock(mtx);
			auto it = mNameToHandle.find(name);
			if (it != mNameToHandle.end()) {
				mResourceStorage[it->second].refCount++;
				return it->second;
			}

			auto resource = std::make_unique<T>();
			if (!LoadResourceInternal(name, resource.get()))
			{
				LOG_ERROR("Failed to load resource from name: {}", name);
				return 0; // Return an invalid handle if loading fails
			}

			RepoHandle handle = mNextHandle++;
			mResourceStorage[handle] = { std::move(resource), 1 };
			mNameToHandle[name] = handle;
			return handle;
		}

		RepoHandle Load() {
			auto resource = std::make_unique<T>();
			if (!LoadResourceInternal(resource.get()))
			{
				LOG_ERROR("Failed to load resource without name");
				return 0; // Return an invalid handle if loading fails
			}
			RepoHandle handle;
			{
				std::lock_guard<std::mutex> lock(mtx);
				handle = mNextHandle++;
				mResourceStorage[handle] = { std::move(resource), 1 };
			}
			
			return handle;
		}

		T* Get(RepoHandle handle) {
			std::lock_guard<std::mutex> lock(mtx);
			auto it = mResourceStorage.find(handle);
			if (it == mResourceStorage.end()) {
				LOG_ERROR("Resource with handle {} not found", handle);
				return nullptr;
			}

			// it->second.refCount++;		// 현재는 Unique_ptr와 같이 활용 (혹여 Shared_ptr과 같이 활용하게 되는 시점에 다시 고민할 예정)
			LOG_VERBOSE("Resource with handle {} accessed, refCount: {}", handle, it->second.refCount);
			return it->second.resource.get();
		}

		void Release(RepoHandle handle) {
			std::lock_guard<std::mutex> lock(mtx);
			auto it = mResourceStorage.find(handle);
			if (it == mResourceStorage.end()) return;

			UnloadResource();
			LOG_INFO("Resource with handle {} released, refCount: {}", handle, it->second.refCount);
			if (--it->second.refCount <= 0) {
				UnloadResource(handle, it->second.resource);
				mResourceStorage.erase(it);
				for (auto nameIt = mNameToHandle.begin(); nameIt != mNameToHandle.end(); ++nameIt) {
					if (nameIt->second == handle) {
						mNameToHandle.erase(nameIt);
						break;
					}
				}
			}
		}

		void Shutdown() {
			std::lock_guard<std::mutex> lock(mtx);
			mResourceStorage.clear();
			mNameToHandle.clear();
			mNextHandle = 1;
		}

	protected:
		IRepository() = default;
		virtual ~IRepository() = default;
		// User-defined behavior
		virtual bool LoadResourceInternal(const std::string& name, T* ptr)
		{
			// Default implementation does nothing, can be overridden by derived classes
			LOG_ERROR("LoadResourceInternal not implemented for type {}", typeid(T).name());
			return true;
		};
		virtual bool LoadResourceInternal(T* ptr)
		{
			// Default implementation does nothing, can be overridden by derived classes
			LOG_ERROR("LoadResourceInternal not implemented for type {}", typeid(T).name());
			return true;
		};

		virtual bool UnloadResource(ECS::RepoHandle handle)
		{
			// Default implementation does nothing, can be overridden by derived classes
			LOG_ERROR("UnloadResource not implemented for type {}", typeid(T).name());
			return true;
		}

		struct RepoEntry {
			std::unique_ptr<T> resource;
			int refCount = 1;
		};

		mutable std::mutex mtx;
		std::unordered_map<std::string, RepoHandle> mNameToHandle;
		std::unordered_map<RepoHandle, RepoEntry> mResourceStorage;
		RepoHandle mNextHandle = 1;
	};
}