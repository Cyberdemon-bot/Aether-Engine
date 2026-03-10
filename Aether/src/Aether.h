#pragma once

#include "Aether/Core/Application.h"
#include "Aether/Core/Layer.h"
#include "Aether/Core/Log.h"
#include "Aether/Core/UUID.h"
#include "Aether/Core/AssetsRegister.h"
#include "Aether/Core/Timestep.h"

#include "Aether/Core/Input.h"
#include "Aether/Core/KeyCodes.h"
#include "Aether/Core/MouseCodes.h"
#include "Aether/Core/JobSystem.h"

#include "Aether/ImGui/ImGuiLayer.h"
#include "Aether/Console/ConsoleLayer.h"

#include "Aether/Renderer/RenderCommand.h"
#include "Aether/Renderer/Renderer.h"

#include "Aether/Renderer/Buffer.h"
#include "Aether/Renderer/VertexArray.h"
#include "Aether/Renderer/UniformBuffer.h"
#include "Aether/Renderer/FrameBuffer.h"
#include "Aether/Renderer/EditorCamera.h"

#include "Aether/Renderer/Shader.h"
#include "Aether/Renderer/Texture.h"

#include "Aether/Assets/Mesh.h"
#include "Aether/Assets/Material.h"
#include "Aether/Assets/Sound.h"

#include "Aether/Scene/Component.h"
#include "Aether/Scene/Scene.h"

#include "Aether/Importer/Importer.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Audio/AudioSystem.h"

#include "Aether/Assets/AssetManager.h"