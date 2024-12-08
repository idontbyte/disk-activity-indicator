#include "AppWindow.h"
#include "DiskActivityMonitor.h"
#include "SoundManager.h"
#include "framework.h"
#include "DiskActivityApp.h"
#include <windows.h>

#define MAX_LOADSTRING 100

extern HINSTANCE hInst;
extern WCHAR szTitle[MAX_LOADSTRING];
extern WCHAR szWindowClass[MAX_LOADSTRING];

extern DiskActivityMonitor monitor;
bool isActivityOn = false;
bool isBeeping = false;
int beepTimer = 0;

ATOM MyRegisterClass(HINSTANCE hInstance, LPCWSTR className)
{
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = className;

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, LPCWSTR className)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(className, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 200, 400, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
        return FALSE;

    if (!monitor.Initialize())
    {
        MessageBox(hWnd, L"Failed to initialize DiskActivityMonitor", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        SetTimer(hWnd, 1, 100, NULL); // Check activity every 100ms
        break;

    case WM_TIMER:
    {
        monitor.Update();
        bool newActivityState = monitor.IsActivityAboveThreshold();

        isActivityOn = newActivityState;

        if (newActivityState)
        {
            OutputDebugString(L"Activity is on.\n");
            if (!isBeeping)
            {
                OutputDebugString(L"Starting beeps.\n");
                PlayBeepSoundAsync();
                isBeeping = true;
                beepTimer = 10;
            }
            else if (beepTimer <= 0)
            {
                OutputDebugString(L"Playing another beep.\n");
                PlayBeepSoundAsync();
                beepTimer = 10;
            }
            else
            {
                wchar_t buf[100];
                swprintf_s(buf, L"Countdown: %d\n", beepTimer);
                OutputDebugString(buf);
                beepTimer--;
            }
        }
        else
        {
            OutputDebugString(L"No activity.\n");
            isBeeping = false;
        }


        InvalidateRect(hWnd, NULL, TRUE);
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rect;
        GetClientRect(hWnd, &rect);

        int diameter = 50;
        int radius = diameter / 2;
        int x = (rect.right / 2) - radius;
        int y = (rect.bottom / 3) - radius;

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        HBRUSH hBrush = isActivityOn ? CreateSolidBrush(RGB(0, 255, 0))
            : CreateSolidBrush(RGB(255, 255, 255));
        SelectObject(hdc, hPen);
        SelectObject(hdc, hBrush);
        Ellipse(hdc, x, y, x + diameter, y + diameter);

        DeleteObject(hPen);
        DeleteObject(hBrush);

        EndPaint(hWnd, &ps);
    }
    break;

    case MM_WOM_DONE:
        // WaveOut done message if using CALLBACK_WINDOW instead of CALLBACK_FUNCTION
        break;

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
