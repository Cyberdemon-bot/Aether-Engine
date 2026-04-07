#pragma once

#include <string>

namespace Aether {
 
	class AETHER_API FileDialogs
	{
	public:
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);
	};

	class AETHER_API Time
	{
	public:
		static float GetTime();
	};

}
