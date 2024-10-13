#pragma once

#include "glm.hpp"

struct Particle {
    Particle();

    glm::vec<2, float> position = { 0, 0 };
    glm::vec<2, float>  velocity = { 0, 0 };
    glm::vec<2, float> accumulatedForces = { 0, 0 };
    float radius = 1.0f;
    float mass = 1;
    float colorFast[4] = {1, 0.25, 0, 1};
    float colorSlow[4] = {0, -1, 1, 1};
};
