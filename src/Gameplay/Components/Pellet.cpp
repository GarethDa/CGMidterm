#include "Pellet.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils\GlmBulletConversions.h"
#include <GLFW/glfw3.h>
#include "Gameplay/Components/SimpleCameraControl.h"

void Pellet::Awake()
{
	scene = GetGameObject()->GetScene();
	player = scene->MainCamera->GetGameObject();
}

void Pellet::Update(float deltaTime)
{
	if (glm::length(player->GetPosition() - GetGameObject()->GetPosition()) < 3.0f)
	{
		player->Get<SimpleCameraControl>()->score--;
		std::cout << "\n\nREMAINING PELLETS " << player->Get<SimpleCameraControl>()->score;
		GetGameObject()->SetPostion(glm::vec3(-100.0f));
	}
}

void Pellet::RenderImGui() {

	//ImGui::DragFloat3("Starting Position", &startPos.x);
}

nlohmann::json Pellet::ToJson() const {

	nlohmann::json result;
	//result["StartingPosition"] = startPos;
	return result;


}

Pellet::Sptr Pellet::FromJson(const nlohmann::json& blob) {
	Pellet::Sptr result = std::make_shared<Pellet>();
	//result->startPos = JsonGet(blob, "StartingPosition", result->startPos);
	return result;
}
