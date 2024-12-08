#include "framework.h"
#include "DiskActivityApp.h"
#include "DiskActivityMonitor.h"
#include "AppWindow.h"
#include <pdh.h>
#pragma comment(lib, "pdh.lib")

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
DiskActivityMonitor monitor;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DISKACTIVITYAPP, szWindowClass, MAX_LOADSTRING);

    if (!MyRegisterClass(hInstance, szWindowClass))
    {
        return FALSE;
    }

    if (!InitInstance(hInstance, nCmdShow, szWindowClass))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DISKACTIVITYAPP));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}