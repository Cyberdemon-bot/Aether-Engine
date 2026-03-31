#shader vertex
#version 330 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main()
{
    v_TexCoord  = a_TexCoord;
    gl_Position = vec4(a_Position, 0.0, 1.0);
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 FragColor;

in vec2 v_TexCoord;

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

uniform sampler2D u_SceneColor;
uniform sampler2D u_SceneDepth;

// 4 shadow map slots matching Standard.shader
uniform sampler2D u_Shadowmap0;
uniform sampler2D u_Shadowmap1;
uniform sampler2D u_Shadowmap2;
uniform sampler2D u_Shadowmap3;

uniform float u_Density;
uniform float u_Intensity;
uniform int   u_Steps;
uniform float u_VolBias;
uniform float u_MaxDistance;

// ─── World position reconstruction ───────────────────────────────────────────
vec3 ReconstructWorldPos(vec2 uv, float depth)
{
    vec4 ndc      = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldPos = inverse(u_ViewProjection) * ndc;
    return worldPos.xyz / worldPos.w;
}

// ─── Shadow sampling per slot ─────────────────────────────────────────────────
float SampleShadowVol(int slot, vec3 worldPos, mat4 lightSpaceMatrix)
{
    vec4 lsp  = lightSpaceMatrix * vec4(worldPos, 1.0);
    vec3 proj = lsp.xyz / lsp.w;
    proj      = proj * 0.5 + 0.5;
    if (proj.z > 1.0)                          return 1.0;
    if (any(lessThan(proj.xy, vec2(0.0))))     return 1.0;
    if (any(greaterThan(proj.xy, vec2(1.0))))  return 1.0;

    float closestDepth;
    if      (slot == 0) closestDepth = texture(u_Shadowmap0, proj.xy).r;
    else if (slot == 1) closestDepth = texture(u_Shadowmap1, proj.xy).r;
    else if (slot == 2) closestDepth = texture(u_Shadowmap2, proj.xy).r;
    else                closestDepth = texture(u_Shadowmap3, proj.xy).r;

    return (proj.z - u_VolBias) > closestDepth ? 0.0 : 1.0;
}

// ─── Spot light attenuation ───────────────────────────────────────────────────
float SpotAttenuation(vec3 worldPos, int i)
{
    Light light      = u_Lights.lights[i];
    vec3  lightPos   = light.positionAndType.xyz;
    vec3  lightDir   = normalize(light.directionAndRange.xyz);
    float lightRange = light.directionAndRange.w;
    vec3  toPoint    = worldPos - lightPos;
    float dist       = length(toPoint);
    float atten      = 1.0 / (1.0 + dist * dist / (lightRange * lightRange));
    float theta      = dot(normalize(toPoint), lightDir);
    float innerCone  = light.coneAngles.x;
    float outerCone  = light.coneAngles.y;
    float epsilon    = innerCone - outerCone;
    float spotFactor = clamp((theta - outerCone) / epsilon, 0.0, 1.0);
    return atten * spotFactor;
}

// ─── Main ─────────────────────────────────────────────────────────────────────
void main()
{
    vec4  sceneColor = texture(u_SceneColor, v_TexCoord);
    float rawDepth   = texture(u_SceneDepth, v_TexCoord).r;
    vec3  surfacePos = ReconstructWorldPos(v_TexCoord, rawDepth);

    vec3  toSurface = surfacePos - u_Position;
    float rayLen    = rawDepth < 0.9999 ? min(length(toSurface), u_MaxDistance) : u_MaxDistance;
    vec3  rayDir    = rawDepth < 0.9999
                    ? normalize(toSurface)
                    : normalize(ReconstructWorldPos(v_TexCoord, 1.0) - u_Position);

    float stepSize = rayLen / float(u_Steps);
    float jitter   = fract(sin(dot(v_TexCoord, vec2(12.9898, 78.233))) * 43758.5453);

    vec3 scattering = vec3(0.0);

    // Build a per-light shadow slot table using shadowMask bits,
    // in the same order the CPU binds u_ShadowMap0..3.
    int shadowSlots[16];
    int shadowSlotCount = 0;
    for (int i = 0; i < u_Lights.lightCount && i < 16; i++)
    {
        if ((u_Lights.shadowMask & (1u << uint(i))) != 0u && shadowSlotCount < 4)
        {
            shadowSlots[i] = shadowSlotCount;
            shadowSlotCount++;
        }
        else
        {
            shadowSlots[i] = -1; // no shadow for this light
        }
    }

    for (int s = 0; s < u_Steps; s++)
    {
        float t         = (float(s) + jitter) * stepSize;
        vec3  samplePos = u_Position + rayDir * t;

        for (int i = 0; i < u_Lights.lightCount && i < 16; i++)
        {
            int lightType = int(u_Lights.lights[i].positionAndType.w);
            if (lightType != 1) continue; // volumetric only for spot lights

            float inLight = 1.0;
            if (shadowSlots[i] >= 0)
                inLight = SampleShadowVol(shadowSlots[i],
                                          samplePos,
                                          u_Lights.lights[i].lightSpaceMatrix);

            float atten      = SpotAttenuation(samplePos, i);
            vec3  lColor     = u_Lights.lights[i].colorAndIntensity.xyz;
            float lIntensity = u_Lights.lights[i].colorAndIntensity.w;
            scattering += lColor * lIntensity * atten * inLight * u_Density;
        }
    }

    scattering *= u_Intensity;
    FragColor = vec4(sceneColor.rgb + scattering, 1.0);
}