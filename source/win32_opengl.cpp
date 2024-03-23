#include "opengl.cpp"


#define WGL_DRAW_TO_WINDOW_ARB			0x2000
#define WGL_SUPPORT_OPENGL_ARB			0x2010
#define WGL_DOUBLE_BUFFER_ARB			0x2011
#define WGL_PIXEL_TYPE_ARB			0x2013
#define WGL_COLOR_BITS_ARB			0x2014
#define WGL_DEPTH_BITS_ARB			0x2022
#define WGL_TYPE_RGBA_ARB			0x202B

#define WGL_CONTEXT_MAJOR_VERSION_ARB		0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB		0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB		0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB	0x00000001
#define WGL_CONTEXT_FLAGS_ARB			0x2094
#define WGL_CONTEXT_DEBUG_BIT_ARB		0x0001

typedef void (__stdcall *GLDEBUGPROCARB)(GLenum, GLenum, GLuint, GLenum, GLsizei, char const*, void const*);
#define GL_DEBUG_OUTPUT				0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS		0x8242
#define GL_DONT_CARE				0x1100


INTERNAL HMODULE OpenGLDLL;
INTERNAL HGLRC   OpenGLContext;


INTERNAL VoidFunc *load_gl_function(char const *name) {
    VoidFunc *ptr = (VoidFunc*)wglGetProcAddress(name);
    if (ptr == 0 || ptr == (void*)1 || ptr == (void*)2 || ptr == (void*)3 || ptr == (void*)-1) {
        ptr = (VoidFunc*)GetProcAddress(OpenGLDLL, name);
    }
    return ptr;
}

INTERNAL void __stdcall debug_output(GLenum source, GLenum type, u32 id, GLenum severity, GLsizei length, char const *msg, void const *user)
{
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    print("------------------------------\n");
    print("Debug message (%u): %s\n", id, msg);
}

void show_message_box_and_crash(String str);

INTERNAL void setup_gl_context(HWND window) {
    OpenGLDLL = LoadLibraryW(L"opengl32.dll");
    if (!OpenGLDLL) show_message_box_and_crash("ERROR_WIN32_LOAD_OPENGL_DLL");

    BOOL (__stdcall *wglChoosePixelFormatARB)(HDC, int const *, FLOAT const*, UINT, int*, UINT*);
    HGLRC (__stdcall *wglCreateContextAttribsARB)(HDC, HGLRC, int const*);

    HDC dc = GetDC(window);

    { // dummy context creation
        u32 format = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;

        PIXELFORMATDESCRIPTOR desc = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1, format,
            PFD_TYPE_RGBA,
            32,
            0, 0, 0, 0, 0, 0,
            0, 0, 0,
            0, 0, 0, 0,
            24,
            8,
            0,
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
        };

        int pixel_format = ChoosePixelFormat(dc, &desc);
        if (pixel_format == 0) show_message_box_and_crash("ERROR_WIN32_CHOOSE_PIXEL_FORMAT");

        if (!SetPixelFormat(dc, pixel_format, &desc)) show_message_box_and_crash("ERROR_WIN32_SET_PIXEL_FORMAT");

        HGLRC context = wglCreateContext(dc);
        if (!context) show_message_box_and_crash("ERROR_WIN32_WGL_CREATE_CONTEXT");

        if (!wglMakeCurrent(dc, context)) show_message_box_and_crash("ERROR_WIN32_WGL_MAKE_CURRENT");

        wglChoosePixelFormatARB = (decltype(wglChoosePixelFormatARB))load_gl_function("wglChoosePixelFormatARB");
        wglCreateContextAttribsARB = (decltype(wglCreateContextAttribsARB))load_gl_function("wglCreateContextAttribsARB");

        wglMakeCurrent(dc, 0);
        wglDeleteContext(context);
    }

    if (wglChoosePixelFormatARB == 0)    show_message_box_and_crash("ERROR_WIN32_WGL_CHOOSE_PIXEL_FORMAT_ARB_NOT_LOADED");
    if (wglCreateContextAttribsARB == 0) show_message_box_and_crash("ERROR_WIN32_WGL_CREATE_CONTEXT_ATTRIBS_ARB_NOT_LOADED");

    int pixel_format;
    UINT format_count;

    int pixel_attributes[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 32,
        0
    };
    if (!wglChoosePixelFormatARB(dc, pixel_attributes, 0, 1, &pixel_format, &format_count) || format_count == 0) show_message_box_and_crash("ERROR_WIN32_WGL_CHOOSE_PIXEL_FORMAT_ARB");

    int create_args[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifdef DEVELOPER
        WGL_CONTEXT_FLAGS_ARB,	       WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
        0
    };
    OpenGLContext = wglCreateContextAttribsARB(dc, 0, create_args);
    if (!OpenGLContext) show_message_box_and_crash("ERROR_WIN32_WGL_CREATE_CONTEXT_ATTRIBS_ARB");

    wglMakeCurrent(dc, OpenGLContext);

#ifdef DEVELOPER
    {
        void (__stdcall *glEnable)(GLenum);
        void (__stdcall *glDebugMessageCallbackARB)(GLDEBUGPROCARB, void const*);
        void (__stdcall *glDebugMessageControlARB)(GLenum, GLenum, GLenum, GLsizei, GLuint const*, GLboolean);

        glEnable = (decltype(glEnable))load_gl_function("glEnable");
        glDebugMessageCallbackARB = (decltype(glDebugMessageCallbackARB))load_gl_function("glDebugMessageCallbackARB");
        glDebugMessageControlARB = (decltype(glDebugMessageControlARB))load_gl_function("glDebugMessageControlARB");

        if (glEnable && glDebugMessageCallbackARB && glDebugMessageControlARB) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallbackARB(debug_output, 0);
            glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
        } else {
            print("NOTE: Could not init OpenGL debug functionality.\n");
        }
    }
#endif
}

