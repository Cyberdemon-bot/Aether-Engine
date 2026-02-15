#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Tangent;
layout(location = 3) in vec2 a_TexCoord;
layout(location = 4) in uvec4 a_Joints;
layout(location = 5) in vec4 a_Weights;
layout(location = 6) in mat4 a_InstanceModel;

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

void main()
{
    vec4 localPos = vec4(a_Position, 1.0);
    
    if (u_HasAnimation == 1)
    {
        mat4 skinMatrix = 
            a_Weights.x * u_BoneMatrices[a_Joints.x] +
            a_Weights.y * u_BoneMatrices[a_Joints.y] +
            a_Weights.z * u_BoneMatrices[a_Joints.z] +
            a_Weights.w * u_BoneMatrices[a_Joints.w];
        
        localPos = skinMatrix * localPos;
    }
    
    mat4 modelMatrix = u_UseInstancing == 1 ? a_InstanceModel : u_Model;
    vec4 worldPos = modelMatrix * localPos;
    
    if (u_Lights.lightCount > 0)
    {
        gl_Position = u_Lights.lights[0].lightSpaceMatrix * worldPos;
    }
    else
    {
        gl_Position = vec4(0.0);
    }
}

#shader fragment
#version 330 core

void main()
{
    
}