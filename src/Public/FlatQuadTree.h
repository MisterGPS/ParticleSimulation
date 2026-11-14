#pragma once

#include <glm.hpp>
#include <vector>
#include "Particle.h"

#define MAX_DEPTH 20
#define EXPECTED_NODES_OVERSIZE_MULTIPLIER 4
#define GRAVITATIONAL_CONSTANT 1.0f
#define COEFFICIENT_OF_RESTITUTION 1.0
#define SIMULATION_SPEED 1
#define FORCE_SCALE 1.0
#define THETA 0.5f  // Value between 0 and 1. The smaller the number the more accurate the simulation. 0.5 provides a good trade off between performance and simulation accuracy. A very large value >1 approximates a single force in the screens center
#define DISTANCE_EPSILON 5
#define NUM_THREADS 10

struct Node {
    int firstChild = -1;  // -1 if Node is leaf
    int particleIndex = -1;  // -1 if Node is internal
    float halfWidth = 0;
    float mass = 0;
    glm::vec2 center = glm::vec2(0, 0);
    glm::vec2 COM = glm::vec2(0, 0);  // Center of mass
};

enum RelativeDirection
{
	NE = 0,
	NW = 1,
	SW = 2,
	SE = 3
};

class FlatQuadTree {
private:
    std::vector<Node> nodes;
    std::vector<int> nextInLeaf;
    size_t usedNodes = 1;

public:
    FlatQuadTree(size_t numNodes, glm::vec2 center, float maxTreeWidth) 
    {
        nodes = std::vector<Node>(EXPECTED_NODES_OVERSIZE_MULTIPLIER * numNodes);
        nextInLeaf = std::vector<int>(numNodes);
    }
    
    void clear(glm::vec2 center, float maxTreeWidth);
    void insert(Particle* particlesArray, int particleIndex);
    void calculateForces(Particle* particlesArray, int arraySize);
    void calculateBranchWeights(Particle* particlesArray, int arraySize);

    const std::vector<Node>& GetNodes() { return nodes; };
    size_t GetNumUsedNodes() { return usedNodes; };
    
private:
    static RelativeDirection GetRelativeDirection(const glm::vec2& centerPosition, const glm::vec2& compPosition);
    
    void splitNodeAndReInsert(Particle* particlesArray, int existingNodeIndex, int particleIndex, size_t recursionSteps);
    void insert(Particle* particlesArray, int particleIndex, int insertPositionIndex, size_t steps);
    glm::vec2 calculateAcceleration(glm::vec2 position1, glm::vec2 position2, float mass2);
};
