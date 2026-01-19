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
    
    // LUT Configuration (256x16 for 16x16x16 color cube)
    const float SIZE = 16.0;
    const float COLORS = SIZE - 1.0; // 15.0
    
    // Map color from [0,1] to LUT coordinates [0,15]
    vec3 lutCoords = sceneColor.rgb * COLORS;
    
    // Split into integer and fractional parts
    vec3 lutCoords_floor = floor(lutCoords);
    vec3 lutCoords_fract = fract(lutCoords);
    
    // Calculate which blue slice we're in
    float blueSlice1 = lutCoords_floor.b;
    float blueSlice2 = min(blueSlice1 + 1.0, COLORS);
    
    // Convert 3D LUT coordinates to 2D texture coordinates
    // Formula: x = (blueSlice * 16 + red) / 256
    //          y = green / 16
    vec2 uv1, uv2;
    
    // Add 0.5 to sample from pixel centers
    uv1.x = (blueSlice1 * SIZE + lutCoords_floor.r + 0.5) / (SIZE * SIZE);
    uv1.y = (lutCoords_floor.g + 0.5) / SIZE;
    
    uv2.x = (blueSlice2 * SIZE + lutCoords_floor.r + 0.5) / (SIZE * SIZE);
    uv2.y = (lutCoords_floor.g + 0.5) / SIZE;
    
    // Sample both slices
    vec4 color1 = texture(u_LutTexture, uv1);
    vec4 color2 = texture(u_LutTexture, uv2);
    
    // Interpolate between the two blue slices
    vec4 lutColor = mix(color1, color2, lutCoords_fract.b);
    
    // Apply LUT with intensity control
    color = mix(sceneColor, lutColor, u_LutIntensity);
}