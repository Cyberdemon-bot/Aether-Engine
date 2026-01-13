#pragma once
#include "RendererAPI.h"

namespace Aether {
    class RenderCommand {
    public:
        static void Init() {
            s_RendererAPI->Init();
        }
        
        static void SetClearColor(const glm::vec4& color) {
            s_RendererAPI->SetClearColor(color);
        }

        static void Clear() {
            s_RendererAPI->Clear();
        }

    private:
        static Scope<RendererAPI> s_RendererAPI;
    };
}