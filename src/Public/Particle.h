#pragma once

#include "glm.hpp"

struct Particle {
    Particle();

    glm::vec<2, float> position = { 0, 0 };
    glm::vec<2, float>  velocity = { 0, 0 };
    glm::vec<2, float> acceleration = { 0, 0 };
    float radius = 1.0f;
    float mass = 1;
};
