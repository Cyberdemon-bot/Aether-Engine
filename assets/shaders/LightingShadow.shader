#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Pos;
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
    v_FragPos = vec3(u_Model * vec4(a_Pos, 1.0));
    v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal; // Xử lý normal khi scale lệch
    v_TexCoord = a_TexCoord;
    
    // Chuyển vị trí vertex sang không gian ánh sáng để so sánh với Shadow Map
    v_FragPosLightSpace = u_LightSpaceMatrix * vec4(v_FragPos, 1.0);
    
    gl_Position = u_Projection * u_View * vec4(v_FragPos, 1.0);
}

#shader fragment
#version 330 core

out vec4 FragColor;

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec4 v_FragPosLightSpace;

uniform sampler2D u_Texture;    // Slot 0 (Texture gỗ)
uniform sampler2D u_ShadowMap;  // Slot 1 (Shadow Map - Depth Texture)

uniform int u_IsLightSource;   
uniform vec3 u_FlatColor;

uniform vec3 u_LightPos;
uniform vec3 u_ViewPos;

// Hàm tính toán bóng đổ (0.0 = tối hoàn toàn, 1.0 = sáng hoàn toàn)
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // 1. Chuyển từ clip space (-1, 1) sang range [0, 1] để sample texture
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Nếu nằm ngoài vùng nhìn thấy của đèn thì coi như không có bóng
    if(projCoords.z > 1.0)
        return 0.0;

    // 2. Lấy độ sâu gần nhất từ Shadow Map
    float closestDepth = texture(u_ShadowMap, projCoords.xy).r; 
    
    // 3. Lấy độ sâu hiện tại của pixel
    float currentDepth = projCoords.z;

    // 4. Tính Bias để tránh hiện tượng Shadow Acne (sọc đen trên bề mặt)
    // Bias thay đổi dựa trên góc chiếu sáng
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);  

    // 5. PCF (Percentage-closer filtering) - Làm mềm bóng
    // Lấy trung bình các mẫu xung quanh pixel hiện tại thay vì chỉ 1 điểm
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
    if (u_IsLightSource == 1)
    {
        FragColor = vec4(u_FlatColor, 1.0); 
        return; 
    }

    vec3 color = texture(u_Texture, v_TexCoord).rgb;
    vec3 normal = normalize(v_Normal);
    vec3 lightColor = vec3(1.0);

    // -- Ambient --
    vec3 ambient = 0.3 * color;

    // -- Diffuse --
    vec3 lightDir = normalize(u_LightPos - v_FragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // -- Specular --
    vec3 viewDir = normalize(u_ViewPos - v_FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    

    // -- Tính Shadow --
    float shadow = ShadowCalculation(v_FragPosLightSpace, normal, lightDir);       
    
    // Tổng hợp ánh sáng (Shadow chỉ ảnh hưởng Diffuse và Specular, Ambient giữ nguyên)
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    FragColor = vec4(lighting, 1.0);
}