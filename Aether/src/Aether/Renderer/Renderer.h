#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Renderer/RenderCommand.h"
#include "Aether/Renderer/EditorCamera.h"
#include "Aether/Renderer/UniformBuffer.h"
#include "Aether/Renderer/Framebuffer.h"
#include "Aether/Renderer/Shader.h"
#include "Aether/Resources/Mesh.h"
#include "Aether/Resources/Material.h"
#include "Aether/Renderer/Camera.h"
#include "Aether/Core/UUID.h"
#include <glm/glm.hpp>
#include <tuple>
#define MAX_LIGHTS 16

namespace Aether {

	enum class LightType
	{
		None = 0, Spot, Directional
	};
	
	enum class TextureType
	{
		None = 0, Depth, Color
	};

	struct LightParam
	{
		LightType type;
		glm::vec3 position;
		glm::vec3 direction;   
		glm::vec3 color;
		float intensity;
		float range;           
		float innerCone;       
		float outerCone;      
		bool castShadows;
	};

	struct Light
	{
		glm::vec4 positionAndType;    
		glm::vec4 directionAndRange;  
		glm::vec4 colorAndIntensity; 
		glm::vec4 coneAngles;     
		glm::mat4 lightSpaceMatrix;
	};

	struct LightsData
	{
		Light lights[MAX_LIGHTS];
		int lightCount;
		float _pad[3];
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

	struct RenderPass
	{
		Ref<Framebuffer> TargetFBO;        
		Ref<Shader> Shader;                
		bool IsActive = true;
		bool ClearColor = true;
		bool ClearDepth = true;
		bool OnScreen = true;
		bool UsingSkybox = false;
		bool UsingMaterial = true;
		bool UsingGeometry = true;
		State CullFace = State::None;
		std::vector<std::tuple<TextureType, std::string, uint32_t>> readList;
		glm::vec4 ClearValue = {0, 0, 0, 1};
		float LutIntensity = 0.0f;
	};

	class AETHER_API Renderer
	{
	public:
		static void Init();
		static void Shutdown();
		
		static void OnWindowResize(uint32_t width, uint32_t height);

		static void SetPipeline(const std::vector<RenderPass>& list);
		static void SetPassReadIndex(uint32_t PassIdx, uint32_t AttribIdx, uint32_t val);
		static void ActivatePass(uint32_t PassIdx);
		static void DeactivatePass(uint32_t PassIdx);

		static void BeginScene(const Camera& camera, const std::vector<LightParam>& lights = {});
		static void EndScene();

		static void DrawMesh(UUID meshID,  UUID animatorID, const glm::mat4& transform); //UUID(0) animator for static
		static void DrawBox(UUID meshID, const glm::mat4& transform);

		static void RenderBox(const glm::vec3& boundMin, const glm::vec3& boundMax, const glm::mat4& transform, const glm::vec4& color);

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	private:
		static void Flush(const RenderPass& pass);
		static void RenderOnScreen(const RenderPass& pass);
		static void RenderSkybox();
		static void CalculateDirectionalMat(const Camera& camera, const LightParam& light, glm::mat4& view, glm::mat4& proj, float zMultiplier = 10.0f);

		struct SceneData
		{
			CameraData camera;
			LightsData lights; 
			std::map<RenderKey, BatchData> s_RenderBatches;
			std::vector<std::pair<UUID, glm::mat4>> s_RenderBounds;
		};

		struct RenderData
		{
			Ref<UniformBuffer> CameraUB;
			Ref<UniformBuffer> BoneUB;
			Ref<UniformBuffer> LightUB;
			Ref<VertexBuffer> s_InstanceVBO;
			Ref<Mesh> s_Screen;
			Ref<Mesh> s_SkyMesh;
			Ref<Shader> s_ScreenShader;
			Ref<Shader> s_SkyboxShader;
			Ref<Texture2D> s_LutMap;
			Ref<TextureCube> s_Skybox;
			Ref<Shader> lineShader;
			
			std::vector<RenderPass> s_PassList;
			std::unordered_map<UUID, bool> s_MeshInstanceAssigned;
		};

		static Scope<SceneData> s_SceneData;
    	static Scope<RenderData> s_RenderData;
	};
}
