#pragma once
#include "ECSConfig.h"

#include "DX12_MeshSystem.h"
#include "InstanceData.h"

namespace ECS
{
    class ISharedComponentPool {
    public:
        virtual ~ISharedComponentPool() = default;
    };

    // 특정 타입 T의 공유 컴포넌트를 관리하는 실제 풀입니다.
    template<typename T, typename Hasher = std::hash<T>>
    class SharedComponentPool : public ISharedComponentPool {
    private:
        // 값 -> ID 맵
        std::unordered_map<T, size_t, Hasher> mValueToIdMap;
        // ID -> 값 맵
        std::vector<T> mIdToValueMap;
        size_t mNextId = 0;

    public:
        // 특정 값에 해당하는 ID를 가져오거나 새로 생성합니다.
        size_t GetId(const T& props) {
            auto it = mValueToIdMap.find(props);
            if (it == mValueToIdMap.end()) {
                size_t newId = mNextId++;
                mValueToIdMap[props] = newId;
                mIdToValueMap.push_back(props);
                return newId;
            }
            return it->second;
        }

        // ID를 통해 실제 값 데이터를 가져옵니다.
        const T& GetValue(size_t id) const {
            assert(id < mIdToValueMap.size() && "Shared component ID out of bounds.");
            return mIdToValueMap[id];
        }
    };

    class SharedComponentManager {
    private:
        // 각 공유 컴포넌트 타입에 대한 풀(Pool)을 저장합니다.
        // 키: 컴포넌트 타입 정보 (type_index)
        // 값: 해당 타입의 풀에 대한 포인터
        std::unordered_map<std::type_index, std::unique_ptr<ISharedComponentPool>> mSharedComponentPools;

        // 특정 타입의 풀을 안전하게 가져오는 private 헬퍼 함수입니다.
        template<typename T, typename Hasher = std::hash<T>>
        SharedComponentPool<T, Hasher>* GetSharedComponentPool() {
            std::type_index type = std::type_index(typeid(T));
            assert(mSharedComponentPools.find(type) != mSharedComponentPools.end() && "Shared component type not registered.");
            // 저장된 포인터를 실제 타입으로 캐스팅하여 반환합니다.
            return static_cast<SharedComponentPool<T, Hasher>*>(mSharedComponentPools[type].get());
        }

    public:
        // 새로운 종류의 공유 컴포넌트를 관리하도록 등록합니다.
        template<typename T, typename Hasher = std::hash<T>>
        void RegisterSharedComponent() {
            std::type_index type = std::type_index(typeid(T));
            assert(mSharedComponentPools.find(type) == mSharedComponentPools.end() && "Registering shared component type more than once.");
            mSharedComponentPools[type] = std::make_unique<SharedComponentPool<T, Hasher>>();
        }

        // 특정 공유 컴포넌트 값에 대한 고유 ID를 얻습니다.
        template<typename T, typename Hasher = std::hash<T>>
        size_t GetId(const T& props) {
            return GetSharedComponentPool<T, Hasher>()->GetId(props);
        }

        // ID를 통해 공유 컴포넌트의 실제 값을 얻습니다.
        template<typename T, typename Hasher = std::hash<T>>
        const T& GetValue(size_t id) {
            return GetSharedComponentPool<T, Hasher>()->GetValue(id);
        }
    };
}

struct SharedRenderProperties {
    eRenderLayer TargetLayer = eRenderLayer::Opaque;
    ECS::RepoHandle MeshHandle = 0;
    ECS::RepoHandle GeometryHandle = 0;

    // std::unordered_map의 키로 사용하기 위해 두 가지가 반드시 필요합니다.
    // 1. 값이 같은지 비교하는 연산자
    bool operator==(const SharedRenderProperties& other) const {
        return TargetLayer == other.TargetLayer &&
            MeshHandle == other.MeshHandle &&
            GeometryHandle == other.GeometryHandle;
    }
};

struct SharedRenderPropertiesHasher {
    std::size_t operator()(const SharedRenderProperties& props) const {
        // 각 멤버의 해시 값을 조합하여 고유한 해시를 생성합니다.
        std::size_t h1 = std::hash<int>()(static_cast<int>(props.TargetLayer));
        std::size_t h2 = std::hash<ECS::RepoHandle>()(props.MeshHandle);
        std::size_t h3 = std::hash<ECS::RepoHandle>()(props.GeometryHandle);
        return h1 ^ (h2 << 1) ^ (h3 << 2); // 간단한 조합 예시
    }
};