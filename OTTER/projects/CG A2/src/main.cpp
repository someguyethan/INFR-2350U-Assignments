//Just a simple handler for simple initialization stuffs
#include "Utilities/BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <Texture2D.h>
#include <Texture2DData.h>
#include <MeshBuilder.h>
#include <MeshFactory.h>
#include <NotObjLoader.h>
#include <ObjLoader.h>
#include <VertexTypes.h>
#include <ShaderMaterial.h>
#include <RendererComponent.h>
#include <TextureCubeMap.h>
#include <TextureCubeMapData.h>

#include <Timing.h>
#include <GameObjectTag.h>
#include <InputHelpers.h>

#include <IBehaviour.h>
#include <CameraControlBehaviour.h>
#include <FollowPathBehaviour.h>
#include <SimpleMoveBehaviour.h>

int main() {
	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select cube by default
	std::vector<GameObject> controllables;

	BackendHandler::InitAll();

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(BackendHandler::GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Push another scope so most memory should be freed *before* we exit the app
	{
		#pragma region Shader and ImGui
		Shader::sptr passthroughShader = Shader::Create();
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
		passthroughShader->Link();

		Shader::sptr colourCorrectionShader = Shader::Create();
		colourCorrectionShader->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		colourCorrectionShader->LoadShaderPartFromFile("shaders/Post/colour_correction_frag.glsl", GL_FRAGMENT_SHADER);
		colourCorrectionShader->Link();

		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		glm::vec3		lightPos = glm::vec3(0.0f, 0.0f, 2.0f);
		glm::vec3		lightCol = glm::vec3(1.0f);
		float			lightAmbientPow = 1.0f;
		float			lightSpecularPow = 1.0f;
		glm::vec3		ambientCol = glm::vec3(1.0f);
		float			ambientPow = 0.5f;
		float			lightLinearFalloff = 0.09f;
		float			lightQuadraticFalloff = 0.032f;
		int				diffuseFactor = 1;
		int				ambientFactor = 1;
		int				specularFactor = 1;
		int				toonFactor = 1;

		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader->SetUniform("u_LightPos", lightPos);
		shader->SetUniform("u_LightCol", lightCol);
		shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader->SetUniform("u_AmbientCol", ambientCol);
		shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
		shader->SetUniform("u_DiffuseFactor", diffuseFactor);
		shader->SetUniform("u_AmbientFactor", ambientFactor);
		shader->SetUniform("u_SpecularFactor", specularFactor);
		shader->SetUniform("u_ToonFactor", toonFactor);

		//Effects
		PostEffect* basicEffect;

		int activeEffect = 0;
		std::vector<PostEffect*> effects;
		std::vector<LUT3D> LUTs;

		SepiaEffect* sepiaEffect;

		GreyscaleEffect* greyscaleEffect;

		ColourCorrectionEffect* colourCorrect;

		LUT3D testCube("cubes/test.cube");
		LUTs.push_back(testCube);
		LUT3D coolCube("cubes/Cool LUT.cube");
		LUTs.push_back(coolCube);
		LUT3D warmCube("cubes/Warm LUT.cube");
		LUTs.push_back(warmCube);
		// We'll add some ImGui controls to control our shader
		BackendHandler::imGuiCallbacks.push_back([&]() {
			if (ImGui::CollapsingHeader("Effect Controls"))
			{
				ImGui::SliderInt("Chosen Effect", &activeEffect, 0, LUTs.size() - 1);
				if (activeEffect == 2)
				{
					ImGui::Text("Active Effect: Warm Effect");
				}
				if (activeEffect == 1)
				{
					ImGui::Text("Active Effect: Cool Effect");
				}
				if (activeEffect == 0)
				{
					ImGui::Text("Active Effect: Custom Effect");
				}
			}
			if (ImGui::CollapsingHeader("Environment generation"))
			{
				if (ImGui::Button("Regenerate Environment", ImVec2(200.0f, 40.0f)))
				{
					EnvironmentGenerator::RegenerateEnvironment();
				}
			}
			if (ImGui::CollapsingHeader("Scene Level Lighting Settings"))
			{
				if (ImGui::ColorPicker3("Ambient Color", glm::value_ptr(ambientCol))) {
					shader->SetUniform("u_AmbientCol", ambientCol);
				}
				if (ImGui::SliderFloat("Fixed Ambient Power", &ambientPow, 0.01f, 1.0f)) {
					shader->SetUniform("u_AmbientStrength", ambientPow);
				}
			}
			if (ImGui::CollapsingHeader("Light Level Lighting Settings"))
			{
				if (ImGui::DragFloat3("Light Pos", glm::value_ptr(lightPos), 0.01f, -10.0f, 10.0f)) {
					shader->SetUniform("u_LightPos", lightPos);
				}
				if (ImGui::ColorPicker3("Light Col", glm::value_ptr(lightCol))) {
					shader->SetUniform("u_LightCol", lightCol);
				}
				if (ImGui::SliderFloat("Light Ambient Power", &lightAmbientPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				}
				if (ImGui::SliderFloat("Light Specular Power", &lightSpecularPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				}
				if (ImGui::DragFloat("Light Linear Falloff", &lightLinearFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
				}
				if (ImGui::DragFloat("Light Quadratic Falloff", &lightQuadraticFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
				}
			}

			auto name = controllables[selectedVao].get<GameObjectTag>().Name;
			ImGui::Text(name.c_str());
			auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
			ImGui::Checkbox("Relative Rotation", &behaviour->Relative);

			ImGui::Text("Q/E -> Yaw\nLeft/Right -> Roll\nUp/Down -> Pitch\nY -> Toggle Mode");
		
			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);

			if (ImGui::Button("Diffuse"))
			{
				if (diffuseFactor == 1)
					diffuseFactor = 0;
				else if (diffuseFactor == 0)
					diffuseFactor = 1;
				shader->SetUniform("u_DiffuseFactor", diffuseFactor);
			}
			if (ImGui::Button("Ambient"))
			{
				if (ambientFactor == 1)
					ambientFactor = 0;
				else if (ambientFactor == 0)
					ambientFactor = 1;
				shader->SetUniform("u_AmbientFactor", ambientFactor);
			}
			if (ImGui::Button("Specular"))
			{
				if (specularFactor == 1)
					specularFactor = 0;
				else if (specularFactor == 0)
					specularFactor = 1;
				shader->SetUniform("u_SpecularFactor", specularFactor);
			}
			if (ImGui::Button("Toon"))
			{
				if (toonFactor == 1)
					toonFactor = 0;
				else if (toonFactor == 0)
					toonFactor = 1;
				shader->SetUniform("u_ToonFactor", toonFactor);
			}
			});

		#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 

		#pragma region TEXTURE LOADING

		// Load some textures from files
		Texture2D::sptr diffuse = Texture2D::LoadFromFile("images/Stone_001_Diffuse.png");
		Texture2D::sptr diffuse2 = Texture2D::LoadFromFile("images/box.bmp");
		Texture2D::sptr specular = Texture2D::LoadFromFile("images/Stone_001_Specular.png");
		Texture2D::sptr reflectivity = Texture2D::LoadFromFile("images/box-reflections.bmp");
		Texture2D::sptr islandTex = Texture2D::LoadFromFile("images/plains_island_texture.png");
		Texture2D::sptr swordTex = Texture2D::LoadFromFile("images/Sword.png");
		Texture2D::sptr stoneTex = Texture2D::LoadFromFile("images/stone_tex.JPG");

		// Load the cube map
		//TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/sample.jpg");
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ToonSky.jpg"); 

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();  
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

		#pragma endregion

		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
		#pragma region Scene Generation
		
		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// Create a material and set some properties for it
		ShaderMaterial::sptr material0 = ShaderMaterial::Create();
		material0->Shader = shader;
		material0->Set("s_Diffuse", diffuse);
		material0->Set("s_Diffuse2", diffuse2);
		material0->Set("s_Specular", specular);
		material0->Set("u_Shininess", 8.0f);
		material0->Set("u_TextureMix", 0.5f);

		ShaderMaterial::sptr islandMat = ShaderMaterial::Create();
		islandMat->Shader = shader;
		islandMat->Set("s_Diffuse", islandTex);
		islandMat->Set("s_Diffuse2", islandTex);
		islandMat->Set("s_Specular", specular);
		islandMat->Set("u_Shininess", 8.0f);
		islandMat->Set("u_TextureMix", 0.5f);

		ShaderMaterial::sptr swordMat = ShaderMaterial::Create();
		swordMat->Shader = shader;
		swordMat->Set("s_Diffuse", swordTex);
		swordMat->Set("s_Diffuse2", swordTex);
		swordMat->Set("s_Specular", specular);
		swordMat->Set("u_Shininess", 8.0f);
		swordMat->Set("u_TextureMix", 0.5f);

		ShaderMaterial::sptr stoneMat = ShaderMaterial::Create();
		stoneMat->Shader = shader;
		stoneMat->Set("s_Diffuse", stoneTex);
		stoneMat->Set("s_Diffuse2", stoneTex);
		stoneMat->Set("s_Specular", specular);
		stoneMat->Set("u_Shininess", 8.0f);
		stoneMat->Set("u_TextureMix", 0.5f);

		GameObject obj2 = scene->CreateEntity("monkey_quads");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/monkey_quads.obj");
			obj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(stoneMat);
			obj2.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			obj2.get<Transform>().SetLocalRotation(0.0f, 0.0f, -90.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj2);
		}

		GameObject islandObj = scene->CreateEntity("scene_geo");
		{
			VertexArrayObject::sptr sceneVao = ObjLoader::LoadFromFile("models/plains island.obj");
			islandObj.emplace<RendererComponent>().SetMesh(sceneVao).SetMaterial(islandMat);
			islandObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			islandObj.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(islandObj);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 0.0f, 0.0f, 0.1f });
			pathing->Points.push_back({ 0.0f, 0.0f, 0.0f });
			pathing->Speed = 0.05f;
		}

		GameObject islandObj2 = scene->CreateEntity("scene_geo");
		{
			VertexArrayObject::sptr sceneVao = ObjLoader::LoadFromFile("models/plains island.obj");
			islandObj2.emplace<RendererComponent>().SetMesh(sceneVao).SetMaterial(islandMat);
			islandObj2.get<Transform>().SetLocalPosition(50.0f, 40.0f, 10.0f);
			islandObj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(islandObj2);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 50.0f, 40.0f, 13.0f });
			pathing->Points.push_back({ 50.0f, 40.0f, 10.0f });
			pathing->Speed = 1.0f;
		}

		GameObject islandObj3 = scene->CreateEntity("scene_geo");
		{
			VertexArrayObject::sptr sceneVao = ObjLoader::LoadFromFile("models/plains island.obj");
			islandObj3.emplace<RendererComponent>().SetMesh(sceneVao).SetMaterial(islandMat);
			islandObj3.get<Transform>().SetLocalPosition(-50.0f, -40.0f, 11.0f);
			islandObj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			islandObj3.get<Transform>().SetLocalScale(glm::vec3(0.5f));

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(islandObj3);
			// Set up a path for the object to follow
			pathing->Points.push_back({ -50.0f, -40.0f, 14.0f });
			pathing->Points.push_back({ -50.0f, -40.0f, 11.0f });
			pathing->Speed = 1.0f;
		}

		GameObject islandObj4 = scene->CreateEntity("scene_geo");
		{
			VertexArrayObject::sptr sceneVao = ObjLoader::LoadFromFile("models/plains island.obj");
			islandObj4.emplace<RendererComponent>().SetMesh(sceneVao).SetMaterial(islandMat);
			islandObj4.get<Transform>().SetLocalPosition(-50.0f, 40.0f, 5.0f);
			islandObj4.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			islandObj4.get<Transform>().SetLocalScale(glm::vec3(0.75f));

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(islandObj4);
			// Set up a path for the object to follow
			pathing->Points.push_back({ -50.0f, 40.0f, 8.0f });
			pathing->Points.push_back({ -50.0f, 40.0f, 5.0f });
			pathing->Speed = 1.0f;
		}

		GameObject islandObj5 = scene->CreateEntity("scene_geo");
		{
			VertexArrayObject::sptr sceneVao = ObjLoader::LoadFromFile("models/plains island.obj");
			islandObj5.emplace<RendererComponent>().SetMesh(sceneVao).SetMaterial(islandMat);
			islandObj5.get<Transform>().SetLocalPosition(50.0f, -40.0f, 8.0f);
			islandObj5.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			islandObj5.get<Transform>().SetLocalScale(glm::vec3(1.5f));

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(islandObj5);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 50.0f, -40.0f, 11.0f });
			pathing->Points.push_back({ 50.0f, -40.0f, 8.0f });
			pathing->Speed = 1.0f;
		}

		GameObject swordObj = scene->CreateEntity("sword");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Sword.obj");
			swordObj.emplace<RendererComponent>().SetMesh(vao).SetMaterial(swordMat);
			swordObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 2.5f);
			swordObj.get<Transform>().SetLocalRotation(90.0f, 170.0f, 0.0f);
			swordObj.get<Transform>().SetLocalScale(0.1f, 0.1f, 0.1f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(swordObj);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(swordObj);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 0.0f, 0.0f, 2.6f });
			pathing->Points.push_back({ 0.0f, 0.0f, 2.5f });
			pathing->Speed = 0.05f;
		}

		GameObject stoneObj = scene->CreateEntity("stone");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/monkey.obj");
			stoneObj.emplace<RendererComponent>().SetMesh(vao).SetMaterial(stoneMat);
			stoneObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, -0.3f);
			stoneObj.get<Transform>().SetLocalRotation(0.0f, 0.0f, 0.0f);
			stoneObj.get<Transform>().SetLocalScale(glm::vec3(2.0f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(stoneObj);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(stoneObj);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 0.0f, 0.0f, -0.2f });
			pathing->Points.push_back({ 0.0f, 0.0f, -0.3f });
			pathing->Speed = 0.05f;
		}

		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			cameraObject.get<Transform>().SetLocalPosition(0, 3, 3).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 3, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}

		int width, height;
		glfwGetWindowSize(BackendHandler::window, &width, &height);

		GameObject colourCorrectObject = scene->CreateEntity("Colour Correct Effect");
		{
			colourCorrect = &colourCorrectObject.emplace<ColourCorrectionEffect>();
			colourCorrect->Init(width, height);
		}
		//effects.push_back(colourCorrect);

		GameObject framebufferObject = scene->CreateEntity("Basic Effect");
		{
			basicEffect = &framebufferObject.emplace<PostEffect>();
			basicEffect->Init(width, height);
		}

		GameObject sepiaEffectObject = scene->CreateEntity("Sepia Effect");
		{
			sepiaEffect = &sepiaEffectObject.emplace<SepiaEffect>();
			sepiaEffect->Init(width, height);
		}
		effects.push_back(sepiaEffect);

		GameObject greyscaleEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			greyscaleEffect = &greyscaleEffectObject.emplace<GreyscaleEffect>();
			greyscaleEffect->Init(width, height);
		}
		effects.push_back(greyscaleEffect);
		
		#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		{
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;  
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();
			
			GameObject skyboxObj = scene->CreateEntity("skybox");  
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat);
		}
		////////////////////////////////////////////////////////////////////////////////////////


		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });

			controllables.push_back(obj2);

			keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao++;
				if (selectedVao >= controllables.size())
					selectedVao = 0;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});
			keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao--;
				if (selectedVao < 0)
					selectedVao = controllables.size() - 1;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});

			keyToggles.emplace_back(GLFW_KEY_Y, [&]() {
				auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
				behaviour->Relative = !behaviour->Relative;
				});
		}

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		///// Game loop /////
		while (!glfwWindowShouldClose(BackendHandler::window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(BackendHandler::window);
				}
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
			});

			// Clear the screen
			basicEffect->Clear();
			colourCorrect->Clear();

			for (int i = 0; i < effects.size(); i++)
				effects[i]->Clear();

			glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
			});
			
			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;
						
			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;
				
				return false;
			});

			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			colourCorrect->BindBuffer(0);

			// Iterate over the render group components and draw them
			renderGroup.each( [&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					BackendHandler::SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}
				// Render the mesh
				BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
			});

			colourCorrect->UnbindBuffer();

			colourCorrectionShader->Bind();
			colourCorrect->BindColourAsTexture(0, 0, 0);
			LUTs[activeEffect].bind(30);
			colourCorrect->DrawToScreen();
			LUTs[activeEffect].unbind(30);
			colourCorrect->UnbindTexture(0);
			colourCorrectionShader->UnBind();

			/*effects[activeEffect]->ApplyEffect(basicEffect);

			effects[activeEffect]->DrawToScreen();*/

			//basicEffect->DrawToScreen();

			// Draw our ImGui content
			BackendHandler::RenderImGui();

			scene->Poll();
			glfwSwapBuffers(BackendHandler::window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		//Clean up the environment generator so we can release references
		EnvironmentGenerator::CleanUpPointers();
		BackendHandler::ShutdownImGui();
	}	

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}