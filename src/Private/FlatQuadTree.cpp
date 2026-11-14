#include "../Public/FlatQuadTree.h"
#include <iostream>

void FlatQuadTree::clear(glm::vec2 center, float maxTreeWidth)
{
    usedNodes = 1;
    nodes[0].center = center;
    nodes[0].halfWidth = maxTreeWidth / 2;
    nodes[0].firstChild = -1;
    nodes[0].particleIndex = -1;
}

void FlatQuadTree::insert(Particle* particlesArray, int particleIndex)
{
    insert(particlesArray, particleIndex, 0, 1);
}

void FlatQuadTree::insert(Particle *particlesArray, int insertParticleIndex, int insertPositionIndex, size_t steps)
{
    int currentNodeID = insertPositionIndex;
    auto particle = &particlesArray[insertParticleIndex];

    while (true)
    {
        Node& currentNode = nodes.at(currentNodeID);

        // Check whether the node is a leaf
        if (currentNode.firstChild == -1)  // Node is a leaf
        {
            if (currentNode.particleIndex == -1)   // Node is an empty leaf
            {
                currentNode.particleIndex = insertParticleIndex;
                nextInLeaf[currentNode.particleIndex] = -1;
            } else if (steps >= MAX_DEPTH) {
                int nextNode = currentNode.particleIndex;
                while (nextInLeaf[nextNode] != -1)
                {
                    nextNode = nextInLeaf[nextNode];
                }
                nextInLeaf[nextNode] = insertParticleIndex;
                nextInLeaf[insertParticleIndex] = -1;

                //currentNode.sharedParticles.push_back(currentNode.particleIndex);
                //currentNode.sharedParticles.push_back(insertParticleIndex);
                //currentNode.particleIndex = -1;

            } else {   // Node has no children - Split node
                splitNodeAndReInsert(particlesArray, currentNodeID, insertParticleIndex, steps);
            }
            return;
        } else {
            auto direction = GetRelativeDirection(currentNode.center, particle->position);
            currentNodeID = currentNode.firstChild + direction;
            steps++;
        }
    }
}

RelativeDirection FlatQuadTree::GetRelativeDirection(const glm::vec2& centerPosition, const glm::vec2& compPosition)
{
    bool bEast = compPosition.x > centerPosition.x;
	bool bNorth = compPosition.y > centerPosition.y;
	if (bEast)
		return bNorth ? RelativeDirection::NE : RelativeDirection::SE;
	return bNorth ? RelativeDirection::NW : RelativeDirection::SW;
}

glm::vec2 getDirectionVector(int direction)
{
    glm::vec2 map[4] = { glm::vec2(1, 1), glm::vec2(-1, 1), glm::vec2(-1, -1), glm::vec2(1, -1) };
    return map[direction];
}

void FlatQuadTree::splitNodeAndReInsert(Particle* particlesArray, int existingNodeIndex, int particleIndex, size_t recursionSteps)
{
    int firstChildIndex = usedNodes;
    usedNodes += 4;
    if (firstChildIndex + 3 >= nodes.size())
    {
        // Increase nodes size by 4 - slightly more than needed, but we might need it later
        for (int i = 0; i < 4; i++)
        {
            nodes.push_back(Node());
        }
    }

    Node& currentNode = nodes.at(existingNodeIndex);

    // Initialize child nodes
    for (int i = 0; i <= 3; i++)
    {
        Node& childNode = nodes.at(firstChildIndex + i);
        childNode.firstChild = -1;
        childNode.particleIndex = -1;
        childNode.center = currentNode.center + 0.5f * currentNode.halfWidth * getDirectionVector(i);
        childNode.halfWidth = currentNode.halfWidth / 2;
        childNode.particleIndex = -1;
    }

    // Insert - we don't call insert, as we already know exactly where to insert what
    // The indices from firstChildIndex onwards are all already set to nullptr
    // 1. Copy the original nodes values over
    // 2. Insert the other particle normally with the insert function - not the fastest, but easiest
    auto direction = GetRelativeDirection(currentNode.center, particlesArray[currentNode.particleIndex].position);
    Node& newNode = nodes.at(firstChildIndex + direction);
    newNode.particleIndex = currentNode.particleIndex;
    nextInLeaf[currentNode.particleIndex] = -1;

    currentNode.firstChild = firstChildIndex;
    currentNode.particleIndex = -1;

    insert(particlesArray, particleIndex, existingNodeIndex, recursionSteps);
}

void FlatQuadTree::calculateForces(Particle* particlesArray, int arraySize)
{

    for (int i = usedNodes; i-- > 0; )
    {
        Node& currentNode = nodes.at(i);
        if (currentNode.firstChild >= (int)nodes.size() || currentNode.firstChild < -1)
        {
            std::cerr << "Detected invalid firstChild: " << currentNode.firstChild << std::endl;
        }
    }

    calculateBranchWeights(particlesArray, arraySize);

#pragma omp parallel for schedule(dynamic, 256)
    for (int64_t i = 0; i < arraySize; i++)
    {
        Particle& currentParticle = particlesArray[i];

        int activeNodeToCompare = 0;  // Always points to the last active node in nodesToCompare
        int nodesToCompare[3 * MAX_DEPTH + 4];
        
        nodesToCompare[0] = 0;
        
        while (activeNodeToCompare >= 0)
        {
            Node& currentNode = nodes[nodesToCompare[activeNodeToCompare]];

            if (currentNode.mass == 0) 
            {
                activeNodeToCompare--;
                continue;
            }

            if (currentNode.firstChild == -1)  // Leaf
            {
                // Calculate forces and apply them
                currentParticle.acceleration += calculateAcceleration(
                    currentParticle.position, currentNode.COM, currentNode.mass);

                activeNodeToCompare--;
                continue;
            }

            float distance = glm::distance(currentParticle.position, currentNode.COM);
            double distanceRatio = currentNode.halfWidth * 2 / distance;

            if (distanceRatio < THETA)
            {
                currentParticle.acceleration += calculateAcceleration(
                    currentParticle.position, currentNode.COM, currentNode.mass);
                activeNodeToCompare--;
            } else {
                // Remove the current node and add its children
                activeNodeToCompare--;

                for (short childIdx = 0; childIdx < 4; childIdx++)
                {
                    nodesToCompare[++activeNodeToCompare] = currentNode.firstChild + childIdx;
                }
            }
        }
    }
}

glm::vec2 FlatQuadTree::calculateAcceleration(glm::vec2 position1, glm::vec2 position2, float mass2)
{
    glm::vec<2, double> deltaVector = (position2 - position1);  // TODO: Handle case, where position1 == position2 to avoid 'nan' values	
    float d2 = glm::dot(deltaVector, deltaVector) + DISTANCE_EPSILON * DISTANCE_EPSILON;
    glm::vec<2, double> affectingForce = double(GRAVITATIONAL_CONSTANT) * static_cast<double>(mass2) / (d2 * sqrtf(d2)) * deltaVector;

    return affectingForce * FORCE_SCALE;
}

void FlatQuadTree::calculateBranchWeights(Particle* particlesArray, int arraySize)
{
    for (int i = usedNodes; i-- > 0; )
    {
        Node& currentNode = nodes.at(i);
        if (currentNode.firstChild >= (int)nodes.size())
        {
            std::cerr << "Detected invalid firstChild: " << currentNode.firstChild << std::endl;
        }

        // Verify whether the current node has children to be added, or not
        if (currentNode.firstChild == -1)
        {
            float totalMass = 0;
            int currentParticleIndex = currentNode.particleIndex;

            while (currentParticleIndex != -1)
            {
                totalMass += particlesArray[currentParticleIndex].mass;
                currentParticleIndex = nextInLeaf[currentParticleIndex];
            }

            glm::vec2 com = glm::vec2(0, 0);
            currentParticleIndex = currentNode.particleIndex;
            while (currentParticleIndex != -1)
            {
                com += (particlesArray[currentParticleIndex].mass / totalMass) * particlesArray[currentParticleIndex].position;
                currentParticleIndex = nextInLeaf[currentParticleIndex];
            }
            currentNode.mass = totalMass;
            currentNode.COM = com;
        }
        else
        {
            float totalMass = 0;
            for (int j = 0; j <= 3; j++)
            {
                totalMass += nodes.at(currentNode.firstChild + j).mass;
            }

            glm::vec2 com = glm::vec2(0, 0);
            for (int j = 0; j <= 3; j++)
            {
                Node childNode = nodes.at(currentNode.firstChild + j);
                com += (childNode.mass / totalMass) * childNode.COM;
            }

            currentNode.mass = totalMass;
            currentNode.COM = com;
        }
    }
}
