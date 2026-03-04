#pragma once

namespace Aether {
    static const char* VScreenShader = R"(
        #version 330 core

        layout(location = 0) in vec2 a_Position;
        layout(location = 1) in vec2 a_TexCoord;

        out vec2 v_TexCoord;

        void main()
        {
            v_TexCoord = a_TexCoord;
            gl_Position = vec4(a_Position.x, a_Position.y, 0.0, 1.0);
        }
    )";

    static const char* FScreenShader = R"(
        #version 330 core

        out vec4 color;
        in vec2 v_TexCoord;

        uniform sampler2D u_SceneTexture;
        uniform sampler2D u_LutTexture;
        uniform float u_LutIntensity;
        uniform int u_HasLut;

        void main()
        {
            vec4 sceneColor = texture(u_SceneTexture, v_TexCoord);

            if (u_HasLut == 0)
            {
                color = sceneColor;
                return;
            }
            
            const float SIZE = 16.0;
            const float COLORS = SIZE - 1.0; 
            
            vec3 lutCoords = sceneColor.rgb * COLORS;
            
            vec3 lutCoords_floor = floor(lutCoords);
            vec3 lutCoords_fract = fract(lutCoords);
            
            float blueSlice1 = lutCoords_floor.b;
            float blueSlice2 = min(blueSlice1 + 1.0, COLORS);
            
            vec2 uv1, uv2;
            uv1.x = (blueSlice1 * SIZE + lutCoords_floor.r + 0.5) / (SIZE * SIZE);
            uv1.y = (lutCoords_floor.g + 0.5) / SIZE;
            
            uv2.x = (blueSlice2 * SIZE + lutCoords_floor.r + 0.5) / (SIZE * SIZE);
            uv2.y = (lutCoords_floor.g + 0.5) / SIZE;
            
            vec4 color1 = texture(u_LutTexture, uv1);
            vec4 color2 = texture(u_LutTexture, uv2);
            
            vec4 lutColor = mix(color1, color2, lutCoords_fract.b);
            color = mix(sceneColor, lutColor, u_LutIntensity);
        }
    )";

    static const char* VSkyboxShader = R"(
        #version 330 core
        layout(location = 0) in vec3 a_Position;

        out vec3 v_TexCoords;

        layout(std140) uniform Camera
        {
            mat4 u_ViewProjection;
            mat4 u_View;
            vec3 u_Position;
            float _pad;
        };

        void main()
        {
            v_TexCoords = a_Position;
            vec3 worldPos = a_Position + u_Position;
            vec4 pos = u_ViewProjection * vec4(worldPos, 1.0);
            gl_Position = pos.xyww;
        }
    )";

    static const char* FSkyboxShader = R"(
        #version 330 core
        in vec3 v_TexCoords;
        out vec4 color;

        uniform samplerCube u_Skybox;

        void main()
        {
            color = texture(u_Skybox, v_TexCoords);
        }
    )";

    static const char* VLineShader = R"(
        #version 330 core
        layout(location = 0) in vec3 a_Position;

        uniform mat4 u_ViewProjection; 

        void main()
        {
            gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
        }
    )";

    static const char* FLineShader = R"(
        #version 330 core
        uniform vec4 u_Color;

        out vec4 color;

        void main()
        {
            color = u_Color;
        }
    )";
}