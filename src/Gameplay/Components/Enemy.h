#pragma once
#include "IComponent.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Light.h"

struct GLFWwindow;

using namespace Gameplay;

class Enemy : public IComponent {
public:

	typedef std::shared_ptr<Enemy> Sptr;
	Enemy() = default;

	GameObject* player;
	Scene* scene;
	GLFWwindow* window;
	Gameplay::Physics::RigidBody::Sptr body;
	glm::quat currentRot;

	glm::vec3 startPos = glm::vec3(0);
	glm::vec3 target;
	Light* enemyLight;

	//Steering Movement
	float maxVelocity = 2.0f;
	float maxRotationSpeed = 0.1f;
	glm::vec3 desiredVelocity;
	glm::vec3 targetRotation;
	float avoidanceRange = 2.5f;
	float avoidanceStrength = 1000.0f;

	void Move(float deltaTime);
	void Steering(float deltaTime);
	void AvoidanceReflect(glm::vec3 dir, float deltaTime);
	void Avoidance(glm::vec3 dir, float deltaTime);

	glm::vec3 speed = glm::vec3(0.0f);

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;
	virtual void RenderImGui() override;
	virtual nlohmann::json ToJson() const override;
	static Enemy::Sptr FromJson(const nlohmann::json& data);

	glm::vec3 playerStartingPos = glm::vec3( -9, 2, 2 );

	MAKE_TYPENAME(Enemy);
};