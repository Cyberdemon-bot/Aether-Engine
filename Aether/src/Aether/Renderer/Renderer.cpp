#include "aepch.h"
#include "Aether/Renderer/Renderer.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Core/Application.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Renderer/BuiltinShader.h"
#include "Aether/Renderer/EditorCamera.h"
#include "Aether/Renderer/UniformBuffer.h"
#include "Aether/Renderer/StorageBuffer.h"

float quadVertices[] = {
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
};

uint32_t quadIndices[] = { 0, 1, 2, 2, 3, 0 };

float skyboxVertices[] = {
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f
};

uint32_t skyboxIndices[] = {
	1, 2, 6, 6, 5, 1,
	0, 4, 7, 7, 3, 0,
	4, 5, 6, 6, 7, 4,
	0, 3, 2, 2, 1, 0,
	0, 1, 5, 5, 4, 0,
	3, 7, 6, 6, 2, 3
};

namespace Aether {

	Scope<Renderer::SceneData>  Renderer::s_SceneData  = CreateScope<Renderer::SceneData>();
	Scope<Renderer::RenderData> Renderer::s_RenderData = CreateScope<Renderer::RenderData>();

	void Renderer::Init()
	{
		RenderCommand::Init();
		BufferLayout layout = { { "a_InstanceModel", ShaderDataType::Mat4 }, { "a_InstanceRigIdx", ShaderDataType::Int} };
		RenderCommand::SetDepthFuncEqual(State::LEQUAL);

		s_RenderData->s_InstanceVBO = ResourceManager::CreateResource<VertexBuffer>(10 * (sizeof(glm::mat4) + sizeof(glm::vec4)));
		ResourceManager::GetResource<VertexBuffer>(s_RenderData->s_InstanceVBO)->SetLayout(layout);

		s_RenderData->CameraUB = ResourceManager::CreateResource<UniformBuffer>(sizeof(CameraData));
		ResourceManager::GetResource<UniformBuffer>(s_RenderData->CameraUB)->Bind(0);

		uint32_t BoneStorageSize = 400;
		s_SceneData->BoneStorage.reserve(BoneStorageSize);
		s_SceneData->OffsetStorage.reserve(BoneStorageSize);
		s_RenderData->BoneStorage = ResourceManager::CreateResource<StorageBuffer>(sizeof(glm::mat4) * BoneStorageSize);
		s_RenderData->OffsetStorage = ResourceManager::CreateResource<StorageBuffer>(sizeof(glm::vec4) * BoneStorageSize);

		s_RenderData->LightUB = ResourceManager::CreateResource<UniformBuffer>(sizeof(LightsData));
		ResourceManager::GetResource<UniformBuffer>(s_RenderData->LightUB)->Bind(2);

		s_RenderData->s_ScreenShader    = ResourceManager::CreateResource<Shader>(ShaderProgramSource{VScreenShader,    FScreenShader});
		s_RenderData->lineShader         = ResourceManager::CreateResource<Shader>(ShaderProgramSource{VLineShader,      FLineShader});
		s_RenderData->s_SkyboxShader    = ResourceManager::CreateResource<Shader>(ShaderProgramSource{VSkyboxShader,    FSkyboxShader});
		s_RenderData->s_ShadowmapShader = ResourceManager::CreateResource<Shader>(ShaderProgramSource{VShadowmapShader, FShadowmapShader});

		ResourceManager::GetResource<Shader>(s_RenderData->s_SkyboxShader)   ->SetUBOSlot("Camera", 0);
		ResourceManager::GetResource<Shader>(s_RenderData->s_ShadowmapShader)->SetUBOSlot("Lights", 2);

		Aether::FramebufferSpec shadowFbSpec;
		shadowFbSpec.Width       = 2048;
		shadowFbSpec.Height      = 2048;
		shadowFbSpec.Attachments = { Aether::ImageFormat::DEPTH24STENCIL8 };

		for (uint32_t i = 0; i < MaxShadowCaster; i++)
			s_RenderData->s_ShadowFBO[i] = ResourceManager::CreateResource<FrameBuffer>(shadowFbSpec);

		s_RenderData->s_ShadowPipeline.reserve(MaxShadowCaster);
		s_SceneData->CandList.reserve(MaxShadowCaster);
		for (uint32_t i = 0; i < MaxShadowCaster; i++)
		{
			RenderPass shadowPass;
			shadowPass.TargetFBO     = ResourceManager::GetResource<FrameBuffer>(s_RenderData->s_ShadowFBO[i]);
			shadowPass.Shader        = ResourceManager::GetResource<Shader>(s_RenderData->s_ShadowmapShader);
			shadowPass.ClearDepth    = true;
			shadowPass.ClearColor    = false;
			shadowPass.OnScreen      = false;
			shadowPass.UsingMaterial = false;
			shadowPass.UsingShadowmap = false;
			shadowPass.CullFace      = Aether::State::FRONT_CULL;
			shadowPass.attribList    = { {"u_LightIndex", (int)i} };
			shadowPass.IsActive      = false; 
			s_RenderData->s_ShadowPipeline.push_back(shadowPass);
		}

		s_RenderData->s_Screen  = new Mesh(MeshSpec{{VertexStream{quadVertices,    4, MeshLayout::Quad()}},   quadIndices,    6});
		s_RenderData->s_SkyMesh = new Mesh(MeshSpec{{VertexStream{skyboxVertices,  8, MeshLayout::Vertex()}}, skyboxIndices, 36});
	}

	void Renderer::Shutdown()
	{
		auto& rd = *s_RenderData;
		ResourceManager::Unload(rd.CameraUB);
		//ResourceManager::Unload(rd.BoneUB);
		ResourceManager::Unload(rd.LightUB);
		ResourceManager::Unload(rd.s_InstanceVBO);
		ResourceManager::Unload(rd.s_ScreenShader);
		ResourceManager::Unload(rd.s_SkyboxShader);
		ResourceManager::Unload(rd.s_ShadowmapShader); 
		ResourceManager::Unload(rd.lineShader);
		if (rd.s_LutMap.IsValid()) ResourceManager::Unload(rd.s_LutMap);
		if (rd.s_Skybox.IsValid()) ResourceManager::Unload(rd.s_Skybox);

		for (uint32_t i = 0; i < MaxShadowCaster; i++)
			if (rd.s_ShadowFBO[i].IsValid()) ResourceManager::Unload(rd.s_ShadowFBO[i]);

		delete rd.s_Screen;
		delete rd.s_SkyMesh;
		s_SceneData.reset();
		s_RenderData.reset();
	}

	void Renderer::SetPipeline(const std::vector<RenderPass>& list)
	{
		s_RenderData->s_PassList = list;
	}

	void Renderer::SetLutMap(const std::string& filepath)
	{
		if (s_RenderData->s_LutMap.IsValid())
			ResourceManager::Unload(s_RenderData->s_LutMap);
		s_RenderData->s_LutMap = ResourceManager::CreateResource<Texture2D>(filepath, WrapMode::CLAMP_TO_EDGE, false);
	}

	void Renderer::SetSkyBox(const std::string& filepath)
	{
		if (s_RenderData->s_Skybox.IsValid())
			ResourceManager::Unload(s_RenderData->s_Skybox);
		s_RenderData->s_Skybox = ResourceManager::CreateResource<TextureCube>(filepath);
	}

	void Renderer::ActivatePass(uint32_t PassIdx)
	{
		if (PassIdx < s_RenderData->s_PassList.size())
			s_RenderData->s_PassList[PassIdx].IsActive = true;
	}

	void Renderer::DeactivatePass(uint32_t PassIdx)
	{
		if (PassIdx < s_RenderData->s_PassList.size())
			s_RenderData->s_PassList[PassIdx].IsActive = false;
	}

	Texture2D* Renderer::GetShadowDepthAttachment(uint32_t slot)
	{
		if (slot >= MaxShadowCaster) return nullptr;
		auto* fbo = ResourceManager::GetResource<FrameBuffer>(s_RenderData->s_ShadowFBO[slot]);
		return fbo ? fbo->GetDepthAttachment() : nullptr;
	}

	void Renderer::BeginScene(const Camera& camera, const std::vector<LightParam>& lights)
	{
		s_SceneData->camera.Position       = camera.GetPosition();
		s_SceneData->camera.View           = camera.GetView();
		s_SceneData->camera.ViewProjection = camera.GetViewProjection();

		s_SceneData->lights.shadowMask = 0;
		uint32_t shadowSlot = 0; 

		for (uint32_t i = 0; i < MaxShadowCaster; i++)
			s_RenderData->s_ShadowPipeline[i].IsActive = false;

		s_SceneData->lights.lightCount = (int)std::min(lights.size(), (size_t)MAX_LIGHTS);
		s_SceneData->CandList.clear();

		for (int i = 0; i < lights.size(); i++)
		{
			const LightParam& light = lights[i];
			float score = 0.0f;
			if (light.type == LightType::Directional) score = 999999.0f;
			else
			{
				glm::vec3 diff = camera.GetPosition() - light.position;
				float distSq = glm::dot(diff, diff);
				distSq = glm::max(distSq, 0.001f);
				score = (light.intensity * light.range) / distSq;
			}
			s_SceneData->CandList.push_back({i, score});
		}
		std::sort(s_SceneData->CandList.begin(), s_SceneData->CandList.end(), [](const auto& a, const auto& b) { return a.score > b.score; });

		for (size_t i = 0; i < s_SceneData->lights.lightCount; i++)
		{
			const LightParam& light = lights[s_SceneData->CandList[i].index];
			s_SceneData->lights.lights[i].positionAndType   = glm::vec4(light.position,  (float)light.type);
			s_SceneData->lights.lights[i].directionAndRange = glm::vec4(light.direction, light.range);
			s_SceneData->lights.lights[i].colorAndIntensity = glm::vec4(light.color,     light.intensity);
			s_SceneData->lights.lights[i].coneAngles        = glm::vec4(light.innerCone, light.outerCone, light.castShadows ? 1.0f : 0.0f, 0.0f);

			if (light.castShadows && shadowSlot < MaxShadowCaster)
			{
				s_SceneData->lights.shadowMask |= (1u << i);

				glm::mat4 lightProjection, lightView;
				if (light.type == LightType::Spot)
				{
					float fov = glm::acos(light.outerCone) * 2.0f;
					lightProjection = glm::perspective(fov, 1.0f, 0.1f, light.range);
					glm::vec3 up  = glm::vec3(0.0f, 1.0f, 0.0f);
					glm::vec3 dir = glm::normalize(light.direction);
					if (glm::abs(glm::dot(dir, up)) > 0.99f) up = glm::vec3(0.0f, 0.0f, 1.0f);
					lightView = glm::lookAt(light.position, light.position + dir, up);
				}
				else if (light.type == LightType::Directional)
					CalculateDirectionalMat(camera, light, lightView, lightProjection);

				s_SceneData->lights.lights[i].lightSpaceMatrix = lightProjection * lightView;

				auto& shadowPass = s_RenderData->s_ShadowPipeline[shadowSlot];
				shadowPass.IsActive = true;
				for (auto& attrib : shadowPass.attribList)
					if (attrib.first == "u_LightIndex") { attrib.second = i; break; }

				shadowSlot++;
			}
			else s_SceneData->lights.lights[i].lightSpaceMatrix = glm::mat4(1.0f);
		}
	}

	void Renderer::EndScene()
	{
		RenderPass* mainPass = nullptr;
		ResourceManager::GetResource<UniformBuffer>(s_RenderData->CameraUB)->SetData(&s_SceneData->camera, sizeof(CameraData));
		ResourceManager::GetResource<UniformBuffer>(s_RenderData->LightUB) ->SetData(&s_SceneData->lights,  sizeof(LightsData));

		auto* boneStorage = ResourceManager::GetResource<StorageBuffer>(s_RenderData->BoneStorage);
		auto* offsetStorage = ResourceManager::GetResource<StorageBuffer>(s_RenderData->OffsetStorage);
		auto skelSystem   = AnimationSystem::GetModule<RigModule>();
		auto& CommandList = s_SceneData->CommandList;
		sort(CommandList.begin(), CommandList.end());

		for (size_t i = 0; i < CommandList.size(); i++)
		{
			const auto& command = CommandList[i];
			int currentAnimIndex = -1;
			if (command.pose.IsValid()) 
			{
				const auto [boneMatrices, size] = skelSystem->GetPose(command.pose);
				if (boneMatrices != nullptr && size > 0)
				{
					int boneBaseIndex = static_cast<int>(s_SceneData->BoneStorage.size());
					s_SceneData->BoneStorage.insert(s_SceneData->BoneStorage.end(), boneMatrices, boneMatrices + size);
					s_SceneData->OffsetStorage.push_back(glm::vec4(static_cast<float>(boneBaseIndex), static_cast<float>(size), 0.0f, 0.0f));
					currentAnimIndex = static_cast<int>(s_SceneData->OffsetStorage.size() - 1);
				}
			}
			s_SceneData->BoneIndices.push_back(currentAnimIndex);
		}

		if (boneStorage->GetSize() < s_SceneData->BoneStorage.size() * sizeof(glm::mat4)) 
		{
			boneStorage->Resize(s_SceneData->BoneStorage.size() * sizeof(glm::mat4));
			offsetStorage->Resize(s_SceneData->OffsetStorage.size() * sizeof(glm::vec4));
		}

		if (!s_SceneData->BoneStorage.empty())
		{
			boneStorage->SetData(s_SceneData->BoneStorage.data(), s_SceneData->BoneStorage.size() * sizeof(glm::mat4));
			offsetStorage->SetData(s_SceneData->OffsetStorage.data(), s_SceneData->OffsetStorage.size() * sizeof(glm::vec4));
		}

		for (auto& pass : s_RenderData->s_ShadowPipeline)
			if (pass.IsActive) Flush(pass);

		for (auto& pass : s_RenderData->s_PassList)
		{
			if (pass.IsActive)
			{
				Flush(pass);
				if (pass.OnScreen) mainPass = &pass;
			}
		}

		if (mainPass) RenderOnScreen(*mainPass);
		s_SceneData->CommandList.clear();
		s_SceneData->BoneStorage.clear();
		s_SceneData->OffsetStorage.clear();
		s_SceneData->BoneIndices.clear();
		s_SceneData->lights.lightCount = 0;
	}

	void Renderer::RenderSkybox()
	{
		auto* skybox = ResourceManager::GetResource<TextureCube>(s_RenderData->s_Skybox);
		if (!skybox) return;
		skybox->Bind(0);
		auto* shader = ResourceManager::GetResource<Shader>(s_RenderData->s_SkyboxShader);
		shader->Bind();
		shader->SetInt("u_Skybox", 0);
		RenderCommand::SetCullingMode(State::None);
		RenderCommand::DrawIndexed(
			ResourceManager::GetResource<VertexArray>(s_RenderData->s_SkyMesh->GetVertexArray()));
	}

	void Renderer::RenderOnScreen(const RenderPass& pass)
	{
		auto& window = Application::Get().GetWindow();
		RenderCommand::SetClearColor({0, 0, 0, 1});
		RenderCommand::Clear();
		RenderCommand::SetViewport(0, 0, window.GetFramebufferWidth(), window.GetFramebufferHeight());
		pass.TargetFBO->BindColorTexture(0);

		auto* screenShader = ResourceManager::GetResource<Shader>(s_RenderData->s_ScreenShader);
		screenShader->Bind();
		screenShader->SetInt("u_SceneTexture", 0);

		auto* lutMap = ResourceManager::GetResource<Texture2D>(s_RenderData->s_LutMap);
		if (lutMap)
		{
			lutMap->Bind(1);
			screenShader->SetInt("u_HasLut",      1);
			screenShader->SetInt("u_LutTexture",  1);
			screenShader->SetFloat("u_LutIntensity", pass.LutIntensity);
		}
		else screenShader->SetInt("u_HasLut", 0);

		RenderCommand::DrawIndexed(
			ResourceManager::GetResource<VertexArray>(s_RenderData->s_Screen->GetVertexArray()));
	}

	void Renderer::DrawMesh(Mesh* mesh, const std::vector<Material*> materials, Handle<PoseTag> pose, const glm::mat4& transform)
	{
		if (!mesh) return;
		const auto& submeshes = mesh->GetSubMeshes();
		if (!mesh->HasInstanceBuffer()) mesh->AddInstanceBuffer(s_RenderData->s_InstanceVBO);

		for (uint32_t i = 0; i < submeshes.size(); i++)
		{
			if (submeshes[i].MaterialIdx >= materials.size()) continue;
			Command command;
			command.mesh = mesh;
			command.material = materials[submeshes[i].MaterialIdx];
			command.subIdx = i;
			command.pose  = pose;
			command.transform = transform;
			s_SceneData->CommandList.push_back(command);
		}
	}

	void Renderer::SetPassAtrib(uint32_t passIdx, const std::string& name, int value)
	{
		auto& pass = s_RenderData->s_PassList[passIdx];
		for (auto& attrib : pass.attribList)
			if (attrib.first == name) { attrib.second = value; return; }
		pass.attribList.emplace_back(name, value);
	}

	void Renderer::Flush(const RenderPass& pass)
	{
		Mesh* currentMesh = nullptr;
		Material* currentMaterial = nullptr;
		int startSlot = 3;

		if (!pass.Shader || !pass.TargetFBO)
		{
			AE_CORE_ERROR("RenderPass has null Shader or TargetFBO — skipping.");
			return;
		}

		auto* shader = pass.Shader; shader->Bind();
		auto* fbo = pass.TargetFBO; fbo->Bind();
		auto* screen_vao = ResourceManager::GetResource<VertexArray>(s_RenderData->s_Screen->GetVertexArray());
		auto* instanceVBO = ResourceManager::GetResource<VertexBuffer>(s_RenderData->s_InstanceVBO);
		auto* boneStorage = ResourceManager::GetResource<StorageBuffer>(s_RenderData->BoneStorage);
		auto* offsetStorage = ResourceManager::GetResource<StorageBuffer>(s_RenderData->OffsetStorage);

		if (pass.ClearColor && pass.ClearDepth)
		{
			RenderCommand::SetClearColor(pass.ClearValue);
			RenderCommand::Clear();
		}
		else if (pass.ClearColor)
		{
			RenderCommand::SetClearColor(pass.ClearValue);
			RenderCommand::ClearColor();
		}
		else if (pass.ClearDepth) RenderCommand::ClearDepth();

		if (pass.CullFace == State::FRONT_CULL) RenderCommand::SetCullingMode(State::FRONT_CULL);
		else if (pass.CullFace == State::BACK_CULL) RenderCommand::SetCullingMode(State::BACK_CULL);
		else RenderCommand::SetCullingMode(State::None);

		RenderCommand::SetViewport(0, 0, fbo->GetSpecification().Width, fbo->GetSpecification().Height);

		for (auto [name, texture] : pass.readList)
		{
			texture->Bind(startSlot);
			shader->SetInt(name, startSlot);
			startSlot++;
		}
		for (auto& [name, value] : pass.attribList) shader->SetInt(name, value);

		if (pass.UsingShadowmap)
		{
			for (size_t i = 0; i < MaxShadowCaster; i++)
			{
				GetShadowDepthAttachment(i)->Bind(startSlot);
				shader->SetInt("u_Shadowmap" + std::to_string(i), startSlot);
				startSlot++;
			}
		}

		if (pass.UsingGeometry)
		{
			boneStorage->Bind(1); pass.Shader->SetInt("u_BoneStorage", 1);
			offsetStorage->Bind(2); pass.Shader->SetInt("u_OffsetStorage", 2);
			auto& CommandList = s_SceneData->CommandList;

			for (size_t i = 0; i < CommandList.size(); i++)
			{
				auto& command = CommandList[i];
				auto* material = command.material;
				auto* mesh = command.mesh;
				if (!material || !mesh) continue;

				const auto& submesh = mesh->GetSubMeshes()[command.subIdx];
				void* indexOffset = (void*)(submesh.BaseIndex * sizeof(uint32_t));

				if (currentMaterial != material && pass.UsingMaterial)
				{
					material->UploadMaterial(shader, startSlot);
					currentMaterial = material;
				}
				if (currentMesh != mesh)
				{
					mesh->UploadMesh();
					currentMesh = mesh;
				}

				s_SceneData->batchInstance.push_back({command.transform, s_SceneData->BoneIndices[i]});
				if ((i == CommandList.size() - 1) || (command != CommandList[i + 1]))
				{
					uint32_t dataSize = (uint32_t)(s_SceneData->batchInstance.size() * sizeof(InstanceData));
					if (instanceVBO->GetSize() < dataSize)
						instanceVBO->Resize(dataSize * 2);
					instanceVBO->SetData(s_SceneData->batchInstance.data(), dataSize, 0);
					RenderCommand::DrawInstancedBaseVertex(nullptr, submesh.IndexCount, indexOffset, submesh.BaseVertex, s_SceneData->batchInstance.size());
					s_SceneData->batchInstance.clear();
				}
			}
		}
		else RenderCommand::DrawIndexed(screen_vao);

		if (pass.UsingSkybox) RenderSkybox();
		fbo->Unbind();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::RenderBox(const glm::vec3& boundMin, const glm::vec3& boundMax, const glm::mat4& transform, const glm::vec4& color)
	{
		static ResourceHandle s_VAO, s_VBO, s_IBO;
		static glm::vec3 s_LastMin(FLT_MAX), s_LastMax(FLT_MAX);

		glm::vec3 center  = (boundMin + boundMax) * 0.5f;
		glm::vec3 extents = glm::max((boundMax - boundMin) * 0.5f, glm::vec3(0.1f));
		glm::vec3 clampedMin = center - extents;
		glm::vec3 clampedMax = center + extents;

		glm::vec3 l[8] = {
			{clampedMin.x, clampedMin.y, clampedMin.z}, {clampedMax.x, clampedMin.y, clampedMin.z},
			{clampedMax.x, clampedMax.y, clampedMin.z}, {clampedMin.x, clampedMax.y, clampedMin.z},
			{clampedMin.x, clampedMin.y, clampedMax.z}, {clampedMax.x, clampedMin.y, clampedMax.z},
			{clampedMax.x, clampedMax.y, clampedMax.z}, {clampedMin.x, clampedMax.y, clampedMax.z}
		};

		if (!s_VAO.IsValid())
		{
			uint32_t indices[24] = {
				0,1, 1,2, 2,3, 3,0,
				4,5, 5,6, 6,7, 7,4,
				0,4, 1,5, 2,6, 3,7
			};
			s_VBO = ResourceManager::CreateResource<VertexBuffer>(sizeof(l));
			ResourceManager::GetResource<VertexBuffer>(s_VBO)->SetLayout(MeshLayout::Vertex());
			s_IBO = ResourceManager::CreateResource<IndexBuffer>(indices, 24);
			s_VAO = ResourceManager::CreateResource<VertexArray>();
			auto* vao = ResourceManager::GetResource<VertexArray>(s_VAO);
			vao->AddVertexBuffer(ResourceManager::GetResource<VertexBuffer>(s_VBO));
			vao->SetIndexBuffer(ResourceManager::GetResource<IndexBuffer>(s_IBO));
		}

		if (clampedMin != s_LastMin || clampedMax != s_LastMax)
		{
			ResourceManager::GetResource<VertexBuffer>(s_VBO)->SetData(l, sizeof(l), 0);
			s_LastMin = clampedMin;
			s_LastMax = clampedMax;
		}

		auto* shader = ResourceManager::GetResource<Shader>(s_RenderData->lineShader);
		shader->Bind();
		shader->SetMat4("u_ViewProjection", s_SceneData->camera.ViewProjection);
		shader->SetMat4("u_Model", transform);
		shader->SetFloat4("u_Color", color);
		RenderCommand::SetDepthFuncEqual(State::ALWAYS);
		RenderCommand::DrawIndexedLines(ResourceManager::GetResource<VertexArray>(s_VAO), 24);
		RenderCommand::SetDepthFuncEqual(State::LEQUAL);
	}

	void Renderer::RenderCapsule(float radius, float halfCylinderHeight, const glm::mat4& transform, const glm::vec4& color)
	{
		constexpr int segments     = 24;
		constexpr int hemiSegments = segments / 2;
		constexpr int maxVertices  = segments * 2 + (hemiSegments + 1) * 2 * 2;

		static ResourceHandle s_VAO, s_VBO, s_IBO_Cylinder, s_IBO_Hemisphere;
		static glm::vec3 s_Vertices[maxVertices];
		static float s_LastRadius = -1.f, s_LastHalfHeight = -1.f;

		if (radius != s_LastRadius || halfCylinderHeight != s_LastHalfHeight)
		{
			int v = 0;
			glm::vec3 topCenter(0,  halfCylinderHeight, 0);
			glm::vec3 bottomCenter(0, -halfCylinderHeight, 0);

			int cylinderStart = v;
			for (int s = 0; s < segments; s++)
			{
				float theta = (float)s / segments * glm::two_pi<float>();
				float x = cos(theta) * radius;
				float z = sin(theta) * radius;
				s_Vertices[v++] = topCenter    + glm::vec3(x, 0, z);
				s_Vertices[v++] = bottomCenter + glm::vec3(x, 0, z);
			}

			int hemiStart = v;
			for (int axis = 0; axis < 2; axis++)
			{
				for (int s = 0; s <= hemiSegments; s++)
				{
					float phi = (float)s / hemiSegments * glm::pi<float>();
					float y   = sin(phi) * radius;
					float r   = cos(phi) * radius;
					float x   = axis == 0 ? r : 0.0f;
					float z   = axis == 1 ? r : 0.0f;
					s_Vertices[v++] = topCenter    + glm::vec3( x,  y,  z);
					s_Vertices[v++] = bottomCenter - glm::vec3( x,  y,  z);
				}
			}

			if (!s_VAO.IsValid())
			{
				s_VBO = ResourceManager::CreateResource<VertexBuffer>(sizeof(s_Vertices));
				ResourceManager::GetResource<VertexBuffer>(s_VBO)->SetLayout(MeshLayout::Vertex());
				s_VAO = ResourceManager::CreateResource<VertexArray>();
				ResourceManager::GetResource<VertexArray>(s_VAO)
					->AddVertexBuffer(ResourceManager::GetResource<VertexBuffer>(s_VBO));

				{
					uint32_t cylIndices[segments * 6];
					int idx = 0;
					for (int s = 0; s < segments; s++)
					{
						int curr = cylinderStart + s * 2;
						int next = cylinderStart + ((s + 1) % segments) * 2;
						cylIndices[idx++] = curr;     cylIndices[idx++] = next;
						cylIndices[idx++] = curr + 1; cylIndices[idx++] = next + 1;
						cylIndices[idx++] = curr;     cylIndices[idx++] = curr + 1;
					}
					s_IBO_Cylinder = ResourceManager::CreateResource<IndexBuffer>(cylIndices, idx);
				}
				{
					uint32_t hemiIndices[2048];
					int idx = 0;
					for (int axis = 0; axis < 2; axis++)
					{
						int offset = hemiStart + axis * (hemiSegments + 1) * 2;
						for (int s = 1; s <= hemiSegments; s++)
						{
							int curr = offset + (s - 1) * 2;
							int next = offset + s * 2;
							hemiIndices[idx++] = curr;     hemiIndices[idx++] = next;
							hemiIndices[idx++] = curr + 1; hemiIndices[idx++] = next + 1;
						}
					}
					s_IBO_Hemisphere = ResourceManager::CreateResource<IndexBuffer>(hemiIndices, idx);
				}
			}

			ResourceManager::GetResource<VertexBuffer>(s_VBO)->SetData(s_Vertices, sizeof(glm::vec3) * v, 0);
			s_LastRadius     = radius;
			s_LastHalfHeight = halfCylinderHeight;
		}

		auto* vao    = ResourceManager::GetResource<VertexArray>(s_VAO);
		auto* shader = ResourceManager::GetResource<Shader>(s_RenderData->lineShader);
		shader->Bind();
		shader->SetMat4("u_ViewProjection", s_SceneData->camera.ViewProjection);
		shader->SetMat4("u_Model", transform);
		shader->SetFloat4("u_Color", color);
		RenderCommand::SetDepthFuncEqual(State::ALWAYS);
		vao->SetIndexBuffer(ResourceManager::GetResource<IndexBuffer>(s_IBO_Cylinder));
		RenderCommand::DrawIndexedLines(vao, ResourceManager::GetResource<IndexBuffer>(s_IBO_Cylinder)->GetCount());
		vao->SetIndexBuffer(ResourceManager::GetResource<IndexBuffer>(s_IBO_Hemisphere));
		RenderCommand::DrawIndexedLines(vao, ResourceManager::GetResource<IndexBuffer>(s_IBO_Hemisphere)->GetCount());
		RenderCommand::SetDepthFuncEqual(State::LEQUAL);
	}

	void Renderer::RenderSphere(float radius, const glm::mat4& transform, const glm::vec4& color)
	{
		constexpr int segments    = 32;
		constexpr int maxVertices = segments * 3;

		static ResourceHandle s_VAO, s_VBO, s_IBO;

		if (!s_VAO.IsValid())
		{
			glm::vec3 vertices[maxVertices];
			int v = 0;
			for (int s = 0; s < segments; s++) { float t = (float)s / segments * glm::two_pi<float>(); vertices[v++] = glm::vec3(cos(t), sin(t), 0.0f); }
			for (int s = 0; s < segments; s++) { float t = (float)s / segments * glm::two_pi<float>(); vertices[v++] = glm::vec3(cos(t), 0.0f,  sin(t)); }
			for (int s = 0; s < segments; s++) { float t = (float)s / segments * glm::two_pi<float>(); vertices[v++] = glm::vec3(0.0f,  cos(t), sin(t)); }

			s_VBO = ResourceManager::CreateResource<VertexBuffer>(sizeof(vertices));
			ResourceManager::GetResource<VertexBuffer>(s_VBO)->SetLayout(MeshLayout::Vertex());

			uint32_t indices[segments * 2 * 3];
			int idx = 0;
			for (int circle = 0; circle < 3; circle++)
			{
				int base = circle * segments;
				for (int s = 0; s < segments; s++) { indices[idx++] = base + s; indices[idx++] = base + (s + 1) % segments; }
			}
			s_IBO = ResourceManager::CreateResource<IndexBuffer>(indices, idx);
			s_VAO = ResourceManager::CreateResource<VertexArray>();
			auto* vao = ResourceManager::GetResource<VertexArray>(s_VAO);
			vao->AddVertexBuffer(ResourceManager::GetResource<VertexBuffer>(s_VBO));
			vao->SetIndexBuffer(ResourceManager::GetResource<IndexBuffer>(s_IBO));
			ResourceManager::GetResource<VertexBuffer>(s_VBO)->SetData(vertices, sizeof(vertices), 0);
		}

		glm::mat4 model  = transform * glm::scale(glm::mat4(1.0f), glm::vec3(radius));
		auto*     shader = ResourceManager::GetResource<Shader>(s_RenderData->lineShader);
		shader->Bind();
		shader->SetMat4("u_ViewProjection", s_SceneData->camera.ViewProjection);
		shader->SetMat4("u_Model", model);
		shader->SetFloat4("u_Color", color);
		RenderCommand::SetDepthFuncEqual(State::ALWAYS);
		RenderCommand::DrawIndexedLines(
			ResourceManager::GetResource<VertexArray>(s_VAO),
			ResourceManager::GetResource<IndexBuffer>(s_IBO)->GetCount());
		RenderCommand::SetDepthFuncEqual(State::LEQUAL);
	}

	void Renderer::CalculateDirectionalMat(const Camera& camera, const LightParam& light, glm::mat4& view, glm::mat4& proj, float zMultiplier)
	{
		glm::mat4 invCam = glm::inverse(camera.GetProjection() * camera.GetView());
		std::vector<glm::vec4> frustumCorners;
		for (unsigned int x = 0; x < 2; ++x)
			for (unsigned int y = 0; y < 2; ++y)
				for (unsigned int z = 0; z < 2; ++z)
				{
					glm::vec4 pt = invCam * glm::vec4(2.0f*x-1.0f, 2.0f*y-1.0f, 2.0f*z-1.0f, 1.0f);
					frustumCorners.push_back(pt / pt.w);
				}

		glm::vec3 center(0.0f);
		for (const auto& v : frustumCorners) center += glm::vec3(v);
		center /= (float)frustumCorners.size();

		glm::vec3 up = (std::abs(light.direction.y) > 0.999f) ? glm::vec3(0,0,1) : glm::vec3(0,1,0);

		auto computeAABB = [&](const glm::mat4& v, float& minX, float& maxX, float& minY, float& maxY, float& minZ, float& maxZ)
		{
			minX = minY = minZ = std::numeric_limits<float>::max();
			maxX = maxY = maxZ = std::numeric_limits<float>::lowest();
			for (const auto& fc : frustumCorners)
			{
				glm::vec4 trf = v * fc;
				minX = std::min(minX, trf.x); maxX = std::max(maxX, trf.x);
				minY = std::min(minY, trf.y); maxY = std::max(maxY, trf.y);
				minZ = std::min(minZ, trf.z); maxZ = std::max(maxZ, trf.z);
			}
		};

		float minX, maxX, minY, maxY, minZ, maxZ;
		glm::vec3 lightPos = center - glm::normalize(light.direction);
		view = glm::lookAt(lightPos, center, up);
		computeAABB(view, minX, maxX, minY, maxY, minZ, maxZ);

		float radius = glm::distance(glm::vec3(maxX,maxY,maxZ), glm::vec3(minX,minY,minZ)) / 2.0f;
		lightPos = center - glm::normalize(light.direction) * radius;
		view = glm::lookAt(lightPos, center, up);
		computeAABB(view, minX, maxX, minY, maxY, minZ, maxZ);

		if (minZ < 0) minZ *= zMultiplier; else minZ /= zMultiplier;
		if (maxZ < 0) maxZ /= zMultiplier; else maxZ *= zMultiplier;
		proj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
	}
}