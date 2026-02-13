#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Tangent;
layout(location = 3) in vec2 a_TexCoord;
layout(location = 4) in uvec4 a_Joints;
layout(location = 5) in vec4 a_Weights;
layout(location = 6) in mat4 a_InstanceModel;

layout(std140) uniform Camera
{
    mat4 u_ViewProjection;
    mat4 u_View;
    vec3 u_Position;
    float _pad;
};

layout(std140) uniform Bones
{
    mat4 u_BoneMatrices[100];
};

uniform mat4 u_Model;
uniform int u_HasAnimation;
uniform int u_UseInstancing;

out vec3 v_WorldPos;
out vec3 v_WorldNormal;
out vec2 v_TexCoord;
out mat3 v_TBN;

void main()
{
    vec4 localPos = vec4(a_Position, 1.0);
    vec3 localNormal = a_Normal;
    vec3 localTangent = a_Tangent.xyz;

    // Skeletal animation skinning
    if (u_HasAnimation == 1)
    {
        mat4 skinMatrix = 
            a_Weights.x * u_BoneMatrices[a_Joints.x] +
            a_Weights.y * u_BoneMatrices[a_Joints.y] +
            a_Weights.z * u_BoneMatrices[a_Joints.z] +
            a_Weights.w * u_BoneMatrices[a_Joints.w];

        localPos = skinMatrix * localPos;
        mat3 skinMat3 = mat3(skinMatrix);
        localNormal = skinMat3 * localNormal;
        localTangent = skinMat3 * localTangent;
    }

    // Choose model matrix based on instancing
    mat4 modelMatrix = u_UseInstancing == 1 ? a_InstanceModel : u_Model;

    // Transform to world space
    vec4 worldPos = modelMatrix * localPos;
    v_WorldPos = worldPos.xyz;

    // Normal matrix (assumes uniform scale for performance)
    // For non-uniform scale, use: transpose(inverse(mat3(modelMatrix)))
    mat3 normalMatrix = mat3(modelMatrix);
    
    // Build TBN matrix in world space
    vec3 T = normalize(normalMatrix * localTangent);
    vec3 N = normalize(normalMatrix * localNormal);
    
    // Gram-Schmidt re-orthogonalization
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * a_Tangent.w;
    
    v_TBN = mat3(T, B, N);
    v_WorldNormal = N;
    
    v_TexCoord = a_TexCoord;
    gl_Position = u_ViewProjection * worldPos;
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 FragColor;

in vec3 v_WorldPos;
in vec3 v_WorldNormal;
in vec2 v_TexCoord;
in mat3 v_TBN;

layout(std140) uniform Camera
{
    mat4 u_ViewProjection;
    mat4 u_View;
    vec3 u_Position;
    float _pad;
};

uniform sampler2D u_AlbedoMap;
uniform sampler2D u_MetallicRoughnessMap;
uniform sampler2D u_NormalMap;

uniform vec4 u_AlbedoColor;
uniform float u_Metallic;
uniform float u_Roughness;
uniform int u_HasNormalMap;

const float PI = 3.14159265359;
const vec3 F0_DIELECTRIC = vec3(0.04);

// Single directional light (stronger and from better angle for PBR visibility)
const vec3 LIGHT_DIR_WORLD = normalize(vec3(-0.5, -1.0, -0.3));  // From upper-left
const vec3 LIGHT_COLOR = vec3(2.5);  // Brighter light to show specular highlights
const vec3 AMBIENT = vec3(0.08);     // More ambient to see shapes better

// GGX/Trowbridge-Reitz normal distribution function
float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    
    return a2 / max(denom, 0.0000001); // Prevent division by zero
}

// Schlick-GGX geometry function
float GeometrySchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith's method for geometry obstruction
float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) * 0.125; // k = (r^2) / 8 for direct lighting
    return GeometrySchlickGGX(NdotV, k) * GeometrySchlickGGX(NdotL, k);
}

// Fresnel-Schlick approximation
vec3 FresnelSchlick(float HdotV, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - HdotV, 0.0, 1.0), 5.0);
}

void main()
{
    // Sample material textures
    vec4 albedo = texture(u_AlbedoMap, v_TexCoord) * u_AlbedoColor;
    
    // glTF 2.0 standard: Green = Roughness, Blue = Metallic
    vec3 metallicRoughnessSample = texture(u_MetallicRoughnessMap, v_TexCoord).rgb;
    float roughness = clamp(metallicRoughnessSample.g * u_Roughness, 0.04, 1.0); // Clamp to avoid artifacts
    float metallic = clamp(metallicRoughnessSample.b * u_Metallic, 0.0, 1.0);
    
    // Normal mapping
    vec3 N = v_WorldNormal;
    if (u_HasNormalMap == 1)
    {
        vec3 tangentNormal = texture(u_NormalMap, v_TexCoord).xyz * 2.0 - 1.0;
        N = normalize(v_TBN * tangentNormal);
    }
    
    // Calculate lighting vectors in world space
    vec3 V = normalize(u_Position - v_WorldPos); // View direction
    vec3 L = -LIGHT_DIR_WORLD;                   // Light direction
    vec3 H = normalize(V + L);                   // Half vector
    
    // Calculate dot products
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    // Cook-Torrance BRDF
    vec3 F0 = mix(F0_DIELECTRIC, albedo.rgb, metallic);
    
    float D = DistributionGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    vec3 F = FresnelSchlick(HdotV, F0);
    
    // Specular contribution
    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL;
    vec3 specular = numerator / max(denominator, 0.001);
    
    // Diffuse contribution (energy conservation)
    vec3 kS = F;                           // Specular reflection
    vec3 kD = (1.0 - kS) * (1.0 - metallic); // Diffuse reflection (metals have no diffuse)
    vec3 diffuse = kD * albedo.rgb / PI;
    
    // Direct lighting
    vec3 Lo = (diffuse + specular) * LIGHT_COLOR * NdotL;
    
    // Ambient lighting (cheap approximation)
    vec3 ambient = AMBIENT * albedo.rgb;
    
    // Final color
    vec3 color = ambient + Lo;
    
    // Tone mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction (sRGB)
    color = pow(color, vec3(1.0 / 2.2));
    
    FragColor = vec4(color, 1.0);
}
