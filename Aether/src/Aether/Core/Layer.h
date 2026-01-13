#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/Timestep.h"
#include "Aether/Events/Event.h"

namespace Aether {

	class AETHER_API Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void Attach() {}
		virtual void Detach() {}
		virtual void Update(Timestep ts) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};

}