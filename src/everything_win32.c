#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <assert.h>

#include "config.h"
#include "env.h"
#include "hotreload.h"

Env env = {0};
AppModule module = {0};
bool appInitialised = false;
double lastFrameTime = 0.0;

HWND hWnd;
HDC hdcMem;
HBITMAP hBitmap;

#define TIMER_ID 1
#define FPS 60
#define FRAME_TIME (1000 / FPS)

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void AllocateBuffer(int width, int height);
void FreeBuffer(void);
void UpdateBuffer(void);
double GetUnixTime(void);

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
	load_module(&module, "everything.dll");
	module.app_load();

	WNDCLASS wc = {0};

	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = WINDOW_NAME;

	RegisterClass(&wc);

	hWnd = CreateWindow(
		wc.lpszClassName,
		WINDOW_NAME,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0,
		0,
		INIT_WIDTH, INIT_WIDTH,
		0,
		0,
		hInstance,
		0);

	if (hWnd == NULL)
	{
		MessageBox(0, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
		ExitProcess(1);
	}

	ShowWindow(hWnd, nShowCmd);
	UpdateWindow(hWnd);

    SetTimer(hWnd, TIMER_ID, FRAME_TIME, NULL);

	MSG msg = {0};
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT CALLBACK WndProc(
    HWND hWnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
		AllocateBuffer(INIT_WIDTH, INIT_HEIGHT);
		lastFrameTime = GetUnixTime();
	}
	break;

	case WM_MOUSEMOVE:
	{
		env.mouse_x = GET_X_LPARAM(lParam);
		env.mouse_y = GET_Y_LPARAM(lParam);
	}
	break;

	case WM_LBUTTONDOWN:
	{
		env.mouse_left_down = true;
	}
	break;

	case WM_LBUTTONUP:
	{
		env.mouse_left_down = false;
	}
	break;

	case WM_RBUTTONDOWN:
	{
		env.mouse_right_down = true;
	}

	case WM_RBUTTONUP:
	{
		env.mouse_right_down = false;
	}
	break;

	case WM_SIZE:
	{
		RECT rect;
		GetClientRect(hWnd, &rect);

		int new_width = rect.right - rect.left;
		int new_height = rect.bottom - rect.top;

		if (new_width != env.width || new_height != env.height)
		{
			FreeBuffer();
			AllocateBuffer(new_width, new_height);
			InvalidateRect(hWnd, NULL, TRUE);
		}
	}
	break;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		if (env.buffer != NULL)
		{
            BitBlt(hdc, 0, 0, env.width, env.height, hdcMem, 0, 0, SRCCOPY);
		}

		EndPaint(hWnd, &ps);
	}
	break;

	case WM_TIMER:
	if (wParam == TIMER_ID) {
		UpdateBuffer();
		InvalidateRect(hWnd, NULL, TRUE); 
	}
	break;


	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}


void UpdateBuffer(void)
{
	double startTime = GetUnixTime();
	double dt = startTime - lastFrameTime;
	env.delta_time = dt;

	if (module.app_init && !appInitialised)
	{
		module.app_init(&env);
		appInitialised = true;
	}

	if (module.app_update)
	{
		module.app_update(&env);
	}

	lastFrameTime = startTime;
}

void AllocateBuffer(int width, int height)
{
	if (width <= 0 || height <= 0)
	{
		return;
	}
	
	BITMAPINFO bmi;
    HDC hdc = GetDC(hWnd);

    hdcMem = CreateCompatibleDC(hdc);
    ReleaseDC(hWnd, hdc);

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Negative to indicate top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (void**)&env.buffer, NULL, 0);
    SelectObject(hdcMem, hBitmap);

	env.width = width;
	env.height = height;
}

void FreeBuffer(void)
{
	if (hBitmap != NULL) 
	{
        DeleteObject(hBitmap);
        hBitmap = NULL;
    }

    if (hdcMem != NULL) 
	{
        DeleteDC(hdcMem);
        hdcMem = NULL;
    }

	// env.buffer is freed when the DIB section is deleted
}

double GetUnixTime(void)
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	ULARGE_INTEGER ull;
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;

	return (double)ull.QuadPart / 10000000.0;
}