#include <glad.h>
#include <glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <ctime>
#include <random>
#include <iostream>
#include <math.h>
#include "../Public/Particle.h"
#include "../Public/Shader.h"
#include "../Public/Camera.h"
#include "../Public/QuadTree.h"
#include "../Shaders/ShadersInComp.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define DISTANCE_SCALE 1.0f    // Unit in meters; defines the size of the window in meters
#define GRAVITY_SCALE 0.0015
#define NUM_PARTICLES 600000
#define PARTICLE_RADIUS 1
#define COEFFICIENT_OF_RESTITUTION 1.0  // 1 = elastic, 0 = perfectly inelastic
#define TOTAL_MASS 40000000
#define INITIAL_MEAN_VELOCITY 200
#define GALAXY_STDDEV_SPREAD 0.5

#define BCOLLISION NUM_PARTICLES < 1000 ? true : false
#define B_RENDER_FRAME false

#define DELTA_TIME (1.0 / 10.0)

#ifdef BENCHMARK
#define BENCHMARK_WARMUP_STEPS 50       // Steps run before measurement starts
#define BENCHMARK_STEPS 500             // Measured steps; the application closes after warmup + measured steps
#define BENCHMARK_DELTA_TIME (1.0 / 60.0)  // Fixed timestep resembling a 60fps clock
#endif


typedef std::tuple<glm::vec<2, double>, glm::vec<2, double>> Boundaries;

void windowSizeCallback(GLFWwindow* window, int width, int height);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void mouseCallback(GLFWwindow* window, double xPos, double yPos);
void mouseEnterCallback(GLFWwindow* window, int entered);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void handleKeyInput(GLFWwindow* window, double deltaTime);

void updateParticles(Particle* particles, unsigned int numParticles, double deltaTime, const Boundaries& boundaries);
void calculateForces(Particle* particles, unsigned int numParticles);
void halfKickAndDrift(Particle* particles, unsigned int numParticles, double deltaTime);
void halfKick(Particle* particles, unsigned int numParticles, double deltaTime);
void addGravityForce(Particle* particles, unsigned int numParticles, double deltaTime);
void boundariesConstraintImpulse(Particle* particles, unsigned int numParticles, double deltaTime, const Boundaries& boundaries);
void boundariesConstraintBounce(Particle* particles, unsigned int numParticles, double deltaTime, const Boundaries& boundaries);
void collisionConstraintSolver(Particle* particles, unsigned int numParticles, double deltaTime);

inline glm::vec2 getAttractiveForce(MassObject& firstPoint, MassObject& secondPoint);

void DrawBoundingBoxes(const std::vector<BoundingBox>* boxes, unsigned int VAO, unsigned int VBO, unsigned int EBO);
Particle* spawnGalaxy(Particle* particles, int numParticles, float velocity, float radius, float particleRadius, float stddev = 0, glm::vec2 center = glm::vec2(0), glm::vec2 galaxyVelocity = glm::vec2(0));  // Spawns a circle galaxy at the specified position
inline double normalDistribution(double x, double mean, double stddev);

Camera* camera = nullptr;
bool bShowQuadTree = false;
std::vector<BoundingBox>* boundingBoxes;


int main() {
#ifdef DEBUG
    std::cout << "Running debug version" << std::endl;
#elif RELEASE
    std::cout << "Running release version" << std::endl;
#endif
    
    double WindowWidth = DISTANCE_SCALE * (double)WINDOW_WIDTH;
    double WindowHeight = WINDOW_HEIGHT * (double)DISTANCE_SCALE;
    Boundaries boundaries = Boundaries(glm::vec<2, double>(WindowWidth, WindowHeight), glm::vec<2, double>(0, 0));

    float particleDiameter = PARTICLE_RADIUS;
    boundingBoxes = new std::vector<BoundingBox>();

    std::srand(std::time(0));
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Particle Simulation", nullptr, nullptr);

    if (!window) {
        std::cout << "Couldn't initialize window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize glad" << std::endl;
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, windowSizeCallback);

    // TODO: Calculate distance of camera to simulation bounds to fit image
    camera = new Camera(window, glm::vec3(0, 0, 2.42));
    camera->setCameraSpeed(3.0f);
    camera->setWindowDimensions(WINDOW_HEIGHT, WINDOW_WIDTH);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorEnterCallback(window, mouseEnterCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetKeyCallback(window, keyCallback);

    // End of glfw initialization


    Shader shader = Shader();
    Shader shaderFrame = Shader();

#ifdef RELEASE
    ShaderSource shaders = ShaderSource();
    shader.LoadShader(shaders.VertexParticle, SHADER_VERTEX);
    shader.LoadShader(shaders.FragmentParticle, SHADER_FRAGMENT);

    shaderFrame.LoadShader(shaders.VertexFrame, SHADER_VERTEX);
    shaderFrame.LoadShader(shaders.FragmentFrame, SHADER_FRAGMENT);
#else
    shader.LoadShaderFromFile("./Shaders/VertexParticle.glsl", SHADER_VERTEX);
    shader.LoadShaderFromFile("./Shaders/FragmentParticle.glsl", SHADER_FRAGMENT);

    shaderFrame.LoadShaderFromFile("./Shaders/VertexFrame.glsl", SHADER_VERTEX);
    shaderFrame.LoadShaderFromFile("./Shaders/FragmentFrame.glsl", SHADER_FRAGMENT);
#endif

    shader.BuildShaderProgram();
    shader.Use();
    shader.SetFloat("velocityMax", INITIAL_MEAN_VELOCITY * 3);

    shaderFrame.BuildShaderProgram();
    shaderFrame.Use();
    shaderFrame.SetVec4("aColor", glm::vec4(0.375f, 1.0f, 1.0f, 0.0f));
    
    unsigned int VBOfV2D, VAOfV2D, EBOfV2D;
    float frameVertices2D[8] = { -1.0, 1.0, -1.0, -1.0,
                                  1.0, -1.0, 1.0, 1.0 };

    unsigned int frameIndices[8] = { 0, 1,
                                     1, 2,
                                     2, 3,
                                     3, 0 };
    
    glGenBuffers(1, &VBOfV2D);
    glGenVertexArrays(1, &VAOfV2D);
    glBindVertexArray(VAOfV2D);
    glBindBuffer(GL_ARRAY_BUFFER, VBOfV2D);
    glBufferData(GL_ARRAY_BUFFER, sizeof(frameVertices2D), &frameVertices2D, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &EBOfV2D);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOfV2D);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(frameIndices), frameIndices, GL_STATIC_DRAW);


    Particle* particles = new Particle[NUM_PARTICLES];

    glm::vec2 galaxyCenter = glm::vec2(WindowWidth / 2.0, WindowHeight / 2.0);
    float galaxyVelocity = 0;
    float galaxyRadius = 250 * DISTANCE_SCALE * 2;
    float stddev = galaxyRadius * GALAXY_STDDEV_SPREAD;
    particles = new Particle[NUM_PARTICLES];
    spawnGalaxy(particles, NUM_PARTICLES, INITIAL_MEAN_VELOCITY, galaxyRadius, particleDiameter / 2.0f, stddev, galaxyCenter, glm::vec2(-0.4, -0.2) * galaxyVelocity);


    unsigned int ParticlesVBO, ParticlesVAO;
    glGenBuffers(1, &ParticlesVBO);
    glGenVertexArrays(1, &ParticlesVAO);
    glBindVertexArray(ParticlesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ParticlesVBO);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)nullptr);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, velocity));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, colorFast));
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, colorSlow));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);


    // Render loop
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glClearColor(0, 0, 0, 0);
    glPointSize(PARTICLE_RADIUS);
    glLineWidth(1.0f);

    glm::mat4 locTransform = glm::mat4(1.0f);
    locTransform = glm::scale(locTransform, glm::vec3(2.0f / WindowWidth, 2.0f / WindowHeight, 2.0f / WindowHeight));
    locTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-1, -1, 0)) * locTransform;


    // Pre-calculate forces for the verlet solver
    calculateForces(particles, NUM_PARTICLES);

    double loopTime = glfwGetTime();
    int frameCount = 0;
    while (!glfwWindowShouldClose(window)) 
    {
        frameCount++;

        glClear(GL_COLOR_BUFFER_BIT);

        // Update frame values
        double frameTime = (glfwGetTime() - loopTime);
        double deltaTime = frameTime * SIMULATION_SPEED;
        loopTime = glfwGetTime();  

        deltaTime = DELTA_TIME * SIMULATION_SPEED;

#ifdef BENCHMARK
        deltaTime = BENCHMARK_DELTA_TIME * SIMULATION_SPEED;
#endif

        camera->HandleCameraInput(frameTime);
        handleKeyInput(window, deltaTime);

        glm::mat4 transform = camera->GetTransformMatrix();
        glm::mat4 frameTransform = transform;
        transform = transform * locTransform;


        updateParticles(particles, NUM_PARTICLES, deltaTime, boundaries);


        // Render frame

        if (B_RENDER_FRAME)
        {
            shaderFrame.Use();
            shaderFrame.SetMat4("transformMatrix", frameTransform);
            shaderFrame.SetVec4("aColor", glm::vec4(0.375f, 1.0f, 1.0f, 0.0f));
            glBindVertexArray(VAOfV2D);
            glBindBuffer(GL_ARRAY_BUFFER, VBOfV2D);
            glBufferData(GL_ARRAY_BUFFER, sizeof(frameVertices2D), &frameVertices2D, GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOfV2D);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(frameIndices), frameIndices, GL_STATIC_DRAW);
            glDrawElements(GL_LINES, 8, GL_UNSIGNED_INT, 0);
        }

        if (bShowQuadTree)
        {
            if (!B_RENDER_FRAME)
            {
                shaderFrame.Use();
            }
            shaderFrame.SetMat4("transformMatrix", transform);
            shaderFrame.SetVec4("aColor", glm::vec4(0.0f, 1.0f, 0.1f, 0.0f));
            DrawBoundingBoxes(boundingBoxes, VAOfV2D, VBOfV2D, EBOfV2D);
        }

        shader.Use();
        shader.SetMat4("transformMatrix", transform);

        glBindVertexArray(ParticlesVAO);
        glBindBuffer(GL_ARRAY_BUFFER, ParticlesVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(particles[0]) * NUM_PARTICLES, &particles[0], GL_STATIC_DRAW);
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
        
        glfwSwapBuffers(window);
        glfwPollEvents();

#ifdef BENCHMARK
        if (frameCount >= BENCHMARK_WARMUP_STEPS + BENCHMARK_STEPS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
#endif
    }

    delete[] particles;

    glfwTerminate();

    return 0;
}

void windowSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void updateParticles(Particle* particles, unsigned int numParticles, double deltaTime, const Boundaries& boundaries)
{
    halfKickAndDrift(particles, numParticles, deltaTime);

    calculateForces(particles, numParticles);
    // 2. Apply forces
    // addGravityForce(particles, numParticles, deltaTime);

    // 3. Handle constraints
    halfKick(particles, NUM_PARTICLES, deltaTime);  // TODO: Do this as compute shader

    if (BCOLLISION)
    {
        collisionConstraintSolver(particles, NUM_PARTICLES, deltaTime);
    }

    // Boundary constraint must be activated until the QuadTrees size is dynamically adjusted to guarantee particles far out are not out of bounds
    // boundariesConstraintBounce(particles, NUM_PARTICLES, deltaTime, boundaries);  
}

void calculateForces(Particle* particles, unsigned int numParticles)
{
    float maxX, minX, maxY, minY;
    maxX = maxY = INT_MIN;
    minX = minY = INT_MAX;
    for (size_t i = 0; i < numParticles; i++)
    {
        const Particle& particle = particles[i];
        maxX = std::max(particle.position.x, maxX);
        maxY = std::max(particle.position.y, maxY);
        minX = std::min(particle.position.x, minX);
        minY = std::min(particle.position.y, minY);
    }

    maxX += DISTANCE_SCALE;
    maxY += DISTANCE_SCALE;
    minX -= DISTANCE_SCALE;
    minY -= DISTANCE_SCALE;

    double centerX = (maxX + minX) / 2.0f;
    double centerY = (maxY + minY) / 2.0f;
    glm::vec2 treeCenter = glm::vec2(centerX, centerY);
    double treeWidthX = std::max(maxX - centerX, centerX - minX);
    double treeWidthY = std::max(maxY - centerY, centerY - minY);
    float treeMaxWidth = std::max(treeWidthX, treeWidthY) * 2.0f;

    QuadTree* quadTree = new QuadTree(THETA, treeMaxWidth, treeCenter);
    for (size_t i = 0; i < NUM_PARTICLES; ++i)
    {
        quadTree->InsertParticle(&particles[i]);
    }

    // quadTree->CalculateNodeIDs();
    quadTree->CalculateForces(particles, NUM_PARTICLES);  // TODO: Optimize this. It is now multithreaded, but the calculation per particle is still really slow
    
    if (bShowQuadTree)
    {
        delete boundingBoxes;
        boundingBoxes = quadTree->GetBoundingBoxes();
    }
    delete quadTree;
}

glm::vec2 getAttractiveForce(MassObject& firstPoint, MassObject& secondPoint)
{
    auto& [mass1, position1] = firstPoint;
    auto& [mass2, position2] = secondPoint;

    glm::vec<2, double> deltaVector = (position2 - position1);
    double pointDistanceSquared = sqrt(powf(deltaVector.x, 2) + powf(deltaVector.y, 2));

    glm::vec<2, double> pointDirection = glm::normalize(deltaVector);
    double affectingForce = GRAVITATIONAL_CONSTANT * (mass1 * mass2) / pointDistanceSquared;

    return pointDirection * affectingForce * FORCE_SCALE;
}

void halfKickAndDrift(Particle* particles, unsigned int numParticles, double deltaTime)
{
#pragma omp parallel for
    for (signed int i = 0; i < numParticles; ++i)
    {
        glm::vec2 acceleration = particles[i].accelerationForce / particles[i].mass;
        glm::vec<2, double> deltaPosition = particles[i].velocity * static_cast<float>(deltaTime) + 0.5f * acceleration * static_cast<float>(deltaTime * deltaTime);
        
        particles[i].velocity += 0.5f * static_cast<float>(deltaTime) * acceleration;
        particles[i].position += static_cast<float>(deltaTime) * particles[i].velocity;
        particles[i].accelerationForce = acceleration;
    }
}

void halfKick(Particle* particles, unsigned int numParticles, double deltaTime)
{
#pragma omp parallel for
    for (signed int i = 0; i < numParticles; ++i)
    {
        glm::vec2 acceleration = particles[i].accelerationForce / particles[i].mass;
        particles[i].velocity += 0.5f * static_cast<float>(deltaTime) * acceleration;

#ifdef DEBUG
        if (i == 0)
            std::cout << "Particle 0 velocity: x" << particles[i].velocity.x << " y" << particles[i].velocity.x <<
                         "\tposition: x" << particles[i].position.x << " y" << particles[i].position.y << std::endl;
#endif
    }
}

void addGravityForce(Particle* particles, unsigned int numParticles, double deltaTime)
{
    for (uint32_t i = 0; i < numParticles; ++i)
    {
        // TODO: Investigate formula: should be m * a; a = g
        particles[i].accelerationForce += glm::vec2(0, -particles[i].mass * (9.81 * DISTANCE_SCALE * GRAVITY_SCALE) / (deltaTime * deltaTime));
    }
}

void boundariesConstraintImpulse(Particle* particles, unsigned int numParticles, double deltaTime, const Boundaries& boundaries)
{
    auto& [boundaryBox, boundaryCenter] = boundaries;

    glm::vec2 collisionNormal;
    glm::vec<2, double> deltaVelocity;
    double particleRadius;

    double rightBorder = boundaryCenter.x + boundaryBox.x;
    double leftBorder = boundaryCenter.x;
    double topBorder = boundaryCenter.y + boundaryBox.y;
    double bottomBorder = boundaryCenter.y;

    for (uint32_t i = 0; i < numParticles; ++i)
    {
        particleRadius = static_cast<double>(particles[i].radius);
        double ParticlePX = particles[i].position.x;
        double ParticlePY = particles[i].position.y;

        // smaller border to adjust for a particles radius

        if (ParticlePX + particleRadius > rightBorder)
        {
            collisionNormal = { 1.0f, 0.0f };
        }

        else if (ParticlePX - particleRadius < leftBorder)
        {
            collisionNormal = { -1.0f, 0.0f };
        }

        else if (ParticlePY + particleRadius > topBorder)
        {
            collisionNormal = { 0.0f, 1.0f };
        }
        else if (ParticlePY - particleRadius < bottomBorder)
        {
            collisionNormal = { 0.0f, -1.0f };
        }
        else continue;

        // -2 : -(1 + CoefficientOfRestitution)    Full conservation of energy
        float collisionVelocity = -(1 + COEFFICIENT_OF_RESTITUTION) * glm::dot(particles[i].velocity, collisionNormal);


        glm::vec2 Impulse = collisionVelocity * particles[i].mass * collisionNormal;
        particles[i].velocity += Impulse;
    }
}

void boundariesConstraintBounce(Particle* particles, unsigned int numParticles, double deltaTime, const Boundaries& boundaries)
{
    auto& [boundaryBox, boundaryCenter] = boundaries;
    for (uint32_t i = 0; i < numParticles; ++i)
    {
        float ParticlePX = particles[i].position.x;
        float ParticlePY = particles[i].position.y;

        float rightBorder = boundaryCenter.x + boundaryBox.x;
        float leftBorder = boundaryCenter.x;
        float topBorder = boundaryCenter.y + boundaryBox.y;
        float bottomBorder = boundaryCenter.y;

        if (ParticlePX > rightBorder)
        {
            particles[i].velocity.x = -particles[i].velocity.x;
            particles[i].position.x = rightBorder - ParticlePX + rightBorder;
        }

        else if (ParticlePX < leftBorder)
        {
            particles[i].velocity.x = -particles[i].velocity.x;
            particles[i].position.x = leftBorder - ParticlePX + leftBorder;
        }

        if (ParticlePY > topBorder)
        {
            particles[i].velocity.y = -particles[i].velocity.y;
            particles[i].position.y = topBorder - ParticlePY + topBorder;
        }
        else if (ParticlePY < bottomBorder)
        {
            particles[i].velocity.y = -particles[i].velocity.y;
            particles[i].position.y = bottomBorder - ParticlePY + bottomBorder;
        }
    }
}

void collisionConstraintSolver(Particle* particles, unsigned int numParticles, double deltaTime)
{
    for (uint32_t i = 0; i < numParticles - 1; ++i)
    {
        for (uint32_t j = i + 1; j < numParticles; ++j)
        {
            if (i == j) continue;

            // Check for collision
            Particle& particle1 = particles[i];
            Particle& particle2 = particles[j];
            double distance = glm::distance(particle1.position, particle2.position);
            if (distance < particle1.radius + particle2.radius)
            {
                glm::vec<2, double> deltaVelocity = particle1.velocity - particle2.velocity;
                glm::vec<2, double> collisionNormal = glm::normalize(particle2.position - particle1.position);
                double collisionVelocity = -(1 + COEFFICIENT_OF_RESTITUTION) * glm::dot(deltaVelocity, collisionNormal);
                
                double approachSpeed = glm::dot(deltaVelocity, collisionNormal);
                if (approachSpeed <= 0) continue;  // already separating

                double Impulse = collisionVelocity * (particle1.mass * particle2.mass / (particle1.mass + particle2.mass));
                glm::vec2 ImpulseVector = Impulse * collisionNormal;

                particle1.velocity += ImpulseVector / particle1.mass;
                particle2.velocity -= ImpulseVector / particle2.mass;
            }
        }
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    camera->mouseButtonCallback(button, action, mods);
}

void mouseCallback(GLFWwindow* window, double xPos, double yPos)
{
    camera->mouseCallback(xPos, yPos);
}

void mouseEnterCallback(GLFWwindow* window, int entered)
{
    camera->mouseEnterCallback(entered);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        bShowQuadTree = bShowQuadTree ^ 1;
    }
}

void handleKeyInput(GLFWwindow* window, double deltaTime)
{
    
}

void DrawBoundingBoxes(const std::vector<BoundingBox>* boxes, unsigned int VAO, unsigned int VBO, unsigned int EBO)
{
    int numBoxes = boxes->size();
    int arrayLength = 8 * numBoxes;
    float* vertices = new float[arrayLength];
    unsigned int* indices = new unsigned int[arrayLength];

#pragma omp parallel for
    for (signed int i = 0; i < numBoxes; ++i)
    {
        const BoundingBox& box = (*boxes)[i];

        auto& [boxPosition, dimension] = box;
        float halfDim = dimension / 2.0f;

        float newVertices[] = { boxPosition.x + halfDim, boxPosition.y + halfDim,  // NE
                                boxPosition.x + halfDim, boxPosition.y - halfDim,  // SE
                                boxPosition.x - halfDim, boxPosition.y - halfDim,  // SW
                                boxPosition.x - halfDim, boxPosition.y + halfDim};  // NW

        unsigned int offset = i * 4;
        unsigned int newIndices[] = { offset + 0, offset + 1,
                                      offset + 1, offset + 2,
                                      offset + 2, offset + 3,
                                      offset + 3, offset + 0 };

        std::copy(newVertices, newVertices + 8, &vertices[i * 8]);
        std::copy(newIndices, newIndices + 8, &indices[i * 8]);
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * numBoxes, vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 8 * numBoxes, indices, GL_STATIC_DRAW);
    glDrawElements(GL_LINES, 8 * numBoxes, GL_UNSIGNED_INT, 0);
}

Particle* spawnGalaxy(Particle* particles, int numParticles, float velocity, float radius, float particleRadius, float stddev, glm::vec2 center, glm::vec2 galaxyVelocity)
{
    std::random_device rd{}; 

    // Mersenne twister PRNG, initialized with seed from previous random device instance
    std::mt19937 gen{rd()}; 
    std::normal_distribution<double> d{radius, stddev};
    std::uniform_real_distribution<double> r(0, std::_Pi_val * 2);

#pragma omp parallel for
    for (signed int i = 0; i < numParticles; ++i)
    {
        // Particles spawn in a normal distribution distance from the radius
        double centerDeviation = d(gen);

        double randAngle = r(gen);
        double velAngle = randAngle + std::_Pi_val / 2.0;
        particles[i].position = glm::vec2(cos(randAngle), -sin(randAngle)) * (float)centerDeviation + center;
        particles[i].velocity = glm::vec2(cos(velAngle), -sin(velAngle)) * velocity + galaxyVelocity;
        particles[i].mass = TOTAL_MASS / float(NUM_PARTICLES);
        particles[i].radius = particleRadius;
    }

    return particles;
}

inline double normalDistribution(double x, double mean, double stddev)
{
    double e = std::exp(1.0);
    double exponent = -(1.0 / 2.0) * std::pow((x - mean) / stddev, 2);
    double scale = 1.0 / (stddev * std::sqrt(2.0 * std::_Pi_val));
    
    return scale * std::pow(e, exponent);
}
