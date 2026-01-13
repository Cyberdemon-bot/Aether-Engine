#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec4 v_FragPosLightSpace;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_LightSpaceMatrix;



void main()
{
    v_FragPos = vec3(u_Model * vec4(a_Position, 1.0));
    v_Normal = transpose(inverse(mat3(u_Model))) * a_Normal;
    v_TexCoord = a_TexCoord;
    v_FragPosLightSpace = u_LightSpaceMatrix * vec4(v_FragPos, 1.0);
    
    gl_Position = u_Projection * u_View * vec4(v_FragPos, 1.0);
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec4 v_FragPosLightSpace;

uniform sampler2D u_Texture;
uniform sampler2D u_ShadowMap;

uniform vec3 u_LightPos;
uniform vec3 u_LightDir;      
uniform float u_CutOff;       
uniform float u_OuterCutOff; 
uniform vec3 u_ViewPos;

uniform int u_IsLightSource;
uniform vec3 u_FlatColor;

uniform bool u_FogEnabled;
uniform vec3 u_FogColor;
uniform float u_FogStart; 
uniform float u_FogEnd;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 lightDir, vec3 normal)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0)
        return 0.0;

    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    float currentDepth = projCoords.z;

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
    if (u_IsLightSource == 1) {
        color = vec4(u_FlatColor, 1.0);
        return;
    }

    vec3 colorTex = texture(u_Texture, v_TexCoord).rgb;
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(u_LightPos - v_FragPos);

    float theta = dot(lightDir, normalize(-u_LightDir)); 
    float epsilon = u_CutOff - u_OuterCutOff;
    float intensity = clamp((theta - u_OuterCutOff) / epsilon, 0.0, 1.0);

    vec3 ambient = 0.15 * colorTex;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * colorTex;

    vec3 viewDir = normalize(u_ViewPos - v_FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = vec3(0.3) * spec; 
    float shadow = ShadowCalculation(v_FragPosLightSpace, lightDir, normal);
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular) * intensity;   

    vec3 finalColor = lighting;

    if (u_FogEnabled)
    {
        float distance = length(u_ViewPos - v_FragPos);
        float fogFactor = (u_FogEnd - distance) / (u_FogEnd - u_FogStart);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        finalColor = mix(u_FogColor, finalColor, fogFactor);
    } 
    
    color = vec4(finalColor, 1.0);
}