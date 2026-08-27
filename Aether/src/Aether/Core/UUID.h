#pragma once
#include "Aether/Core/Base.h"

namespace Aether {

	class AETHER_API UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		operator uint64_t() const { return m_UUID; }

		static UUID Null() { return UUID(0); }
	private:
		uint64_t m_UUID;
	};

}

namespace std {
	template<>
	struct hash<Aether::UUID>
	{
		std::size_t operator()(const Aether::UUID& uuid) const
		{
			return (uint64_t)uuid;
		}
	};
}
