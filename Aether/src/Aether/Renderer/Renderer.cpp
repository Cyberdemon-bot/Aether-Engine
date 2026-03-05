#include "aepch.h"
#include "Aether/Renderer/Renderer.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Core/Application.h"
#include "Aether/Assets/AssetsManager.h"
#include "Aether/Renderer/BuiltinShader.h"

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
		RenderCommand::SetDepthFuncEqual(State::LEQUAL);

		s_RenderData->s_InstanceVBO = VertexBuffer::Create(10 * sizeof(glm::mat4));
		s_RenderData->s_InstanceVBO->SetLayout(layout);

		s_RenderData->CameraUB = UniformBuffer::Create(sizeof(CameraData)); s_RenderData->CameraUB->Bind(0);
		s_RenderData->BoneUB = UniformBuffer::Create(sizeof(glm::mat4) * 100); s_RenderData->BoneUB->Bind(1);
		s_RenderData->LightUB = UniformBuffer::Create(sizeof(LightsData)); s_RenderData->LightUB->Bind(2);
		
		s_RenderData->s_ScreenShader = Shader::Create(ShaderProgramSource{VScreenShader, FScreenShader});
		s_RenderData->lineShader = Shader::Create(ShaderProgramSource{VLineShader, FLineShader});
		s_RenderData->s_SkyboxShader = Shader::Create(ShaderProgramSource{VSkyboxShader, FSkyboxShader}); s_RenderData->s_SkyboxShader->SetUBOSlot("Camera", 0);

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

	void Renderer::SetLutMap(Ref<Texture2D> lut_map)
	{
		s_RenderData->s_LutMap = lut_map;
	}
	
	void Renderer::SetSkyBox(Ref<TextureCube> skybox)
	{
		s_RenderData->s_Skybox = skybox;
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

	void Renderer::BeginScene(const Camera& camera, const std::vector<LightParam>& lights)
	{
		s_SceneData->camera.Position = camera.GetPosition();
		s_SceneData->camera.View = camera.GetView();
		s_SceneData->camera.ViewProjection = camera.GetViewProjection();

		s_SceneData->lights.lightCount = std::min(lights.size(), (size_t)16);
		for (size_t i = 0; i < s_SceneData->lights.lightCount; i++)
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
					glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
					glm::vec3 dir = glm::normalize(light.direction); 
					if (glm::abs(glm::dot(dir, up)) > 0.99f) 
					{
						up = glm::vec3(0.0f, 0.0f, 1.0f); 
					}
					lightView = glm::lookAt(light.position, light.position + dir, up);
				}
				else if (light.type == LightType::Directional) CalculateDirectionalMat(camera, light, lightView, lightProjection);
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
			if (pass.IsActive)
			{
				Flush(pass);
				if (pass.OnScreen) mainPass = &pass;
			}
		}
		if (mainPass) RenderOnScreen(*mainPass);
		s_SceneData->s_RenderBatches.clear();
		s_SceneData->lights.lightCount = 0;
	}

	void Renderer::RenderSkybox()
	{
		if (!s_RenderData->s_Skybox) return;
		s_RenderData->s_Skybox->Bind(0);
		s_RenderData->s_SkyboxShader->Bind();
		s_RenderData->s_SkyboxShader->SetInt("u_Skybox", 0);
		
		RenderCommand::SetCullingMode(State::None);
		RenderCommand::DrawIndexed(s_RenderData->s_SkyMesh->GetVertexArray());
	}

	void Renderer::RenderOnScreen(const RenderPass& pass)
	{
		auto& window = Application::Get().GetWindow();
		RenderCommand::SetClearColor({0, 0, 0, 1});
		RenderCommand::Clear();
		RenderCommand::SetViewport(0, 0, window.GetFramebufferWidth(), window.GetFramebufferHeight());
		pass.TargetFBO->BindColorTexture(0);
		s_RenderData->s_ScreenShader->Bind();
		s_RenderData->s_ScreenShader->SetInt("u_SceneTexture", 0); 
		if (s_RenderData->s_LutMap)
		{
			s_RenderData->s_LutMap->Bind(1);
			s_RenderData->s_ScreenShader->SetInt("u_HasLut", 1);
			s_RenderData->s_ScreenShader->SetInt("u_LutTexture", 1);
			s_RenderData->s_ScreenShader->SetFloat("u_LutIntensity", pass.LutIntensity);
		}
		else s_RenderData->s_ScreenShader->SetInt("u_HasLut", 0);
		RenderCommand::DrawIndexed(s_RenderData->s_Screen->GetVertexArray());
	}	

	void Renderer::DrawMesh(Ref<Mesh> mesh, const std::vector<Ref<Material>> materials, UUID animatorID, const glm::mat4& transform)
	{
		if (!mesh) return;
		const auto& submeshes = mesh->GetSubMeshes();
		if (!s_RenderData->s_MeshPtrInstanceAssigned[mesh])
		{
			mesh->GetVertexArray()->AddInstanceBuffer(s_RenderData->s_InstanceVBO, 6); 
			s_RenderData->s_MeshPtrInstanceAssigned[mesh] = true;
		}

		for (uint32_t i = 0; i < submeshes.size(); i++)
		{
			if (submeshes[i].MaterialIdx >= materials.size()) continue;
			RenderKey key;
			key.mesh = mesh;
			key.subIdx = i;
			key.material = materials[submeshes[i].MaterialIdx];

			if (animatorID == UUID(0)) s_SceneData->s_RenderBatches[key].static_obj.push_back(transform);
			else s_SceneData->s_RenderBatches[key].dynamic_obj.push_back({transform, animatorID});
		}
	}

	void Renderer::SetPassAtrib(uint32_t passIdx, const std::string& name, int value)
	{
		auto& pass = s_RenderData->s_PassList[passIdx];
		for (auto& attrib : pass.attribList)
			if (attrib.first == name) attrib.second = value;
	}

	void Renderer::Flush(const RenderPass& pass)
	{
		Ref<Mesh> currentMesh;
		Ref<Material> currentMaterial;
		UUID currentAnimatorID = 0;
		int startSlot = 0;
		auto shader = pass.Shader; shader->Bind();
		auto fbo = pass.TargetFBO; fbo->Bind();

		if (pass.ClearColor && pass.ClearDepth) 
		{
        	if (pass.ClearColor) RenderCommand::SetClearColor(pass.ClearValue);
			RenderCommand::Clear();  
		}
		else if (pass.ClearColor)
		{
			if (pass.ClearColor) RenderCommand::SetClearColor(pass.ClearValue);
			RenderCommand::ClearColor();  
		}
		else if (pass.ClearDepth) RenderCommand::ClearDepth();  
		if (pass.CullFace == State::FRONT_CULL) RenderCommand::SetCullingMode(State::FRONT_CULL);
		if (pass.CullFace == State::BACK_CULL) RenderCommand::SetCullingMode(State::BACK_CULL);
		if (pass.CullFace == State::None) RenderCommand::SetCullingMode(State::None);
		RenderCommand::SetViewport(0, 0, fbo->GetSpecification().Width, fbo->GetSpecification().Height);

		for (auto [name, texture] : pass.readList)
		{
			texture->Bind(startSlot);
			shader->SetInt(name, startSlot);
			startSlot++;
		}
		for (auto& [name, value] : pass.attribList) shader->SetInt(name, value);

		if (pass.UsingGeometry)
		{
			auto skelSystem = AnimationSystem::GetModule<RigModule>();
			for (auto& [key, transforms] : s_SceneData->s_RenderBatches)
			{
				std::sort(transforms.dynamic_obj.begin(), transforms.dynamic_obj.end(), 
				[](const std::pair<glm::mat4, UUID>& a, const std::pair<glm::mat4, UUID>& b) {return a.second < b.second;}); 
				for (const auto& transform : transforms.dynamic_obj) skelSystem->RequestMatrices(transform.second);
				skelSystem->ProcessRequests(); // calc animation
			}

			for (auto& [key, transforms] : s_SceneData->s_RenderBatches)
			{
				auto material = key.material; 
				if (currentMaterial != material && pass.UsingMaterial)
				{
					material->UploadMaterial(shader, startSlot);
					currentMaterial = material;
				}

				auto mesh = key.mesh;
				if (currentMesh != key.mesh) 
				{
					mesh->GetVertexArray()->Bind();
					currentMesh = key.mesh;
				}
				const auto& submesh = mesh->GetSubMeshes()[key.subIdx];
				void* indexOffset = (void*)(submesh.BaseIndex * sizeof(uint32_t));

				if (!transforms.dynamic_obj.empty())
				{
					shader->SetInt("u_UseInstancing", 0);
					shader->SetInt("u_HasAnimation", 1);

					for (const auto& transform : transforms.dynamic_obj) // render non static
					{
						shader->SetMat4("u_Model", transform.first);
						if (currentAnimatorID != transform.second)
						{
							const auto& boneMatrices = skelSystem->GetMatrices(transform.second);
							if (!boneMatrices.empty()) s_RenderData->BoneUB->SetData(boneMatrices.data(), boneMatrices.size() * sizeof(glm::mat4));
							currentAnimatorID = transform.second;
						}
						RenderCommand::DrawIndexedBaseVertex(mesh->GetVertexArray(), submesh.IndexCount, indexOffset, submesh.BaseVertex);
					}
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
			
		}
		else RenderCommand::DrawIndexed(s_RenderData->s_Screen->GetVertexArray());
		if (pass.UsingSkybox) RenderSkybox();
		fbo->Unbind();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::RenderBox(const glm::vec3& boundMin, const glm::vec3& boundMax, const glm::mat4& transform, const glm::vec4& color)
	{
		glm::vec3 l[8] = {
			{boundMin.x, boundMin.y, boundMin.z}, {boundMax.x, boundMin.y, boundMin.z},
			{boundMax.x, boundMax.y, boundMin.z}, {boundMin.x, boundMax.y, boundMin.z},
			{boundMin.x, boundMin.y, boundMax.z}, {boundMax.x, boundMin.y, boundMax.z},
			{boundMax.x, boundMax.y, boundMax.z}, {boundMin.x, boundMax.y, boundMax.z}
		};
		glm::vec3 w[512];
		for (int i = 0; i < 8; i++) w[i] = glm::vec3(transform * glm::vec4(l[i], 1.0f));
		static Aether::Ref<Aether::VertexArray> s_VAO;
		static Aether::Ref<Aether::VertexBuffer> s_VBO;

		if (!s_VAO)
		{
			s_VAO = Aether::VertexArray::Create();
			s_VBO = Aether::VertexBuffer::Create(sizeof(w)); 
			s_VBO->SetLayout(MeshLayout::Vertex());
			s_VAO->AddVertexBuffer(s_VBO);
			uint32_t indices[24] = {
				0,1, 1,2, 2,3, 3,0,
				4,5, 5,6, 6,7, 7,4, 
				0,4, 1,5, 2,6, 3,7  
			};
			Aether::Ref<Aether::IndexBuffer> ibo = Aether::IndexBuffer::Create(indices, 24);
			s_VAO->SetIndexBuffer(ibo);
		}

		s_VBO->SetData(w, sizeof(w), 0); 
		auto shader = s_RenderData->lineShader;
		shader->Bind();
		shader->SetMat4("u_ViewProjection", s_SceneData->camera.ViewProjection);
		shader->SetFloat4("u_Color", color);
		RenderCommand::SetDepthFuncEqual(State::ALWAYS);
		RenderCommand::DrawIndexedLines(s_VAO, 24);
		RenderCommand::SetDepthFuncEqual(State::LEQUAL);
	}

	void Renderer::RenderCapsule(float radius, float halfHeight, const glm::mat4& transform, const glm::vec4& color)
	{
		constexpr int segments      = 24;
		constexpr int hemiSegments  = segments / 2;
		constexpr int maxVertices   = 512;

		static Aether::Ref<Aether::VertexArray> s_VAO;
		static Aether::Ref<Aether::VertexBuffer> s_VBO;

		static Aether::Ref<Aether::IndexBuffer> s_IBO_Cylinder;
		static Aether::Ref<Aether::IndexBuffer> s_IBO_Hemisphere;

		static glm::vec3 vertices[maxVertices];

		int v = 0;

		glm::vec3 topCenter(0,  halfHeight, 0);
		glm::vec3 bottomCenter(0, -halfHeight, 0);

		int cylinderStart = v;

		for (int s = 0; s < segments; s++)
		{
			float theta = (float)s / segments * glm::two_pi<float>();
			float x = cos(theta) * radius;
			float z = sin(theta) * radius;

			vertices[v++] = topCenter    + glm::vec3(x, 0, z);
			vertices[v++] = bottomCenter + glm::vec3(x, 0, z);
		}

		

		int hemiStart = v;

		for (int axis = 0; axis < 2; axis++)
		{
			for (int s = 0; s <= hemiSegments; s++)
			{
				float phi = (float)s / hemiSegments * glm::pi<float>();
				float y = sin(phi) * radius;
				float r = cos(phi) * radius;

				float x = axis == 0 ? r : 0.0f;
				float z = axis == 1 ? r : 0.0f;

				vertices[v++] = topCenter    + glm::vec3( x, y,  z);
				vertices[v++] = bottomCenter - glm::vec3( x, y,  z);
			}
		}

		

		for (int k = 0; k < v; k++)
			vertices[k] = glm::vec3(transform * glm::vec4(vertices[k], 1.0f));

		

		if (!s_VAO)
		{
			s_VAO = Aether::VertexArray::Create();
			s_VBO = Aether::VertexBuffer::Create(sizeof(vertices));
			s_VBO->SetLayout(MeshLayout::Vertex());
			s_VAO->AddVertexBuffer(s_VBO);

			
			{
				uint32_t cylIndices[segments * 4];
				int idx = 0;

				for (int s = 0; s < segments; s++)
				{
					int curr = cylinderStart + s * 2;
					int next = cylinderStart + ((s + 1) % segments) * 2;

					cylIndices[idx++] = curr;
					cylIndices[idx++] = next;

					cylIndices[idx++] = curr + 1;
					cylIndices[idx++] = next + 1;

					cylIndices[idx++] = curr;
    				cylIndices[idx++] = curr + 1;
				}

				s_IBO_Cylinder = Aether::IndexBuffer::Create(cylIndices, idx);
			}

			
			{
				uint32_t hemiIndices[2048];
				int idx = 0;

				int base = hemiStart;

				for (int axis = 0; axis < 2; axis++)
				{
					int offset = base + axis * (hemiSegments + 1) * 2;

					for (int s = 1; s <= hemiSegments; s++)
					{
						int curr = offset + (s - 1) * 2;
						int next = offset + s * 2;

						hemiIndices[idx++] = curr;
						hemiIndices[idx++] = next;

						hemiIndices[idx++] = curr + 1;
						hemiIndices[idx++] = next + 1;
					}
				}

				s_IBO_Hemisphere = Aether::IndexBuffer::Create(hemiIndices, idx);
			}
		}

		s_VBO->SetData(vertices, sizeof(glm::vec3) * v, 0);

		auto shader = s_RenderData->lineShader;
		shader->Bind();
		shader->SetMat4("u_ViewProjection", s_SceneData->camera.ViewProjection);
		shader->SetFloat4("u_Color", color);

		RenderCommand::SetDepthFuncEqual(State::ALWAYS);
		s_VAO->SetIndexBuffer(s_IBO_Cylinder);
		RenderCommand::DrawIndexedLines(s_VAO,
			s_IBO_Cylinder->GetCount());
		s_VAO->SetIndexBuffer(s_IBO_Hemisphere);
		RenderCommand::DrawIndexedLines(s_VAO,
			s_IBO_Hemisphere->GetCount());

		RenderCommand::SetDepthFuncEqual(State::LEQUAL);
	}

	void Renderer::CalculateDirectionalMat(const Camera& camera, const LightParam& light, glm::mat4& view, glm::mat4& proj, float zMultiplier)
	{
		glm::mat4 invCam = glm::inverse(camera.GetProjection() * camera.GetView());
		std::vector<glm::vec4> frustumCorners;
		for (unsigned int x = 0; x < 2; ++x) 
		{
			for (unsigned int y = 0; y < 2; ++y) 
			{
				for (unsigned int z = 0; z < 2; ++z) 
				{
					glm::vec4 pt = invCam * glm::vec4(
						2.0f * x - 1.0f,
						2.0f * y - 1.0f,
						2.0f * z - 1.0f,
						1.0f);
					frustumCorners.push_back(pt / pt.w); 
				}
			}
		}

		glm::vec3 center = glm::vec3(0.0f);
		for (const auto& v : frustumCorners) center += glm::vec3(v);
		center /= frustumCorners.size();

		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		if (std::abs(light.direction.y) > 0.999f) up = glm::vec3(0.0f, 0.0f, 1.0f);

		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::lowest();
		float minZ = std::numeric_limits<float>::max();
		float maxZ = std::numeric_limits<float>::lowest();

		float radius = glm::distance(glm::vec3(maxX, maxY, maxZ), glm::vec3(minX, minY, minZ)) / 2.0f;
		glm::vec3 lightPos = center - (glm::normalize(light.direction) * radius);
		view = glm::lookAt(lightPos, center, up);

		for (const auto& v : frustumCorners) {
			glm::vec4 trf = view * v;
			minX = std::min(minX, trf.x);
			maxX = std::max(maxX, trf.x);
			minY = std::min(minY, trf.y);
			maxY = std::max(maxY, trf.y);
			minZ = std::min(minZ, trf.z);
			maxZ = std::max(maxZ, trf.z);
		}

		if (minZ < 0) minZ *= zMultiplier; else minZ /= zMultiplier;
		if (maxZ < 0) maxZ /= zMultiplier; else maxZ *= zMultiplier;

		proj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
	}
}
