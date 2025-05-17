/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <unordered_map>
#include <array>
#include <optional>
#include <memory>

#include "../DonutCore/Math.h"

// #define GLFW_INCLUDE_NONE // Do not include any OpenGL headers
// #include <GLFW/glfw3.h>

/* Printable keys */
#define GLFW_KEY_SPACE              32
#define GLFW_KEY_APOSTROPHE         39  /* ' */
#define GLFW_KEY_COMMA              44  /* , */
#define GLFW_KEY_MINUS              45  /* - */
#define GLFW_KEY_PERIOD             46  /* . */
#define GLFW_KEY_SLASH              47  /* / */
#define GLFW_KEY_0                  48
#define GLFW_KEY_1                  49
#define GLFW_KEY_2                  50
#define GLFW_KEY_3                  51
#define GLFW_KEY_4                  52
#define GLFW_KEY_5                  53
#define GLFW_KEY_6                  54
#define GLFW_KEY_7                  55
#define GLFW_KEY_8                  56
#define GLFW_KEY_9                  57
#define GLFW_KEY_SEMICOLON          59  /* ; */
#define GLFW_KEY_EQUAL              61  /* = */
#define GLFW_KEY_A                  65
#define GLFW_KEY_B                  66
#define GLFW_KEY_C                  67
#define GLFW_KEY_D                  68
#define GLFW_KEY_E                  69
#define GLFW_KEY_F                  70
#define GLFW_KEY_G                  71
#define GLFW_KEY_H                  72
#define GLFW_KEY_I                  73
#define GLFW_KEY_J                  74
#define GLFW_KEY_K                  75
#define GLFW_KEY_L                  76
#define GLFW_KEY_M                  77
#define GLFW_KEY_N                  78
#define GLFW_KEY_O                  79
#define GLFW_KEY_P                  80
#define GLFW_KEY_Q                  81
#define GLFW_KEY_R                  82
#define GLFW_KEY_S                  83
#define GLFW_KEY_T                  84
#define GLFW_KEY_U                  85
#define GLFW_KEY_V                  86
#define GLFW_KEY_W                  87
#define GLFW_KEY_X                  88
#define GLFW_KEY_Y                  89
#define GLFW_KEY_Z                  90
#define GLFW_KEY_LEFT_BRACKET       91  /* [ */
#define GLFW_KEY_BACKSLASH          92  /* \ */
#define GLFW_KEY_RIGHT_BRACKET      93  /* ] */
#define GLFW_KEY_GRAVE_ACCENT       96  /* ` */
#define GLFW_KEY_WORLD_1            161 /* non-US #1 */
#define GLFW_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define GLFW_KEY_ESCAPE             256
#define GLFW_KEY_ENTER              257
#define GLFW_KEY_TAB                258
#define GLFW_KEY_BACKSPACE          259
#define GLFW_KEY_INSERT             260
#define GLFW_KEY_DELETE             261
#define GLFW_KEY_RIGHT              262
#define GLFW_KEY_LEFT               263
#define GLFW_KEY_DOWN               264
#define GLFW_KEY_UP                 265
#define GLFW_KEY_PAGE_UP            266
#define GLFW_KEY_PAGE_DOWN          267
#define GLFW_KEY_HOME               268
#define GLFW_KEY_END                269
#define GLFW_KEY_CAPS_LOCK          280
#define GLFW_KEY_SCROLL_LOCK        281
#define GLFW_KEY_NUM_LOCK           282
#define GLFW_KEY_PRINT_SCREEN       283
#define GLFW_KEY_PAUSE              284
#define GLFW_KEY_F1                 290
#define GLFW_KEY_F2                 291
#define GLFW_KEY_F3                 292
#define GLFW_KEY_F4                 293
#define GLFW_KEY_F5                 294
#define GLFW_KEY_F6                 295
#define GLFW_KEY_F7                 296
#define GLFW_KEY_F8                 297
#define GLFW_KEY_F9                 298
#define GLFW_KEY_F10                299
#define GLFW_KEY_F11                300
#define GLFW_KEY_F12                301
#define GLFW_KEY_F13                302
#define GLFW_KEY_F14                303
#define GLFW_KEY_F15                304
#define GLFW_KEY_F16                305
#define GLFW_KEY_F17                306
#define GLFW_KEY_F18                307
#define GLFW_KEY_F19                308
#define GLFW_KEY_F20                309
#define GLFW_KEY_F21                310
#define GLFW_KEY_F22                311
#define GLFW_KEY_F23                312
#define GLFW_KEY_F24                313
#define GLFW_KEY_F25                314
#define GLFW_KEY_KP_0               320
#define GLFW_KEY_KP_1               321
#define GLFW_KEY_KP_2               322
#define GLFW_KEY_KP_3               323
#define GLFW_KEY_KP_4               324
#define GLFW_KEY_KP_5               325
#define GLFW_KEY_KP_6               326
#define GLFW_KEY_KP_7               327
#define GLFW_KEY_KP_8               328
#define GLFW_KEY_KP_9               329
#define GLFW_KEY_KP_DECIMAL         330
#define GLFW_KEY_KP_DIVIDE          331
#define GLFW_KEY_KP_MULTIPLY        332
#define GLFW_KEY_KP_SUBTRACT        333
#define GLFW_KEY_KP_ADD             334
#define GLFW_KEY_KP_ENTER           335
#define GLFW_KEY_KP_EQUAL           336
#define GLFW_KEY_LEFT_SHIFT         340
#define GLFW_KEY_LEFT_CONTROL       341
#define GLFW_KEY_LEFT_ALT           342
#define GLFW_KEY_LEFT_SUPER         343
#define GLFW_KEY_RIGHT_SHIFT        344
#define GLFW_KEY_RIGHT_CONTROL      345
#define GLFW_KEY_RIGHT_ALT          346
#define GLFW_KEY_RIGHT_SUPER        347
#define GLFW_KEY_MENU               348

#define GLFW_KEY_LAST               GLFW_KEY_MENU

/*! @} */

/*! @defgroup mods Modifier key flags
 *  @brief Modifier key flags.
 *
 *  See [key input](@ref input_key) for how these are used.
 *
 *  @ingroup input
 *  @{ */

 /*! @brief If this bit is set one or more Shift keys were held down.
  *
  *  If this bit is set one or more Shift keys were held down.
  */
#define GLFW_MOD_SHIFT           0x0001
  /*! @brief If this bit is set one or more Control keys were held down.
   *
   *  If this bit is set one or more Control keys were held down.
   */
#define GLFW_MOD_CONTROL         0x0002
   /*! @brief If this bit is set one or more Alt keys were held down.
    *
    *  If this bit is set one or more Alt keys were held down.
    */
#define GLFW_MOD_ALT             0x0004
    /*! @brief If this bit is set one or more Super keys were held down.
     *
     *  If this bit is set one or more Super keys were held down.
     */
#define GLFW_MOD_SUPER           0x0008
     /*! @brief If this bit is set the Caps Lock key is enabled.
      *
      *  If this bit is set the Caps Lock key is enabled and the @ref
      *  GLFW_LOCK_KEY_MODS input mode is set.
      */
#define GLFW_MOD_CAPS_LOCK       0x0010
      /*! @brief If this bit is set the Num Lock key is enabled.
       *
       *  If this bit is set the Num Lock key is enabled and the @ref
       *  GLFW_LOCK_KEY_MODS input mode is set.
       */
#define GLFW_MOD_NUM_LOCK        0x0020

       /*! @} */

       /*! @defgroup buttons Mouse buttons
        *  @brief Mouse button IDs.
        *
        *  See [mouse button input](@ref input_mouse_button) for how these are used.
        *
        *  @ingroup input
        *  @{ */
#define GLFW_MOUSE_BUTTON_1         0
#define GLFW_MOUSE_BUTTON_2         1
#define GLFW_MOUSE_BUTTON_3         2
#define GLFW_MOUSE_BUTTON_4         3
#define GLFW_MOUSE_BUTTON_5         4
#define GLFW_MOUSE_BUTTON_6         5
#define GLFW_MOUSE_BUTTON_7         6
#define GLFW_MOUSE_BUTTON_8         7
#define GLFW_MOUSE_BUTTON_LAST      GLFW_MOUSE_BUTTON_8
#define GLFW_MOUSE_BUTTON_LEFT      GLFW_MOUSE_BUTTON_1
#define GLFW_MOUSE_BUTTON_RIGHT     GLFW_MOUSE_BUTTON_2
#define GLFW_MOUSE_BUTTON_MIDDLE    GLFW_MOUSE_BUTTON_3
        /*! @} */

        /*! @defgroup joysticks Joysticks
         *  @brief Joystick IDs.
         *
         *  See [joystick input](@ref joystick) for how these are used.
         *
         *  @ingroup input
         *  @{ */
#define GLFW_JOYSTICK_1             0
#define GLFW_JOYSTICK_2             1
#define GLFW_JOYSTICK_3             2
#define GLFW_JOYSTICK_4             3
#define GLFW_JOYSTICK_5             4
#define GLFW_JOYSTICK_6             5
#define GLFW_JOYSTICK_7             6
#define GLFW_JOYSTICK_8             7
#define GLFW_JOYSTICK_9             8
#define GLFW_JOYSTICK_10            9
#define GLFW_JOYSTICK_11            10
#define GLFW_JOYSTICK_12            11
#define GLFW_JOYSTICK_13            12
#define GLFW_JOYSTICK_14            13
#define GLFW_JOYSTICK_15            14
#define GLFW_JOYSTICK_16            15
#define GLFW_JOYSTICK_LAST          GLFW_JOYSTICK_16
         /*! @} */

         /*! @defgroup gamepad_buttons Gamepad buttons
          *  @brief Gamepad buttons.
          *
          *  See @ref gamepad for how these are used.
          *
          *  @ingroup input
          *  @{ */
#define GLFW_GAMEPAD_BUTTON_A               0
#define GLFW_GAMEPAD_BUTTON_B               1
#define GLFW_GAMEPAD_BUTTON_X               2
#define GLFW_GAMEPAD_BUTTON_Y               3
#define GLFW_GAMEPAD_BUTTON_LEFT_BUMPER     4
#define GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER    5
#define GLFW_GAMEPAD_BUTTON_BACK            6
#define GLFW_GAMEPAD_BUTTON_START           7
#define GLFW_GAMEPAD_BUTTON_GUIDE           8
#define GLFW_GAMEPAD_BUTTON_LEFT_THUMB      9
#define GLFW_GAMEPAD_BUTTON_RIGHT_THUMB     10
#define GLFW_GAMEPAD_BUTTON_DPAD_UP         11
#define GLFW_GAMEPAD_BUTTON_DPAD_RIGHT      12
#define GLFW_GAMEPAD_BUTTON_DPAD_DOWN       13
#define GLFW_GAMEPAD_BUTTON_DPAD_LEFT       14
#define GLFW_GAMEPAD_BUTTON_LAST            GLFW_GAMEPAD_BUTTON_DPAD_LEFT

#define GLFW_GAMEPAD_BUTTON_CROSS       GLFW_GAMEPAD_BUTTON_A
#define GLFW_GAMEPAD_BUTTON_CIRCLE      GLFW_GAMEPAD_BUTTON_B
#define GLFW_GAMEPAD_BUTTON_SQUARE      GLFW_GAMEPAD_BUTTON_X
#define GLFW_GAMEPAD_BUTTON_TRIANGLE    GLFW_GAMEPAD_BUTTON_Y
          /*! @} */

          /*! @defgroup gamepad_axes Gamepad axes
           *  @brief Gamepad axes.
           *
           *  See @ref gamepad for how these are used.
           *
           *  @ingroup input
           *  @{ */
#define GLFW_GAMEPAD_AXIS_LEFT_X        0
#define GLFW_GAMEPAD_AXIS_LEFT_Y        1
#define GLFW_GAMEPAD_AXIS_RIGHT_X       2
#define GLFW_GAMEPAD_AXIS_RIGHT_Y       3
#define GLFW_GAMEPAD_AXIS_LEFT_TRIGGER  4
#define GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER 5
#define GLFW_GAMEPAD_AXIS_LAST          GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER

// 



namespace donut::engine
{
    class PlanarView;
    class SceneCamera;
}

namespace donut::app
{

    // A camera with position and orientation. Methods for moving it come from derived classes.
    class BaseCamera
    {
    public:
        virtual void KeyboardUpdate(int key, int scancode, int action, int mods) {}
        virtual void MousePosUpdate(double xpos, double ypos) {}
        virtual void MouseButtonUpdate(int button, int action, int mods) {}
        virtual void MouseScrollUpdate(double xoffset, double yoffset) {}
        virtual void JoystickButtonUpdate(int button, bool pressed) {}
        virtual void JoystickUpdate(int axis, float value) {}
        virtual void Animate(float deltaT) {}
        virtual ~BaseCamera() = default;

        void SetMoveSpeed(float value) { m_MoveSpeed = value; }
        void SetRotateSpeed(float value) { m_RotateSpeed = value; }

        [[nodiscard]] const dm::affine3& GetWorldToViewMatrix() const { return m_MatWorldToView; }
        [[nodiscard]] const dm::affine3& GetTranslatedWorldToViewMatrix() const { return m_MatTranslatedWorldToView; }
        [[nodiscard]] const dm::float3& GetPosition() const { return m_CameraPos; }
        [[nodiscard]] const dm::float3& GetDir() const { return m_CameraDir; }
        [[nodiscard]] const dm::float3& GetUp() const { return m_CameraUp; }

    protected:
        // This can be useful for derived classes while not necessarily public, i.e., in a third person
        // camera class, public clients cannot direct the gaze point.
        void BaseLookAt(dm::float3 cameraPos, dm::float3 cameraTarget, dm::float3 cameraUp = dm::float3{ 0.f, 1.f, 0.f });
        void UpdateWorldToView();

        dm::affine3 m_MatWorldToView = dm::affine3::identity();
        dm::affine3 m_MatTranslatedWorldToView = dm::affine3::identity();

        dm::float3 m_CameraPos = 0.f;   // in worldspace
        dm::float3 m_CameraDir = dm::float3(1.f, 0.f, 0.f); // normalized
        dm::float3 m_CameraUp = dm::float3(0.f, 1.f, 0.f); // normalized
        dm::float3 m_CameraRight = dm::float3(0.f, 0.f, 1.f); // normalized

        float m_MoveSpeed = 1.f;      // movement speed in units/second
        float m_RotateSpeed = .005f;  // mouse sensitivity in radians/pixel
    };

    class FirstPersonCamera : public BaseCamera
    {
    public:
        void KeyboardUpdate(int key, int scancode, int action, int mods) override;
        void MousePosUpdate(double xpos, double ypos) override;
        void MouseButtonUpdate(int button, int action, int mods) override;
        void Animate(float deltaT) override;
        void AnimateSmooth(float deltaT);

        void LookAt(dm::float3 cameraPos, dm::float3 cameraTarget, dm::float3 cameraUp = dm::float3{ 0.f, 1.f, 0.f });
        void LookTo(dm::float3 cameraPos, dm::float3 cameraDir, dm::float3 cameraUp = dm::float3{ 0.f, 1.f, 0.f });

    private:
        std::pair<bool, dm::affine3> AnimateRoll(dm::affine3 initialRotation);
        std::pair<bool, dm::float3> AnimateTranslation(float deltaT);
        void UpdateCamera(dm::float3 cameraMoveVec, dm::affine3 cameraRotation);

        dm::float2 mousePos;
        dm::float2 mousePosPrev;
        // fields used only for AnimateSmooth()
        dm::float2 mousePosDamp;
        bool isMoving = false;

        typedef enum
        {
            MoveUp,
            MoveDown,
            MoveLeft,
            MoveRight,
            MoveForward,
            MoveBackward,

            YawRight,
            YawLeft,
            PitchUp,
            PitchDown,
            RollLeft,
            RollRight,

            SpeedUp,
            SlowDown,

            KeyboardControlCount,
        } KeyboardControls;

        typedef enum
        {
            Left,
            Middle,
            Right,

            MouseButtonCount,
            MouseButtonFirst = Left,
        } MouseButtons;

        const std::unordered_map<int, int> keyboardMap = {
            { GLFW_KEY_Q, KeyboardControls::MoveDown },
            { GLFW_KEY_E, KeyboardControls::MoveUp },
            { GLFW_KEY_A, KeyboardControls::MoveLeft },
            { GLFW_KEY_D, KeyboardControls::MoveRight },
            { GLFW_KEY_W, KeyboardControls::MoveForward },
            { GLFW_KEY_S, KeyboardControls::MoveBackward },
            { GLFW_KEY_LEFT, KeyboardControls::YawLeft },
            { GLFW_KEY_RIGHT, KeyboardControls::YawRight },
            { GLFW_KEY_UP, KeyboardControls::PitchUp },
            { GLFW_KEY_DOWN, KeyboardControls::PitchDown },
            { GLFW_KEY_Z, KeyboardControls::RollLeft },
            { GLFW_KEY_C, KeyboardControls::RollRight },
            { GLFW_KEY_LEFT_SHIFT, KeyboardControls::SpeedUp },
            { GLFW_KEY_RIGHT_SHIFT, KeyboardControls::SpeedUp },
            { GLFW_KEY_LEFT_CONTROL, KeyboardControls::SlowDown },
            { GLFW_KEY_RIGHT_CONTROL, KeyboardControls::SlowDown },
        };

        const std::unordered_map<int, int> mouseButtonMap = {
            { GLFW_MOUSE_BUTTON_LEFT, MouseButtons::Left },
            { GLFW_MOUSE_BUTTON_MIDDLE, MouseButtons::Middle },
            { GLFW_MOUSE_BUTTON_RIGHT, MouseButtons::Right },
        };

        std::array<bool, KeyboardControls::KeyboardControlCount> keyboardState = { false };
        std::array<bool, MouseButtons::MouseButtonCount> mouseButtonState = { false };
    };

    class ThirdPersonCamera : public BaseCamera
    {
    public:
        void KeyboardUpdate(int key, int scancode, int action, int mods) override;
        void MousePosUpdate(double xpos, double ypos) override;
        void MouseButtonUpdate(int button, int action, int mods) override;
        void MouseScrollUpdate(double xoffset, double yoffset) override;
        void JoystickButtonUpdate(int button, bool pressed) override;
        void JoystickUpdate(int axis, float value) override;
        void Animate(float deltaT) override;

        dm::float3 GetTargetPosition() const { return m_TargetPos; }
        void SetTargetPosition(dm::float3 position) { m_TargetPos = position; }

        float GetDistance() const { return m_Distance; }
        void SetDistance(float distance) { m_Distance = distance; }

        float GetRotationYaw() const { return m_Yaw; }
        float GetRotationPitch() const { return m_Pitch; }
        void SetRotation(float yaw, float pitch);

        float GetMaxDistance() const { return m_MaxDistance; }
        void SetMaxDistance(float value) { m_MaxDistance = value; }

        void SetView(const engine::PlanarView& view);

        void LookAt(dm::float3 cameraPos, dm::float3 cameraTarget);
        void LookTo(dm::float3 cameraPos, dm::float3 cameraDir,
            std::optional<float> targetDistance = std::optional<float>());

    private:
        void AnimateOrbit(float deltaT);
        void AnimateTranslation(const dm::float3x3& viewMatrix);

        // View parameters to derive translation amounts
        dm::float4x4 m_ProjectionMatrix = dm::float4x4::identity();
        dm::float4x4 m_InverseProjectionMatrix = dm::float4x4::identity();
        dm::float2 m_ViewportSize = dm::float2::zero();

        dm::float2 m_MousePos = 0.f;
        dm::float2 m_MousePosPrev = 0.f;

        dm::float3 m_TargetPos = 0.f;
        float m_Distance = 30.f;

        float m_MinDistance = 0.f;
        float m_MaxDistance = std::numeric_limits<float>::max();

        float m_Yaw = 0.f;
        float m_Pitch = 0.f;

        float m_DeltaYaw = 0.f;
        float m_DeltaPitch = 0.f;
        float m_DeltaDistance = 0.f;

        typedef enum
        {
            HorizontalPan,

            KeyboardControlCount,
        } KeyboardControls;

        const std::unordered_map<int, int> keyboardMap = {
            { GLFW_KEY_LEFT_ALT, KeyboardControls::HorizontalPan },
        };

        typedef enum
        {
            Left,
            Middle,
            Right,

            MouseButtonCount
        } MouseButtons;

        std::array<bool, KeyboardControls::KeyboardControlCount> keyboardState = { false };
        std::array<bool, MouseButtons::MouseButtonCount> mouseButtonState = { false };
    };

    // The SwitchableCamera class provides a combination of first-person, third-person, and scene graph cameras.
    // The active camera can be chosen from those options, and switches between the camera types
    // can preserve the current camera position and orientation when switching to user-controllable types.
    class SwitchableCamera
    {
    public:
        // Returns the active user-controllable camera (first-person or third-person),
        // or nullptr if a scene camera is active.
        BaseCamera* GetActiveUserCamera();

        // A constant version of GetActiveUserCamera.
        BaseCamera const* GetActiveUserCamera() const;

        bool IsFirstPersonActive() const { return !m_SceneCamera && m_UseFirstPerson; }
        bool IsThirdPersonActive() const { return !m_SceneCamera && !m_UseFirstPerson; }
        bool IsSceneCameraActive() const { return !!m_SceneCamera; }

        // Always returns the first-person camera object.
        FirstPersonCamera& GetFirstPersonCamera() { return m_FirstPerson; }

        // Always returns the third-person camera object.
        ThirdPersonCamera& GetThirdPersonCamera() { return m_ThirdPerson; }

        // Returns the active scene camera object, or nullptr if a user camera is active.
        std::shared_ptr<engine::SceneCamera>& GetSceneCamera() { return m_SceneCamera; }

        // Returns the view matrix for the currently active camera.
        dm::affine3 GetWorldToViewMatrix() const;

        // Fills out the projection parameters from a scene camera, if there is a perspective camera active.
        // Returns true when the parameters were filled, false if no such camera available.
        // In the latter case, the input values for the parameters are left unmodified.
        bool GetSceneCameraProjectionParams(float& verticalFov, float& zNear) const;

        // Switches to the first-person camera, optionally copying the position and direction
        // from another active camera type.
        void SwitchToFirstPerson(bool copyView = true);

        // Switches to the third-person camera, optionally copying the position and direction
        // from another active camera type. When 'targetDistance' is specified, it overrides the current
        // distance stored in the third-person camera. Suggested use is to determine the distance to the
        // object in the center of the view at the time of the camera switch and use that distance.
        void SwitchToThirdPerson(bool copyView = true, std::optional<float> targetDistance = std::optional<float>());

        // Switches to the provided scene graph camera that must not be a nullptr.
        // The user-controllable cameras are not affected by this call.
        void SwitchToSceneCamera(std::shared_ptr<engine::SceneCamera> const& sceneCamera);

        // The following methods direct user input events to the active user camera
        // and return 'true' if such camera is active.

        bool KeyboardUpdate(int key, int scancode, int action, int mods);
        bool MousePosUpdate(double xpos, double ypos);
        bool MouseButtonUpdate(int button, int action, int mods);
        bool MouseScrollUpdate(double xoffset, double yoffset);
        bool JoystickButtonUpdate(int button, bool pressed);
        bool JoystickUpdate(int axis, float value);

        // Calls 'Animate' on the active user camera.
        // It is necessary to call Animate on the camera once per frame to correctly update its state.
        void Animate(float deltaT);

    private:
        FirstPersonCamera m_FirstPerson;
        ThirdPersonCamera m_ThirdPerson;
        std::shared_ptr<engine::SceneCamera> m_SceneCamera;
        bool m_UseFirstPerson = false;
    };
}
