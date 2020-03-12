// ImGui GLUT binding with OpenGL3 + shaders (modfied from glfw version)
// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "imgui.h"

IMGUI_API bool        ImGui_ImplGlut_Init();
IMGUI_API void        ImGui_ImplGlut_Shutdown();
IMGUI_API void        ImGui_ImplGlut_NewFrame();

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplGlut_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplGlut_CreateDeviceObjects();

// GLUT callbacks
IMGUI_API void        ImGui_ImplGlut_MouseButtonCallback(int button, int state);
IMGUI_API void        ImGui_ImplGlut_MouseMotionCallback(int x, int y);
IMGUI_API void        ImGui_ImplGlut_PassiveMouseMotionCallback(int x, int y);
IMGUI_API void        ImGui_ImplGlut_ScrollCallback(float scroll);

IMGUI_API void        ImGui_ImplGlut_SpecialCallback(int key);
IMGUI_API void        ImGui_ImplGlut_SpecialUpCallback(int key);

IMGUI_API void        ImGui_ImplGlut_KeyCallback(unsigned int c);
IMGUI_API void        ImGui_ImplGlut_KeyUpCallback(unsigned int c);
