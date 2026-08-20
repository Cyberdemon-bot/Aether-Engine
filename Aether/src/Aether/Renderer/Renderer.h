#pragma once

#include <string_view>
#include "Aether/Core/Base.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Assets/Material.h"
#include "Aether/Renderer/Camera.h"
#include "Aether/Renderer/ResourceManager.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Renderer/FrameBuffer.h"
#include "Aether/Renderer/Shader.h"
#include "Aether/Renderer/RenderCommand.h"
#include <glm/glm.hpp>

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
		Light lights[MAX_LIGHTS];
		uint32_t shadowMask;
		int lightCount;
		float _pad[3];
	};

	struct CameraData
	{
		glm::mat4 ViewProjection;
		glm::mat4 View;
		glm::vec3 Position; float _pad;
	};

	struct CommandKey
	{
		std::pair<uint64_t, uint64_t> key;
		uint32_t index;
	};

	struct Command
	{
		Handle<Asset> mesh;
		Handle<Asset> sheet;
		uint32_t subIdx;
		uint32_t matIdx;
		Handle<Pose> pose;

		glm::mat4 transform;
		AMesh* meshPtr = nullptr;
		AMaterial* matPtr = nullptr;

		bool operator!=(const Command& other) const
		{
			return (mesh.Blend() != other.mesh.Blend()) || 
				(sheet.Blend() != other.sheet.Blend()) || 
				(matIdx != other.matIdx) || 
				(subIdx != other.subIdx);
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
		void Init();
		void Shutdown();

		void OnWindowResize(uint32_t width, uint32_t height);

		void SetPipeline(RenderPass* list, size_t size);
		void SetLutMap(const std::string& filepath);
		void SetSkyBox(const std::string& filepath);

		void ActivatePass(uint32_t PassIdx);
		void DeactivatePass(uint32_t PassIdx);
		void SetPassAtrib(uint32_t passIdx, std::string_view name, int value);
		
		void BeginScene(const Camera& camera, LightParam* lights = nullptr, size_t size = 0);
		void EndScene();

		void DrawMesh(Handle<Asset> mesh, Handle<Asset> sheet, Handle<Pose> pose, const glm::mat4& transform);

		void RenderBox(const glm::vec3& boundMin, const glm::vec3& boundMax, const glm::mat4& transform, const glm::vec4& color);
		void RenderCapsule(float radius, float halfHeight, const glm::mat4& transform, const glm::vec4& color);
		void RenderSphere(float radius, const glm::mat4& transform, const glm::vec4& color);

		RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

	private:
		void SortCommandList();
		void Flush(const RenderPass& pass);
		void RenderOnScreen(const RenderPass& pass);
		void RenderSkybox();
		void CalculateDirectionalMat(const Camera& camera, const LightParam& light, glm::mat4& view, glm::mat4& proj, float zMultiplier = 10.0f);
		Texture2D* GetShadowDepthAttachment(uint32_t slot);

		struct InstanceData
		{
			glm::mat4 transform;
			int rigidx;
		};

		struct SceneData
		{
			CameraData camera;
			LightsData lights;
			std::vector<Command> CommandList;
			std::vector<Command> CommandTempList;
			std::vector<CommandKey> sortKeys;
			std::vector<InstanceData> batchInstance;
			std::vector<LightCandidate> CandList;
			std::vector<glm::mat4> BoneStorage;
			std::vector<glm::vec4> OffsetStorage;
			std::vector<int> BoneIndices;
			uint32_t activeShadowSlots = 0;
			std::vector<int> PoseIndexLookup;
			std::vector<uint32_t> PoseIndexTouched;
		};

		struct RenderData
		{
			Handle<Resource> CameraUB;
			Handle<Resource> BoneUB;
			Handle<Resource> LightUB;
			Handle<Resource> s_InstanceVBO;
			Handle<Resource> s_ScreenShader;
			Handle<Resource> s_SkyboxShader;
			Handle<Resource> s_ShadowmapShader;
			Handle<Resource> lineShader;
			Handle<Resource> s_LutMap;
			Handle<Resource> s_Skybox;
			Handle<Resource> s_ShadowFBO[MAX_SHADOW_CASTER]; 
			Handle<Resource> BoneStorage;
			Handle<Resource> OffsetStorage;
			AMesh* s_Quad   = nullptr;
			AMesh* s_SkyMesh  = nullptr;

			RenderPass s_ShadowPipeline[MAX_SHADOW_CASTER];
			RenderPass s_PassList[MAX_RENDER_PASSES];
			std::vector<std::string> s_ShadowmapUniformNames;
			uint32_t s_PassCount = 0;
		};

		Scope<SceneData>  s_SceneData;
		Scope<RenderData> s_RenderData;
	};
}