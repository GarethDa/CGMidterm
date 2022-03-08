#pragma once
#include "IComponent.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Light.h"

struct GLFWwindow;

using namespace Gameplay;

class Pellet : public IComponent {
public:

	typedef std::shared_ptr<Pellet> Sptr;
	Pellet() = default;

	GameObject* player;
	Scene* scene;
	GLFWwindow* window;

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;
	virtual void RenderImGui() override;
	virtual nlohmann::json ToJson() const override;
	static Pellet::Sptr FromJson(const nlohmann::json& data);

	MAKE_TYPENAME(Pellet);
};