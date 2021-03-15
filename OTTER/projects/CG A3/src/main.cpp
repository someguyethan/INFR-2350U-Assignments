//Just a simple handler for simple initialization stuffs
#include "Utilities/BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

//TODO: New for this tutorial
#include <DirectionalLight.h>
#include <PointLight.h>
#include <UniformBuffer.h>
/////////////////////////////

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

	bool drawGBuffer = false;
	bool drawIllumBuffer = false;

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

		Shader::sptr simpleDepthShader = Shader::Create();
		simpleDepthShader->LoadShaderPartFromFile("shaders/simple_depth_vert.glsl", GL_VERTEX_SHADER);
		simpleDepthShader->LoadShaderPartFromFile("shaders/simple_depth_frag.glsl", GL_FRAGMENT_SHADER);
		simpleDepthShader->Link();

		Shader::sptr gBufferShader = Shader::Create();
		gBufferShader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		gBufferShader->LoadShaderPartFromFile("shaders/gBuffer_pass_frag.glsl", GL_FRAGMENT_SHADER);
		gBufferShader->Link();

		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		//Directional Light Shader
		shader->LoadShaderPartFromFile("shaders/directional_blinn_phong_frag.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		/*//Creates our directional Light
		DirectionalLight theSun;
		UniformBuffer directionalLightBuffer;

		//Allocates enough memory for one directional light (we can change this easily, but we only need 1 directional light)
		directionalLightBuffer.AllocateMemory(sizeof(DirectionalLight));
		//Casts our sun as "data" and sends it to the shader
		directionalLightBuffer.SendData(reinterpret_cast<void*>(&theSun), sizeof(DirectionalLight));

		directionalLightBuffer.Bind(0);*/

		//Basic effect for drawing to
		PostEffect* basicEffect;
		Framebuffer* shadowBuffer;
		GBuffer* gBuffer;
		IlluminationBuffer* illuminationBuffer;

		//Post Processing Effects
		int activeEffect = 0;
		std::vector<PostEffect*> effects;
		SepiaEffect* sepiaEffect;
		GreyscaleEffect* greyscaleEffect;
		ColorCorrectEffect* colorCorrectEffect;
		
		// We'll add some ImGui controls to control our shader
		BackendHandler::imGuiCallbacks.push_back([&]() {
			if (ImGui::CollapsingHeader("Effect controls"))
			{
				ImGui::SliderInt("Chosen Effect", &activeEffect, 0, effects.size() - 1);

				if (activeEffect == 0)
				{
					ImGui::Text("Active Effect: Sepia Effect");

					SepiaEffect* temp = (SepiaEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 1)
				{
					ImGui::Text("Active Effect: Greyscale Effect");
					
					GreyscaleEffect* temp = (GreyscaleEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 2)
				{
					ImGui::Text("Active Effect: Color Correct Effect");

					ColorCorrectEffect* temp = (ColorCorrectEffect*)effects[activeEffect];
					static char input[BUFSIZ];
					ImGui::InputText("Lut File to Use", input, BUFSIZ);

					if (ImGui::Button("SetLUT", ImVec2(200.0f, 40.0f)))
					{
						temp->SetLUT(LUT3D(std::string(input)));
					}
				}
			}
			if (ImGui::CollapsingHeader("Environment generation"))
			{
				if (ImGui::Button("Regenerate Environment", ImVec2(200.0f, 40.0f)))
				{
					EnvironmentGenerator::RegenerateEnvironment();
				}
			}
			if (ImGui::CollapsingHeader("Light Level Lighting Settings"))
			{
				if (ImGui::DragFloat3("Light Direction/Position", glm::value_ptr(illuminationBuffer->GetSunRef()._lightDirection), 0.01f, -10.0f, 10.0f)) 
				{
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
			});

		#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 

		///////////////////////////////////// Texture Loading //////////////////////////////////////////////////
		#pragma region Texture

		// Load some textures from files
		Texture2D::sptr stone = Texture2D::LoadFromFile("images/Stone_001_Diffuse.png");
		Texture2D::sptr stoneSpec = Texture2D::LoadFromFile("images/Stone_001_Specular.png");
		Texture2D::sptr grass = Texture2D::LoadFromFile("images/grass.jpg");
		Texture2D::sptr noSpec = Texture2D::LoadFromFile("images/grassSpec.png");
		Texture2D::sptr box = Texture2D::LoadFromFile("images/box.bmp");
		Texture2D::sptr boxSpec = Texture2D::LoadFromFile("images/box-reflections.bmp");
		Texture2D::sptr simpleFlora = Texture2D::LoadFromFile("images/SimpleFlora.png");
		Texture2D::sptr islandTex = Texture2D::LoadFromFile("images/plains_island_texture.png");
		Texture2D::sptr swordTex = Texture2D::LoadFromFile("images/Sword.png");
		Texture2D::sptr tree1Tex = Texture2D::LoadFromFile("images/tree1_texture.png");
		Texture2D::sptr tree2Tex = Texture2D::LoadFromFile("images/tree2_texture.png");
		Texture2D::sptr tree3Tex = Texture2D::LoadFromFile("images/tree3_texture.png");
		Texture2D::sptr tree4Tex = Texture2D::LoadFromFile("images/tree4_texture.png");
		Texture2D::sptr missingTex = Texture2D::LoadFromFile("images/missing_texture.jpg");
		LUT3D testCube("cubes/BrightenedCorrection.cube");

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
		//////////////////////////////////////////////////////////////////////////////////////////

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
		ShaderMaterial::sptr stoneMat = ShaderMaterial::Create();  
		stoneMat->Shader = gBufferShader;
		stoneMat->Set("s_Diffuse", stone);
		stoneMat->Set("s_Specular", stoneSpec);
		stoneMat->Set("u_Shininess", 2.0f);
		stoneMat->Set("u_TextureMix", 0.0f); 

		ShaderMaterial::sptr grassMat = ShaderMaterial::Create();
		grassMat->Shader = gBufferShader;
		grassMat->Set("s_Diffuse", grass);
		grassMat->Set("s_Specular", noSpec);
		grassMat->Set("u_Shininess", 2.0f);
		grassMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr boxMat = ShaderMaterial::Create();
		boxMat->Shader = gBufferShader;
		boxMat->Set("s_Diffuse", box);
		boxMat->Set("s_Specular", boxSpec);
		boxMat->Set("u_Shininess", 8.0f);
		boxMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr simpleFloraMat = ShaderMaterial::Create();
		simpleFloraMat->Shader = gBufferShader;
		simpleFloraMat->Set("s_Diffuse", simpleFlora);
		simpleFloraMat->Set("s_Specular", noSpec);
		simpleFloraMat->Set("u_Shininess", 8.0f);
		simpleFloraMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr islandMat = ShaderMaterial::Create();
		islandMat->Shader = gBufferShader;
		islandMat->Set("s_Diffuse", islandTex);
		islandMat->Set("s_Diffuse2", missingTex);
		islandMat->Set("s_Specular", noSpec);
		islandMat->Set("u_Shininess", 2.0f);

		ShaderMaterial::sptr swordMat = ShaderMaterial::Create();
		swordMat->Shader = gBufferShader;
		swordMat->Set("s_Diffuse", swordTex);
		swordMat->Set("s_Diffuse2", missingTex);
		swordMat->Set("s_Specular", noSpec);
		swordMat->Set("u_Shininess", 2.0f);

		ShaderMaterial::sptr tree1Mat = ShaderMaterial::Create();
		tree1Mat->Shader = gBufferShader;
		tree1Mat->Set("s_Diffuse", tree1Tex);
		tree1Mat->Set("s_Diffuse2", missingTex);
		tree1Mat->Set("s_Specular", noSpec);
		tree1Mat->Set("u_Shininess", 2.0f);

		ShaderMaterial::sptr tree2Mat = ShaderMaterial::Create();
		tree2Mat->Shader = gBufferShader;
		tree2Mat->Set("s_Diffuse", tree2Tex);
		tree2Mat->Set("s_Diffuse2", missingTex);
		tree2Mat->Set("s_Specular", noSpec);
		tree2Mat->Set("u_Shininess", 2.0f);

		ShaderMaterial::sptr tree3Mat = ShaderMaterial::Create();
		tree3Mat->Shader = gBufferShader;
		tree3Mat->Set("s_Diffuse", tree3Tex);
		tree3Mat->Set("s_Diffuse2", missingTex);
		tree3Mat->Set("s_Specular", noSpec);
		tree3Mat->Set("u_Shininess", 2.0f);

		ShaderMaterial::sptr tree4Mat = ShaderMaterial::Create();
		tree4Mat->Shader = gBufferShader;
		tree4Mat->Set("s_Diffuse", tree4Tex);
		tree4Mat->Set("s_Diffuse2", missingTex);
		tree4Mat->Set("s_Specular", noSpec);
		tree4Mat->Set("u_Shininess", 2.0f);

		GameObject obj2 = scene->CreateEntity("monkey_quads");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/monkey_quads.obj");
			obj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(stoneMat);
			obj2.get<Transform>().SetLocalPosition(0.0f, 0.0f, -2.0f);
			obj2.get<Transform>().SetLocalRotation(0.0f, 0.0f, -90.0f);
			obj2.get<Transform>().SetLocalScale(glm::vec3(0.01f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj2);
		}

		GameObject swordObj = scene->CreateEntity("sword");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Sword.obj");
			swordObj.emplace<RendererComponent>().SetMesh(vao).SetMaterial(swordMat);
			swordObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.5f);
			swordObj.get<Transform>().SetLocalRotation(90.0f, 170.0f, 0.0f);
			swordObj.get<Transform>().SetLocalScale(0.1f, 0.1f, 0.1f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(swordObj);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(swordObj);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 0.0f, 0.0f, 0.6f });
			pathing->Points.push_back({ 0.0f, 0.0f, 0.5f });
			pathing->Speed = 0.05f;
		}

		GameObject stoneObj = scene->CreateEntity("stone");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/monkey.obj");
			stoneObj.emplace<RendererComponent>().SetMesh(vao).SetMaterial(stoneMat);
			stoneObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, -2.3f);
			stoneObj.get<Transform>().SetLocalRotation(0.0f, 0.0f, 0.0f);
			stoneObj.get<Transform>().SetLocalScale(glm::vec3(2.0f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(stoneObj);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(stoneObj);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 0.0f, 0.0f, -2.2f });
			pathing->Points.push_back({ 0.0f, 0.0f, -2.3f });
			pathing->Speed = 0.05f;
		}

		GameObject plainsIslandObj1 = scene->CreateEntity("Plains Island");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plains island.obj");
			plainsIslandObj1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(islandMat);
			plainsIslandObj1.get<Transform>().SetLocalPosition(0.0f, 0.0f, -2.0f);
			plainsIslandObj1.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(plainsIslandObj1);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(plainsIslandObj1);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 0.0f, 0.0f, -1.9f });
			pathing->Points.push_back({ 0.0f, 0.0f, -2.0f });
			pathing->Speed = 0.05f;
		}

		GameObject tree1Obj1 = scene->CreateEntity("Tree");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree1.obj");
			tree1Obj1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(tree1Mat);
			tree1Obj1.get<Transform>().SetLocalPosition(-4.0f, -3.0f, -0.8f);
			tree1Obj1.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			tree1Obj1.get<Transform>().SetLocalScale(glm::vec3(0.5f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(tree1Obj1);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(tree1Obj1);
			// Set up a path for the object to follow
			pathing->Points.push_back({ -4.0f, -3.0f, -0.7f });
			pathing->Points.push_back({ -4.0f, -3.0f, -0.8f });
			pathing->Speed = 0.05f;
		}

		GameObject tree1Obj2 = scene->CreateEntity("Tree");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree2.obj");
			tree1Obj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(tree2Mat);
			tree1Obj2.get<Transform>().SetLocalPosition(4.0f, 7.0f, -0.8f);
			tree1Obj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			tree1Obj2.get<Transform>().SetLocalScale(glm::vec3(0.8f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(tree1Obj2);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(tree1Obj2);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 4.0f, 7.0f, -0.7f });
			pathing->Points.push_back({ 4.0f, 7.0f, -0.8f });
			pathing->Speed = 0.05f;
		}

		{//Tree Stuff
			GameObject treeObj1 = scene->CreateEntity("Tree");
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree1.obj");
				treeObj1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(tree1Mat);
				treeObj1.get<Transform>().SetLocalPosition(41.0f, 1.0f, 0.5f);
				treeObj1.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				treeObj1.get<Transform>().SetLocalScale(glm::vec3(0.8f));
				BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(treeObj1);

				auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(treeObj1);
				// Set up a path for the object to follow
				pathing->Points.push_back({ 41.0f, 1.0f, 1.5f });
				pathing->Points.push_back({ 41.0f, 1.0f, 0.5f });
				pathing->Speed = 0.6f;
			}
			GameObject treeObj2 = scene->CreateEntity("Tree");
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree2.obj");
				treeObj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(tree2Mat);
				treeObj2.get<Transform>().SetLocalPosition(52.0f, -30.0f, 10.5f);
				treeObj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				treeObj2.get<Transform>().SetLocalScale(glm::vec3(0.7f));
				BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(treeObj2);

				auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(treeObj2);
				// Set up a path for the object to follow
				pathing->Points.push_back({ 52.0f, -30.0f, 11.5f });
				pathing->Points.push_back({ 52.0f, -30.0f, 10.5f });
				pathing->Speed = 0.4f;
			}
			GameObject treeObj3 = scene->CreateEntity("Tree");
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree1.obj");
				treeObj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(tree1Mat);
				treeObj3.get<Transform>().SetLocalPosition(0.0f, 52.0f, -4.5f);
				treeObj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				treeObj3.get<Transform>().SetLocalScale(glm::vec3(0.6f));
				BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(treeObj3);

				auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(treeObj3);
				// Set up a path for the object to follow
				pathing->Points.push_back({ 0.0f, 52.0f, -3.5f });
				pathing->Points.push_back({ 0.0f, 52.0f, -4.5f });
				pathing->Speed = 0.5f;
			}
			GameObject treeObj4 = scene->CreateEntity("Tree");
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree3.obj");
				treeObj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(tree3Mat);
				treeObj4.get<Transform>().SetLocalPosition(-40.0f, 29.0f, -9.5f);
				treeObj4.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				treeObj4.get<Transform>().SetLocalScale(glm::vec3(0.7f));
				BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(treeObj4);

				auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(treeObj4);
				// Set up a path for the object to follow
				pathing->Points.push_back({ -40.0f, 29.0f, -8.5f });
				pathing->Points.push_back({ -40.0f, 29.0f, -9.5f });
				pathing->Speed = 0.3f;
			}
			GameObject treeObj5 = scene->CreateEntity("Tree");
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree1.obj");
				treeObj5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(tree1Mat);
				treeObj5.get<Transform>().SetLocalPosition(-29.0f, 0.0f, 10.5f);
				treeObj5.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				treeObj5.get<Transform>().SetLocalScale(glm::vec3(0.5f));
				BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(treeObj5);

				auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(treeObj5);
				// Set up a path for the object to follow
				pathing->Points.push_back({ -29.0f, 0.0f, 11.5f });
				pathing->Points.push_back({ -29.0f, 0.0f, 10.5f });
				pathing->Speed = 0.6f;
			}
			GameObject treeObj6 = scene->CreateEntity("Tree");
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree4.obj");
				treeObj6.emplace<RendererComponent>().SetMesh(vao).SetMaterial(tree4Mat);
				treeObj6.get<Transform>().SetLocalPosition(-29.0f, -39.0, 5.5f);
				treeObj6.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				treeObj6.get<Transform>().SetLocalScale(glm::vec3(0.7f));
				BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(treeObj6);

				auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(treeObj6);
				// Set up a path for the object to follow
				pathing->Points.push_back({ -29.0f, -39.0, 6.5f });
				pathing->Points.push_back({ -29.0f, -39.0, 5.5f });
				pathing->Speed = 0.4f;
			}
		}

		{//Island stuff
			GameObject plainsIslandObj2 = scene->CreateEntity("Plains Island");
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plains island.obj");
				plainsIslandObj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(islandMat);
				plainsIslandObj2.get<Transform>().SetLocalPosition(40.0f, 0.0f, 0.0f);
				plainsIslandObj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				plainsIslandObj2.get<Transform>().SetLocalScale(glm::vec3(0.8f));
				BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(plainsIslandObj2);

				auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(plainsIslandObj2);
				// Set up a path for the object to follow
				pathing->Points.push_back({ 40.0f, 0.0f, 1.0f });
				pathing->Points.push_back({ 40.0f, 0.0f, 0.0f });
				pathing->Speed = 0.6f;
			}
			GameObject plainsIslandObj3 = scene->CreateEntity("Plains Island");
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plains island.obj");
				plainsIslandObj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(islandMat);
				plainsIslandObj3.get<Transform>().SetLocalPosition(50.0f, -30.0f, 10.0f);
				plainsIslandObj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				plainsIslandObj3.get<Transform>().SetLocalScale(glm::vec3(0.7f));
				BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(plainsIslandObj3);

				auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(plainsIslandObj3);
				// Set up a path for the object to follow
				pathing->Points.push_back({ 50.0f, -30.0f, 11.0f });
				pathing->Points.push_back({ 50.0f, -30.0f, 10.0f });
				pathing->Speed = 0.4f;
			}
			GameObject plainsIslandObj4 = scene->CreateEntity("Plains Island");
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plains island.obj");
				plainsIslandObj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(islandMat);
				plainsIslandObj4.get<Transform>().SetLocalPosition(0.0f, 50.0f, -5.0f);
				plainsIslandObj4.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				plainsIslandObj4.get<Transform>().SetLocalScale(glm::vec3(0.6f));
				BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(plainsIslandObj4);

				auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(plainsIslandObj4);
				// Set up a path for the object to follow
				pathing->Points.push_back({ 0.0f, 50.0f, -4.0f });
				pathing->Points.push_back({ 0.0f, 50.0f, -5.0f });
				pathing->Speed = 0.5f;
			}
			GameObject plainsIslandObj5 = scene->CreateEntity("Plains Island");
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plains island.obj");
				plainsIslandObj5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(islandMat);
				plainsIslandObj5.get<Transform>().SetLocalPosition(-40.0f, 30.0f, -10.0f);
				plainsIslandObj5.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				plainsIslandObj5.get<Transform>().SetLocalScale(glm::vec3(0.7f));
				BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(plainsIslandObj5);

				auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(plainsIslandObj5);
				// Set up a path for the object to follow
				pathing->Points.push_back({ -40.0f, 30.0f, -9.0f });
				pathing->Points.push_back({ -40.0f, 30.0f, -10.0f });
				pathing->Speed = 0.3f;
			}
			GameObject plainsIslandObj6 = scene->CreateEntity("Plains Island");
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plains island.obj");
				plainsIslandObj6.emplace<RendererComponent>().SetMesh(vao).SetMaterial(islandMat);
				plainsIslandObj6.get<Transform>().SetLocalPosition(-30.0f, 0.0f, 10.0f);
				plainsIslandObj6.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				plainsIslandObj6.get<Transform>().SetLocalScale(glm::vec3(0.5f));
				BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(plainsIslandObj6);

				auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(plainsIslandObj6);
				// Set up a path for the object to follow
				pathing->Points.push_back({ -30.0f, 0.0f, 11.0f });
				pathing->Points.push_back({ -30.0f, 0.0f, 10.0f });
				pathing->Speed = 0.6f;
			}
			GameObject plainsIslandObj7 = scene->CreateEntity("Plains Island");
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plains island.obj");
				plainsIslandObj7.emplace<RendererComponent>().SetMesh(vao).SetMaterial(islandMat);
				plainsIslandObj7.get<Transform>().SetLocalPosition(-30.0f, -40.0f, 5.0f);
				plainsIslandObj7.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				plainsIslandObj7.get<Transform>().SetLocalScale(glm::vec3(0.7f));
				BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(plainsIslandObj7);

				auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(plainsIslandObj7);
				// Set up a path for the object to follow
				pathing->Points.push_back({ -30.0f, -40.0f, 6.0f });
				pathing->Points.push_back({ -30.0f, -40.0f, 5.0f });
				pathing->Speed = 0.4f;
			}
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

		GameObject gBufferObject = scene->CreateEntity("GBuffer");
		{
			gBuffer = &gBufferObject.emplace<GBuffer>();
			gBuffer->Init(width, height);
		}

		GameObject illuminationBufferObject = scene->CreateEntity("Illumination Buffer");
		{
			illuminationBuffer = &illuminationBufferObject.emplace<IlluminationBuffer>();
			illuminationBuffer->Init(width, height);
		}

		int shadowWidth = 4096;
		int shadowHeight = 4096;

		GameObject shadowBufferObject = scene->CreateEntity("Shadow Buffer");
		{
			shadowBuffer = &shadowBufferObject.emplace<Framebuffer>();
			shadowBuffer->AddDepthTarget();
			shadowBuffer->Init(shadowWidth, shadowHeight);
		}

		GameObject framebufferObject = scene->CreateEntity("Basic Effect");
		{
			basicEffect = &framebufferObject.emplace<PostEffect>();
			basicEffect->Init(width, height);
		}

		GameObject sepiaEffectObject = scene->CreateEntity("Sepia Effect");
		{
			sepiaEffect = &sepiaEffectObject.emplace<SepiaEffect>();
			sepiaEffect->Init(width, height);
			sepiaEffect->SetIntensity(0.0f);
		}
		effects.push_back(sepiaEffect);

		GameObject greyscaleEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			greyscaleEffect = &greyscaleEffectObject.emplace<GreyscaleEffect>();
			greyscaleEffect->Init(width, height);
		}
		effects.push_back(greyscaleEffect);
		
		GameObject colorCorrectEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			colorCorrectEffect = &colorCorrectEffectObject.emplace<ColorCorrectEffect>();
			colorCorrectEffect->Init(width, height);
		}
		effects.push_back(colorCorrectEffect);

		#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		
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
			//skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat).SetCastShadow(false);
		
		////////////////////////////////////////////////////////////////////////////////////////


		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });
			keyToggles.emplace_back(GLFW_KEY_F1, [&]() { drawGBuffer = !drawGBuffer; });
			keyToggles.emplace_back(GLFW_KEY_F2, [&]() { drawIllumBuffer = !drawIllumBuffer; });

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
			//greyscaleEffect->Clear();
			//sepiaEffect->Clear();
			for (int i = 0; i < effects.size(); i++)
			{
				effects[i]->Clear();
			}
			shadowBuffer->Clear();
			gBuffer->Clear();
			illuminationBuffer->Clear();

			glClearColor(1.0f, 1.0f, 1.0f, 0.3f);
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

			//Set up light space matrix
			glm::mat4 lightProjectionMatrix = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, -30.0f, 30.0f);
			glm::mat4 lightViewMatrix = glm::lookAt(glm::vec3(-illuminationBuffer->GetSunRef()._lightDirection), glm::vec3(), glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightSpaceViewProj = lightProjectionMatrix * lightViewMatrix;

			illuminationBuffer->SetLightSpaceViewProj(lightSpaceViewProj);
			glm::vec3 camPos = glm::inverse(view) * glm::vec4(0, 0, 0, 1);
			illuminationBuffer->SetCamPos(camPos);

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

			glViewport(0, 0, shadowWidth, shadowHeight);
			shadowBuffer->Bind();

			renderGroup.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// Render the mesh
				if (renderer.CastShadows)
				{
					BackendHandler::RenderVAO(simpleDepthShader, renderer.Mesh, viewProjection, transform, lightSpaceViewProj);
				}
			});

			shadowBuffer->Unbind();

			glfwGetWindowSize(BackendHandler::window, &width, &height);

			glViewport(0, 0, width, height);
			gBuffer->Bind();
			// Iterate over the render group components and draw them
			renderGroup.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
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
				BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform, lightSpaceViewProj);
				
			});
			gBuffer->Unbind();

			illuminationBuffer->BindBuffer(0);

			skybox->Bind();
			BackendHandler::SetupShaderForFrame(skybox, view, projection);
			skyboxMat->Apply();
			BackendHandler::RenderVAO(skybox, meshVao, viewProjection, skyboxObj.get<Transform>(), lightSpaceViewProj);
			skybox->UnBind();

			illuminationBuffer->UnbindBuffer();

			shadowBuffer->BindDepthAsTexture(30);

			illuminationBuffer->ApplyEffect(gBuffer);

			shadowBuffer->UnbindTexture(30);

			effects[activeEffect]->ApplyEffect(illuminationBuffer);

			if (drawGBuffer)
				gBuffer->DrawBuffersToScreen();
			else if (drawIllumBuffer)
				illuminationBuffer->DrawIllumBuffer();
			else
				effects[activeEffect]->DrawToScreen();
			
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