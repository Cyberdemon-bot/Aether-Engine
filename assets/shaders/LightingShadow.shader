#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec4 v_FragPosLightSpace;

// --- UNIFORM BLOCK (Binding = 0) ---
layout (std140) uniform CameraData
{
    mat4 u_Projection;
    mat4 u_View;
    vec3 u_ViewPos;
};
// -----------------------------------

uniform mat4 u_Model;
uniform mat4 u_LightSpaceMatrix;

void main()
{
    v_FragPos = vec3(u_Model * vec4(position, 1.0));
    v_Normal = mat3(transpose(inverse(u_Model))) * normal;
    v_TexCoord = texCoord;
    v_FragPosLightSpace = u_LightSpaceMatrix * vec4(v_FragPos, 1.0);
    
    // Sử dụng u_Projection và u_View từ UBO
    gl_Position = u_Projection * u_View * vec4(v_FragPos, 1.0);
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec4 v_FragPosLightSpace;

// --- UNIFORM BLOCK ---
// Ta vẫn khai báo block này dù chỉ dùng u_ViewPos trong Fragment shader
// để đảm bảo layout khớp với Vertex shader và UBO
layout (std140) uniform CameraData
{
    mat4 u_Projection;
    mat4 u_View;
    vec3 u_ViewPos;
};
// ---------------------

uniform sampler2D u_Texture;
uniform sampler2D u_ShadowMap;

uniform vec3 u_LightPos;
uniform vec3 u_LightDir;
uniform float u_CutOff;
uniform float u_OuterCutOff;

uniform bool u_IsLightSource;
uniform vec3 u_FlatColor;

uniform bool u_FogEnabled;
uniform vec3 u_FogColor;
uniform float u_FogStart;
uniform float u_FogEnd;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0) return 0.0;
    
    float closestDepth = texture(u_ShadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(u_LightPos - v_FragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF
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
    if (u_IsLightSource) {
        color = vec4(u_FlatColor, 1.0);
        return;
    }

    vec3 lightDir = normalize(u_LightPos - v_FragPos);
    
    // Spotlight (Soft edges)
    float theta = dot(lightDir, normalize(-u_LightDir)); 
    float epsilon = u_CutOff - u_OuterCutOff;
    float intensity = clamp((theta - u_OuterCutOff) / epsilon, 0.0, 1.0);

    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * vec3(1.0);
    
    // Diffuse
    vec3 norm = normalize(v_Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);
    
    // Specular
    float specularStrength = 0.5;
    // Sử dụng u_ViewPos từ UBO
    vec3 viewDir = normalize(u_ViewPos - v_FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0);
    
    // Shadow
    float shadow = ShadowCalculation(v_FragPosLightSpace);       
    
    // Calculate Lighting
    diffuse *= intensity;
    specular *= intensity;
    
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * texture(u_Texture, v_TexCoord).rgb;    
    
    color = vec4(lighting, 1.0);

    // Fog
    if (u_FogEnabled) {
        float distance = length(u_ViewPos - v_FragPos);
        float fogFactor = (u_FogEnd - distance) / (u_FogEnd - u_FogStart);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        color = mix(vec4(u_FogColor, 1.0), color, fogFactor);
    }
}