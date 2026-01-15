#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

// --- INSTANCING INPUTS (Matrix 4x4) ---
// Chiếm 4 location liên tiếp từ 3 đến 6
layout(location = 3) in vec4 a_Row0;
layout(location = 4) in vec4 a_Row1;
layout(location = 5) in vec4 a_Row2;
layout(location = 6) in vec4 a_Row3;

// UBO Camera
layout (std140) uniform CameraData
{
    mat4 u_ViewProjection;
    vec3 u_ViewPos;
};

uniform mat4 u_Transform; // Dùng cho object vẽ lẻ (Floor)
uniform bool u_UseInstancing;
uniform mat4 u_LightSpaceMatrix;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec4 v_FragPosLightSpace;

void main()
{
    v_TexCoord = a_TexCoord;
    
    mat4 modelMatrix;
    if (u_UseInstancing) {
        // Tái tạo mat4 từ 4 vec4 attribute
        // GLSL mat4 constructor nhận cột (column-major)
        // Nhưng dữ liệu ta gửi là Row, nên cần transpose lại
        mat4 instanceMatrix = mat4(a_Row0, a_Row1, a_Row2, a_Row3);
        modelMatrix = transpose(instanceMatrix);
    } else {
        modelMatrix = u_Transform;
    }

    v_FragPos = vec3(modelMatrix * vec4(a_Position, 1.0));
    v_Normal = mat3(transpose(inverse(modelMatrix))) * a_Normal;
    v_FragPosLightSpace = u_LightSpaceMatrix * vec4(v_FragPos, 1.0);
    
    gl_Position = u_ViewProjection * vec4(v_FragPos, 1.0);
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec4 v_FragPosLightSpace;

// UBO Camera
layout (std140) uniform CameraData
{
    mat4 u_ViewProjection;
    vec3 u_ViewPos;
};

uniform sampler2D u_Texture;
uniform sampler2D u_ShadowMap;
uniform vec3 u_LightPos;

uniform bool u_FogEnabled;
uniform vec3 u_FogColor;
uniform float u_FogStart;
uniform float u_FogEnd;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(v_Normal, normalize(u_LightPos - v_FragPos))), 0.0005);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    return shadow;
}

void main()
{
    vec3 colorObj = texture(u_Texture, v_TexCoord).rgb;
    vec3 normal = normalize(v_Normal);
    vec3 lightColor = vec3(1.0);
    
    // Ambient
    vec3 ambient = 0.3 * colorObj;
    
    // Diffuse
    vec3 lightDir = normalize(u_LightPos - v_FragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    vec3 viewDir = normalize(u_ViewPos - v_FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * lightColor;
    
    // Shadow
    float shadow = ShadowCalculation(v_FragPosLightSpace);
    
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * colorObj;
    
    // Fog
    vec4 finalColor = vec4(lighting, 1.0);
    if (u_FogEnabled) {
        float dist = length(u_ViewPos - v_FragPos);
        float fogFactor = (u_FogEnd - dist) / (u_FogEnd - u_FogStart);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        finalColor = mix(vec4(u_FogColor, 1.0), finalColor, fogFactor);
    }
    
    color = finalColor;
}