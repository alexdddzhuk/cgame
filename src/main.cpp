#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <string>
#include <iostream>

// ── Shaders ──────────────────────────────────────────────────────────────────

const char* vertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragColor;
out vec3 fragNormal;
out vec3 fragPos;
out vec2 fragTexCoord;

void main() {
    vec4 worldPos   = model * vec4(aPos, 1.0);
    gl_Position     = projection * view * worldPos;
    fragColor       = aColor;
    fragNormal      = mat3(transpose(inverse(model))) * aNormal;
    fragPos         = worldPos.xyz;
    fragTexCoord    = aTexCoord;
}
)";

const char* fragSrc = R"(
#version 330 core
in  vec3 fragColor;
in  vec3 fragNormal;
in  vec3 fragPos;
in  vec2 fragTexCoord;

uniform bool      uLighting;
uniform vec3      uLightDir;
uniform vec3      uLightColor;
uniform vec3      uCamPos;
uniform float     uSpecStrength;
uniform float     uShininess;
uniform bool      uUseTexture;
uniform bool      uBumpMap;
uniform float     uBumpStrength;
uniform sampler2D uTexture;

out vec4 outColor;

void main() {
    vec3 base = uUseTexture ? texture(uTexture, fragTexCoord).rgb : fragColor;
    if (uLighting) {
        vec3 norm = normalize(fragNormal);

        // Bump mapping: sample height at +U and +V neighbors, build TBN, perturb normal
        if (uBumpMap && uUseTexture) {
            float step = 0.004;
            vec3 lumW = vec3(0.299, 0.587, 0.114);
            float h00 = dot(texture(uTexture, fragTexCoord                     ).rgb, lumW);
            float h10 = dot(texture(uTexture, fragTexCoord + vec2(step, 0.0)   ).rgb, lumW);
            float h01 = dot(texture(uTexture, fragTexCoord + vec2(0.0,  step)  ).rgb, lumW);
            float dhdx = (h10 - h00) / step;
            float dhdy = (h01 - h00) / step;

            // Reconstruct tangent/bitangent from screen-space position + UV derivatives
            vec3 dp1  = dFdx(fragPos);
            vec3 dp2  = dFdy(fragPos);
            vec2 duv1 = dFdx(fragTexCoord);
            vec2 duv2 = dFdy(fragTexCoord);
            float det = duv1.x * duv2.y - duv1.y * duv2.x;
            if (abs(det) > 1e-6) {
                vec3 T = normalize(( duv2.y * dp1 - duv1.y * dp2) / det);
                vec3 B = normalize((-duv2.x * dp1 + duv1.x * dp2) / det);
                norm = normalize(norm + uBumpStrength * (-dhdx * T - dhdy * B));
            }
        }

        vec3  lightDir = normalize(uLightDir);
        float diff     = max(dot(norm, lightDir), 0.0);
        // Blinn-Phong specular
        vec3  viewDir  = normalize(uCamPos - fragPos);
        vec3  halfDir  = normalize(lightDir + viewDir);
        float spec     = pow(max(dot(norm, halfDir), 0.0), uShininess);
        vec3  ambient  = 0.18 * base;
        vec3  diffuse  = diff * uLightColor * base;
        vec3  specular = uSpecStrength * spec * uLightColor;
        outColor = vec4(ambient + diffuse + specular, 1.0);
    } else {
        outColor = vec4(base, 1.0);
    }
}
)";

// ── Cube geometry (position + color per vertex, 4 verts per face) ─────────

//  pos(3) + color(3) + normal(3) + uv(2) = 11 floats per vertex
//  Each face: UV (0,0)→(1,0)→(1,1)→(0,1)
static const float vertices[] = {
    // front  (z=+0.5)  normal  0, 0, 1
    -0.5f,-0.5f, 0.5f,  1.f,0.f,0.f,  0.f, 0.f, 1.f,  0.f,0.f,
     0.5f,-0.5f, 0.5f,  1.f,0.f,0.f,  0.f, 0.f, 1.f,  1.f,0.f,
     0.5f, 0.5f, 0.5f,  1.f,0.f,0.f,  0.f, 0.f, 1.f,  1.f,1.f,
    -0.5f, 0.5f, 0.5f,  1.f,0.f,0.f,  0.f, 0.f, 1.f,  0.f,1.f,
    // back   (z=-0.5)  normal  0, 0,-1
     0.5f,-0.5f,-0.5f,  0.f,1.f,0.f,  0.f, 0.f,-1.f,  0.f,0.f,
    -0.5f,-0.5f,-0.5f,  0.f,1.f,0.f,  0.f, 0.f,-1.f,  1.f,0.f,
    -0.5f, 0.5f,-0.5f,  0.f,1.f,0.f,  0.f, 0.f,-1.f,  1.f,1.f,
     0.5f, 0.5f,-0.5f,  0.f,1.f,0.f,  0.f, 0.f,-1.f,  0.f,1.f,
    // left   (x=-0.5)  normal -1, 0, 0
    -0.5f,-0.5f,-0.5f,  0.f,0.f,1.f, -1.f, 0.f, 0.f,  0.f,0.f,
    -0.5f,-0.5f, 0.5f,  0.f,0.f,1.f, -1.f, 0.f, 0.f,  1.f,0.f,
    -0.5f, 0.5f, 0.5f,  0.f,0.f,1.f, -1.f, 0.f, 0.f,  1.f,1.f,
    -0.5f, 0.5f,-0.5f,  0.f,0.f,1.f, -1.f, 0.f, 0.f,  0.f,1.f,
    // right  (x=+0.5)  normal  1, 0, 0
     0.5f,-0.5f, 0.5f,  1.f,1.f,0.f,  1.f, 0.f, 0.f,  0.f,0.f,
     0.5f,-0.5f,-0.5f,  1.f,1.f,0.f,  1.f, 0.f, 0.f,  1.f,0.f,
     0.5f, 0.5f,-0.5f,  1.f,1.f,0.f,  1.f, 0.f, 0.f,  1.f,1.f,
     0.5f, 0.5f, 0.5f,  1.f,1.f,0.f,  1.f, 0.f, 0.f,  0.f,1.f,
    // top    (y=+0.5)  normal  0, 1, 0
    -0.5f, 0.5f, 0.5f,  0.f,1.f,1.f,  0.f, 1.f, 0.f,  0.f,0.f,
     0.5f, 0.5f, 0.5f,  0.f,1.f,1.f,  0.f, 1.f, 0.f,  1.f,0.f,
     0.5f, 0.5f,-0.5f,  0.f,1.f,1.f,  0.f, 1.f, 0.f,  1.f,1.f,
    -0.5f, 0.5f,-0.5f,  0.f,1.f,1.f,  0.f, 1.f, 0.f,  0.f,1.f,
    // bottom (y=-0.5)  normal  0,-1, 0
    -0.5f,-0.5f,-0.5f,  1.f,0.f,1.f,  0.f,-1.f, 0.f,  0.f,0.f,
     0.5f,-0.5f,-0.5f,  1.f,0.f,1.f,  0.f,-1.f, 0.f,  1.f,0.f,
     0.5f,-0.5f, 0.5f,  1.f,0.f,1.f,  0.f,-1.f, 0.f,  1.f,1.f,
    -0.5f,-0.5f, 0.5f,  1.f,0.f,1.f,  0.f,-1.f, 0.f,  0.f,1.f,
};

static const unsigned int indices[] = {
     0, 1, 2,  2, 3, 0,   // front
     4, 5, 6,  6, 7, 4,   // back
     8, 9,10, 10,11, 8,   // left
    12,13,14, 14,15,12,   // right
    16,17,18, 18,19,16,   // top
    20,21,22, 22,23,20,   // bottom
};

// 12 cube edges as GL_LINES — uses only the first face's corner vertices
// front corners: 0,1,2,3   back corners: 4,5,6,7
static const unsigned int edgeIndices[] = {
    0,1,  1,2,  2,3,  3,0,   // front face
    4,5,  5,6,  6,7,  7,4,   // back  face
    0,5,  1,4,  2,7,  3,6,   // side connections
};

// ── Helpers ───────────────────────────────────────────────────────────────────

static GLuint compileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader error: " << log << '\n';
    }
    return s;
}

static GLuint buildProgram()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    GLuint p  = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

// ── Settings ──────────────────────────────────────────────────────────────────

struct Settings {
    bool  autoRotate       = true;
    float autoSpeedX       = 23.f;
    float autoSpeedY       = 47.f;
    float keySpeed         = 90.f;
    float mouseSens        = 0.4f;
    float bgColor[3]       = { 0.05f, 0.05f, 0.10f };
    bool  wireframe        = false;
    float cubeScale        = 1.0f;
    // lighting
    bool  lighting         = true;
    float lightDir[3]      = { 1.f, 2.f, 2.f };
    float lightColor[3]    = { 1.f, 1.f, 1.f };
    float specStrength     = 0.55f;
    float shininess        = 48.f;
    // texture
    bool  useTexture       = true;
    bool  bumpMap          = true;
    float bumpStrength     = 0.06f;
};

// ── Texture state ─────────────────────────────────────────────────────────────

static GLuint g_texID      = 0;
static char   g_texPath[512] = "No texture loaded";

static GLuint loadTexture(const char* path)
{
    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
    if (!data) return 0;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return tex;
}

static void browseTexture()
{
    char path[512] = {};
    OPENFILENAMEA ofn   = {};
    ofn.lStructSize     = sizeof(ofn);
    ofn.lpstrFilter     = "Images\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files\0*.*\0";
    ofn.lpstrFile       = path;
    ofn.nMaxFile        = sizeof(path);
    ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle      = "Choose Texture";
    if (!GetOpenFileNameA(&ofn)) return;

    GLuint newTex = loadTexture(path);
    if (newTex) {
        if (g_texID) glDeleteTextures(1, &g_texID);
        g_texID = newTex;
        strncpy_s(g_texPath, path, sizeof(g_texPath) - 1);
    }
}

// Creates a simple grey checkerboard used as fallback
static GLuint createDefaultTexture()
{
    const int SZ = 64;
    unsigned char data[SZ * SZ * 4];
    for (int y = 0; y < SZ; y++) for (int x = 0; x < SZ; x++) {
        unsigned char v = (((x / 8) + (y / 8)) & 1) ? 210 : 80;
        int i = (y * SZ + x) * 4;
        data[i] = data[i+1] = data[i+2] = v;  data[i+3] = 255;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SZ, SZ, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return tex;
}

// Tries to auto-load a texture from next to the .exe
static void autoLoadTexture()
{
    char exeDir[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exeDir, MAX_PATH);
    char* slash = strrchr(exeDir, '\\');
    if (slash) *(slash + 1) = '\0';

    const char* names[] = { "texture.png","texture.jpg","texture.jpeg","texture.bmp","texture.tga" };
    for (auto& name : names) {
        char full[MAX_PATH];
        sprintf_s(full, "%s%s", exeDir, name);
        GLuint t = loadTexture(full);
        if (t) {
            if (g_texID) glDeleteTextures(1, &g_texID);
            g_texID = t;
            strncpy_s(g_texPath, full, sizeof(g_texPath) - 1);
            return;
        }
    }
    // Fallback: procedural checkerboard
    g_texID = createDefaultTexture();
    strncpy_s(g_texPath, "default checkerboard", sizeof(g_texPath) - 1);
}

// ── State ─────────────────────────────────────────────────────────────────────

static float    g_rotX         = 20.f;
static float    g_rotY         = 30.f;
static Settings g_set;
static bool     g_showSettings = false;

// Camera
static float    g_camDist      = 3.f;

// Mouse drag + inertia
static bool     g_dragging     = false;
static double   g_dragLastX    = 0.0;
static double   g_dragLastY    = 0.0;
static float    g_velX         = 0.f;   // degrees/sec inertia
static float    g_velY         = 0.f;
static float    g_dragDX       = 0.f;   // last-frame drag delta (for handoff)
static float    g_dragDY       = 0.f;

// Scroll callback
static void scrollCallback(GLFWwindow*, double /*xoff*/, double yoff)
{
    g_camDist -= (float)yoff * 0.25f;
    if (g_camDist < 0.5f) g_camDist = 0.5f;
    if (g_camDist > 20.f) g_camDist = 20.f;
}

// Fullscreen toggle
static GLFWwindow* g_window       = nullptr;
static bool        g_fullscreen   = false;
static int         g_winX=100, g_winY=100, g_winW=800, g_winH=600;

static void toggleFullscreen()
{
    g_fullscreen = !g_fullscreen;
    if (g_fullscreen) {
        // Save windowed position/size
        glfwGetWindowPos (g_window, &g_winX, &g_winY);
        glfwGetWindowSize(g_window, &g_winW, &g_winH);
        GLFWmonitor*       mon  = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(mon);
        glfwSetWindowMonitor(g_window, mon, 0, 0,
                             mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(g_window, nullptr,
                             g_winX, g_winY, g_winW, g_winH, 0);
    }
}

// ── Input ─────────────────────────────────────────────────────────────────────

static void processInput(GLFWwindow* w, float dt)
{
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(w, true);

    // Alt+Enter — toggle fullscreen (edge-triggered)
    static bool altEnterWas = false;
    bool altEnterNow = !ImGui::GetIO().WantCaptureKeyboard &&
                       (glfwGetKey(w, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
                        glfwGetKey(w, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) &&
                       glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS;
    if (altEnterNow && !altEnterWas) toggleFullscreen();
    altEnterWas = altEnterNow;

    ImGuiIO& io = ImGui::GetIO();

    // ── Auto-rotate (classic D3D style) ──
    if (g_set.autoRotate) {
        g_rotX += g_set.autoSpeedX * dt;
        g_rotY += g_set.autoSpeedY * dt;
        g_velX = g_velY = 0.f;
        return;
    }

    // ── Keyboard ──
    if (!io.WantCaptureKeyboard) {
        if (glfwGetKey(w, GLFW_KEY_UP)    == GLFW_PRESS) { g_rotX -= g_set.keySpeed * dt; g_velX = -g_set.keySpeed; }
        if (glfwGetKey(w, GLFW_KEY_DOWN)  == GLFW_PRESS) { g_rotX += g_set.keySpeed * dt; g_velX =  g_set.keySpeed; }
        if (glfwGetKey(w, GLFW_KEY_LEFT)  == GLFW_PRESS) { g_rotY -= g_set.keySpeed * dt; g_velY = -g_set.keySpeed; }
        if (glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS) { g_rotY += g_set.keySpeed * dt; g_velY =  g_set.keySpeed; }
    }

    // ── Mouse drag + inertia handoff ──
    if (!io.WantCaptureMouse) {
        bool btnDown = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        double mx, my;
        glfwGetCursorPos(w, &mx, &my);

        if (btnDown) {
            if (!g_dragging) {
                g_dragging  = true;
                g_dragLastX = mx;
                g_dragLastY = my;
                g_velX = g_velY = 0.f;
            } else {
                g_dragDX = (float)(mx - g_dragLastX) * g_set.mouseSens;
                g_dragDY = (float)(my - g_dragLastY) * g_set.mouseSens;
                g_rotY  += g_dragDX;
                g_rotX  += g_dragDY;
                g_dragLastX = mx;
                g_dragLastY = my;
            }
        } else if (g_dragging) {
            // hand off last-frame delta as inertia velocity
            if (dt > 0.f) {
                g_velY = g_dragDX / dt;
                g_velX = g_dragDY / dt;
            }
            g_dragging = g_dragDX = g_dragDY = false;
        }
    } else {
        g_dragging = false;
    }

    // ── Apply inertia (exponential decay) ──
    if (!g_dragging) {
        float decay = expf(-4.5f * dt); // ~half-life ≈ 0.15s
        g_rotY += g_velY * dt;
        g_rotX += g_velX * dt;
        g_velY *= decay;
        g_velX *= decay;
        // kill tiny velocities
        if (fabsf(g_velX) < 0.1f) g_velX = 0.f;
        if (fabsf(g_velY) < 0.1f) g_velY = 0.f;
    }
}

// ── UI helpers ────────────────────────────────────────────────────────────────

// Tinted separator label
static void Section(const char* label)
{
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.85f, 1.f, 1.f));
    ImGui::SeparatorText(label);
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

// ── UI ────────────────────────────────────────────────────────────────────────

static void drawUI(int winW, int winH)
{
    ImGuiIO& io = ImGui::GetIO();

    // ── FPS overlay (top-right) ───────────────────────────────────────────────
    {
        const float PAD = 10.f;
        float fps = io.Framerate;
        // color: green > 55, yellow > 30, red otherwise
        ImVec4 col = fps > 55.f ? ImVec4(0.2f,1.f,0.4f,1.f)
                   : fps > 30.f ? ImVec4(1.f,0.85f,0.1f,1.f)
                                : ImVec4(1.f,0.3f,0.3f,1.f);

        ImGui::SetNextWindowPos(ImVec2((float)winW - PAD, PAD), ImGuiCond_Always, ImVec2(1.f, 0.f));
        ImGui::SetNextWindowBgAlpha(0.55f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
        ImGui::Begin("##fps", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::Text("%.0f FPS", fps);
        ImGui::PopStyleColor();
        ImGui::End();
        ImGui::PopStyleVar();
    }

    // ── Burger button (top-left) ──────────────────────────────────────────────
    {
        ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(36, 36), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(g_showSettings ? 0.85f : 0.55f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(0, 2));
        ImGui::Begin("##burger", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav);

        // Draw three horizontal bars manually
        ImDrawList* dl  = ImGui::GetWindowDrawList();
        ImVec2      pos = ImGui::GetCursorScreenPos();
        ImU32       col = IM_COL32(200, 220, 255, 255);
        float x0 = pos.x + 4, x1 = pos.x + 28;
        for (int i = 0; i < 3; ++i)
        {
            float y = pos.y + 4 + i * 8.f;
            dl->AddRectFilled(ImVec2(x0, y), ImVec2(x1, y + 3), col, 1.f);
        }
        // Invisible full-size button over the bars
        if (ImGui::InvisibleButton("##b", ImVec2(30, 28)))
            g_showSettings = !g_showSettings;

        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    if (!g_showSettings) return;

    // ── Settings panel ────────────────────────────────────────────────────────
    ImGui::SetNextWindowPos(ImVec2(52, 8), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(280, 0), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.92f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(12, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    ImVec2(8,  6));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding,   4.f);
    ImGui::Begin("  Settings", &g_showSettings,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize);

    // ── Rotation ──
    Section("  Rotation");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##pitch", &g_rotX, -360.f, 360.f, "Pitch  %.0f deg");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##yaw",   &g_rotY, -360.f, 360.f, "Yaw    %.0f deg");
    ImGui::Spacing();
    if (ImGui::Button("Reset rotation", ImVec2(-1, 0)))
        { g_rotX = 25.f; g_rotY = 30.f; }

    // ── Auto-rotate ──
    Section("  Auto-Rotate");
    ImGui::Checkbox("Enabled (D3D style)##ar", &g_set.autoRotate);
    ImGui::BeginDisabled(!g_set.autoRotate);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##arx", &g_set.autoSpeedX, -180.f, 180.f, "X speed  %.0f/s");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##ary", &g_set.autoSpeedY, -180.f, 180.f, "Y speed  %.0f/s");
    ImGui::EndDisabled();
    if (!g_set.autoRotate) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f,0.6f,0.6f,1.f));
        ImGui::TextUnformatted("LMB drag or arrow keys to rotate");
        ImGui::PopStyleColor();
    }

    // ── Display ──
    Section("  Display");
    // Fullscreen toggle
    bool fs = g_fullscreen;
    if (ImGui::Checkbox("Fullscreen  (Alt+Enter)", &fs))
        toggleFullscreen();
    ImGui::Spacing();
    ImGui::Checkbox("Wireframe", &g_set.wireframe);
    ImGui::SameLine(0, 20);
    ImGui::Checkbox("Lighting",  &g_set.lighting);
    ImGui::Spacing();
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##scale", &g_set.cubeScale, 0.1f, 3.0f, "Scale  %.2f");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##kspd",  &g_set.keySpeed, 10.f, 360.f, "Arrow speed  %.0f/s");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##msens", &g_set.mouseSens, 0.05f, 2.0f, "Mouse sens  %.2f");
    ImGui::Spacing();
    ImGui::ColorEdit3("Background", g_set.bgColor);

    // ── Texture ──
    Section("  Texture");
    ImGui::Checkbox("Use texture##tex", &g_set.useTexture);
    ImGui::SameLine(0, 12);
    if (ImGui::Button("Browse..."))
        browseTexture();
    if (g_texID) {
        // truncate long paths from left
        const char* disp = g_texPath;
        int len = (int)strlen(disp);
        if (len > 32) disp += (len - 32);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.9f,0.5f,1.f));
        ImGui::TextUnformatted(disp);
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f,0.6f,0.6f,1.f));
        ImGui::TextUnformatted("No texture loaded");
        ImGui::PopStyleColor();
    }
    // Bump map
    ImGui::Spacing();
    ImGui::BeginDisabled(!g_set.useTexture || !g_texID);
    ImGui::Checkbox("Use as bump map##bmp", &g_set.bumpMap);
    ImGui::BeginDisabled(!g_set.bumpMap);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##bmpstr", &g_set.bumpStrength, 0.01f, 5.f, "Bump strength  %.2f");
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.5f,1.f));
    ImGui::TextUnformatted("Uses luminance of texture as height field");
    ImGui::PopStyleColor();

    // ── Lighting ──
    ImGui::BeginDisabled(!g_set.lighting);
    Section("  Lighting");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat3("##ldir", g_set.lightDir, -5.f, 5.f, "Dir  %.1f");
    ImGui::ColorEdit3("Light color", g_set.lightColor);
    ImGui::Spacing();
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##spec",  &g_set.specStrength, 0.f, 1.f,   "Specular  %.2f");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##shin",  &g_set.shininess,    2.f, 256.f, "Shininess %.0f");
    ImGui::EndDisabled();

    // ── Camera ──
    Section("  Camera");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##zoom", &g_camDist, 0.5f, 20.f, "Distance  %.1f");
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.55f,0.55f,1.f));
    ImGui::TextUnformatted("Scroll wheel to zoom");
    ImGui::PopStyleColor();

    // ── Footer ──
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.5f,1.f));
    ImGui::TextUnformatted("Arrow keys rotate   ESC quit");
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleVar(4);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
    if (!glfwInit()) { std::cerr << "glfwInit failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "3D Cube", nullptr, nullptr);
    if (!window) { std::cerr << "Window creation failed\n"; glfwTerminate(); return -1; }
    g_window = window;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync
    glfwSetScrollCallback(window, scrollCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "GLEW init failed\n"; return -1; }

    glEnable(GL_DEPTH_TEST);

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 6.f;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Build GPU objects
    GLuint prog = buildProgram();

    // VAO for solid faces
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    // VAO for wireframe edges (shares VBO, separate EBO)
    GLuint VAO_wire, EBO_wire;
    glGenVertexArrays(1, &VAO_wire);
    glGenBuffers(1, &EBO_wire);

    glBindVertexArray(VAO_wire);
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // same vertex data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_wire);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(edgeIndices), edgeIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    // Cache uniform locations
    GLint uModel      = glGetUniformLocation(prog, "model");
    GLint uView       = glGetUniformLocation(prog, "view");
    GLint uProj       = glGetUniformLocation(prog, "projection");
    GLint uLighting    = glGetUniformLocation(prog, "uLighting");
    GLint uLightDir    = glGetUniformLocation(prog, "uLightDir");
    GLint uLightColor  = glGetUniformLocation(prog, "uLightColor");
    GLint uCamPos      = glGetUniformLocation(prog, "uCamPos");
    GLint uSpecStr     = glGetUniformLocation(prog, "uSpecStrength");
    GLint uShininess   = glGetUniformLocation(prog, "uShininess");
    GLint uUseTexture  = glGetUniformLocation(prog, "uUseTexture");
    GLint uBumpMap     = glGetUniformLocation(prog, "uBumpMap");
    GLint uBumpStr     = glGetUniformLocation(prog, "uBumpStrength");
    GLint uTexture     = glGetUniformLocation(prog, "uTexture");

    // Bind texture unit 0
    glUseProgram(prog);
    glUniform1i(uTexture, 0);

    // Auto-load texture from exe dir (or fallback checkerboard)
    autoLoadTexture();

    double prevTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        double now = glfwGetTime();
        float  dt  = static_cast<float>(now - prevTime);
        prevTime   = now;

        glfwPollEvents();
        processInput(window, dt);

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        // ImGui new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        drawUI(w, h);

        glClearColor(g_set.bgColor[0], g_set.bgColor[1], g_set.bgColor[2], 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(prog);

        // Model: scale, then X/Y rotation
        glm::mat4 model = glm::mat4(1.f);
        model = glm::scale(model, glm::vec3(g_set.cubeScale));
        model = glm::rotate(model, glm::radians(g_rotX), glm::vec3(1.f, 0.f, 0.f));
        model = glm::rotate(model, glm::radians(g_rotY), glm::vec3(0.f, 1.f, 0.f));

        // View: camera at g_camDist
        glm::vec3 camPos(0.f, 0.f, g_camDist);
        glm::mat4 view = glm::lookAt(
            camPos,
            glm::vec3(0.f, 0.f, 0.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        // Projection
        float aspect = (h > 0) ? (float)w / (float)h : 1.f;
        glm::mat4 proj = glm::perspective(glm::radians(45.f), aspect, 0.1f, 100.f);

        glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uView,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uProj,  1, GL_FALSE, glm::value_ptr(proj));

        // Lighting + texture uniforms
        bool litNow = g_set.lighting && !g_set.wireframe;
        glUniform1i(uLighting,  litNow ? 1 : 0);
        glUniform3f(uLightDir,   g_set.lightDir[0],   g_set.lightDir[1],   g_set.lightDir[2]);
        glUniform3f(uLightColor, g_set.lightColor[0], g_set.lightColor[1], g_set.lightColor[2]);
        glUniform3f(uCamPos,     camPos.x, camPos.y, camPos.z);
        glUniform1f(uSpecStr,    g_set.specStrength);
        glUniform1f(uShininess,  g_set.shininess);

        bool texNow = g_set.useTexture && g_texID && !g_set.wireframe;
        glUniform1i(uUseTexture, texNow ? 1 : 0);
        glUniform1i(uBumpMap,    (texNow && g_set.bumpMap) ? 1 : 0);
        glUniform1f(uBumpStr,    g_set.bumpStrength);
        if (texNow) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, g_texID);
        }

        if (g_set.wireframe) {
            glBindVertexArray(VAO_wire);
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
        } else {
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
        }

        // ImGui render (always filled)
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (g_texID) glDeleteTextures(1, &g_texID);
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &VAO_wire);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &EBO_wire);
    glDeleteProgram(prog);
    glfwTerminate();
    return 0;
}
