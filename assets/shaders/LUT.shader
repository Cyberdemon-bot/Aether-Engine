#shader vertex
#version 330 core

// Input vertex chuẩn cho Quad (đã sửa trong C++ để dùng DrawIndexed)
layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main()
{
    v_TexCoord = a_TexCoord;
    // Vẽ full màn hình (-1 đến 1)
    gl_Position = vec4(a_Position.x, a_Position.y, 0.0, 1.0);
}

#shader fragment
#version 330 core

out vec4 color;
in vec2 v_TexCoord;

uniform sampler2D u_SceneTexture; // Texture 0: Ảnh game
uniform sampler2D u_LutTexture;   // Texture 1: Ảnh LUT
uniform float u_LutIntensity;     // 0.0 - 1.0

void main()
{
    vec4 sceneColor = texture(u_SceneTexture, v_TexCoord);
    
    // --- CẤU HÌNH LUT ---
    // Hard code size 16 như yêu cầu (LUT Strip 256x16)
    float size = 16.0; 
    
    // 1. Tính toán lớp (Slice) dựa trên kênh Blue
    float blueColor = sceneColor.b * (size - 1.0);
    
    float quad1_y = floor(floor(blueColor) / size);
    float quad1_x = floor(blueColor) - (quad1_y * size);
    
    float quad2_y = floor(ceil(blueColor) / size);
    float quad2_x = ceil(blueColor) - (quad2_y * size);
    
    // 2. Tính toán UV chuẩn xác (Pixel Perfect Sampling)
    // Để tránh lỗi viền (bleeding), ta phải map 0.0-1.0 vào vùng an toàn: [0.5/Size, 1.0 - 0.5/Size]
    
    // Kích thước texture thực tế
    float texWidth  = size * size; // 256.0
    float texHeight = size;        // 16.0

    // Offset nửa pixel (Half-pixel offset)
    float halfPixelX = 0.5 / texWidth;
    float halfPixelY = 0.5 / texHeight;

    // Phạm vi quét an toàn (trừ đi 1 pixel mỗi chiều để không liếm sang ô bên cạnh)
    float rangeX = (1.0 / size) - (1.0 / texWidth);
    float rangeY = (size - 1.0) / size; // Tương đương: 1.0 - (1.0 / texHeight)

    vec2 texPos1;
    // X: Vị trí bắt đầu Slice + Offset an toàn + Màu Đỏ * Phạm vi an toàn
    texPos1.x = (quad1_x / size) + halfPixelX + (sceneColor.r * rangeX);
    // Y: Offset an toàn + Màu Xanh * Phạm vi an toàn
    texPos1.y = halfPixelY + (sceneColor.g * rangeY);

    vec2 texPos2;
    texPos2.x = (quad2_x / size) + halfPixelX + (sceneColor.r * rangeX);
    texPos2.y = texPos1.y; // Y giống nhau vì strip nằm ngang 1 hàng

    // 3. Trộn màu (Linear Interpolation)
    vec4 newColor1 = texture(u_LutTexture, texPos1);
    vec4 newColor2 = texture(u_LutTexture, texPos2);
    
    // Trộn giữa 2 slice dựa trên phần lẻ của màu Blue
    vec4 lutColor = mix(newColor1, newColor2, fract(blueColor));
    
    // 4. Kết quả cuối cùng (Trộn với ảnh gốc theo Intensity)
    color = mix(sceneColor, lutColor, u_LutIntensity);
}