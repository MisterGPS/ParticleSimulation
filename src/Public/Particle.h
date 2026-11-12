#pragma once

#include "glm.hpp"

struct Particle {
    Particle();

    glm::vec<2, float> position = { 0, 0 };
    glm::vec<2, float>  velocity = { 0, 0 };
    glm::vec<2, float> accelerationForce = { 0, 0 };
    float radius = 1.0f;
    float mass = 1;
    float colorFast[4] = {1.0f, 0.35f, 0.1f, 1};    // orange-red
    float colorSlow[4] = {0.15f, 0.3f, 1.0f, 1};     // deep blue
};
