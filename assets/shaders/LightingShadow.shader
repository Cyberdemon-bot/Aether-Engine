#shader vertex
#version 330 core

// Attributes
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

// Attribute cho Instancing (Model Matrix chiem location 3,4,5,6)
layout(location = 3) in mat4 a_InstanceModel;

// Camera Uniform Block
layout (std140) uniform CameraData
{
    mat4 u_Projection;
    mat4 u_View;
    vec3 u_ViewPos;
};

uniform mat4 u_Model;            
uniform mat4 u_LightSpaceMatrix;
uniform bool u_UseInstancing;    

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec4 v_FragPosLightSpace;

void main()
{
    // Chon Model Matrix dua tren che do ve
    mat4 model = u_UseInstancing ? a_InstanceModel : u_Model;

    vec4 worldPos = model * vec4(a_Position, 1.0);
    v_FragPos = vec3(worldPos);
    
    // Tinh Normal Matrix
    v_Normal = mat3(transpose(inverse(model))) * a_Normal;
    
    v_TexCoord = a_TexCoord;
    v_FragPosLightSpace = u_LightSpaceMatrix * worldPos;
    
    gl_Position = u_Projection * u_View * worldPos;
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec4 v_FragPosLightSpace;

// --- SỬA LỖI: Thêm Uniform Block CameraData vào Fragment Shader để lấy u_ViewPos ---
layout (std140) uniform CameraData
{
    mat4 u_Projection;
    mat4 u_View;
    vec3 u_ViewPos;
};
// ----------------------------------------------------------------------------------

uniform sampler2D u_Texture;
uniform sampler2D u_ShadowMap;

uniform vec3 u_LightPos;
uniform vec3 u_LightDir;
uniform float u_CutOff;
uniform float u_OuterCutOff;

uniform bool u_IsLightSource;
uniform vec3 u_FlatColor;

// Fog
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
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);  

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

    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(u_LightPos - v_FragPos);
    
    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * texture(u_Texture, v_TexCoord).rgb;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * texture(u_Texture, v_TexCoord).rgb;
    
    // Specular (Blinn-Phong)
    // u_ViewPos giờ đã được khai báo qua Uniform Block ở trên
    vec3 viewDir = normalize(u_ViewPos - v_FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = vec3(0.5) * spec; 
    
    // Spotlight (Soft edges)
    float theta = dot(lightDir, normalize(-u_LightDir)); 
    float epsilon = (u_CutOff - u_OuterCutOff);
    float intensity = clamp((theta - u_OuterCutOff) / epsilon, 0.0, 1.0);
    
    diffuse  *= intensity;
    specular *= intensity;
    
    // Shadow
    float shadow = ShadowCalculation(v_FragPosLightSpace);       
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular));
    
    vec4 finalColor = vec4(lighting, 1.0);

    // Fog calculation
    if (u_FogEnabled) {
        // SỬA LỖI: Đổi tên biến 'distance' thành 'viewDist' để không trùng hàm có sẵn
        float viewDist = length(u_ViewPos - v_FragPos);
        float fogFactor = (u_FogEnd - viewDist) / (u_FogEnd - u_FogStart);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        finalColor = mix(vec4(u_FogColor, 1.0), finalColor, fogFactor);
    }

    color = finalColor;
}