#pragma once
#include "IComponent.h"

struct GLFWwindow;

/// <summary>
/// A simple behaviour that allows movement of a gameobject with WASD, mouse,
/// and ctrl + space
/// </summary>
class PlayerMovement : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<PlayerMovement> Sptr;

	PlayerMovement();
	virtual ~PlayerMovement();

	virtual void Update(float deltaTime) override;

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(PlayerMovement);
	virtual nlohmann::json ToJson() const override;
	static PlayerMovement::Sptr FromJson(const nlohmann::json& blob);

protected:
	float _shiftMultipler;
	glm::vec2 _mouseSensitivity;
	glm::vec3 _moveSpeeds;
	glm::dvec2 _prevMousePos;
	glm::vec2 _currentRot;

	bool _isMousePressed = false;

	bool controlWithMouse = false;
};
