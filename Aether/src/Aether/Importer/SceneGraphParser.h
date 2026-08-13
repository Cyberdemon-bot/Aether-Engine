#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Aether/Core/Base.h"

namespace Aether {
    struct Node
    {
        std::string name;
        glm::vec3 translation = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        int meshIdx = -1;
        int animatorIdx = -1;
        std::vector<int> children;
    };

    struct SceneHierarchy
    {
        std::vector<Node> nodes;
        std::vector<int> roots;
    };

    class SceneGraphParser
    {
    public:
        virtual ~SceneGraphParser() = default;
        virtual Ref<SceneHierarchy> Parsing(void* data) = 0;
        static Ref<SceneGraphParser> Create();
    };
}