#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Tangent;
layout(location = 3) in vec2 a_TexCoord;

layout(std140) uniform Camera
{
    mat4 u_ViewProjection;
    mat4 u_View;
    vec3 u_CameraPosition;
};

uniform mat4 u_Model;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_Tangent;
out vec3 v_Bitangent;

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_FragPos = worldPos.xyz;
    
    mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
    v_Normal = normalize(normalMatrix * a_Normal);
    v_Tangent = normalize(normalMatrix * a_Tangent.xyz);
    v_Bitangent = cross(v_Normal, v_Tangent) * a_Tangent.w;
    
    v_TexCoord = a_TexCoord;
    
    gl_Position = u_ViewProjection * worldPos;
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 FragColor;

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_Tangent;
in vec3 v_Bitangent;

layout(std140) uniform Camera
{
    mat4 u_ViewProjection;
    mat4 u_View;
    vec3 u_CameraPosition;
};

uniform sampler2D u_AlbedoMap;
uniform sampler2D u_MetallicRoughnessMap;
uniform sampler2D u_NormalMap;

uniform vec4 u_AlbedoColor;
uniform float u_Metallic;
uniform float u_Roughness;
uniform int u_HasNormalMap;

const float PI = 3.14159265359;

// Simple directional light
vec3 g_LightDir = normalize(vec3(0.3, -1.0, 0.5));
vec3 g_LightColor = vec3(1.0);

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    // Sample textures
    vec4 albedo = texture(u_AlbedoMap, v_TexCoord) * u_AlbedoColor;
    vec2 metallicRoughness = texture(u_MetallicRoughnessMap, v_TexCoord).bg;
    float metallic = metallicRoughness.r * u_Metallic;
    float roughness = metallicRoughness.g * u_Roughness;
    
    // Normal mapping
    vec3 N = v_Normal;
    if (u_HasNormalMap == 1)
    {
        vec3 tangentNormal = texture(u_NormalMap, v_TexCoord).xyz * 2.0 - 1.0;
        mat3 TBN = mat3(v_Tangent, v_Bitangent, v_Normal);
        N = normalize(TBN * tangentNormal);
    }
    
    vec3 V = normalize(u_CameraPosition - v_FragPos);
    vec3 L = -g_LightDir;
    vec3 H = normalize(V + L);
    
    // PBR calculations
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, metallic);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo.rgb / PI + specular) * g_LightColor * NdotL;
    
    // Ambient
    vec3 ambient = vec3(0.03) * albedo.rgb;
    vec3 color = ambient + Lo;
    
    // Gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, albedo.a);
}