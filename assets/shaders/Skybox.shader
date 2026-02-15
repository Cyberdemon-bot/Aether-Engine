#shader vertex
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

#shader fragment
#version 330 core
in vec3 v_TexCoords;
out vec4 color;

uniform samplerCube u_Skybox;

void main()
{
    color = texture(u_Skybox, v_TexCoords);
}