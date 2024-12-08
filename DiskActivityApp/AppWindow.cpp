#include "AppWindow.h"
#include "DiskActivityMonitor.h"
#include "SoundManager.h"
#include "framework.h"
#include "DiskActivityApp.h"
#include <windows.h>
#include <shellapi.h>

#define MAX_LOADSTRING 100
#define WM_TRAYICON (WM_USER + 1) // Custom message for tray icon interactions

extern HINSTANCE hInst;
extern WCHAR szTitle[MAX_LOADSTRING];
extern WCHAR szWindowClass[MAX_LOADSTRING];

extern DiskActivityMonitor monitor;
bool isActivityOn = false;
bool isBeeping = false;
int beepTimer = 0;

NOTIFYICONDATA nid = {}; // Structure to manage the tray icon
HICON hIconActive = nullptr; // Icon for active state
HICON hIconInactive = nullptr; // Icon for inactive state

// Function prototypes
void AddTrayIcon(HWND hWnd);
void RemoveTrayIcon();
void UpdateTrayIcon(bool isActive);
void DrawCircleIcon(bool isActive, HICON& hIcon);

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

    AddTrayIcon(hWnd); // Add the tray icon
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

void AddTrayIcon(HWND hWnd)
{
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    wcscpy_s(nid.szTip, L"Disk Activity Monitor");
    DrawCircleIcon(false, hIconInactive); // Create inactive icon
    nid.hIcon = hIconInactive;
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTrayIcon()
{
    Shell_NotifyIcon(NIM_DELETE, &nid);
    if (hIconActive) DestroyIcon(hIconActive);
    if (hIconInactive) DestroyIcon(hIconInactive);
}

void UpdateTrayIcon(bool isActive)
{
    if (isActive && !hIconActive)
    {
        DrawCircleIcon(true, hIconActive);
    }
    else if (!isActive && !hIconInactive)
    {
        DrawCircleIcon(false, hIconInactive);
    }
    nid.hIcon = isActive ? hIconActive : hIconInactive;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void DrawCircleIcon(bool isActive, HICON& hIcon)
{
    const int size = 16; // Icon size
    HDC hdcMem = nullptr, hdcScreen = GetDC(NULL);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, size, size);
    HDC hdc = CreateCompatibleDC(hdcScreen);

    SelectObject(hdc, hBitmap);
    HBRUSH hBrush = CreateSolidBrush(isActive ? RGB(0, 255, 0) : RGB(255, 255, 255));
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));

    SelectObject(hdc, hBrush);
    SelectObject(hdc, hPen);
    Ellipse(hdc, 0, 0, size, size);

    ICONINFO iconInfo = {};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = hBitmap;
    iconInfo.hbmMask = hBitmap;

    hIcon = CreateIconIndirect(&iconInfo);

    DeleteObject(hBrush);
    DeleteObject(hPen);
    DeleteObject(hBitmap);
    DeleteDC(hdc);
    ReleaseDC(NULL, hdcScreen);
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

        UpdateTrayIcon(isActivityOn); // Update tray icon

        if (newActivityState)
        {
            if (!isBeeping)
            {
                PlayBeepSoundAsync();
                isBeeping = true;
                beepTimer = 10;
            }
            else if (beepTimer <= 0)
            {
                PlayBeepSoundAsync();
                beepTimer = 10;
            }
            else
            {
                beepTimer--;
            }
        }
        else
        {
            isBeeping = false;
        }
        InvalidateRect(hWnd, NULL, TRUE);
    }
    break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_EXIT:
            RemoveTrayIcon();
            PostQuitMessage(0);
            break;
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

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP)
        {
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Exit");
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
        break;

    case WM_CLOSE:
        // Minimize to system tray instead of closing
        ShowWindow(hWnd, SW_HIDE); // Hide the window
        return 0;

    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
