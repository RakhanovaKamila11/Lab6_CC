#include <windows.h>
#include <stdio.h>

#ifndef SM_CYBUTTON
#define SM_CYBUTTON SM_CYMENUSIZE
#endif

#define ID_SEND_DATA 32771
static HWND hwndEdit;
static const char* MappingName = "myFileMapping";
static const char* EventName = "myFileMappingEvent";
char szBuf[512];

void CollectClient2Data(HWND hWnd) {
    HDC hdc = GetDC(hWnd);
    int cyMenuButton = GetSystemMetrics(SM_CYBUTTON);
    int cxMinTrack = GetSystemMetrics(SM_CXMINTRACK);
    int logPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(hWnd, hdc);

    sprintf_s(szBuf,
        "Data from Client 2:\r\n"
        " - Menu buttons height: %d px\r\n"
        " - Tracking Width/Start: %d px\r\n"
        " - Vertical DPI: %d dpi",
        cyMenuButton, cxMinTrack, logPixelsY);

    SetWindowTextA(hwndEdit, szBuf);
}

void SendDataToMapping(HWND hWnd) {
    GetWindowTextA(hwndEdit, szBuf, sizeof(szBuf));

    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MappingName);
    if (hMapFile == NULL || hMapFile == INVALID_HANDLE_VALUE) {
        MessageBoxA(hWnd, "Unable to open the mapped object", "Map", MB_OK);
        return;
    }

    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 512);
    if (pBuf == NULL) {
        MessageBoxA(hWnd, "Viewing the projected file is impossible", "Map", MB_OK);
        CloseHandle(hMapFile);
        return;
    }

    CopyMemory((PVOID)pBuf, szBuf, sizeof(szBuf));
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    HANDLE hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, EventName);
    if (hEvent) {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        HMENU hMenu = CreateMenu();
        HMENU hSubMenu = CreatePopupMenu();
        AppendMenuA(hSubMenu, MF_STRING, ID_SEND_DATA, "Send");
        AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, "File");
        SetMenu(hWnd, hMenu);

        hwndEdit = CreateWindowA("EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            10, 10, 360, 260, hWnd, NULL, ((LPCREATESTRUCTA)lParam)->hInstance, NULL);

        CollectClient2Data(hWnd);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_SEND_DATA) {
            SendDataToMapping(hWnd);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcA(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEXA wcex = { sizeof(WNDCLASSEXA) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = "Client2MappingClass";
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExA(&wcex);

    HWND hWnd = CreateWindowA("Client2MappingClass", "Client 2",
        WS_OVERLAPPEDWINDOW, 560, 480, 400, 340, NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}