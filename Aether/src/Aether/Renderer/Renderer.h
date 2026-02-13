#pragma once

#include "Aether/Renderer/RenderCommand.h"
#include "Aether/Renderer/EditorCamera.h"
#include "Aether/Renderer/UniformBuffer.h"
#include "Aether/Resources/Shader.h"
#include "Aether/Resources/Mesh.h"
#include "Aether/Resources/Material.h"
#include "Aether/Renderer/Camera.h"
#include "Aether/Animation/AnimationManager.h"
#include "Aether/Core/UUID.h"
#include <glm/glm.hpp>

namespace Aether {

	struct LightParameters
    {
        glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;
        float Range = 20.0f;          
        float InnerCone = 12.5f;       
        float OuterCone = 17.5f;      
        bool CastShadows = false;
    };

	enum class LightType
	{
		None = 0,
		Spot, Directional
	};

	struct Environment
    {
        glm::vec3 AmbientColor = { 0.1f, 0.1f, 0.1f };
    };

	struct CameraData
	{
		glm::mat4 ViewProjection;
		glm::mat4 View;
		glm::vec3 Position; float _pad;
	};

	struct RenderKey
	{
		UUID meshID;
		UUID materialID;
		uint32_t subIdx;

		bool operator<(const RenderKey& other) const
		{
			if (materialID != other.materialID) return materialID < other.materialID;
            if (meshID != other.meshID) return meshID < other.meshID;
            return subIdx < other.subIdx;
		}
	};

	struct BatchData
	{
		std::vector<std::pair<glm::mat4, UUID>> dynamic_obj;
		std::vector<glm::mat4> static_obj;
	};

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();
		
		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(const Camera& camera);
		static void EndScene();

		static void AddMesh(UUID meshID,  UUID animatorID, const glm::mat4& transform); //UUID(0) animator for static
		static void AddLight(LightType type, const LightParameters& params, const glm::mat4& transform);

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	private:
		static void Flush();
		struct SceneData
		{
			CameraData camera;
			std::map<RenderKey, BatchData> s_RenderBatches;
		};

		struct RenderData
		{
			Ref<UniformBuffer> CameraUB;
			Ref<UniformBuffer> BoneUB;

			Ref<VertexBuffer> s_InstanceVBO;
			std::unordered_map<UUID, bool> s_MeshInstanceAssigned;
		};

		static Scope<SceneData> s_SceneData;
    	static Scope<RenderData> s_RenderData;
	};
}
