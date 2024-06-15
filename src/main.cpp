#define _CRT_SECURE_NO_WARNINGS
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS 1

#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "draw.h"


#include <algorithm>
#include <stdio.h>
#include <thread>
#include <chrono>

#if defined(_WIN32)
#include <crtdbg.h>
#endif
GLFWwindow* g_mainWindow = nullptr;
static int32 s_testSelection = 0;
//static Test* s_test = nullptr;
//static Settings s_settings;
static bool s_rightMouseDown = false;
static b2Vec2 s_clickPointWS = b2Vec2_zero;
static float s_displayScale = 1.0f;

void glfwErrorCallback(int error, const char* description)
{
    fprintf(stderr, "GLFW error occured. Code: %d. Description: %s\n", error, description);
}

static void CreateUI(GLFWwindow* window, const char* glslVersion = NULL)
{
    //IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    bool success;
    success = ImGui_ImplGlfw_InitForOpenGL(window, false);
    if (success == false)
    {
        printf("ImGui_ImplGlfw_InitForOpenGL failed\n");
        assert(false);
    }

    success = ImGui_ImplOpenGL3_Init(glslVersion);
    if (success == false)
    {
        printf("ImGui_ImplOpenGL3_Init failed\n");
        assert(false);
    }

    // Search for font file
    const char* fontPath1 = "data/droid_sans.ttf";
    const char* fontPath2 = "../data/droid_sans.ttf";
    const char* fontPath = nullptr;
    FILE* file1 = fopen(fontPath1, "rb");
    FILE* file2 = fopen(fontPath2, "rb");
    if (file1)
    {
        fontPath = fontPath1;
        fclose(file1);
    }

    if (file2)
    {
        fontPath = fontPath2;
        fclose(file2);
    }

    if (fontPath)
    {
        ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath, 14.0f * s_displayScale);
    }
}

static void ResizeWindowCallback(GLFWwindow*, int width, int height)
{
    g_camera.m_width = width;
    g_camera.m_height = height;

}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard)
    {
        return;
    }
}

static void CharCallback(GLFWwindow* window, unsigned int c)
{
    ImGui_ImplGlfw_CharCallback(window, c);
}

static void MouseButtonCallback(GLFWwindow* window, int32 button, int32 action, int32 mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

}

static void MouseMotionCallback(GLFWwindow*, double xd, double yd)
{
    b2Vec2 ps((float)xd, (float)yd);

    b2Vec2 pw = g_camera.ConvertScreenToWorld(ps);


    if (s_rightMouseDown)
    {
        b2Vec2 diff = pw - s_clickPointWS;
        g_camera.m_center.x -= diff.x;
        g_camera.m_center.y -= diff.y;
        s_clickPointWS = g_camera.ConvertScreenToWorld(ps);
    }
}

static void ScrollCallback(GLFWwindow* window, double dx, double dy)
{
    ImGui_ImplGlfw_ScrollCallback(window, dx, dy);
    if (ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    if (dy > 0)
    {
        g_camera.m_zoom /= 1.1f;
    }
    else
    {
        g_camera.m_zoom *= 1.1f;
    }
}
int main()
{
#if defined(_WIN32)
    // Enable memory-leak reports
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
#endif

    char buffer[128];


    glfwSetErrorCallback(glfwErrorCallback);


    if (glfwInit() == 0)
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

#if __APPLE__
    const char* glslVersion = "#version 150";
#else
    const char* glslVersion = NULL;
#endif

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    sprintf(buffer, "Magnet Simulation");

    bool fullscreen = false;
    if (fullscreen)
    {
        g_mainWindow = glfwCreateWindow(1920, 1080, buffer, glfwGetPrimaryMonitor(), NULL);
    }
    else
    {
        g_mainWindow = glfwCreateWindow(g_camera.m_width, g_camera.m_height, buffer, NULL, NULL);
    }

    if (g_mainWindow == NULL)
    {
        fprintf(stderr, "Failed to open GLFW g_mainWindow.\n");
        glfwTerminate();
        return -1;
    }

    glfwGetWindowContentScale(g_mainWindow, &s_displayScale, &s_displayScale);

    glfwMakeContextCurrent(g_mainWindow);

    // Load OpenGL functions using glad
    int version = gladLoadGL();
    //printf("GL %d.%d\n", Glad(version), GLAD_VERSION_MINOR(version));
    printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

    glfwSetWindowSizeCallback(g_mainWindow, ResizeWindowCallback);
    glfwSetKeyCallback(g_mainWindow, KeyCallback);
    glfwSetCharCallback(g_mainWindow, CharCallback);
    glfwSetMouseButtonCallback(g_mainWindow, MouseButtonCallback);
    glfwSetCursorPosCallback(g_mainWindow, MouseMotionCallback);
    glfwSetScrollCallback(g_mainWindow, ScrollCallback);

    g_debugDraw.Create();

    CreateUI(g_mainWindow, glslVersion);


    // Control the frame rate. One draw per monitor refresh.
    //glfwSwapInterval(1);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    std::chrono::duration<double> frameTime(0.0);
    std::chrono::duration<double> sleepAdjust(0.0);

    while (!glfwWindowShouldClose(g_mainWindow))
    {
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

        glfwGetWindowSize(g_mainWindow, &g_camera.m_width, &g_camera.m_height);

        int bufferWidth, bufferHeight;
        glfwGetFramebufferSize(g_mainWindow, &bufferWidth, &bufferHeight);
        glViewport(0, 0, bufferWidth, bufferHeight);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();

        if (g_debugDraw.m_showUI)
        {
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(float(g_camera.m_width), float(g_camera.m_height)));
            ImGui::SetNextWindowBgAlpha(0.0f);
            ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
            ImGui::End();

        }


        // ImGui::ShowDemoWindow();

        if (g_debugDraw.m_showUI)
        {
            sprintf(buffer, "%.1f ms", 1000.0 * frameTime.count());
            g_debugDraw.DrawString(5, g_camera.m_height - 20, buffer);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(g_mainWindow);


        glfwPollEvents();

        // Throttle to cap at 60Hz. This adaptive using a sleep adjustment. This could be improved by
        // using mm_pause or equivalent for the last millisecond.
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
        std::chrono::duration<double> target(1.0 / 60.0);
        std::chrono::duration<double> timeUsed = t2 - t1;
        std::chrono::duration<double> sleepTime = target - timeUsed + sleepAdjust;
        if (sleepTime > std::chrono::duration<double>(0))
        {
            std::this_thread::sleep_for(sleepTime);
        }

        std::chrono::steady_clock::time_point t3 = std::chrono::steady_clock::now();
        frameTime = t3 - t1;

        // Compute the sleep adjustment using a low pass filter
        sleepAdjust = 0.9 * sleepAdjust + 0.1 * (target - frameTime);
    }


    g_debugDraw.Destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();


    return 0;
}
