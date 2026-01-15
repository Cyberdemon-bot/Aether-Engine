#shader vertex
#version 330 core
layout(location = 0) in vec3 a_Position;

out vec3 v_TexCoords;

layout (std140) uniform CameraData
{
    mat4 u_Projection;
    mat4 u_View;
    vec3 u_ViewPos;
};

void main()
{
    v_TexCoords = a_Position;
    
    // Remove translation from view matrix
    mat4 viewNoTranslation = mat4(mat3(u_View));
    vec4 pos = u_Projection * viewNoTranslation * vec4(a_Position, 1.0);
    
    // Ensure skybox is always at max depth
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