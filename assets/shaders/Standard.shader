#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Tangent;
layout(location = 3) in vec2 a_TexCoord;
layout(location = 4) in uvec4 a_Joints;
layout(location = 5) in vec4 a_Weights;
layout(location = 6) in mat4 a_InstanceModel;
layout(location = 10) in int a_InstanceRigIdx;

uniform samplerBuffer u_BoneStorage;  
uniform samplerBuffer u_OffsetStorage;

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
    uint shadowMask;
    int lightCount;
} u_Lights;

out vec3 v_WorldPos;
out vec3 v_WorldNormal;
out vec2 v_TexCoord;
out mat3 v_TBN;
out vec4 v_LightSpacePos[4]; 

void main()
{
    vec4 localPos = vec4(a_Position, 1.0);
    vec3 localNormal = a_Normal;
    vec3 localTangent = a_Tangent.xyz;

    if (a_InstanceRigIdx != -1)
    {
        vec4 meta = texelFetch(u_OffsetStorage, a_InstanceRigIdx);
        int boneBase = int(meta.x) * 4; 
        mat4 skinMatrix = mat4(0.0);
        for (int w = 0; w < 4; w++)
        {
            int boneIdx = int(a_Joints[w]);
            int texel   = boneBase + boneIdx * 4;
            mat4 boneMat = mat4(
                texelFetch(u_BoneStorage, texel + 0),
                texelFetch(u_BoneStorage, texel + 1),
                texelFetch(u_BoneStorage, texel + 2),
                texelFetch(u_BoneStorage, texel + 3)
            );
            skinMatrix += a_Weights[w] * boneMat;
        }

        localPos = skinMatrix * localPos;
        mat3 skinRotation = transpose(inverse(mat3(skinMatrix)));
        localNormal = normalize(skinRotation * localNormal);
        localTangent = normalize(skinRotation * localTangent);
    }

    mat4 modelMatrix = a_InstanceModel;
    vec4 worldPos = modelMatrix * localPos;
    v_WorldPos = worldPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    vec3 T = normalize(normalMatrix * localTangent);
    vec3 N = normalize(normalMatrix * localNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * a_Tangent.w;
    v_TBN = mat3(T, B, N);
    v_WorldNormal = N;
    v_TexCoord = a_TexCoord;

    int slot = 0;
    for (int i = 0; i < u_Lights.lightCount && i < 16 && slot < 4; i++)
    {
        if ((u_Lights.shadowMask & (1u << uint(i))) != 0u)
        {
            v_LightSpacePos[slot] = u_Lights.lights[i].lightSpaceMatrix * worldPos;
            slot++;
        }
    }
    for (int s = slot; s < 4; s++)
        v_LightSpacePos[s] = vec4(0.0);

    gl_Position = u_ViewProjection * worldPos;
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 FragColor;

in vec3 v_WorldPos;
in vec3 v_WorldNormal;
in vec2 v_TexCoord;
in mat3 v_TBN;
in vec4 v_LightSpacePos[4];

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
    uint shadowMask;
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

uniform sampler2D u_Shadowmap0;
uniform sampler2D u_Shadowmap1;
uniform sampler2D u_Shadowmap2;
uniform sampler2D u_Shadowmap3;

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
        return 1.0 - clamp((dist - u_FogStart) / (u_FogEnd - u_FogStart), 0.0, 1.0);
    }
    else if (u_FogMode == 2)
    {
        return exp(-u_FogDensity * dist);
    }
    else if (u_FogMode == 3)
    {
        float f = u_FogDensity * dist;
        return exp(-(f * f));
    }
    return 1.0;
}

const float PI = 3.14159265359;
const vec3 F0_DIELECTRIC = vec3(0.04);

// Sample the correct shadow map for the given slot index (0-3).
float SampleShadowMap(int slot, vec4 lightSpacePos, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 1.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;

    float currentDepth = projCoords.z;
    float bias = max(u_Bias * (1.0 - dot(normal, lightDir)), u_Bias * 0.1);

    float closestDepth;
    if      (slot == 0) closestDepth = texture(u_Shadowmap0, projCoords.xy).r;
    else if (slot == 1) closestDepth = texture(u_Shadowmap1, projCoords.xy).r;
    else if (slot == 2) closestDepth = texture(u_Shadowmap2, projCoords.xy).r;
    else                closestDepth = texture(u_Shadowmap3, projCoords.xy).r;

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
    // FIX 2: Đưa màu Albedo Texture về Linear Space chuẩn cho PBR
    vec4 albedoSample = texture(u_AlbedoMap, v_TexCoord);
    vec3 albedoLinear = pow(albedoSample.rgb, vec3(2.2)) * u_AlbedoColor.rgb;

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

    int shadowSlot = 0;

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
            
            float distFactor = clamp(1.0 - pow(dist / lightRange, 4.0), 0.0, 1.0);
            attenuation = (distFactor * distFactor) / (1.0 + (dist * dist) / (lightRange * lightRange));

            float theta        = dot(normalize(lightToFrag), lightDir);
            float innerCone    = light.coneAngles.x;
            float outerCone    = light.coneAngles.y;
            float epsilon      = innerCone - outerCone;
            float spotIntensity = clamp((theta - outerCone) / epsilon, 0.0, 1.0);
            attenuation *= spotIntensity;
        }

        bool castsShadow = (u_Lights.shadowMask & (1u << uint(i))) != 0u;
        int  currentSlot = shadowSlot;
        if (castsShadow) shadowSlot++;

        float shadow = 1.0;
        if (castsShadow && currentSlot < 4)
            shadow = SampleShadowMap(currentSlot, v_LightSpacePos[currentSlot], N, L);

        vec3  H     = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = max(dot(H, V), 0.0);

        vec3  F0       = mix(F0_DIELECTRIC, albedoLinear, metallic);
        float D        = DistributionGGX(NdotH, roughness);
        float G        = GeometrySmith(NdotV, NdotL, roughness);
        vec3  F        = FresnelSchlick(HdotV, F0);
        vec3  specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);
        vec3  kD       = (1.0 - F) * (1.0 - metallic);
        vec3  diffuse  = kD * albedoLinear / PI;

        Lo += (diffuse + specular) * lightColor * lightIntensity * NdotL * attenuation * shadow;
    }

    vec3 ambient = vec3(0.03) * albedoLinear;
    vec3 color   = ambient + Lo;

    float fragDist  = length(u_Position - v_WorldPos);
    float fogFactor = ComputeFogFactor(fragDist);
    color           = mix(u_FogColor, color, fogFactor);

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}