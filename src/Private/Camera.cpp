//
// Created by hmick on 31.05.2023.
//

#include "../Public/Camera.h"
#include <iostream>
#include <gtc/matrix_transform.hpp>

Camera::Camera(GLFWwindow* inWindow, glm::vec3 CameraOrigin)
{
    window = inWindow;

    CameraPos = CameraOrigin;
    ViewMatrix = glm::mat4(1.0f);
    CameraTarget = glm::vec3(0.0f);
    ForwardVector = FORWARD_VEC_DEFAULT;
    RightVector = RIGHT_VEC_DEFAULT;

    bMouseControl = false;
    mouseX = mouseY = 0.0;
    pitchAngle = yawAngle = 0.0f;
    bFirstMouse = true;
    movementSpeed = 0;
}

void Camera::HandleCameraInput(double deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        CameraPos += movementSpeed * (float)deltaTime * UP_VEC_DEFAULT;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        CameraPos += -movementSpeed * (float)deltaTime * UP_VEC_DEFAULT;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        CameraPos += movementSpeed * (float)deltaTime * RightVector;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        CameraPos += -movementSpeed * (float)deltaTime * RightVector;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        CameraPos += movementSpeed * (float)deltaTime * ForwardVector;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        CameraPos += -movementSpeed * (float)deltaTime * ForwardVector;
    }
}

glm::mat4 Camera::CalcViewMatrix() {
    ViewMatrix = glm::lookAt(CameraPos, CameraPos + ForwardVector, UP_VEC_DEFAULT);
    return ViewMatrix;
}

glm::mat4 Camera::GetTransformMatrix()
{
    glm::mat4 view = CalcViewMatrix();
    glm::mat4 perspective = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
    return perspective * view;
}

glm::vec3 Camera::GetPosition() const {
    return CameraPos;
}

glm::vec4 Camera::GetForwardVector() const {
    return glm::vec4(ForwardVector, 1.0f);
}

void Camera::OrthoNormalizeVectors()
{
    RightVector = glm::normalize(glm::cross(ForwardVector, UP_VEC_DEFAULT));
}

void Camera::mouseCallback(double xPos, double yPos)
{
    if (!bMouseControl) return;
    if (bFirstMouse)
    {
        mouseX = xPos;
        mouseY = yPos;
        bFirstMouse = false;
    }

    double mouseDeltaX = (xPos - mouseX) * -0.1f;
    double mouseDeltaY = (yPos - mouseY) * -0.1f;
    mouseX = xPos;
    mouseY = yPos;


    // TODO Base Vector orientation on default vector, DO NOT update the current vectors. This leads to rotation sign switching
    double oldPitch = pitchAngle;
    double oldYaw = yawAngle;
    pitchAngle = fmin(fmax(pitchAngle + mouseDeltaY, -89.0f), 89.0f);
    yawAngle += mouseDeltaX;
    yawAngle = yawAngle > 180 ? yawAngle - 360 : yawAngle < -180 ? yawAngle + 360 : yawAngle;
    mouseDeltaY = pitchAngle - oldPitch;
    mouseDeltaX = yawAngle - oldYaw;

    std::cout << "pitchAngle: " << pitchAngle << std::endl;

    glm::mat4 matrix(1.0);
    glm::mat3 pitchRotation = glm::mat3(glm::rotate(matrix, glm::radians((float)mouseDeltaY), RightVector));
    matrix = glm::mat4(1.0f);
    glm::mat3 yawRotation = glm::mat3(glm::rotate(matrix, glm::radians((float)mouseDeltaX), UP_VEC_DEFAULT));
    glm::mat3 rotationMatrix = pitchRotation * yawRotation;
    
    ForwardVector = glm::normalize(rotationMatrix * ForwardVector);
    RightVector = glm::normalize(rotationMatrix * RightVector);
}

void Camera::mouseButtonCallback(int button, int action, int mods)
{
    if ((button == GLFW_MOUSE_BUTTON_RIGHT) && (action == GLFW_PRESS))
    {
        bMouseControl = !bMouseControl;
        if (bMouseControl)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void Camera::mouseEnterCallback(int entered)
{
    bFirstMouse = true;
}

void Camera::setWindowDimensions(int height, int width)
{
    windowHeight = height;
    windowWidth = width;
}
