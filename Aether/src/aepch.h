#pragma once

#include <memory>
#include <utility>
#include <algorithm>
#include <string>
#include <vector>
#include <queue>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstddef>
#include <mutex>
#include <semaphore>
#include <limits>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Aether/Core/Log.h"
#include "Aether/Core/Base.h"