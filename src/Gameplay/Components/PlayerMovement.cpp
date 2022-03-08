#include "PlayerMovement.h"
#include <GLFW/glfw3.h>
#define  GLM_SWIZZLE
#include <GLM/gtc/quaternion.hpp>

#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"
#include "Application/Application.h"

PlayerMovement::PlayerMovement() :
	IComponent(),
	_mouseSensitivity({ 0.5f, 0.3f }),
	_moveSpeeds(glm::vec3(1.0f)),
	_shiftMultipler(2.0f),
	_currentRot(glm::vec2(0.0f)),
	_isMousePressed(false)
{ }

PlayerMovement::~PlayerMovement() = default;

void PlayerMovement::Update(float deltaTime)
{
	if (Application::Get().IsFocused) {

		Gameplay::GameObject* thisObject = GetGameObject();

		glm::vec3 input = glm::vec3(0.0f);
		if (InputEngine::IsKeyDown(GLFW_KEY_W)) {
			input.z -= _moveSpeeds.x;
		}
		if (InputEngine::IsKeyDown(GLFW_KEY_S)) {
			input.z += _moveSpeeds.x;
		}
		if (InputEngine::IsKeyDown(GLFW_KEY_A)) {
			input.x -= _moveSpeeds.y;
		}
		if (InputEngine::IsKeyDown(GLFW_KEY_D)) {
			input.x += _moveSpeeds.y;
		}
		if (InputEngine::IsKeyDown(GLFW_KEY_LEFT_CONTROL)) {
			input.y -= _moveSpeeds.z;
		}
		if (InputEngine::IsKeyDown(GLFW_KEY_SPACE)) {
			input.y += _moveSpeeds.z;
		}
		if (InputEngine::IsKeyDown(GLFW_KEY_LEFT_SHIFT)) {
			input *= _shiftMultipler;
		}

		input *= deltaTime;

		glm::vec3 worldMovement = glm::vec4(input, 1.0f);
		GetGameObject()->SetPostion(GetGameObject()->GetPosition() + worldMovement);

		if (score <= 0)
		{
			GetGameObject()->SetPostion(glm::vec3(100, 100, 100));
			std::cout << "\n\n\nYOU WIN!!!!!!";
		}
	}
}

void PlayerMovement::RenderImGui()
{

}

nlohmann::json PlayerMovement::ToJson() const {
	return {
		{ "mouse_sensitivity", _mouseSensitivity },
		{ "move_speed", _moveSpeeds },
		{ "shift_mult", _shiftMultipler }
	};
}

PlayerMovement::Sptr PlayerMovement::FromJson(const nlohmann::json & blob) {

	return PlayerMovement::Sptr();
}
