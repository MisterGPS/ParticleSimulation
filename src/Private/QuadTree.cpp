#include <tuple>
#include <thread>
#include <iostream>
#include "../Public/Particle.h"
#include "../Public/QuadTree.h"

// Class QuadTree

QuadTree::QuadTree(float inTheta, double maxWidth, glm::vec2 treeCenter)
{
	// Square theta. This allows theta to be compared to the squared distance of points, avoiding a costly square root operation
	theta = inTheta;
	root = new End(maxWidth, theta, treeCenter);
}

QuadTree::~QuadTree()
{
	delete root;
}

void QuadTree::InsertParticle(Particle* inParticle)
{
	root = root->InsertParticle(inParticle, 0);
}

void QuadTree::CalculateNodeIDs()
{
	int i = 0;
	root->CalculateNodeIDs(i);
}

void QuadTree::MP_CalculateEffecingForce(Particle* particles, int start, int end)
{
	int j = 0;
	for (size_t i = start; i <= end; ++i)
	{
		j++;
		root->CalculateEffectingForce(&particles[i]);
	}
}

void QuadTree::CalculateForces(Particle* particles, int numParticles)
{
	root->CalculateCenterOfMass();  // TODO: Investigate ewhy center of mass can be nan

	// TODO: Add Multithreading/processing  
	int threadLength = floor(numParticles / (float)NUM_THREADS);
	std::thread* threads = new std::thread[NUM_THREADS];
	for (short i = 0; i < NUM_THREADS; ++i)
	{
		int start = i * threadLength;
		int end = (i + 1) * threadLength - 1;
		if (i == NUM_THREADS - 1)
			end = numParticles - 1;

		threads[i] = std::thread(&QuadTree::MP_CalculateEffecingForce, this, particles, start, end);

	}

	for (short i = 0; i < NUM_THREADS; ++i)
	{
		threads[i].join();
	}
	delete[] threads;
}

std::vector<BoundingBox>* QuadTree::GetBoundingBoxes()
{
	std::vector<BoundingBox>* boundingBoxes = new std::vector<BoundingBox>();
	root->GetBoundingBoxes(boundingBoxes);
	return boundingBoxes;
}

std::vector<Node*>* QuadTree::GetOrderedNodes()
{
	std::vector<Node*>* nodes = new std::vector<Node*>();
	root->GetOrderedNodes(nodes);
	return nodes;
}




// Class Node

RelativeDirection Node::GetRelativeDirection(const glm::vec2& centerPosition, const glm::vec2& compPosition)
{
	bool bEast = compPosition.x > centerPosition.x;
	bool bNorth = compPosition.y > centerPosition.y;
	if (bEast)
		return bNorth ? RelativeDirection::NE : RelativeDirection::SE;
	return bNorth ? RelativeDirection::NW : RelativeDirection::SW;
}

MassObject Node::CalculateCenterOfMass()
{
	return MassObject(mass, centerOfMass);
}


// TODO: Optimize this function. It is currently the slowest function and performance heavy
// 1. Idea: Store all function calls in an array and calculate it on the GPU
inline glm::vec2 Node::GetInteractionForce(MassObject& firstPoint, MassObject& secondPoint)
{
	auto& [mass1, position1] = firstPoint;
	auto& [mass2, position2] = secondPoint;

	glm::vec<2, double> deltaVector = (position2 - position1);  // TODO: Handle case, where position1 == position2 to avoid 'nan' values	
	float d2 = glm::dot(deltaVector, deltaVector) + DISTANCE_EPSILON * DISTANCE_EPSILON;
	glm::vec<2, double> affectingForce = double(GRAVITATIONAL_CONSTANT) * static_cast<double>(mass1 * mass2) / (d2 * sqrtf(d2)) * deltaVector;

	return affectingForce * FORCE_SCALE;
}




// Class Internal

Internal::Internal(double inWidth, float inTheta, glm::vec2 nodePosition) : children { nullptr, nullptr, nullptr, nullptr }
{
	theta = inTheta;
	width = inWidth;
	position = nodePosition;

	float childWidth = width / 2.0f;
	glm::vec2 newPosition;
	for (short i = 0; i < numChildren; ++i)
	{
		switch (i)
		{
			case 0: // NE
			{
				newPosition = position + childWidth / 2 * glm::vec2(1, 1);
			} break;
			case 1: // NW
			{
				newPosition = position + childWidth / 2 * glm::vec2(-1, 1);
			} break;
			case 2: // SW
			{
				newPosition = position + childWidth / 2 * glm::vec2(-1, -1);
			} break;
			case 3: // SE
			{
				newPosition = position + childWidth / 2.0f * glm::vec2(1, -1);
			} break;
		}

		children[i] = new End(childWidth, inTheta, newPosition);
	}
}

Internal::~Internal()
{
	for (short i = 0; i < 4; ++i)
	{
		delete children[i];
	}
}

Node* Internal::InsertParticle(Particle* inParticle, int parentDepth)
{
	int currentDepth = parentDepth + 1;

	RelativeDirection direction = GetRelativeDirection(position, inParticle->position);

	Node* updatedChild = children[direction]->InsertParticle(inParticle, currentDepth);
	if (updatedChild != children[direction])
	{
		delete children[direction];
		children[direction] = updatedChild;
	}

	return this;
}

// ISSUE   FIX    TODO: don't add center of mass from particles with zero weight: e.g. end nodes with no contained particle
MassObject Internal::CalculateCenterOfMass()
{
	double massSum = 0;
	centerOfMass = glm::vec2(0);
	for (short i = 0; i < numChildren; ++i)
	{
		auto [childMass, childCOM] = children[i]->CalculateCenterOfMass();
		massSum += childMass;
		centerOfMass += childCOM * childMass;

		if (massSum != massSum || centerOfMass != centerOfMass)
			int i = 0;
	}

	mass = massSum;
	centerOfMass /= mass;
	if (massSum != massSum || centerOfMass != centerOfMass)
		int i = 0;
	return MassObject(massSum, centerOfMass);
}

void Internal::CalculateEffectingForce(Particle* inParticle)
{
	double distance = glm::distance(inParticle->position, centerOfMass);
	// TODO: Test vs 's / d < theta'
	double distanceRatio = width / distance;
	if (distanceRatio < theta)
	{
		MassObject object1 = MassObject(inParticle->mass, inParticle->position);
		MassObject object2 = MassObject(mass, centerOfMass);
		inParticle->acceleration += GetInteractionForce(object1, object2);
	}
	else
	{
		for (short i = 0; i < numChildren; ++i)
		{
			children[i]->CalculateEffectingForce(inParticle);
		}
	}
}

void Internal::GetBoundingBoxes(std::vector<BoundingBox>* boundingBoxes)
{
	for (short i = 0; i < numChildren; ++i)
	{
		boundingBoxes->push_back(BoundingBox(position, width));
		children[i]->GetBoundingBoxes(boundingBoxes);
	}
}

void Internal::CalculateNodeIDs(int& id)
{
	for (short i = 0; i < numChildren; ++i)
	{
		children[i]->CalculateNodeIDs(id);
	}
	id += 1;
	nodeID = id;
}

void Internal::GetOrderedNodes(std::vector<Node*>* nodes)
{
	for (short i = 0; i < numChildren; ++i)
	{
		children[i]->GetOrderedNodes(nodes);
	}
	nodes->push_back(this);
}




// Class End

End::End(double inWidth, float inTheta, glm::vec2 nodePosition, Particle* inParticle)
{
	theta = inTheta;
	width = inWidth;
	position = inParticle ? inParticle->position : nodePosition;
	particle = inParticle;
}

Node* End::InsertParticle(Particle* inParticle, int parentDepth)
{
	int currentDepth = parentDepth + 1;
	if (particle == nullptr && particles == nullptr)
	{
		particle = inParticle;
		return this;
	}

	if (currentDepth < MAX_RECURSION)
	{
		// Further subdivide tree section
		Internal* newInternal = new Internal(width, theta, position);
		newInternal->InsertParticle(inParticle, currentDepth);
		if (particles)
		{
			for (Particle* item : *particles)
			{
				if (!item) break;
				newInternal->InsertParticle(item, currentDepth);
			}
#ifdef DEBUG
			if (particle != nullptr)
				exit(-99);
#endif // DEBUG
		}
		else
		{
			newInternal->InsertParticle(particle, currentDepth);
			particle = nullptr;
		}

		return newInternal;
	}

	if (particles == nullptr)
	{
		particles = new std::vector<Particle*>(2);
		particles->at(0) = particle;
		particles->at(1) = inParticle;
		particle = nullptr;
	}
	else
	{
		particles->push_back(inParticle);
	}
	
	return this;
}

MassObject End::CalculateCenterOfMass()
{
	if (particle != nullptr)
		return MassObject(particle->mass, particle->position);

	else if (particles != nullptr)
	{

		double massSum = 0;
		centerOfMass = glm::vec2(0);
		for (int i = 0; i < particles->size(); ++i)
		{
			Particle* targetParticle = particles->at(i);
			if (targetParticle == nullptr) break;

			massSum += targetParticle->mass;
			centerOfMass += targetParticle->mass * targetParticle->position;

			if (massSum != massSum || centerOfMass != centerOfMass)
				int i = 0;
		}
		mass = massSum;
		centerOfMass /= mass;

		return MassObject(mass, centerOfMass);
	}

	return MassObject(0, 0);
}

void End::CalculateEffectingForce(Particle* inParticle)
{
	if (particle == inParticle) return;
	if (particle == nullptr)
	{
		if (particles == nullptr) return;

		// TODO: Investigate whether an accumulated force of zero makes more sense
		// inParticle->accumulatedForces += glm::vec2(0);
		/*
		for (int i = 0; i < particles->size(); ++i)
		{
			Particle* targetParticle = particles->at(i);
			if (targetParticle == inParticle) continue;
			if (targetParticle == nullptr) break;

			MassObject object1 = MassObject(inParticle->mass, inParticle->position);
			MassObject object2 = MassObject(targetParticle->mass, targetParticle->position);

			inParticle->accumulatedForces += GetInteractionForce(object1, object2);
		}
		*/
	}

	else
	{
		MassObject object1 = MassObject(inParticle->mass, inParticle->position);
		MassObject object2 = MassObject(particle->mass, particle->position);

		inParticle->acceleration += GetInteractionForce(object1, object2);
	}
}

void End::GetBoundingBoxes(std::vector<BoundingBox>* boundingBoxes)
{
	boundingBoxes->push_back(BoundingBox(position, width));
}

void End::CalculateNodeIDs(int& id)
{
	id += 1;
	nodeID = id;
}

void End::GetOrderedNodes(std::vector<Node*>* nodes)
{
	nodes->push_back(this);
}
