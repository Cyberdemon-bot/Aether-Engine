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

struct Light
{
    vec4 positionAndType;
    vec4 directionAndRange;
    vec4 colorAndIntensity;
    vec4 coneAngles;
    mat4 lightSpaceMatrix;
};

layout(std140) uniform Lights
{
    Light lights[16];
    int lightCount;
} u_Lights;

layout(std140) uniform Bones
{
    mat4 u_BoneMatrices[100];
};

uniform mat4 u_Model;
uniform int u_HasAnimation;
uniform int u_UseInstancing;
uniform int u_LightIndex;

out vec3 v_WorldPos;
out vec3 v_WorldNormal;
out vec2 v_TexCoord;
out mat3 v_TBN;
out vec4 v_LightSpacePos;

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

    mat4 modelMatrix = u_UseInstancing == 1 ? a_InstanceModel : u_Model;
    vec4 worldPos = modelMatrix * localPos;
    v_WorldPos = worldPos.xyz;

    mat3 normalMatrix = mat3(modelMatrix);
    vec3 T = normalize(normalMatrix * localTangent);
    vec3 N = normalize(normalMatrix * localNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * a_Tangent.w;
    v_TBN = mat3(T, B, N);
    v_WorldNormal = N;
    v_TexCoord = a_TexCoord;

    if (u_Lights.lightCount > 0 && u_LightIndex < u_Lights.lightCount && u_Lights.lights[u_LightIndex].coneAngles.z > 0.5)
        v_LightSpacePos = u_Lights.lights[u_LightIndex].lightSpaceMatrix * worldPos;
    else
        v_LightSpacePos = vec4(0.0);

    gl_Position = u_ViewProjection * worldPos;
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 FragColor;

in vec3 v_WorldPos;
in vec3 v_WorldNormal;
in vec2 v_TexCoord;
in mat3 v_TBN;
in vec4 v_LightSpacePos;

struct Light
{
    vec4 positionAndType;
    vec4 directionAndRange;
    vec4 colorAndIntensity;
    vec4 coneAngles;
    mat4 lightSpaceMatrix;
};

layout(std140) uniform Lights
{
    Light lights[16];
    int lightCount;
} u_Lights;

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
uniform sampler2D u_DepthTex;

uniform vec4 u_AlbedoColor;
uniform float u_Metallic;
uniform float u_Roughness;
uniform int u_HasNormalMap;
uniform float u_Bias;

uniform int   u_FogMode;   
uniform vec3  u_FogColor;
uniform float u_FogDensity;
uniform float u_FogStart;
uniform float u_FogEnd;

float ComputeFogFactor(float dist)
{
    if (u_FogMode == 1)
    {
        // Linear fog
        return 1.0 - clamp((dist - u_FogStart) / (u_FogEnd - u_FogStart), 0.0, 1.0);
    }
    else if (u_FogMode == 2)
    {
        // Exponential fog
        return exp(-u_FogDensity * dist);
    }
    else if (u_FogMode == 3)
    {
        // Exponential squared fog (thicker/more realistic)
        float f = u_FogDensity * dist;
        return exp(-(f * f));
    }
    return 1.0; // Mode 0 = no fog, return 1 so mix() changes nothing
}

const float PI = 3.14159265359;
const vec3 F0_DIELECTRIC = vec3(0.04);

float SampleShadowMap(vec4 lightSpacePos, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
    float currentDepth = projCoords.z;
    float bias = max(u_Bias * (1.0 - dot(normal, lightDir)), u_Bias * 0.1);
    float closestDepth = texture(u_DepthTex, projCoords.xy).r;
    return currentDepth - bias > closestDepth ? 0.0 : 1.0;
}

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0000001);
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
    return F0 + (1.0 - F0) * pow(clamp(1.0 - HdotV, 0.0, 1.0), 5.0);
}

void main()
{
    vec4 albedo = texture(u_AlbedoMap, v_TexCoord) * u_AlbedoColor;
    vec3 metallicRoughnessSample = texture(u_MetallicRoughnessMap, v_TexCoord).rgb;
    float roughness = clamp(metallicRoughnessSample.g * u_Roughness, 0.04, 1.0);
    float metallic  = clamp(metallicRoughnessSample.b * u_Metallic,  0.0,  1.0);

    vec3 N = v_WorldNormal;
    if (u_HasNormalMap == 1)
    {
        vec3 tangentNormal = texture(u_NormalMap, v_TexCoord).xyz * 2.0 - 1.0;
        N = normalize(v_TBN * tangentNormal);
    }

    vec3 V = normalize(u_Position - v_WorldPos);
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < u_Lights.lightCount && i < 16; i++)
    {
        Light light = u_Lights.lights[i];
        vec3  lightPos       = light.positionAndType.xyz;
        int   lightType      = int(light.positionAndType.w);
        vec3  lightDir       = normalize(light.directionAndRange.xyz);
        float lightRange     = light.directionAndRange.w;
        vec3  lightColor     = light.colorAndIntensity.xyz;
        float lightIntensity = light.colorAndIntensity.w;

        vec3  L = vec3(0.0);
        float attenuation = 1.0;

        if (lightType == 2)
        {
            L = -lightDir;
        }
        else if (lightType == 1)
        {
            vec3  lightToFrag = v_WorldPos - lightPos;
            float dist        = length(lightToFrag);
            L = normalize(-lightToFrag);
            attenuation = 1.0 / (1.0 + dist * dist / (lightRange * lightRange));
            float theta        = dot(normalize(lightToFrag), lightDir);
            float innerCone    = light.coneAngles.x;
            float outerCone    = light.coneAngles.y;
            float epsilon      = innerCone - outerCone;
            float spotIntensity = clamp((theta - outerCone) / epsilon, 0.0, 1.0);
            attenuation *= spotIntensity;
        }

        float shadow = 1.0;
        if (i == 0 && light.coneAngles.z > 0.5)
            shadow = SampleShadowMap(v_LightSpacePos, N, L);

        vec3  H     = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = max(dot(H, V), 0.0);

        vec3  F0        = mix(F0_DIELECTRIC, albedo.rgb, metallic);
        float D         = DistributionGGX(NdotH, roughness);
        float G         = GeometrySmith(NdotV, NdotL, roughness);
        vec3  F         = FresnelSchlick(HdotV, F0);
        vec3  specular  = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);
        vec3  kD        = (1.0 - F) * (1.0 - metallic);
        vec3  diffuse   = kD * albedo.rgb / PI;

        float shadowFactor = (i == 0 && light.coneAngles.z > 0.5) ? shadow : 1.0;
        Lo += (diffuse + specular) * lightColor * lightIntensity * NdotL * attenuation * shadowFactor;
    }

    vec3 ambient = vec3(0.03) * albedo.rgb;
    vec3 color   = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));
    // fog
    float fragDist   = length(u_Position - v_WorldPos);
    float fogFactor  = ComputeFogFactor(fragDist);
    color            = mix(u_FogColor, color, fogFactor);

    FragColor = vec4(color, 1.0);
}