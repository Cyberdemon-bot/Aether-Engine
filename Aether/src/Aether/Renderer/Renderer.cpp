#include "aepch.h"
#include "Aether/Renderer/Renderer.h"
#include "Aether/Core/Application.h"

namespace Aether {

	Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<Renderer::SceneData>();
	Scope<Renderer::RenderData> Renderer::s_RenderData = CreateScope<Renderer::RenderData>();

	void Renderer::Init()
	{
		RenderCommand::Init();
		BufferLayout layout = { { "a_InstanceModel", ShaderDataType::Mat4 } };

		s_RenderData->s_InstanceVBO = VertexBuffer::Create(10 * sizeof(glm::mat4));
		s_RenderData->s_InstanceVBO->SetLayout(layout);
		s_RenderData->CameraUB = UniformBuffer::Create(sizeof(CameraData));
		s_RenderData->BoneUB = UniformBuffer::Create(sizeof(glm::mat4) * 100);

		s_RenderData->CameraUB->Bind(0);
		s_RenderData->BoneUB->Bind(1);

		FramebufferSpecification sceneFbSpec;
		sceneFbSpec.Width = Application::Get().GetWindow().GetWidth();
		sceneFbSpec.Height = Application::Get().GetWindow().GetHeight();
		sceneFbSpec.Attachments = { 
			FramebufferTextureFormat::RGBA8,          
			FramebufferTextureFormat::DEPTH24STENCIL8  
		};

		s_RenderData->mainPass.TargetFBO = FrameBuffer::Create(sceneFbSpec);
		s_RenderData->mainPass.Shader = Shader::Create("assets/shaders/Standard.shader");
		s_RenderData->s_ScreenShader = Shader::Create("assets/shaders/LUT.shader");
		s_RenderData->s_LutMap = Texture2D::Create("assets/textures/LUT.png", true, false);

		s_RenderData->mainPass.Shader->Bind();
		s_RenderData->mainPass.Shader->SetUBOSlot("Camera", 0);
		s_RenderData->mainPass.Shader->SetUBOSlot("Bones", 1);

		float quadVertices[] = { 
        -1.0f,  1.0f,  0.0f, 1.0f,  
        -1.0f, -1.0f,  0.0f, 0.0f,  
         1.0f, -1.0f,  1.0f, 0.0f,  
         1.0f,  1.0f,  1.0f, 1.0f   
		};

		uint32_t quadIndices[] = { 0, 1, 2, 2, 3, 0 };

		s_RenderData->s_Screen = Aether::Mesh::Create(
			Aether::MeshSpec{{Aether::VertexStream{quadVertices, 4, Aether::MeshLayout::Quad()}}, quadIndices, 6});
	}

	void Renderer::Shutdown()
	{
		s_SceneData.reset();
		s_RenderData.reset(); 
	}

	void Renderer::BeginScene(const Camera& camera)
	{
		s_SceneData->camera.Position = camera.GetPosition();
		s_SceneData->camera.View = camera.GetView();
		s_SceneData->camera.ViewProjection = camera.GetViewProjection();
	}	

	void Renderer::EndScene()
	{
		Flush(s_RenderData->mainPass);
		auto& window = Application::Get().GetWindow();
		RenderCommand::Clear();
		RenderCommand::SetViewport(0, 0, window.GetFramebufferWidth(), window.GetFramebufferHeight());

		s_RenderData->mainPass.TargetFBO->BindColorTexture(0);
		s_RenderData->s_LutMap->Bind(1);
		s_RenderData->s_ScreenShader->Bind();
		s_RenderData->s_ScreenShader->SetInt("u_SceneTexture", 0); 
		s_RenderData->s_ScreenShader->SetInt("u_LutTexture", 1);
		s_RenderData->s_ScreenShader->SetFloat("u_LutIntensity", s_RenderData->mainPass.m_LutIntensity);

		RenderCommand::DrawIndexed(s_RenderData->s_Screen->GetVertexArray());
		s_SceneData->s_RenderBatches.clear();
	}

	void Renderer::AddMesh(UUID meshID,  UUID animatorID, const glm::mat4& transform)
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
		s_RenderData->CameraUB->SetData(&s_SceneData->camera, sizeof(CameraData));
		auto shader = pass.Shader; shader->Bind();
		auto fbo = pass.TargetFBO; fbo->Bind();
		if (pass.ClearColor || pass.ClearDepth) 
		{
        	if (pass.ClearColor) RenderCommand::SetClearColor(pass.ClearValue);
			RenderCommand::Clear();  
		}
		RenderCommand::SetViewport(0, 0, fbo->GetSpecification().Width, fbo->GetSpecification().Height);

		for (auto& [key, transforms] : s_SceneData->s_RenderBatches)
		{
			if (currentMatID != key.materialID)
			{
				auto material = MaterialLibrary::Get(key.materialID);
				material->UploadMaterial(shader);
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
			auto skelSystem = AnimationManager::GetSystem<SkeletalAnimationSystem>(AnimationType::Skeletal);

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

				Aether::RenderCommand::DrawIndexedBaseVertex(
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
		fbo->Unbind();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}
}
