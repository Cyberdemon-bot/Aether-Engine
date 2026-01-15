#shader vertex
#version 330 core

// Attribute vị trí cơ bản
layout(location = 0) in vec3 a_Position;

// Attribute cho Instancing (Model Matrix chiếm location 3,4,5,6)
// Phải khai báo dòng này để khớp với layout trong C++
layout(location = 3) in mat4 a_InstanceModel;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Model;
uniform bool u_UseInstancing; // Thêm biến này để nhận tín hiệu từ C++

void main()
{
    // Logic chọn matrix y hệt như shader chính
    mat4 model = u_UseInstancing ? a_InstanceModel : u_Model;
    
    gl_Position = u_LightSpaceMatrix * model * vec4(a_Position, 1.0);
}

#shader fragment
#version 330 core

void main()
{
    // Fragment shader cho Shadow Map thường để trống 
    // vì chúng ta chỉ quan tâm đến Depth Buffer (được ghi tự động)
}