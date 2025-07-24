#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include <string>
#include "Component.h"

class GameObject {
private:
    std::vector<std::unique_ptr<Component>> mComponents;

public:
    std::string name;

    GameObject(const std::string& name = "GameObject") : name(name) {}

    void Update() {
        for (const auto& component : mComponents) {
            component->Update();
        }
    }

    // 템플릿으로 컴포넌트 추가
    template<typename T, typename... TArgs>
    T* AddComponent(TArgs&&... args) {
        // T가 Component를 상속했는지 컴파일 타임에 확인
        static_assert(std::is_base_of<Component, T>::value, "T must be a type of Component");

        // 새로운 컴포넌트를 생성하고 vector에 추가
        auto newComponent = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* componentPtr = newComponent.get();
        componentPtr->gameObject = this; // 소유자 GameObject 설정
        mComponents.push_back(std::move(newComponent));

        return componentPtr;
    }

    // 템플릿으로 컴포넌트 검색
    template<typename T>
    T* GetComponent() {
        // T가 Component를 상속했는지 컴파일 타임에 확인
        static_assert(std::is_base_of<Component, T>::value, "T must be a type of Component");

        for (const auto& component : components) {
            // dynamic_cast를 사용해 타입이 일치하는 첫 번째 컴포넌트를 찾아 반환
            T* target = dynamic_cast<T*>(component.get());
            if (target) {
                return target;
            }
        }
        return nullptr; // 찾지 못하면 nullptr 반환
    }
};