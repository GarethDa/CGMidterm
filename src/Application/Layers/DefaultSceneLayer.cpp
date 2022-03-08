#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Components/Enemy.h"
#include "Gameplay/Components/Pellet.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json & config) {
	_CreateScene();
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	}
	else {
		// This time we'll have 2 different shaders, and share data between both of them using the UBO
		// This shader will handle reflective materials 
		ShaderProgram::Sptr reflectiveShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_environment_reflective.glsl" }
		});
		reflectiveShader->SetDebugName("Reflective");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr basicShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_blinn_phong_textured.glsl" }
		});
		basicShader->SetDebugName("Blinn-phong");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr specShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/textured_specular.glsl" }
		});
		specShader->SetDebugName("Textured-Specular");

		// This shader handles our foliage vertex shader example
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/screendoor_transparency.glsl" }
		});
		foliageShader->SetDebugName("Foliage");

		// This shader handles our cel shading example
		ShaderProgram::Sptr toonShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/toon_shading.glsl" }
		});
		toonShader->SetDebugName("Toon Shader");

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping");

		// This shader handles our tangent space normal mapping
		ShaderProgram::Sptr tangentSpaceMapping = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		tangentSpaceMapping->SetDebugName("Tangent Space Mapping");

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing");

		// Load in the meshes
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		MeshResource::Sptr enemyMesh = ResourceManager::CreateAsset<MeshResource>("enemy.obj");
		MeshResource::Sptr wallMesh = ResourceManager::CreateAsset<MeshResource>("wall.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    boxSpec = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    monkeyTex = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    leafTex = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    wallTexture = ResourceManager::CreateAsset<Texture2D>("textures/wall-texture.png");

		Texture2D::Sptr    floorTexture = ResourceManager::CreateAsset<Texture2D>("textures/black.png");

		Texture2D::Sptr    enemyTexture = ResourceManager::CreateAsset<Texture2D>("textures/red_ghost.png");

		Texture2D::Sptr    pelletTexture = ResourceManager::CreateAsset<Texture2D>("textures/pelletTexture.png");

		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png");
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" }
		});

		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();

		// Setting up our enviroment map
		//scene->SetSkyboxTexture(testCubemap);
		//scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr wallMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			wallMaterial->Name = "Wall";
			wallMaterial->Set("u_Material.Diffuse", wallTexture);
			wallMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr floorMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			floorMaterial->Name = "Wall";
			floorMaterial->Set("u_Material.Diffuse", floorTexture);
			floorMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr enemyMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			enemyMaterial->Name = "Wall";
			enemyMaterial->Set("u_Material.Diffuse", enemyTexture);
			enemyMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr pelletMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			pelletMaterial->Name = "Wall";
			pelletMaterial->Set("u_Material.Diffuse", pelletTexture);
			pelletMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.Diffuse", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>(reflectiveShader);
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->Set("u_Material.Diffuse", monkeyTex);
			monkeyMaterial->Set("u_Material.Shininess", 0.5f);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(specShader);
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.Diffuse", boxTexture);
			testMaterial->Set("u_Material.Specular", boxSpec);
		}

		// Our foliage vertex shader material
		Material::Sptr foliageMaterial = ResourceManager::CreateAsset<Material>(foliageShader);
		{
			foliageMaterial->Name = "Foliage Shader";
			foliageMaterial->Set("u_Material.Diffuse", leafTex);
			foliageMaterial->Set("u_Material.Shininess", 0.1f);
			foliageMaterial->Set("u_Material.Threshold", 0.1f);

			foliageMaterial->Set("u_WindDirection", glm::vec3(1.0f, 1.0f, 0.0f));
			foliageMaterial->Set("u_WindStrength", 0.5f);
			foliageMaterial->Set("u_VerticalScale", 1.0f);
			foliageMaterial->Set("u_WindSpeed", 1.0f);
		}

		// Our toon shader material
		Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(toonShader);
		{
			toonMaterial->Name = "Toon";
			toonMaterial->Set("u_Material.Diffuse", boxTexture);
			toonMaterial->Set("s_ToonTerm", toonLut);
			toonMaterial->Set("u_Material.Shininess", 0.1f);
			toonMaterial->Set("u_Material.Steps", 8);
		}


		Material::Sptr displacementTest = ResourceManager::CreateAsset<Material>(displacementShader);
		{
			Texture2D::Sptr displacementMap = ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png");
			Texture2D::Sptr normalMap = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			displacementTest->Name = "Displacement Map";
			displacementTest->Set("u_Material.Diffuse", diffuseMap);
			displacementTest->Set("s_Heightmap", displacementMap);
			displacementTest->Set("s_NormalMap", normalMap);
			displacementTest->Set("u_Material.Shininess", 0.5f);
			displacementTest->Set("u_Scale", 0.1f);
		}

		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(tangentSpaceMapping);
		{
			Texture2D::Sptr normalMap = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.Diffuse", diffuseMap);
			normalmapMat->Set("s_NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr multiTextureMat = ResourceManager::CreateAsset<Material>(multiTextureShader);
		{
			Texture2D::Sptr sand = ResourceManager::CreateAsset<Texture2D>("textures/terrain/sand.png");
			Texture2D::Sptr grass = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");

			multiTextureMat->Name = "Multitexturing";
			multiTextureMat->Set("u_Material.DiffuseA", sand);
			multiTextureMat->Set("u_Material.DiffuseB", grass);
			multiTextureMat->Set("u_Material.Shininess", 0.5f);
			multiTextureMat->Set("u_Scale", 0.1f);
		}

		// Create some lights for our scene
		scene->Lights.resize(3);
		scene->Lights[0].Position = glm::vec3(0.0f, 1.0f, 20.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 500.0f;

		scene->Lights[1].Position = glm::vec3(1.0f, 0.0f, 3.0f);
		scene->Lights[1].Color = glm::vec3(0.2f, 0.8f, 0.1f);

		scene->Lights[2].Position = glm::vec3(0.0f, 1.0f, 3.0f);
		scene->Lights[2].Color = glm::vec3(1.0f, 0.2f, 0.1f);

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		MeshResource::Sptr cube = ResourceManager::CreateAsset<MeshResource>();
		cube->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
		cube->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion({ -9, 2, 2 });
			camera->LookAt(glm::vec3(0.0f));

			camera->Add<SimpleCameraControl>();


			RenderComponent::Sptr renderer = camera->Add<RenderComponent>();
			renderer->SetMesh(monkeyMesh);
			//renderer->SetMaterial(monkeyMaterial);
			
			/*
			RigidBody::Sptr physics = camera->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(SphereCollider::Create(0.3f));
			physics->SetMass(0.0000001f);
			*/

			// This is now handled by scene itself!
			//Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			//scene->MainCamera = cam;
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(20.0f)));
			tiledMesh->GenerateMesh();

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(floorMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(BoxCollider::Create(glm::vec3(50.0f, 50.0f, 1.0f)))->SetPosition({ 0,0,-1 });
		}

		/*
		GameObject::Sptr monkey1 = scene->CreateGameObject("Monkey 1");
		{
			// Set position in the scene
			monkey1->SetPostion(glm::vec3(1.5f, 0.0f, 1.0f));

			// Add some behaviour that relies on the physics body
			monkey1->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = monkey1->Add<RenderComponent>();
			renderer->SetMesh(monkeyMesh);
			renderer->SetMaterial(monkeyMaterial);

			// Example of a trigger that interacts with static and kinematic bodies as well as dynamic bodies
			TriggerVolume::Sptr trigger = monkey1->Add<TriggerVolume>();
			trigger->SetFlags(TriggerTypeFlags::Statics | TriggerTypeFlags::Kinematics);
			trigger->AddCollider(BoxCollider::Create(glm::vec3(1.0f)));

			monkey1->Add<TriggerVolumeEnterBehaviour>();
		}
		*/

		GameObject::Sptr wall1 = scene->CreateGameObject("Wall 1");
		{
			// Set position in the scene
			wall1->SetPostion(glm::vec3(-7.0f, 5.0f, 5.0f));
			wall1->SetScale(glm::vec3(10.0f, 1.0f, 10.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = wall1->Add<RenderComponent>();
			renderer->SetMesh(cube);
			renderer->SetMaterial(wallMaterial);

			RigidBody::Sptr physics = wall1->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create({ 40.0f, 1.0f, 10.0f }));
			physics->SetMass(1000000.0f);

		}

		GameObject::Sptr wall7 = scene->CreateGameObject("Wall 7");
		{
			// Set position in the scene
			wall7->SetPostion(glm::vec3(3.0f, 5.0f, 5.0f));
			wall7->SetScale(glm::vec3(10.0f, 1.0f, 10.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = wall7->Add<RenderComponent>();
			renderer->SetMesh(cube);
			renderer->SetMaterial(wallMaterial);

			RigidBody::Sptr physics = wall7->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create({ 40.0f, 1.0f, 10.0f }));
			physics->SetMass(1000000.0f);

		}

		GameObject::Sptr wall8 = scene->CreateGameObject("Wall 8");
		{
			// Set position in the scene
			wall8->SetPostion(glm::vec3(13.0f, 5.0f, 5.0f));
			wall8->SetScale(glm::vec3(10.0f, 1.0f, 10.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = wall8->Add<RenderComponent>();
			renderer->SetMesh(cube);
			renderer->SetMaterial(wallMaterial);

			RigidBody::Sptr physics = wall8->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create({ 40.0f, 1.0f, 10.0f }));
			physics->SetMass(1000000.0f);

		}

		GameObject::Sptr wall9 = scene->CreateGameObject("Wall 9");
		{
			// Set position in the scene
			wall9->SetPostion(glm::vec3(23.0f, 5.0f, 5.0f));
			wall9->SetScale(glm::vec3(10.0f, 1.0f, 10.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = wall9->Add<RenderComponent>();
			renderer->SetMesh(cube);
			renderer->SetMaterial(wallMaterial);

			RigidBody::Sptr physics = wall9->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create({ 40.0f, 1.0f, 10.0f }));
			physics->SetMass(1000000.0f);

		}

		GameObject::Sptr wall2 = scene->CreateGameObject("Wall 2");
		{
			// Set position in the scene
			wall2->SetPostion(glm::vec3(-8.0f, -5.0f, 5.0f));
			wall2->SetScale(glm::vec3(10.0f, 1.0f, 10.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = wall2->Add<RenderComponent>();
			renderer->SetMesh(cube);
			renderer->SetMaterial(wallMaterial);

			RigidBody::Sptr physics = wall2->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create({ 30.0f, 1.0f, 10.0f }));
			physics->SetMass(1000000.0f);
		}

		GameObject::Sptr wall10 = scene->CreateGameObject("Wall 10");
		{
			// Set position in the scene
			wall10->SetPostion(glm::vec3(2.0f, -5.0f, 5.0f));
			wall10->SetScale(glm::vec3(10.0f, 1.0f, 10.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = wall10->Add<RenderComponent>();
			renderer->SetMesh(cube);
			renderer->SetMaterial(wallMaterial);

			RigidBody::Sptr physics = wall10->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create({ 30.0f, 1.0f, 10.0f }));
			physics->SetMass(1000000.0f);
		}

		GameObject::Sptr wall11 = scene->CreateGameObject("Wall 10");
		{
			// Set position in the scene
			wall11->SetPostion(glm::vec3(12.0f, -5.0f, 5.0f));
			wall11->SetScale(glm::vec3(10.0f, 1.0f, 10.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = wall11->Add<RenderComponent>();
			renderer->SetMesh(cube);
			renderer->SetMaterial(wallMaterial);

			RigidBody::Sptr physics = wall11->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create({ 30.0f, 1.0f, 10.0f }));
			physics->SetMass(1000000.0f);
		}

		GameObject::Sptr wall3 = scene->CreateGameObject("Wall 3");
		{
			// Set position in the scene
			wall3->SetPostion(glm::vec3(26.5f, -10.0f, 5.0f));
			wall3->SetScale(glm::vec3(30.0f, 1.0f, 10.0f));
			wall3->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = wall3->Add<RenderComponent>();
			renderer->SetMesh(cube);
			renderer->SetMaterial(wallMaterial);

			RigidBody::Sptr physics = wall3->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create({ 30.0f, 1.0f, 10.0f }));
			physics->SetMass(1000000.0f);
		}

		GameObject::Sptr wall4 = scene->CreateGameObject("Wall 4");
		{
			// Set position in the scene
			wall4->SetPostion(glm::vec3(-13.5f, 0.0f, 5.0f));
			wall4->SetScale(glm::vec3(15.0f, 1.0f, 10.0f));
			wall4->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = wall4->Add<RenderComponent>();
			renderer->SetMesh(cube);
			renderer->SetMaterial(wallMaterial);

			RigidBody::Sptr physics = wall4->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create({ 15.0f, 1.0f, 10.0f }));
			physics->SetMass(1000000.0f);
		}

		GameObject::Sptr wall5 = scene->CreateGameObject("Wall 5");
		{
			// Set position in the scene
			wall5->SetPostion(glm::vec3(20.0f, -24.0f, 5.0f));
			wall5->SetScale(glm::vec3(15.0f, 1.0f, 10.0f));
			//wall5->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = wall5->Add<RenderComponent>();
			renderer->SetMesh(cube);
			renderer->SetMaterial(wallMaterial);

			RigidBody::Sptr physics = wall5->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create({ 15.0f, 1.0f, 10.0f }));
			physics->SetMass(1000000.0f);
		}

		GameObject::Sptr wall6 = scene->CreateGameObject("Wall 6");
		{
			// Set position in the scene
			wall6->SetPostion(glm::vec3(16.0f, -15.0f, 5.0f));
			wall6->SetScale(glm::vec3(21.0f, 1.0f, 10.0f));
			wall6->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = wall6->Add<RenderComponent>();
			renderer->SetMesh(cube);
			renderer->SetMaterial(wallMaterial);

			RigidBody::Sptr physics = wall6->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create({ 21.0f, 1.0f, 10.0f }));
			physics->SetMass(1000000.0f);
		}

		//GameObject::Sptr demoBase = scene->CreateGameObject("Demo Parent");

		/*
		// Box to showcase the specular material
		GameObject::Sptr specBox = scene->CreateGameObject("Specular Object");
		{
			MeshResource::Sptr boxMesh = ResourceManager::CreateAsset<MeshResource>();
			boxMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			boxMesh->GenerateMesh();

			// Set and rotation position in the scene
			specBox->SetPostion(glm::vec3(0, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = specBox->Add<RenderComponent>();
			renderer->SetMesh(boxMesh);
			renderer->SetMaterial(testMaterial);

			demoBase->AddChild(specBox);
		}

		// sphere to showcase the foliage material
		GameObject::Sptr foliageBall = scene->CreateGameObject("Foliage Sphere");
		{
			// Set and rotation position in the scene
			foliageBall->SetPostion(glm::vec3(-4.0f, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = foliageBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(foliageMaterial);

			demoBase->AddChild(foliageBall);
		}

		// Box to showcase the foliage material
		GameObject::Sptr foliageBox = scene->CreateGameObject("Foliage Box");
		{
			MeshResource::Sptr box = ResourceManager::CreateAsset<MeshResource>();
			box->AddParam(MeshBuilderParam::CreateCube(glm::vec3(0, 0, 0.5f), ONE));
			box->GenerateMesh();

			// Set and rotation position in the scene
			foliageBox->SetPostion(glm::vec3(-6.0f, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = foliageBox->Add<RenderComponent>();
			renderer->SetMesh(box);
			renderer->SetMaterial(foliageMaterial);

			demoBase->AddChild(foliageBox);
		}


		// Box to showcase the specular material
		GameObject::Sptr toonBall = scene->CreateGameObject("Toon Object");
		{
			// Set and rotation position in the scene
			toonBall->SetPostion(glm::vec3(-2.0f, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = toonBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(toonMaterial);

			demoBase->AddChild(toonBall);
		}

		GameObject::Sptr displacementBall = scene->CreateGameObject("Displacement Object");
		{
			// Set and rotation position in the scene
			displacementBall->SetPostion(glm::vec3(2.0f, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = displacementBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(displacementTest);

			demoBase->AddChild(displacementBall);
		}
		*/
		GameObject::Sptr multiTextureBall = scene->CreateGameObject("Multitextured Object");
		{
			// Set and rotation position in the scene 
			multiTextureBall->SetPostion(glm::vec3(-2.0f, 0.5f, 1.0f));

			// Add a render component 
			RenderComponent::Sptr renderer = multiTextureBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(pelletMaterial);

			Pellet::Sptr pell = multiTextureBall->Add<Pellet>();
		}

		GameObject::Sptr multiTextureBall2 = scene->CreateGameObject("Multitextured Object2");
		{
			// Set and rotation position in the scene 
			multiTextureBall2->SetPostion(glm::vec3(6.0f, 0.0f, 1.0f));

			// Add a render component 
			RenderComponent::Sptr renderer = multiTextureBall2->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(pelletMaterial);

			Pellet::Sptr pell = multiTextureBall2->Add<Pellet>();
		}

		GameObject::Sptr multiTextureBall3 = scene->CreateGameObject("Multitextured Object3");
		{
			// Set and rotation position in the scene 
			multiTextureBall3->SetPostion(glm::vec3(21.0f, 0.1f, 1.0f));

			// Add a render component 
			RenderComponent::Sptr renderer = multiTextureBall3->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(pelletMaterial);

			Pellet::Sptr pell = multiTextureBall3->Add<Pellet>();
		}

		GameObject::Sptr multiTextureBall4 = scene->CreateGameObject("Multitextured Object4");
		{
			// Set and rotation position in the scene 
			multiTextureBall4->SetPostion(glm::vec3(21.0f, -9.0f, 1.0f));

			// Add a render component 
			RenderComponent::Sptr renderer = multiTextureBall4->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(pelletMaterial);

			Pellet::Sptr pell = multiTextureBall4->Add<Pellet>();
		}


		GameObject::Sptr multiTextureBall5 = scene->CreateGameObject("Multitextured Object5");
		{
			// Set and rotation position in the scene 
			multiTextureBall5->SetPostion(glm::vec3(21.0f, -20.0f, 1.0f));

			// Add a render component 
			RenderComponent::Sptr renderer = multiTextureBall5->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(pelletMaterial);

			Pellet::Sptr pell = multiTextureBall5->Add<Pellet>();
		}

		GameObject::Sptr normalMapBall = scene->CreateGameObject("Normal Mapped Object");
		{
			// Set and rotation position in the scene 
			normalMapBall->SetPostion(glm::vec3(6.0f, -4.0f, 1.0f));
			normalMapBall->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

			// Add a render component 
			RenderComponent::Sptr renderer = normalMapBall->Add<RenderComponent>();
			renderer->SetMesh(enemyMesh);
			renderer->SetMaterial(enemyMaterial);

			// Add a render component 
			RigidBody::Sptr rb = normalMapBall->Add<RigidBody>();
			rb->SetType(RigidBodyType::Dynamic);
			rb->AddCollider(SphereCollider::Create(1.05f));

			Enemy::Sptr enemy = normalMapBall->Add<Enemy>();
		}


		// Create a trigger volume for testing how we can detect collisions with objects!
		GameObject::Sptr trigger = scene->CreateGameObject("Trigger");
		{
			TriggerVolume::Sptr volume = trigger->Add<TriggerVolume>();
			CylinderCollider::Sptr collider = CylinderCollider::Create(glm::vec3(3.0f, 3.0f, 1.0f));
			collider->SetPosition(glm::vec3(0.0f, 0.0f, 0.5f));
			volume->AddCollider(collider);

			trigger->Add<TriggerVolumeEnterBehaviour>();
		}

		/////////////////////////// UI //////////////////////////////
		/*
		GameObject::Sptr canvas = scene->CreateGameObject("UI Canvas");
		{
			RectTransform::Sptr transform = canvas->Add<RectTransform>();
			transform->SetMin({ 16, 16 });
			transform->SetMax({ 256, 256 });

			GuiPanel::Sptr canPanel = canvas->Add<GuiPanel>();


			GameObject::Sptr subPanel = scene->CreateGameObject("Sub Item");
			{
				RectTransform::Sptr transform = subPanel->Add<RectTransform>();
				transform->SetMin({ 10, 10 });
				transform->SetMax({ 128, 128 });

				GuiPanel::Sptr panel = subPanel->Add<GuiPanel>();
				panel->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/upArrow.png"));

				Font::Sptr font = ResourceManager::CreateAsset<Font>("fonts/Roboto-Medium.ttf", 16.0f);
				font->Bake();

				GuiText::Sptr text = subPanel->Add<GuiText>();
				text->SetText("Hello world!");
				text->SetFont(font);

				monkey1->Get<JumpBehaviour>()->Panel = text;
			}

			canvas->AddChild(subPanel);
		}
		*/

		/*
		GameObject::Sptr particles = scene->CreateGameObject("Particles");
		{
			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();
			particleManager->AddEmitter(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 10.0f), 10.0f, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		}
		*/

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}
