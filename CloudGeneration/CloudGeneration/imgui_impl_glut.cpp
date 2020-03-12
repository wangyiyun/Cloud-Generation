// ImGui GLUT binding with OpenGL3 + shaders: modified from glfw version
// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui


#include "imgui_impl_glut.h"

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>


#ifdef _WIN32
#undef APIENTRY

#endif

// Data
static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;
static GLuint       g_FontTexture = 0;
static int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
static unsigned int g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0;

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplGlut_RenderDrawLists(ImDrawData* draw_data)
{
    // Backup GL state
    GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    GLint last_blend_src; glGetIntegerv(GL_BLEND_SRC, &last_blend_src);
    GLint last_blend_dst; glGetIntegerv(GL_BLEND_DST, &last_blend_dst);
    GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
    GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Handle cases of screen coordinates != from framebuffer coordinates (e.g. retina displays)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    const float ortho_projection[4][4] =
    {
        { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
        { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
        {-1.0f,                  1.0f,                   0.0f, 1.0f },
    };
    glUseProgram(g_ShaderHandle);
    glUniform1i(g_AttribLocationTex, 0);
    glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
    glBindVertexArray(g_VaoHandle);

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
        {
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBindVertexArray(last_vertex_array);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFunc(last_blend_src, last_blend_dst);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}

static const char* ImGui_ImplGlut_GetClipboardText()
{
   //TODO how to do this without glfw?
   //return glfwGetClipboardString(g_Window);
   return 0;
}

static void ImGui_ImplGlut_SetClipboardText(const char* text)
{
   //TODO how to do this without glfw?
    //glfwSetClipboardString(g_Window, text);
}

void ImGui_ImplGlut_MouseButtonCallback(int button, int state)
{
   ImGuiIO& io = ImGui::GetIO();
   if (state == GLUT_DOWN && button >= 0 && button < 3)
   {
      g_MousePressed[button] = true;
      io.MouseDown[button] = true;
   }

   if (state == GLUT_UP && button >= 0 && button < 3)
   {
      g_MousePressed[button] = false;
      io.MouseDown[button] = false;
   }

   //freeglut is mapping mousewheel to buttons 3 and 4
   if(state == GLUT_DOWN && button == 3)
   {
      ImGui_ImplGlut_ScrollCallback(+1.0f);
   }

   if(state == GLUT_DOWN && button == 4)
   {
      ImGui_ImplGlut_ScrollCallback(-1.0f);
   }
}

void ImGui_ImplGlut_PassiveMouseMotionCallback(int x, int y)
{
   ImGuiIO& io = ImGui::GetIO();
   io.MousePos = ImVec2((float)x, (float)y); 
}

void ImGui_ImplGlut_MouseMotionCallback(int x, int y)
{
   ImGuiIO& io = ImGui::GetIO();
   io.MousePos = ImVec2((float)x, (float)y); 
}

//we aren't really using this as a callback in freeglut
void ImGui_ImplGlut_ScrollCallback(float scroll)
{
   g_MouseWheel += scroll;
}

void ImGui_ImplGlut_SpecialCallback(int key)
{
   ImGuiIO& io = ImGui::GetIO();
   io.KeysDown[key] = true;

   int mods = glutGetModifiers();
   io.KeyCtrl = (mods & GLUT_ACTIVE_CTRL) != 0;
   io.KeyShift = (mods & GLUT_ACTIVE_SHIFT) != 0;
   io.KeyAlt = (mods & GLUT_ACTIVE_ALT) != 0;
}

void ImGui_ImplGlut_SpecialUpCallback(int key)
{
   ImGuiIO& io = ImGui::GetIO();
   io.KeysDown[key] = false;

   int mods = glutGetModifiers();
   io.KeyCtrl = (mods & GLUT_ACTIVE_CTRL) != 0;
   io.KeyShift = (mods & GLUT_ACTIVE_SHIFT) != 0;
   io.KeyAlt = (mods & GLUT_ACTIVE_ALT) != 0;
}

void ImGui_ImplGlut_KeyCallback(unsigned int c)
{
   ImGuiIO& io = ImGui::GetIO();
   if (c > 0 && c < 0x10000)
   {
      io.AddInputCharacter((unsigned short)c);
   }

   io.KeysDown[c] = true;
   int mods = glutGetModifiers();
   io.KeyCtrl = (mods & GLUT_ACTIVE_CTRL) != 0;
   io.KeyShift = (mods & GLUT_ACTIVE_SHIFT) != 0;
   io.KeyAlt = (mods & GLUT_ACTIVE_ALT) != 0;
   
}

void ImGui_ImplGlut_KeyUpCallback(unsigned int c)
{
   ImGuiIO& io = ImGui::GetIO();
   io.KeysDown[c] = false;

   int mods = glutGetModifiers();
   io.KeyCtrl = (mods & GLUT_ACTIVE_CTRL) != 0;
   io.KeyShift = (mods & GLUT_ACTIVE_SHIFT) != 0;
   io.KeyAlt = (mods & GLUT_ACTIVE_ALT) != 0;
}

bool ImGui_ImplGlut_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);

    return true;
}

bool ImGui_ImplGlut_CreateDeviceObjects()
{
    // Backup GL state
    GLint last_texture, last_array_buffer, last_vertex_array;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    const GLchar *vertex_shader =
        "#version 330\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "	Frag_UV = UV;\n"
        "	Frag_Color = Color;\n"
        "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader =
        "#version 330\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
        "}\n";

    g_ShaderHandle = glCreateProgram();
    g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
    g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
    glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
    glCompileShader(g_VertHandle);
    glCompileShader(g_FragHandle);
    glAttachShader(g_ShaderHandle, g_VertHandle);
    glAttachShader(g_ShaderHandle, g_FragHandle);
    glLinkProgram(g_ShaderHandle);

    g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
    g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
    g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
    g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
    g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");

    glGenBuffers(1, &g_VboHandle);
    glGenBuffers(1, &g_ElementsHandle);

    glGenVertexArrays(1, &g_VaoHandle);
    glBindVertexArray(g_VaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    glEnableVertexAttribArray(g_AttribLocationPosition);
    glEnableVertexAttribArray(g_AttribLocationUV);
    glEnableVertexAttribArray(g_AttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

    ImGui_ImplGlut_CreateFontsTexture();

    // Restore modified GL state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindVertexArray(last_vertex_array);

    return true;
}

void    ImGui_ImplGlut_InvalidateDeviceObjects()
{
    if (g_VaoHandle) glDeleteVertexArrays(1, &g_VaoHandle);
    if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
    if (g_ElementsHandle) glDeleteBuffers(1, &g_ElementsHandle);
    g_VaoHandle = g_VboHandle = g_ElementsHandle = 0;

    glDetachShader(g_ShaderHandle, g_VertHandle);
    glDeleteShader(g_VertHandle);
    g_VertHandle = 0;

    glDetachShader(g_ShaderHandle, g_FragHandle);
    glDeleteShader(g_FragHandle);
    g_FragHandle = 0;

    glDeleteProgram(g_ShaderHandle);
    g_ShaderHandle = 0;

    if (g_FontTexture)
    {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }
}

bool    ImGui_ImplGlut_Init()
{
	ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = 9;                         // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = GLUT_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLUT_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLUT_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLUT_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLUT_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLUT_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLUT_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLUT_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLUT_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = 8; //TODO backspace is not working in text fields
    io.KeyMap[ImGuiKey_Enter] = 13;
    io.KeyMap[ImGuiKey_Escape] = 27;
    io.KeyMap[ImGuiKey_A] = 1;
    io.KeyMap[ImGuiKey_C] = 3;
    io.KeyMap[ImGuiKey_V] = 22;
    io.KeyMap[ImGuiKey_X] = 24;
    io.KeyMap[ImGuiKey_Y] = 25;
    io.KeyMap[ImGuiKey_Z] = 26;

    io.RenderDrawListsFn = ImGui_ImplGlut_RenderDrawLists;       // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    //io.SetClipboardTextFn = ImGui_ImplGlut_SetClipboardText;
    //io.GetClipboardTextFn = ImGui_ImplGlut_GetClipboardText;
#ifdef _WIN32
    //TODO can we get the win32 window handle from freeglut or maybe wgl?
    //io.ImeWindowHandle = glfwGetWin32Window(g_Window);
#endif

    return true;
}

void ImGui_ImplGlut_Shutdown()
{
    ImGui_ImplGlut_InvalidateDeviceObjects();
	ImGui::DestroyContext();
}

void ImGui_ImplGlut_NewFrame()
{
    if (!g_FontTexture)
    {
        ImGui_ImplGlut_CreateDeviceObjects();
    }

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;

    //TODO: what is the difference in freeglut?
    //glfwGetWindowSize(g_Window, &w, &h);
    //glfwGetFramebufferSize(g_Window, &display_w, &display_h);
    w = glutGet(GLUT_WINDOW_WIDTH);
    h = glutGet(GLUT_WINDOW_HEIGHT);
    display_w = glutGet(GLUT_WINDOW_WIDTH);
    display_h = glutGet(GLUT_WINDOW_HEIGHT);

    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);

    // Setup time step (TODO: replace glut timer with something better (but portable)?)
    static int last_time = 0;
    int time = glutGet(GLUT_ELAPSED_TIME);
    int elapsed = time - last_time;
    last_time = time;
    io.DeltaTime = 0.001f*elapsed;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())

   //TODO we can't poll in freeglut, so we can't do this. Double clicks at low framerate are broken because of this.
    /*
    if (glfwGetWindowAttrib(g_Window, GLFW_FOCUSED))
    {
        double mouse_x, mouse_y;
        glfwGetCursorPos(g_Window, &mouse_x, &mouse_y);
        io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
    }
    else
    {
        io.MousePos = ImVec2(-1,-1);
    }

    for (int i = 0; i < 3; i++)
    {
        io.MouseDown[i] = g_MousePressed[i] || glfwGetMouseButton(g_Window, i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        g_MousePressed[i] = false;
    }
*/

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Hide OS mouse cursor if ImGui is drawing it
    glutSetCursor(io.MouseDrawCursor ? GLUT_CURSOR_NONE : GLUT_CURSOR_INHERIT);

    // Start the frame
    ImGui::NewFrame();
}
