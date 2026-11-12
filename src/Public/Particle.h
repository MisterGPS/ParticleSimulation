#pragma once

#include "glm.hpp"

struct Particle {
    Particle();

    glm::vec<2, float> position = { 0, 0 };
    glm::vec<2, float>  velocity = { 0, 0 };
    glm::vec<2, float> accelerationForce = { 0, 0 };
    float radius = 1.0f;
    float mass = 1;
    float colorFast[4] = {1.0f, 0.55f, 0.2f, 1};    // warm orange
    float colorSlow[4] = {0.2f, 0.4f, 1.0f, 1};     // blue
};
