/* The MIT License
 *
 * Copyright (c) 2010 OTClient, https://github.com/edubart/otclient
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "platform.h"
#include "engine.h"

#include <windows.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct Win32PlatformPrivate {
    HWND window;
    HINSTANCE instance;
    HDC hdc;
    HGLRC hrc;

    int x, y;
    int width, height;
    int minWidth, minHeight;
    bool maximized;
} win32;

void Platform::init()
{
    win32.instance = GetModuleHandle(NULL);

    WNDCLASSA wc;
    wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
    wc.lpfnWndProc		= (WNDPROC)WndProc;			// WndProc Handles Messages
    wc.cbClsExtra		= 0;					// No Extra Window Data
    wc.cbWndExtra		= 0;					// No Extra Window Data
    wc.hInstance		= win32.instance;			// Set The Instance
    wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);		// Load The Default Icon
    wc.hCursor			= LoadCursor(NULL, IDC_ARROW);		// Load The Arrow Pointer
    wc.hbrBackground            = NULL;					// No Background Required For GL
    wc.lpszMenuName		= NULL;					// We Don't Want A Menu
    wc.lpszClassName            = "OTClient";		// Set The Class Name

    if(!RegisterClassA(&wc))
        fatal("Failed to register the window class.");
}

void Platform::terminate()
{
    if(win32.window) {
        destroyWindow();
        win32.window = NULL;
    }

    if(win32.instance) {
        if(!UnregisterClassA("OTClient", win32.instance))
            error("Unregister class failed.");

        win32.instance = NULL;
    }
}

void Platform::poll()
{
    MSG msg;
    if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

unsigned long Platform::getTicks()
{
    return GetTickCount();
}

void Platform::sleep(unsigned long miliseconds)
{
    Sleep(miliseconds);
}

bool Platform::createWindow(int x, int y, int width, int height, int minWidth, int minHeight, bool maximized)
{
    DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    win32.x = x;
    win32.y = y;
    win32.width = width;
    win32.height = height;
    win32.minWidth = minWidth;
    win32.minHeight = minHeight;
    win32.maximized = maximized;

    //AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

    win32.window = CreateWindowExA(dwExStyle,		// Extended Style For The Window
                                   "OTClient",		// Class Name
                                   "OTClient",          // Window Title
                                   dwStyle,		// Required Window Style
                                   x,                    // Window X Position
                                   y,                    // Window Y Position
                                   width,                // Calculate Window Width
                                   height,               // Calculate Window Height
                                   NULL,			// No Parent Window
                                   NULL,			// No Menu
                                   win32.instance,	// Instance
                                   NULL);

    if(!win32.window) {
        terminate();
        fatal("Window creation error.");
        return false;
    }

    GLuint pixelFormat;
    static PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
    {
        sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
        1,							// Version Number
        PFD_DRAW_TO_WINDOW |					// Format Must Support Window
        PFD_SUPPORT_OPENGL |					// Format Must Support OpenGL
        PFD_DOUBLEBUFFER,					// Must Support Double Buffering
        PFD_TYPE_RGBA,						// Request An RGBA Format
        32,							// Select Our Color Depth
        0, 0, 0, 0, 0, 0,					// Color Bits Ignored
        0,							// No Alpha Buffer
        0,							// Shift Bit Ignored
        0,							// No Accumulation Buffer
        0, 0, 0, 0,						// Accumulation Bits Ignored
        16,							// 16Bit Z-Buffer (Depth Buffer)
        0,							// No Stencil Buffer
        0,							// No Auxiliary Buffer
        PFD_MAIN_PLANE,						// Main Drawing Layer
        0,							// Reserved
        0, 0, 0							// Layer Masks Ignored
    };

    if(!(win32.hdc = GetDC(win32.window))) {
        terminate();
        fatal("Can't Create A GL Device Context.");
        return false;
    }

    if(!(pixelFormat = ChoosePixelFormat(win32.hdc, &pfd))) {
        terminate();
        fatal("Can't Find A Suitable PixelFormat.");
        return false;
    }

    if(!SetPixelFormat(win32.hdc, pixelFormat, &pfd)) {
        terminate();
        fatal("Can't Set The PixelFormat.");
        return false;
    }

    if(!(win32.hrc = wglCreateContext(win32.hdc))) {
        terminate();
        fatal("Can't Create A GL Rendering Context.");
        return false;
    }

    if(!wglMakeCurrent(win32.hdc, win32.hrc)) {
        terminate();
        fatal("Can't Activate The GL Rendering Context.");
        return false;
    }

    return true;
}

void Platform::destroyWindow()
{
    if(win32.hrc) {
        if(!wglMakeCurrent(NULL, NULL))
            error("Release Of DC And RC Failed.");

        if(!wglDeleteContext(win32.hrc))
            error("Release Rendering Context Failed.");

        win32.hrc = NULL;
    }

    if(win32.hdc) {
        if(!ReleaseDC(win32.window, win32.hdc))
            error("Release Device Context Failed.");

        win32.hdc = NULL;
    }

    if(win32.window) {
        if(!DestroyWindow(win32.window))
            error("Destroy window failed.");

        win32.window = NULL;
    }
}

void Platform::showWindow()
{
    if(win32.maximized)
        ShowWindow(win32.window, SW_MAXIMIZE);
    else
        ShowWindow(win32.window, SW_SHOW);
}

void Platform::setWindowTitle(const char *title)
{
    SetWindowTextA(win32.window, title);
}

void *Platform::getExtensionProcAddress(const char *ext)
{
    return (void*)wglGetProcAddress(ext);
}

bool Platform::isExtensionSupported(const char *ext)
{
    const char *exts = NULL;//glXQueryExtensionsString(x11.display, DefaultScreen(x11.display));
    if(strstr(exts, ext))
        return true;
    return true;
}

void Platform::showMouseCursor()
{
    ShowCursor(false);
    /*XUndefineCursor(x11.display, x11.window);
    if(x11.cursor != None) {
        XFreeCursor(x11.display, x11.cursor);
        x11.cursor = None;
    }*/
}

void Platform::setVsync(bool enable)
{
    typedef GLint (*glSwapIntervalProc)(GLint);
    glSwapIntervalProc glSwapInterval = NULL;

    if(isExtensionSupported("GLX_MESA_swap_control"))
        glSwapInterval = (glSwapIntervalProc)getExtensionProcAddress("glXSwapIntervalMESA");
    else if(isExtensionSupported("GLX_SGI_swap_control"))
        glSwapInterval = (glSwapIntervalProc)getExtensionProcAddress("glXSwapIntervalSGI");

    if(glSwapInterval)
        glSwapInterval(enable ? 1 : 0);
}

void Platform::swapBuffers()
{
    SwapBuffers(win32.hdc);
}

int Platform::getWindowWidth()
{
    return win32.width;
}

int Platform::getWindowHeight()
{
    return win32.height;
}

const char *Platform::getAppUserDir()
{
    /*std::stringstream sdir;
    sdir << PHYSFS_getUserDir() << "/." << APP_NAME << "/";
    if((mkdir(sdir.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) && (errno != EEXIST))
        error("Couldn't create directory for saving configuration file. (%s)", sdir.str().c_str());
    return sdir.str().c_str();*/
    return "lol";
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_CLOSE:
        {
            g_engine.onClose();
            break;
        }
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO *minMax = (MINMAXINFO*)lParam;
            minMax->ptMinTrackSize.x = win32.minWidth;
            minMax->ptMinTrackSize.y = win32.minHeight;
            break;
        }
    case WM_MOVING:
    case WM_SIZING:
        {
            RECT *rect = (RECT*)lParam;
            win32.x = rect->left;
            win32.y = rect->top;
            win32.width = rect->right - rect->left;
            win32.height = rect->bottom - rect->top;
            break;
        }
    case WM_SIZE:
        {
            switch(wParam)
            {
            case SIZE_MAXIMIZED:
                win32.maximized = true;
                break;
            case SIZE_RESTORED:
                win32.maximized = false;
                break;
            }

            g_engine.onResize(LOWORD(lParam), HIWORD(lParam));
            break;
        }
    default:
        {
            return DefWindowProc(hWnd,uMsg,wParam,lParam);
        }
    }
    return 0;
}