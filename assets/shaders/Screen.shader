#shader vertex
#version 330 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main()
{
    v_TexCoord = a_TexCoord;
    gl_Position = vec4(a_Position.x, a_Position.y, 0.0, 1.0);
}

#shader fragment
#version 330 core

out vec4 color;
in vec2 v_TexCoord;

uniform sampler2D u_SceneTexture;
uniform sampler2D u_LutTexture;
uniform float u_LutIntensity;

void main()
{
    vec4 sceneColor = texture(u_SceneTexture, v_TexCoord);
    
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