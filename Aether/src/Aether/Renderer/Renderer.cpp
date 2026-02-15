#include "aepch.h"
#include "Aether/Renderer/Renderer.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigSystem.h"
#include "Aether/Core/Application.h"

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

	Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<Renderer::SceneData>();
	Scope<Renderer::RenderData> Renderer::s_RenderData = CreateScope<Renderer::RenderData>();

	void Renderer::Init()
	{
		RenderCommand::Init();
		BufferLayout layout = { { "a_InstanceModel", ShaderDataType::Mat4 } };

		s_RenderData->s_InstanceVBO = VertexBuffer::Create(10 * sizeof(glm::mat4));
		s_RenderData->s_InstanceVBO->SetLayout(layout);

		s_RenderData->CameraUB = UniformBuffer::Create(sizeof(CameraData)); s_RenderData->CameraUB->Bind(0);
		s_RenderData->BoneUB = UniformBuffer::Create(sizeof(glm::mat4) * 100); s_RenderData->BoneUB->Bind(1);
		s_RenderData->LightUB = UniformBuffer::Create(sizeof(LightsData)); s_RenderData->LightUB->Bind(2);
		
		s_RenderData->s_ScreenShader = Shader::Create("assets/shaders/Screen.shader");
		s_RenderData->s_SkyboxShader = Shader::Create("assets/shaders/Skybox.shader"); s_RenderData->s_SkyboxShader->SetUBOSlot("Camera", 0);
		s_RenderData->s_LutMap = Texture2D::Create("assets/textures/LUT.png", true, false);
		s_RenderData->s_Skybox = TextureCube::Create("assets/textures/skybox.png");

		s_RenderData->s_Screen = Mesh::Create(
			MeshSpec{{VertexStream{quadVertices, 4, MeshLayout::Quad()}}, quadIndices, 6});

		s_RenderData->s_SkyMesh = Mesh::Create(
			MeshSpec{{VertexStream{skyboxVertices, 8, MeshLayout::Vertex()}}, skyboxIndices, 36});
	}

	void Renderer::Shutdown()
	{
		s_SceneData.reset();
		s_RenderData.reset(); 
	}

	void Renderer::SetPipeline(const std::vector<RenderPass>& list)
	{
		s_RenderData->s_PassList = list;
	}

	void Renderer::BeginScene(const Camera& camera, const std::vector<LightParam>& lights)
	{
		s_SceneData->camera.Position = camera.GetPosition();
		s_SceneData->camera.View = camera.GetView();
		s_SceneData->camera.ViewProjection = camera.GetViewProjection();

		s_SceneData->lights.lightCount = lights.size();
		for (size_t i = 0; i < lights.size(); i++)
		{
			const LightParam& light = lights[i];
			s_SceneData->lights.lights[i].positionAndType = glm::vec4(light.position, (float)light.type);
			s_SceneData->lights.lights[i].directionAndRange = glm::vec4(light.direction, light.range);
			s_SceneData->lights.lights[i].colorAndIntensity = glm::vec4(light.color, light.intensity);
			s_SceneData->lights.lights[i].coneAngles = glm::vec4(light.innerCone, light.outerCone, light.castShadows ? 1.0f : 0.0f, 0);
			if (light.castShadows)
			{
				glm::mat4 lightProjection;
				glm::mat4 lightView;
				
				if (light.type == LightType::Spot)
				{
					float fov = glm::acos(light.outerCone) * 2.0f;
					lightProjection = glm::perspective(fov, 1.0f, 0.1f, light.range);
					lightView = glm::lookAt(light.position, light.position + light.direction, glm::vec3(0, 1, 0));
				}
				else if (light.type == LightType::Directional)
				{
					float orthoSize = 20.0f;
					lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 100.0f);
					lightView = glm::lookAt(-light.direction * 10.0f, glm::vec3(0), glm::vec3(0, 1, 0));
				}
				s_SceneData->lights.lights[i].lightSpaceMatrix = lightProjection * lightView;
			}
			else s_SceneData->lights.lights[i].lightSpaceMatrix = glm::mat4(1.0f);
		}
	}	

	void Renderer::EndScene()
	{
		RenderPass* mainPass = nullptr;
		s_RenderData->CameraUB->SetData(&s_SceneData->camera, sizeof(CameraData));
		s_RenderData->LightUB->SetData(&s_SceneData->lights, sizeof(LightsData));
		for (auto& pass : s_RenderData->s_PassList)
		{
			Flush(pass);
			if (pass.OnScreen) mainPass = &pass;
		}
		if (mainPass) RenderOnScreen(*mainPass);
		s_SceneData->s_RenderBatches.clear();
	}

	void Renderer::RenderSkybox()
	{
		s_RenderData->s_Skybox->Bind(0);
		s_RenderData->s_SkyboxShader->Bind();
		s_RenderData->s_SkyboxShader->SetInt("u_Skybox", 0);
		
		RenderCommand::SetDepthFuncEqual();
		RenderCommand::DrawIndexed(s_RenderData->s_SkyMesh->GetVertexArray());
		RenderCommand::SetDepthFuncEqual(false);
	}

	void Renderer::RenderOnScreen(const RenderPass& pass)
	{
		auto& window = Application::Get().GetWindow();
		RenderCommand::SetClearColor({0, 0, 0, 1});
		RenderCommand::Clear();
		RenderCommand::SetViewport(0, 0, window.GetFramebufferWidth(), window.GetFramebufferHeight());
		pass.TargetFBO->BindColorTexture(0);
		s_RenderData->s_LutMap->Bind(1);
		s_RenderData->s_ScreenShader->Bind();
		s_RenderData->s_ScreenShader->SetInt("u_SceneTexture", 0); 
		s_RenderData->s_ScreenShader->SetInt("u_LutTexture", 1);
		s_RenderData->s_ScreenShader->SetFloat("u_LutIntensity", pass.m_LutIntensity);
		RenderCommand::DrawIndexed(s_RenderData->s_Screen->GetVertexArray());
	}	

	void Renderer::DrawMesh(UUID meshID,  UUID animatorID, const glm::mat4& transform)
	{
		auto mesh = MeshLibrary::Get(meshID);
		if (!mesh) return;
		const auto& submeshes = mesh->GetSubMeshes();
		if (!s_RenderData->s_MeshInstanceAssigned[meshID]) 
		{
			mesh->GetVertexArray()->AddInstanceBuffer(s_RenderData->s_InstanceVBO, 6); 
			s_RenderData->s_MeshInstanceAssigned[meshID] = true; 
		}

		for (uint32_t i = 0; i < submeshes.size(); i++)
		{
			RenderKey key;
			key.materialID = submeshes[i].MaterialID;
			key.meshID = meshID;
			key.subIdx = i;

			if (animatorID == UUID(0)) s_SceneData->s_RenderBatches[key].static_obj.push_back(transform);
			else s_SceneData->s_RenderBatches[key].dynamic_obj.push_back({transform, animatorID});
		}
	}

	void Renderer::Flush(const RenderPass& pass)
	{
		UUID currentMatID = 0;
    	UUID currentMeshID = 0;
		UUID currentAnimatorID = 0;
		int startSlot = 0;
		auto shader = pass.Shader; shader->Bind();
		auto fbo = pass.TargetFBO; fbo->Bind();
		if (pass.ClearColor || pass.ClearDepth) 
		{
        	if (pass.ClearColor) RenderCommand::SetClearColor(pass.ClearValue);
			RenderCommand::Clear();  
		}

		if (pass.ColorTexIdx >= 0 && pass.ColorTexIdx < s_RenderData->s_PassList.size())
		{
			s_RenderData->s_PassList[pass.ColorTexIdx].TargetFBO->BindColorTexture(startSlot);
			shader->SetInt("u_ColorTex", startSlot);
			startSlot++;
		}

		if (pass.DepthTexIdx >= 0 && pass.DepthTexIdx < s_RenderData->s_PassList.size())
		{
			s_RenderData->s_PassList[pass.DepthTexIdx].TargetFBO->BindDepthTexture(startSlot);
			shader->SetInt("u_DepthTex", startSlot);
			startSlot++;
		}

		RenderCommand::SetViewport(0, 0, fbo->GetSpecification().Width, fbo->GetSpecification().Height);

		for (auto& [key, transforms] : s_SceneData->s_RenderBatches)
		{
			if (currentMatID != key.materialID)
			{
				auto material = MaterialLibrary::Get(key.materialID);
				material->UploadMaterial(shader, startSlot);
				currentMatID = key.materialID;
			}

			auto mesh = MeshLibrary::Get(key.meshID);
			if (currentMeshID != key.meshID) 
			{
				mesh->GetVertexArray()->Bind();
				currentMeshID = key.meshID;
        	}
        	const auto& submesh = mesh->GetSubMeshes()[key.subIdx];
			void* indexOffset = (void*)(submesh.BaseIndex * sizeof(uint32_t));

			shader->SetInt("u_UseInstancing", 0);
			shader->SetInt("u_HasAnimation", 1);
			auto skelSystem = AnimationSystem::GetModule<RigSystem>();

			std::sort(transforms.dynamic_obj.begin(), transforms.dynamic_obj.end(), 
			[](const std::pair<glm::mat4, UUID>& a, const std::pair<glm::mat4, UUID>& b) {return a.second < b.second;}); //sort by animator id

			for (const auto& transform : transforms.dynamic_obj) // render non static
			{
				shader->SetMat4("u_Model", transform.first);
				if (currentAnimatorID != transform.second)
				{
					const auto& boneMatrices = skelSystem->GetMatrices(transform.second);
					if (!boneMatrices.empty()) s_RenderData->BoneUB->SetData(boneMatrices.data(), boneMatrices.size() * sizeof(glm::mat4));

					currentAnimatorID = transform.second;
				}

				RenderCommand::DrawIndexedBaseVertex(
                    mesh->GetVertexArray(),
                    submesh.IndexCount,
                    indexOffset,
                    submesh.BaseVertex
                );
			}

			if (!transforms.static_obj.empty())
			{
				shader->SetInt("u_UseInstancing", 1);
				shader->SetInt("u_HasAnimation", 0);

				uint32_t dataSize = transforms.static_obj.size() * sizeof(glm::mat4);
				if (s_RenderData->s_InstanceVBO->GetSize() < dataSize) 
				{
					uint32_t newSize = dataSize * 2;
					s_RenderData->s_InstanceVBO->Resize(newSize);
				}
				s_RenderData->s_InstanceVBO->SetData(transforms.static_obj.data(), dataSize, 0);
				Aether::RenderCommand::DrawInstancedBaseVertex(mesh->GetVertexArray(), submesh.IndexCount, indexOffset, submesh.BaseVertex, transforms.static_obj.size());
			}
		}
		if (pass.UsingSkybox) RenderSkybox();
		fbo->Unbind();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}
}
