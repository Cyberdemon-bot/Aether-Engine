#shader vertex
#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec2 a_TexCoord;
layout(location = 4) in ivec4 a_BoneIDs;
layout(location = 5) in vec4 a_Weights;
layout(location = 6) in float a_Orientation;

layout(std140) uniform CameraData
{
    mat4 u_Projection;
    mat4 u_View;
    vec3 u_ViewPos;
};

uniform mat4 u_Model;
uniform mat4 u_LightSpaceMatrix;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    vec4 FragPosLightSpace;
} vs_out;

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    vs_out.FragPos = worldPos.xyz;
    vs_out.Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
    vs_out.TexCoord = a_TexCoord;
    vs_out.FragPosLightSpace = u_LightSpaceMatrix * worldPos;
    
    gl_Position = u_Projection * u_View * worldPos;
}

#shader fragment
#version 410 core

const float PI = 3.14159265359;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    vec4 FragPosLightSpace;
} fs_in;

layout(std140) uniform CameraData
{
    mat4 u_Projection;
    mat4 u_View;
    vec3 u_ViewPos;
};

// PBR Material properties
uniform vec3 u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AO;

// Spotlight properties
uniform vec3 u_LightPos;
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform float u_LightIntensity;
uniform float u_CutOff;
uniform float u_OuterCutOff;

// Shadow map
uniform sampler2D u_ShadowMap;

// IBL (using skybox as irradiance map for now)
uniform samplerCube u_IrradianceMap;

// Fog
uniform int u_FogEnabled;
uniform vec3 u_FogColor;
uniform float u_FogStart;
uniform float u_FogEnd;

// Light source flag
uniform int u_IsLightSource;

out vec4 FragColor;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
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

// Geometry Function (Schlick-GGX)
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

// Fresnel-Schlick approximation
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Shadow calculation
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0)
        return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    
    // PCF (Percentage Closer Filtering)
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
    // If this is the light source, render it with flat color
    if (u_IsLightSource == 1) {
        FragColor = vec4(u_LightColor, 1.0);
        return;
    }
    
    vec3 N = normalize(fs_in.Normal);
    vec3 V = normalize(u_ViewPos - fs_in.FragPos);
    
    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, u_Albedo, u_Metallic);
    
    // Spotlight calculations
    vec3 L = normalize(u_LightPos - fs_in.FragPos);
    vec3 H = normalize(V + L);
    
    float distance = length(u_LightPos - fs_in.FragPos);
    float attenuation = u_LightIntensity / (distance * distance);
    
    // Spotlight intensity (soft edges)
    float theta = dot(L, normalize(-u_LightDir));
    float epsilon = u_CutOff - u_OuterCutOff;
    float intensity = clamp((theta - u_OuterCutOff) / epsilon, 0.0, 1.0);
    
    vec3 radiance = u_LightColor * attenuation * intensity;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, u_Roughness);
    float G = GeometrySmith(N, V, L, u_Roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - u_Metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    
    // Shadow
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, N, L);
    
    // Direct lighting
    vec3 Lo = (kD * u_Albedo / PI + specular) * radiance * NdotL * (1.0 - shadow);
    
    // Ambient lighting (simple IBL approximation using skybox)
    vec3 ambient = vec3(0.03) * u_Albedo * u_AO;
    
    // Sample skybox for simple ambient
    vec3 irradiance = texture(u_IrradianceMap, N).rgb;
    vec3 diffuse = irradiance * u_Albedo;
    ambient = kD * diffuse * u_AO * 0.3; // Scale down skybox contribution
    
    vec3 color = ambient + Lo;
    
    // HDR tonemapping (Reinhard)
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    // Fog
    if (u_FogEnabled == 1) {
        float dist = length(u_ViewPos - fs_in.FragPos);
        float fogFactor = clamp((u_FogEnd - dist) / (u_FogEnd - u_FogStart), 0.0, 1.0);
        color = mix(u_FogColor, color, fogFactor);
    }
    
    FragColor = vec4(color, 1.0);
}