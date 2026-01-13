#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Pos;
// Các attribute khác (Normal, TexCoord) không cần thiết cho Shadow Map pass

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Model;

void main()
{
    gl_Position = u_LightSpaceMatrix * u_Model * vec4(a_Pos, 1.0);
}

#shader fragment
#version 330 core

void main()
{
    // Không cần làm gì cả, Depth Buffer sẽ tự động được ghi
    // Có thể thêm: gl_FragDepth = gl_FragCoord.z; (nhưng không bắt buộc)
}