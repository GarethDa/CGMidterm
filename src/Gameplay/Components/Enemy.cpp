#include "Gameplay/Components/Enemy.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils\GlmBulletConversions.h"
#include <GLFW/glfw3.h>

float Magnitude(glm::vec3 dir)
{
	float dirLength = (dir.x * dir.x) + (dir.y * dir.y) + (dir.z * dir.z);
	return glm::sqrt(dirLength);
}

void Enemy::Awake()
{
	scene = GetGameObject()->GetScene();
	body = GetComponent<Gameplay::Physics::RigidBody>();
	body->SetAngularFactor(glm::vec3(0, 0, 0));
	body->SetLinearVelocity(glm::vec3(0));
	body->SetAngularDamping(100.0f);
	GetGameObject()->SetPostion(startPos);

	player = scene->MainCamera->GetGameObject();
	enemyLight = &scene->Lights[0];
}

void Enemy::Update(float deltaTime)
{
	if (glm::length(player->GetPosition() - GetGameObject()->GetPosition()) < 10.0f)
	{
		target = player->GetPosition();

		if (glm::length(player->GetPosition() - GetGameObject()->GetPosition()) < 2.0f)
			player->SetPostion(playerStartingPos);

		glm::vec3 lerpedCol = glm::mix(enemyLight->Color, glm::vec3(1, 0, 0), deltaTime * 2.0f);
		enemyLight->Color = lerpedCol;
	}
	else
	{
		glm::vec3 lerpedCol = glm::mix(enemyLight->Color, glm::vec3(1, 1, 1), deltaTime * 2.0f);
		enemyLight->Color = lerpedCol;
	}

	Move(deltaTime);
}

void Enemy::RenderImGui() {

	ImGui::DragFloat3("Starting Position", &startPos.x);
}

nlohmann::json Enemy::ToJson() const {

	nlohmann::json result;
	result["StartingPosition"] = startPos;
	return result;
}

Enemy::Sptr Enemy::FromJson(const nlohmann::json& blob) {
	Enemy::Sptr result = std::make_shared<Enemy>();
	result->startPos = JsonGet(blob, "StartingPosition", result->startPos);
	return result;
}

void Enemy::Move(float deltaTime)
{
	Steering(deltaTime);

	AvoidanceReflect(body->GetLinearVelocity(), deltaTime);

	glm::vec3 vel = body->GetLinearVelocity();
	glm::vec3 leftDir = glm::vec3(-vel.y + vel.x, vel.x + vel.y, 0.0f) / 2.0f;
	glm::vec3 rightDir = glm::vec3(vel.y + vel.x, -vel.x + vel.y, 0.0f) / 2.0f;

	Avoidance(leftDir, deltaTime);
	Avoidance(rightDir, deltaTime);
	Avoidance(glm::vec3(-body->GetLinearVelocity().y, body->GetLinearVelocity().x, 0.0f), deltaTime);
	Avoidance(glm::vec3(body->GetLinearVelocity().y, -body->GetLinearVelocity().x, 0.0f), deltaTime);

	GetGameObject()->LookAt(GetGameObject()->GetPosition() + body->GetLinearVelocity() * -1.0f);
}

void Enemy::Steering(float deltaTime)
{
	glm::vec3 newVel = body->GetLinearVelocity();

	if (target == glm::vec3(0.0f))
		return;

	//Steering
	desiredVelocity = target - GetGameObject()->GetPosition();
	targetRotation = desiredVelocity - body->GetLinearVelocity();
	if (Magnitude(targetRotation) > maxRotationSpeed)
		targetRotation = (targetRotation / Magnitude(targetRotation)) * maxRotationSpeed;

	//Velocity
	newVel += targetRotation * 100.0f * deltaTime;
	if (Magnitude(newVel) > maxVelocity)
		newVel = glm::normalize(newVel) * maxVelocity;

	body->SetLinearVelocity(glm::vec3(newVel.x, newVel.y, 0.0f));
}

void Enemy::AvoidanceReflect(glm::vec3 dir, float deltaTime)
{
	//Check if greater than zero before normalizing since that would divide by 0
	if (Magnitude(dir) <= 0.0f)
		return;

	dir = glm::normalize(dir);

	//Perform Raycast
	const glm::vec3 startPoint = GetGameObject()->GetPosition();
	const glm::vec3 endPoint = GetGameObject()->GetPosition() + (dir * avoidanceRange);
	btCollisionWorld::ClosestRayResultCallback hit(ToBt(startPoint), ToBt(endPoint));
	scene->GetPhysicsWorld()->rayTest(ToBt(startPoint), ToBt(endPoint), hit);

	if (!hit.hasHit())
		return;

	//Make sure enemy doesn't avoid player
	glm::vec3 objectPos = ToGlm(hit.m_collisionObject->getWorldTransform().getOrigin());


	if (glm::round(objectPos) == glm::round(player->GetPosition()))
		return;


	//Add avoidance force
	glm::vec3 newDir = glm::reflect(dir, ToGlm(hit.m_hitNormalWorld));
	newDir = (newDir * avoidanceRange) - GetGameObject()->GetPosition();

	body->ApplyForce(glm::normalize(newDir) * avoidanceStrength * deltaTime);
}

void Enemy::Avoidance(glm::vec3 dir, float deltaTime)
{
	if (Magnitude(dir) <= 0.0f)
		return;

	dir = glm::normalize(dir);

	//Perform Raycast
	const glm::vec3 startPoint = GetGameObject()->GetPosition();
	const glm::vec3 endPoint = GetGameObject()->GetPosition() + (dir * avoidanceRange);

	btCollisionWorld::ClosestRayResultCallback hit(ToBt(startPoint), ToBt(endPoint));
	scene->GetPhysicsWorld()->rayTest(ToBt(startPoint), ToBt(endPoint), hit);

	if (!hit.hasHit())
		return;

	//Add avoidance force
	glm::vec3 newDir = glm::normalize(body->GetLinearVelocity()) - dir;
	newDir = glm::vec3(newDir.x, newDir.y, 0.0f);

	body->ApplyForce(glm::normalize(newDir) * avoidanceStrength * deltaTime);
}

