//
// Created by hmick on 31.05.2023.
//

#pragma once

#include <glfw3.h>
#include <glm.hpp>

#define FORWARD_VEC_DEFAULT glm::vec3(0.0f, 0.0f, -1.0f)
#define UP_VEC_DEFAULT glm::vec3(0.0f, 1.0f, 0.0f)
#define RIGHT_VEC_DEFAULT glm::vec3(1.0f, 0.0f, 0.0f)

class Camera
{
public:
    Camera(GLFWwindow* inWindow, glm::vec3 CameraOrigin = glm::vec3(0.0f, 0.0f, 3.0f));

    void HandleCameraInput(double deltaTime);

    glm::mat4 GetTransformMatrix();

    glm::mat4 CalcViewMatrix();
    glm::vec3 GetPosition() const;
    glm::vec4 GetForwardVector() const;

    void OrthoNormalizeVectors();

    void mouseCallback(double xPos, double yPos);
    void mouseButtonCallback(int button, int action, int mods);
    void mouseEnterCallback(int entered);
    void setCameraSpeed(float speed) { movementSpeed = speed; }

    void setWindowDimensions(int height, int width);

private:
    GLFWwindow* window;
    
    glm::mat4 ViewMatrix;
    glm::vec3 CameraPos;
    glm::vec3 CameraTarget;
    glm::vec3 ForwardVector;
    glm::vec3 RightVector;

    float movementSpeed;

    int windowHeight = 600.0f;
    int windowWidth = 800.0f;

    bool bMouseControl;
    double mouseX, mouseY;
    double pitchAngle, yawAngle;
    bool bFirstMouse;
};
