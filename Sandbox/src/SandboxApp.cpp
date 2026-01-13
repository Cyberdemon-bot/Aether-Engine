#include <Aether.h> 
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

class TestLayer : public Aether::Layer
{
public:
    TestLayer()
        : Layer("Shadow Mapping Demo"), m_Camera(glm::vec3(0.0f, 3.0f, 10.0f))
    {
    }

    virtual ~TestLayer() = default;

    virtual void Detach() override 
    {
        m_VAO.reset();
        m_VBO.reset();
        m_IBO.reset();
        m_Shader.reset();
        m_ShadowShader.reset();
        m_Texture.reset();
        m_ShadowFBO.reset(); 
    }

    virtual void Attach() override 
    {
        // Setup ImGui context
        ImGuiContext* IGContext = Aether::ImGuiLayer::GetContext();
        if (IGContext) ImGui::SetCurrentContext(IGContext);
        
        // Initialize rendering
        Aether::Legacy::LegacyAPI::Init();

        // Create cube geometry (24 vertices, properly indexed)
        float vertices[] = {
            // Positions          // Normals           // TexCoords
            // Back face (Z-)
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
            
            // Front face (Z+)
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
            
            // Left face (X-)
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
            
            // Right face (X+)
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
            
            // Bottom face (Y-)
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
            
            // Top face (Y+)
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f
        };

        unsigned int indices[] = {
            0,  1,  2,   2,  3,  0,   // Back face
            4,  5,  6,   6,  7,  4,   // Front face
            8,  9,  10,  10, 11, 8,   // Left face
            12, 13, 14,  14, 15, 12,  // Right face
            16, 17, 18,  18, 19, 16,  // Bottom face
            20, 21, 22,  22, 23, 20   // Top face
        };

        // Setup vertex array and buffers
        m_VAO = std::make_shared<Aether::Legacy::VertexArray>();
        m_VBO = std::make_shared<Aether::Legacy::VertexBuffer>(vertices, sizeof(vertices));
        
        Aether::Legacy::VertexBufferLayout layout;
        layout.Push<float>(3); // Position
        layout.Push<float>(3); // Normal
        layout.Push<float>(2); // TexCoord
        m_VAO->AddBuffer(*m_VBO, layout);

        m_IBO = std::make_shared<Aether::Legacy::IndexBuffer>(indices, 36);
        
        // Load shaders
        AE_CORE_INFO("Loading shaders...");
        m_Shader = std::make_shared<Aether::Legacy::Shader>(
            "assets/shaders/LightingShadow.shader"
        );
        m_ShadowShader = std::make_shared<Aether::Legacy::Shader>(
            "assets/shaders/ShadowMap.shader"
        );
        AE_CORE_INFO("Shaders loaded successfully");
        
        // Load texture
        m_Texture = std::make_shared<Aether::Legacy::Texture>(
            "assets/textures/wood.jpg"
        );

        // Create shadow map framebuffer
        Aether::Legacy::FramebufferSpecification fbSpec;
        fbSpec.Width = m_ShadowMapResolution; 
        fbSpec.Height = m_ShadowMapResolution;
        fbSpec.Attachments = { Aether::Legacy::FramebufferTextureFormat::DEPTH24STENCIL8 };
        
        m_ShadowFBO = std::make_shared<Aether::Legacy::FrameBuffer>(fbSpec);
        
        AE_INFO("Shadow Mapping Demo initialized successfully!");
        AE_INFO("Controls: Click to lock cursor, WASD to move, Space/Ctrl for up/down, ESC to unlock");
    }

    void Update(Aether::Timestep ts) override
    {
        // Update rotation
        if (m_EnableRotation) {
            m_Rotation += ts * m_RotationSpeed;
        }

        // Get window dimensions
        auto& window = Aether::Application::Get().GetWindow();
        uint32_t windowWidth = window.GetFramebufferWidth();
        uint32_t windowHeight = window.GetFramebufferHeight();

        // Handle input
        HandleInput(ts);

        // Calculate light space matrix for shadow mapping
        glm::mat4 lightSpaceMatrix = CalculateLightSpaceMatrix();

        // ========== PASS 1: SHADOW MAP GENERATION ==========
        RenderShadowPass(lightSpaceMatrix);

        // ========== PASS 2: MAIN SCENE RENDERING ==========
        RenderMainPass(windowWidth, windowHeight, lightSpaceMatrix);
    }

    void HandleInput(Aether::Timestep ts)
    {
        // Toggle cursor lock with ESC
        if (Aether::Input::IsKeyPressed(Aether::Key::Escape)) {
            m_CursorLocked = false;
            Aether::Input::SetCursorMode(Aether::CursorMode::Normal);
        }
        
        // Lock cursor on left click (if not over ImGui)
        if (Aether::Input::IsMouseButtonPressed(Aether::Mouse::ButtonLeft) && 
            !m_CursorLocked && !ImGui::GetIO().WantCaptureMouse) {
            m_CursorLocked = true;
            Aether::Input::SetCursorMode(Aether::CursorMode::Locked);
            m_LastMousePos = Aether::Input::GetMousePosition();
        }
        
        // Camera movement when cursor is locked
        if (m_CursorLocked) {
            // Keyboard movement
            if (Aether::Input::IsKeyPressed(Aether::Key::W)) 
                m_Camera.ProcessKeyboard(Aether::Legacy::FORWARD, ts);
            if (Aether::Input::IsKeyPressed(Aether::Key::S)) 
                m_Camera.ProcessKeyboard(Aether::Legacy::BACKWARD, ts);
            if (Aether::Input::IsKeyPressed(Aether::Key::A)) 
                m_Camera.ProcessKeyboard(Aether::Legacy::LEFT, ts);
            if (Aether::Input::IsKeyPressed(Aether::Key::D)) 
                m_Camera.ProcessKeyboard(Aether::Legacy::RIGHT, ts);
            if (Aether::Input::IsKeyPressed(Aether::Key::Space)) 
                m_Camera.ProcessKeyboard(Aether::Legacy::UP, ts);
            if (Aether::Input::IsKeyPressed(Aether::Key::LeftControl)) 
                m_Camera.ProcessKeyboard(Aether::Legacy::DOWN, ts);
            
            // Mouse look
            glm::vec2 currentMousePos = Aether::Input::GetMousePosition();
            glm::vec2 delta = currentMousePos - m_LastMousePos;
            m_LastMousePos = currentMousePos;
            m_Camera.ProcessMouseMovement(delta.x, -delta.y);
        }
    }

    glm::mat4 CalculateLightSpaceMatrix()
    {
        float near_plane = 1.0f, far_plane = 30.0f;
        glm::mat4 lightProjection = glm::ortho(
            -m_LightFrustumSize, m_LightFrustumSize, 
            -m_LightFrustumSize, m_LightFrustumSize, 
            near_plane, far_plane
        );
        glm::mat4 lightView = glm::lookAt(
            m_LightPos, 
            glm::vec3(0.0f), 
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        return lightProjection * lightView;
    }

    void RenderShadowPass(const glm::mat4& lightSpaceMatrix)
    {
        // Bind shadow framebuffer
        m_ShadowFBO->Bind();
        Aether::Legacy::LegacyAPI::SetViewport(0, 0, m_ShadowMapResolution, m_ShadowMapResolution);
        Aether::Legacy::LegacyAPI::Clear();
        
        // Bind shadow shader and set uniforms
        m_ShadowShader->Bind();
        m_ShadowShader->SetUniformMat4f("u_LightSpaceMatrix", lightSpaceMatrix);
        
        // Render scene from light's perspective
        RenderScene(m_ShadowShader);
        
        // Unbind shadow framebuffer
        m_ShadowFBO->Unbind();
    }

    void RenderMainPass(uint32_t width, uint32_t height, const glm::mat4& lightSpaceMatrix)
    {
        // Set viewport to window size
        Aether::Legacy::LegacyAPI::SetViewport(0, 0, width, height);
        
        // Clear screen with background color
        Aether::Legacy::LegacyAPI::SetClearColor(m_BackgroundColor);
        Aether::Legacy::LegacyAPI::Clear();

        // Bind main shader
        m_Shader->Bind();
        
        // Bind textures
        m_Texture->Bind(0);
        m_Shader->SetUniform1i("u_Texture", 0);
        
        m_ShadowFBO->BindDepthTexture(1);
        m_Shader->SetUniform1i("u_ShadowMap", 1);

        // Set camera matrices
        float aspectRatio = (float)width / (float)height;
        glm::mat4 projection = glm::perspective(
            glm::radians(m_Camera.Zoom), 
            aspectRatio, 
            0.1f, 
            100.0f
        );
        glm::mat4 view = m_Camera.GetViewMatrix();
        
        // Set shader uniforms
        m_Shader->SetUniformMat4f("u_Projection", projection);
        m_Shader->SetUniformMat4f("u_View", view);
        m_Shader->SetUniform3f("u_ViewPos", m_Camera.Position);
        m_Shader->SetUniform3f("u_LightPos", m_LightPos);
        m_Shader->SetUniformMat4f("u_LightSpaceMatrix", lightSpaceMatrix);

        // Render scene from camera's perspective
        RenderScene(m_Shader);
    }

    void RenderScene(std::shared_ptr<Aether::Legacy::Shader> shader)
    {
        // Render Cube A (rotating)
        glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationA);
        model = glm::rotate(model, m_Rotation, glm::vec3(0.5f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(m_CubeScale));
        shader->SetUniformMat4f("u_Model", model);
        Aether::Legacy::LegacyAPI::Draw(*m_VAO, *m_IBO, *shader);

        // Render Cube B (rotating in different direction)
        model = glm::translate(glm::mat4(1.0f), m_TranslationB);
        model = glm::rotate(model, m_Rotation * 0.7f, glm::vec3(1.0f, 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(m_CubeScale));
        shader->SetUniformMat4f("u_Model", model);
        Aether::Legacy::LegacyAPI::Draw(*m_VAO, *m_IBO, *shader);

        //Light
        model = glm::translate(glm::mat4(1.0f), m_LightPos);
        model = glm::scale(model, glm::vec3(0.2f));
        shader->SetUniformMat4f("u_Model", model);
        Aether::Legacy::LegacyAPI::Draw(*m_VAO, *m_IBO, *shader);
        
        // Render floor plane
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
        model = glm::scale(model, glm::vec3(m_FloorScale, 0.1f, m_FloorScale));
        shader->SetUniformMat4f("u_Model", model);
        Aether::Legacy::LegacyAPI::Draw(*m_VAO, *m_IBO, *shader);


        //light
        model = glm::translate(glm::mat4(1.0f), m_LightPos);
        model = glm::scale(model, glm::vec3(0.2f));
        shader->SetUniformMat4f("u_Model", model);
        shader->SetUniform1i("u_IsLightSource", 1); 
        shader->SetUniform3f("u_FlatColor", glm::vec3(1.0f, 1.0f, 0.0f)); 
        Aether::Legacy::LegacyAPI::Draw(*m_VAO, *m_IBO, *shader);
        shader->SetUniform1i("u_IsLightSource", 0);
    }

    virtual void OnImGuiRender() override
    {
        ImGui::Begin("Shadow Mapping Controls");
        
        // FPS and Camera Info
        ImGui::Text("FPS: %.1f (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", 
            m_Camera.Position.x, m_Camera.Position.y, m_Camera.Position.z);
        ImGui::Separator();
        
        // Camera Controls
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Controls:");
            ImGui::BulletText("Left Click: Lock/Unlock cursor");
            ImGui::BulletText("WASD: Move horizontally");
            ImGui::BulletText("Space/Ctrl: Move up/down");
            ImGui::BulletText("Mouse: Look around");
            ImGui::BulletText("ESC: Unlock cursor");
            ImGui::Spacing();
            ImGui::SliderFloat("Movement Speed", &m_Camera.MovementSpeed, 1.0f, 20.0f);
            ImGui::SliderFloat("Mouse Sensitivity", &m_Camera.MouseSensitivity, 0.05f, 0.5f);
            ImGui::SliderFloat("FOV (Zoom)", &m_Camera.Zoom, 30.0f, 120.0f);
        }
        
        // Lighting Controls
        if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat3("Light Position", &m_LightPos.x, -15.0f, 15.0f);
            ImGui::SliderFloat("Light Frustum Size", &m_LightFrustumSize, 5.0f, 30.0f);
            ImGui::Separator();
            ImGui::Text("Shadow Map Settings:");
            if (ImGui::SliderInt("Shadow Resolution", &m_ShadowMapResolution, 512, 4096)) {
                // Recreate shadow framebuffer with new resolution
                Aether::Legacy::FramebufferSpecification fbSpec;
                fbSpec.Width = m_ShadowMapResolution;
                fbSpec.Height = m_ShadowMapResolution;
                fbSpec.Attachments = { Aether::Legacy::FramebufferTextureFormat::DEPTH24STENCIL8 };
                m_ShadowFBO = std::make_shared<Aether::Legacy::FrameBuffer>(fbSpec);
            }
        }
        
        // Scene Objects
        if (ImGui::CollapsingHeader("Scene Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Cube A:");
            ImGui::SliderFloat3("Position##CubeA", &m_TranslationA.x, -8.0f, 8.0f);
            ImGui::Spacing();
            
            ImGui::Text("Cube B:");
            ImGui::SliderFloat3("Position##CubeB", &m_TranslationB.x, -8.0f, 8.0f);
            ImGui::Spacing();
            
            ImGui::SliderFloat("Cube Scale", &m_CubeScale, 0.5f, 3.0f);
            ImGui::SliderFloat("Floor Scale", &m_FloorScale, 5.0f, 30.0f);
        }
        
        // Animation
        if (ImGui::CollapsingHeader("Animation")) {
            ImGui::Checkbox("Enable Rotation", &m_EnableRotation);
            ImGui::SliderFloat("Rotation Speed", &m_RotationSpeed, 0.0f, 2.0f);
            if (ImGui::Button("Reset Rotation")) {
                m_Rotation = 0.0f;
            }
        }
        
        // Rendering Settings
        if (ImGui::CollapsingHeader("Rendering")) {
            ImGui::ColorEdit3("Background Color", &m_BackgroundColor.x);
        }
        
        ImGui::End();
    }

    void OnEvent(Aether::Event& event) override 
    {
        // Handle events if needed
    }

private:
    // Rendering resources
    std::shared_ptr<Aether::Legacy::VertexArray> m_VAO;
    std::shared_ptr<Aether::Legacy::VertexBuffer> m_VBO;
    std::shared_ptr<Aether::Legacy::IndexBuffer> m_IBO;
    std::shared_ptr<Aether::Legacy::Shader> m_Shader;
    std::shared_ptr<Aether::Legacy::Shader> m_ShadowShader;
    std::shared_ptr<Aether::Legacy::Texture> m_Texture;
    std::shared_ptr<Aether::Legacy::FrameBuffer> m_ShadowFBO;
    
    // Camera
    Aether::Legacy::Camera m_Camera;
    bool m_CursorLocked = false;
    glm::vec2 m_LastMousePos = { 0.0f, 0.0f };
    
    // Scene objects
    glm::vec3 m_TranslationA = { -2.0f, 0.5f, 0.0f };
    glm::vec3 m_TranslationB = { 2.0f, 0.5f, 0.0f };
    float m_CubeScale = 1.0f;
    float m_FloorScale = 15.0f;
    
    // Lighting
    glm::vec3 m_LightPos = { -5.0f, 8.0f, -3.0f };
    float m_LightFrustumSize = 15.0f;
    
    // Animation
    float m_Rotation = 0.0f;
    float m_RotationSpeed = 0.5f;
    bool m_EnableRotation = true;
    
    // Rendering settings
    glm::vec4 m_BackgroundColor = { 0.2f, 0.3f, 0.4f, 1.0f };
    int m_ShadowMapResolution = 2048;
};


class Sandbox : public Aether::Application
{
public:
    Sandbox()
    {
        PushLayer(new TestLayer());
    }

    ~Sandbox()
    {
    }
};

Aether::Application* Aether::CreateApplication()
{
    return new Sandbox();
}