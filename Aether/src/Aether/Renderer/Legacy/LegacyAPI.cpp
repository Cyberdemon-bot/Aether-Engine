#include "LegacyAPI.h"
#include <glad/glad.h> 

namespace Aether::Legacy {

    void LegacyAPI::Init() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
    }

    void LegacyAPI::SetViewport(int x, int y, int width, int height) {
        glViewport(x, y, width, height);
    }

    void LegacyAPI::SetClearColor(const glm::vec4& color) {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void LegacyAPI::Clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void LegacyAPI::Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) {
        shader.Bind();
        va.Bind();
        ib.Bind();
        glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr);
    }
}