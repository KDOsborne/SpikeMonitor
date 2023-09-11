#include <video.h>
#include <text.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

video_struct* video_;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Video Utilities
LONG WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
    switch(uMsg) {
		case WM_PAINT:
			PAINTSTRUCT ps;
			BeginPaint(video_->hWnd, &ps);
			
			EndPaint(video_->hWnd, &ps);
		return 0;
		
		case WM_KEYDOWN:
		switch (wParam) {

		}
		return 0;
		
		case WM_CLOSE:
			PostQuitMessage(0);
		return 0;
		
		case WM_EXITSIZEMOVE:
			GetClientRect(hWnd,&video_->lPrect);
			glViewport(video_->lPrect.left,video_->lPrect.top,video_->lPrect.right,video_->lPrect.bottom);
			video_->w = video_->lPrect.right-video_->lPrect.left;
			video_->h = video_->lPrect.bottom-video_->lPrect.top;
			
			update_textvp();
		return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam); 
}

static int create_window(int exFlags, int wsFlags, int pfdflags)
{
	WNDCLASS	wc;

    /* only register the window class once - use hInstance as a flag. */
	if(!video_->hInstance)
	{
		video_->hInstance = GetModuleHandle(NULL);
		memset(&wc, 0, sizeof(WNDCLASS));
		wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc   = (WNDPROC)WndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = video_->hInstance;
		wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = "OpenGL";

		if (!RegisterClass(&wc)) {
			MessageBox(NULL, "RegisterClass() failed:  "
				   "Cannot register window class.", "Error", MB_OK);
			return -1;
		}
	}
	
	HWND hWnd;
	memset(&hWnd, 0, sizeof(HWND));
    hWnd = CreateWindowEx(WS_EX_ACCEPTFILES|exFlags, "OpenGL", "Rhythm Suite", WS_CLIPSIBLINGS|WS_CLIPCHILDREN|wsFlags,
			video_->x, video_->y, video_->w, video_->h, NULL, NULL, video_->hInstance, NULL);
	
    if (hWnd == NULL) {
	MessageBox(NULL, "CreateWindow() failed:  Cannot create a window.",
		   "Error", MB_OK);
	return -1;
    }
	
	HDC hDC;
	memset(&hDC, 0, sizeof(HDC));
    hDC = GetDC(hWnd);

    /* there is no guarantee that the contents of the stack that become
       the pfd are zeroed, therefore _make sure_ to clear these bits. */
    PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | pfdflags,  //Flags
		PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
		32,                   // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                   // Number of bits for the depthbuffer
		8,                    // Number of bits for the stencilbuffer
		0,                    // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

    int pf = ChoosePixelFormat(hDC, &pfd);
    if (pf == 0) {
	MessageBox(NULL, "ChoosePixelFormat() failed:  "
		   "Cannot find a suitable pixel format.", "Error", MB_OK); 
	return -1;
    } 
 
    if (SetPixelFormat(hDC, pf, &pfd) == FALSE) {
	MessageBox(NULL, "SetPixelFormat() failed:  "
		   "Cannot set format specified.", "Error", MB_OK);
	return -1;
    } 
	
	GetClientRect(hWnd,&video_->lPrect);

	HGLRC hRC;
	
    hDC = GetDC(hWnd);
    
	hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);
	
	video_->hWnd = hWnd;
	video_->hDC = hDC;
	video_->hRC = hRC;
	
	ShowWindow(video_->hWnd,SW_HIDE);
	
	return 0;
}
void destroy_window()
{
	wglMakeCurrent(NULL, NULL);
    ReleaseDC(video_->hWnd, video_->hDC);
    wglDeleteContext(video_->hRC);
    DestroyWindow(video_->hWnd);
}

int init_video(int w, int h, int exFlags, int wsFlags, int pfdFlags)
{
	video_ = (video_struct*)malloc(sizeof(video_struct));
	memset(video_, 0, sizeof(video_struct));
	video_->hInstance = 0;
	video_->x =	0;
	video_->y =	0;
	video_->w = w;
	video_->h = h;
	
	if(create_window(exFlags,wsFlags,pfdFlags)) {
		free(video_);
		return 0;
	}
	
	if (!gladLoadGL()) {
		fprintf(stderr,"ERROR::Failed to initialize GLAD");

		destroy_video();
        return 0;
    }
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	
	glViewport(video_->lPrect.left,video_->lPrect.top,video_->lPrect.right,video_->lPrect.bottom);
	video_->w = video_->lPrect.right-video_->lPrect.left;
	video_->h = video_->lPrect.bottom-video_->lPrect.top;
	
	return 1;
}

int create_program(const char *vert, const char *frag)
{
	int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	
    glShaderSource(vertexShader, 1, &vert, NULL);
    glCompileShader(vertexShader);
	
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		fprintf(stderr,"ERROR::SHADER::COMPILATION_FAILED: %s\n",infoLog);
		return 0;
    }

    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &frag, NULL);
    glCompileShader(fragmentShader);
	
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        fprintf(stderr,"ERROR::SHADER::COMPILATION_FAILED: %s\n",infoLog);
		glDeleteShader(vertexShader);
		return 0;
    }
	
    // link shaders
    int vertexProgram = glCreateProgram();
	
    glAttachShader(vertexProgram, vertexShader);
    glAttachShader(vertexProgram, fragmentShader);
    glLinkProgram(vertexProgram);
	
    // check for linking errors
    glGetProgramiv(vertexProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(vertexProgram, 512, NULL, infoLog);
		fprintf(stderr,"ERROR::SHADER::PROGRAM::LINKING_FAILED: %s\n",infoLog);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteProgram(vertexProgram);
		return 0;
    }
	
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
	
	return vertexProgram;
}

void destroy_video()
{
	if(video_) {
		destroy_window();
		free(video_);
	}
}