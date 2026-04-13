#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Renderer/RenderCommand.h"
#include "Aether/Renderer/EditorCamera.h"
#include "Aether/Renderer/UniformBuffer.h"
#include "Aether/Renderer/FrameBuffer.h"
#include "Aether/Renderer/Shader.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Assets/Material.h"
#include "Aether/Renderer/Camera.h"
#include "Aether/Renderer/ResourceManager.h"
#include "Aether/Animation/RigModule.h"
#include <glm/glm.hpp>
#include <tuple>
#define MAX_LIGHTS 16

namespace Aether {

	enum class LightType
	{
		None = 0, Spot, Directional
	};

	struct LightParam
	{
		LightType type      = LightType::None;
		glm::vec3 position  = glm::vec3(0.0f);
		glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
		glm::vec3 color     = glm::vec3(1.0f);
		float intensity     = 1.0f;
		float range         = 10.0f;
		float innerCone     = 30.0f;
		float outerCone     = 45.0f;
		bool  castShadows   = false;
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
		Light    lights[MAX_LIGHTS];
		uint32_t shadowMask;
		int      lightCount;
		float    _pad[3];
	};

	struct CameraData
	{
		glm::mat4 ViewProjection;
		glm::mat4 View;
		glm::vec3 Position; float _pad;
	};

	struct RenderKey
	{
		Mesh*     mesh;
		Material* material;
		uint32_t  subIdx;

		bool operator<(const RenderKey& other) const
		{
			if (material != other.material) return material < other.material;
			if (mesh     != other.mesh)     return mesh     < other.mesh;
			return subIdx < other.subIdx;
		}
	};

	struct BatchData
	{
		std::vector<std::pair<glm::mat4, Handle<TaskTag>>> dynamic_obj;
		std::vector<glm::mat4>                  static_obj;
	};

	struct Command
	{
		Mesh*     mesh;
		Material* material;
		uint32_t  subIdx;
		Handle<TaskTag> anim_task;
		glm::mat4 transform;

		bool operator<(const Command& other) const
		{
			bool thisAnim  = anim_task.IsValid();
			bool otherAnim = other.anim_task.IsValid();
			if (thisAnim != otherAnim) return thisAnim > otherAnim; 
			if (material != other.material) return material < other.material;
			if (mesh     != other.mesh)     return mesh     < other.mesh;
			return subIdx < other.subIdx;
		}

		bool operator!=(const Command& other) const
		{
			bool thisAnim  = anim_task.IsValid();
			bool otherAnim = other.anim_task.IsValid();
			return (mesh != other.mesh) || 
				(material != other.material) || 
				(subIdx != other.subIdx) ||
				(thisAnim != otherAnim);
		}
	};

	struct RenderPass
	{
		FrameBuffer* TargetFBO = nullptr;
		Shader* Shader = nullptr;
		bool IsActive = true;
		bool ClearColor = true;
		bool ClearDepth = true;
		bool OnScreen  = true;
		bool UsingSkybox = false;
		bool UsingMaterial = true;
		bool UsingGeometry  = true;
		bool UsingShadowmap = true;
		State CullFace = State::None;
		std::vector<std::pair<std::string, Texture2D*>> readList;
		std::vector<std::pair<std::string, int>> attribList;
		glm::vec4 ClearValue = {0, 0, 0, 1};
		float LutIntensity = 0.0f;
	};

	struct LightCandidate
	{
		int index;
		float score;
	};

	class AETHER_API Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void SetPipeline(const std::vector<RenderPass>& list);
		static void SetLutMap(const std::string& filepath);
		static void SetSkyBox(const std::string& filepath);

		static void ActivatePass(uint32_t PassIdx);
		static void DeactivatePass(uint32_t PassIdx);
		static void SetPassAtrib(uint32_t passIdx, const std::string& name, int value);

		static Texture2D* GetShadowDepthAttachment(uint32_t slot);

		static void BeginScene(const Camera& camera, const std::vector<LightParam>& lights = {});
		static void EndScene();

		static void DrawMesh(Mesh* mesh, const std::vector<Material*> materials, Handle<TaskTag> anim_task, const glm::mat4& transform);

		static void RenderBox(const glm::vec3& boundMin, const glm::vec3& boundMax, const glm::mat4& transform, const glm::vec4& color);
		static void RenderCapsule(float radius, float halfHeight, const glm::mat4& transform, const glm::vec4& color);
		static void RenderSphere(float radius, const glm::mat4& transform, const glm::vec4& color);

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

	private:
		static void Flush(const RenderPass& pass);
		static void RenderOnScreen(const RenderPass& pass);
		static void RenderSkybox();
		static void CalculateDirectionalMat(const Camera& camera, const LightParam& light, glm::mat4& view, glm::mat4& proj, float zMultiplier = 10.0f);

		// ── Change this one constant to scale the entire shadow system ────────
		static const uint32_t MaxShadowCaster = 4;

		struct SceneData
		{
			CameraData camera;
			LightsData lights;
			std::vector<Command>   CommandList;
			std::vector<glm::mat4> batchTransform;
			std::vector<LightCandidate> CandList;
		};

		struct RenderData
		{
			ResourceHandle CameraUB;
			ResourceHandle BoneUB;
			ResourceHandle LightUB;
			ResourceHandle s_InstanceVBO;
			Mesh*          s_Screen   = nullptr;
			Mesh*          s_SkyMesh  = nullptr;
			ResourceHandle s_ScreenShader;
			ResourceHandle s_SkyboxShader;
			ResourceHandle s_ShadowmapShader;
			ResourceHandle lineShader;
			ResourceHandle s_LutMap;
			ResourceHandle s_Skybox;
			ResourceHandle s_ShadowFBO[MaxShadowCaster]; // sized by the constant

			std::vector<RenderPass> s_PassList;
			std::vector<RenderPass> s_ShadowPipeline;    // always MaxShadowCaster entries
		};

		static Scope<SceneData>  s_SceneData;
		static Scope<RenderData> s_RenderData;
	};
}