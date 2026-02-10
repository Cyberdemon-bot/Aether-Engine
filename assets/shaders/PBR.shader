#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Tangent;
layout(location = 3) in vec2 a_TexCoord;
layout(location = 4) in uvec4 a_Joints;
layout(location = 5) in vec4 a_Weights;

layout(std140) uniform Camera
{
    mat4 u_ViewProjection;
    mat4 u_View;
};

layout(std140) uniform Bones
{
    mat4 u_BoneMatrices[100];
};

uniform mat4 u_Model;
uniform int u_HasAnimation;

out vec3 v_ViewPos;
out vec3 v_ViewNormal;
out vec2 v_TexCoord;
out mat3 v_TBN;

void main()
{
    vec4 localPos = vec4(a_Position, 1.0);
    vec3 localNormal = a_Normal;
    vec3 localTangent = a_Tangent.xyz; 

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
    
    vec4 worldPos = u_Model * localPos;
    v_ViewPos = (u_View * worldPos).xyz;

    mat3 normalMatrix = mat3(u_View) * transpose(inverse(mat3(u_Model)));
    vec3 T = normalize(normalMatrix * localTangent);
    vec3 N = normalize(normalMatrix * localNormal);
    
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * a_Tangent.w;
    
    v_TBN = mat3(T, B, N);
    v_ViewNormal = N;
    
    v_TexCoord = a_TexCoord;
    gl_Position = u_ViewProjection * worldPos;
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 FragColor;

in vec3 v_ViewPos;
in vec3 v_ViewNormal;
in vec2 v_TexCoord;
in mat3 v_TBN;

layout(std140) uniform Camera
{
    mat4 u_ViewProjection;
    mat4 u_View;
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

const vec3 LIGHT_DIR_WORLD = vec3(0.3, -1.0, 0.5);
const vec3 LIGHT_COLOR = vec3(1.0);
const vec3 AMBIENT = vec3(0.03);

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) * 0.125; 
    return GeometrySchlickGGX(NdotV, k) * GeometrySchlickGGX(NdotL, k);
}

vec3 FresnelSchlick(float HdotV, vec3 F0)
{
    float fresnel = pow(1.0 - HdotV, 5.0);
    return F0 + (1.0 - F0) * fresnel;
}

void main()
{
    vec4 albedo = texture(u_AlbedoMap, v_TexCoord) * u_AlbedoColor;
    vec2 metallicRoughness = texture(u_MetallicRoughnessMap, v_TexCoord).bg;
    float metallic = metallicRoughness.r * u_Metallic;
    float roughness = metallicRoughness.g * u_Roughness;
    
    vec3 N = v_ViewNormal;
    if (u_HasNormalMap == 1)
    {
        vec3 tangentNormal = texture(u_NormalMap, v_TexCoord).xyz * 2.0 - 1.0;
        N = normalize(v_TBN * tangentNormal);
    }
    
    vec3 L = -normalize(mat3(u_View) * LIGHT_DIR_WORLD);
    vec3 V = normalize(-v_ViewPos);
    vec3 H = normalize(V + L);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    vec3 F0 = mix(F0_DIELECTRIC, albedo.rgb, metallic);
    
    float D = DistributionGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    vec3 F = FresnelSchlick(HdotV, F0);
    
    vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo.rgb / PI;
    
    vec3 Lo = (diffuse + specular) * LIGHT_COLOR * NdotL;
    vec3 ambient = AMBIENT * albedo.rgb;
    vec3 color = ambient + Lo;
    
    color = color / (color + 1.0); 
    color = pow(color, vec3(1.0 / 2.2)); 
    
    FragColor = vec4(color, albedo.a);
}
