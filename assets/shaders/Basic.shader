#shader vertex 
#version 330 core

layout(location = 0) in vec4 position; layout(location = 1) in vec2 texCoord;

out vec2 v_TexCoord;

uniform mat4 u_MVP;

void main() { gl_Position = u_MVP * position; v_TexCoord = texCoord; }

#shader fragment 
#version 330 core

layout(location = 0) out vec4 color;

in vec2 v_TexCoord;

uniform vec4 u_Color; uniform sampler2D u_Texture;

void main() 
{ 
    float r = texture(u_Texture, v_TexCoord + vec2(-aberrationAmount, 0.0)).r;
    float g = texture(u_Texture, v_TexCoord).g;
    float b = texture(u_Texture, v_TexCoord + vec2(aberrationAmount, 0.0)).b;
    vec4 texColor = vec4(r, g, b, texture(u_Texture, v_TexCoord).a);

    vec2 position = (v_TexCoord - 0.5) * 2.0;
    float len = length(position);
    float vignette = smoothstep(1.5, 0.5, len); 

    color = texColor * u_Color * vignette;
}