#pragma once
#include <windows.h>

// Global externs for the main variables controlling the indicator state
extern bool isActivityOn;
extern bool isBeeping;
extern int beepTimer;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Register and create the window
ATOM MyRegisterClass(HINSTANCE hInstance, LPCWSTR className);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, LPCWSTR className);
