#pragma once

class GameObject; // 전방 선언 (Forward Declaration)

class Component {
public:
    GameObject* gameObject; // 자신을 소유한 GameObject를 가리키는 포인터

    virtual ~Component() {} // 자식 클래스의 소멸자 호출을 위한 가상 소멸자
    virtual void Update() {
        // 기본적으로 아무것도 하지 않음. 자식 클래스에서 재정의.
    }
};