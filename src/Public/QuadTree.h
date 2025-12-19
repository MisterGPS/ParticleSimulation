#pragma once

#include <glm.hpp>
#include <vector>

#define GRAVITATIONAL_CONSTANT 0.0000000000667430
#define COEFFICIENT_OF_RESTITUTION 1.0
#define SIMULATION_SPEED 125
#define FORCE_SCALE 100.0
#define ATTRACTION_FORCE 1000.0
#define THETA 0.5f  // Value between 0 and 1. The smaller the number the more accurate the simulation. 0.5 provides a good trade off between performance and simulation accuracy. A very large value >1 approximates a single force in the screens center
#define DISTANCE_EPSILON 1
#define NUM_THREADS 12
#define MAX_RECURSION 20

typedef std::tuple<float, glm::vec2> MassObject;
typedef std::tuple<glm::vec2, float> BoundingBox;

class Particle;
class QuadTree;
class Node;
class Internal;
class End;

enum RelativeDirection
{
	NE = 0,
	NW = 1,
	SW = 2,
	SE = 3
};

class QuadTree
{
public:
	QuadTree(float inTheta = 1.0f, double maxWidth = 1.0, glm::vec2 treeCenter = glm::vec2(0.5f, 0.5f));
	~QuadTree();

	void InsertParticle(Particle* inParticle);
	void CalculateNodeIDs();
	void CalculateForces(Particle* particles, int numParticles);
	void MP_CalculateEffecingForce(Particle* particles, int start, int end);

	std::vector<Node*>* GetOrderedNodes();
	std::vector<BoundingBox>* GetBoundingBoxes();

private:
	float theta;
	Node* root;
};

class Node
{
	friend QuadTree;

public:
	virtual ~Node() {}

	virtual Node* InsertParticle(Particle* inParticle, int parentDepth) { return nullptr; }
	virtual void CalculateNodeIDs(int& i) {}
	virtual MassObject CalculateCenterOfMass();
	virtual void CalculateEffectingForce(Particle* inParticle) { glm::vec2(0); }
	virtual void GetBoundingBoxes(std::vector<BoundingBox>* boundingBoxes) {}

	virtual void GetOrderedNodes(std::vector<Node*>* nodes) {}

	static RelativeDirection GetRelativeDirection(const glm::vec2& centerPosition, const glm::vec2& compPosition);
	inline static glm::vec2 GetInteractionForce(MassObject& firstPoint, MassObject& secondPoint);

protected:
	Node() {}

	// Container properties
	int nodeID = 0;
	int numChildren = 4;
	float mass = 0;
	double width = 0;
	float theta = 0;
	glm::vec2 position = glm::vec2(0);
	glm::vec2 centerOfMass = glm::vec2(0);
};

class Internal : public Node
{
public:
	Internal(double inWidth, float inTheta, glm::vec2 nodePosition);
	~Internal() override;

	Node* InsertParticle(Particle* inParticle, int parentDepth);
	void CalculateNodeIDs(int& i) override;
	MassObject CalculateCenterOfMass() override;
	void CalculateEffectingForce(Particle* inParticle) override;
	void GetBoundingBoxes(std::vector<BoundingBox>* boundingBoxes);

	void GetOrderedNodes(std::vector<Node*>* nodes) override;

private:
	Internal();

	// Child nodes
	Node* children[4];
};

class End : public Node
{
public:
	End(double inWidth, float inTheta, glm::vec2 nodePosition, Particle* inParticle = nullptr);

	Node* InsertParticle(Particle* inParticle, int parentDepth);
	void CalculateNodeIDs(int& i) override;
	MassObject CalculateCenterOfMass() override;
	void CalculateEffectingForce(Particle* inParticle) override;
	void GetBoundingBoxes(std::vector<BoundingBox>* boundingBoxes);

	void GetOrderedNodes(std::vector<Node*>* nodes) override;

private:
	End();

	Particle* particle = nullptr;
	std::vector<Particle*>* particles = nullptr;
};
