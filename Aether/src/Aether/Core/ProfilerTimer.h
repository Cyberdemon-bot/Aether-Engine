#include "Aether/Core/Log.h"

struct ProfileTimer 
{
	const char* name;
	float minTime;
	std::chrono::steady_clock::time_point start;

	ProfileTimer(const char* n, float mt = 0.0f) : name(n), minTime(mt), start(std::chrono::steady_clock::now()) {}
	~ProfileTimer()
	{
		auto end = std::chrono::steady_clock::now();
		float duration = std::chrono::duration<float, std::milli>(end - start).count();
		if (duration > minTime) AE_CORE_TRACE("{0}: {1}ms", name, duration);
	}
};

#define AE_PROFILE_SCOPE(name) ProfileTimer timer##__LINE__(name)