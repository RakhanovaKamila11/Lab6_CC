#include <windows.h>
#include <stdio.h>

#ifndef SM_CPRINT
#define SM_CPRINT 1
#endif

#define ID_SEND_DATA 32771
static HWND hwndEdit;
static const char* MappingName = "myFileMapping";
static const char* EventName = "myFileMappingEvent";
char szBuf[512];

void CollectClient1Data(HWND hWnd) {
    HDC hdc = GetDC(hWnd);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int bitsPerPixel = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(hWnd, hdc);

    char printerName[256] = { 0 };
    DWORD size = sizeof(printerName);
    int hasPrinter = GetDefaultPrinterA(printerName, &size) ? 1 : 0;

    sprintf_s(szBuf,
        "Data from Client 1:\r\n"
        " - Resolution: %d x %d px\r\n"
        " - Printer availability: %d (%s)\r\n"
        " - Color depth: %d біт",
        screenWidth, screenHeight, hasPrinter,
        (hasPrinter ? printerName : "not found"),
        bitsPerPixel);

    SetWindowTextA(hwndEdit, szBuf);
}

void SendDataToMapping(HWND hWnd) {
    // Зчитуємо текст із поля
    GetWindowTextA(hwndEdit, szBuf, sizeof(szBuf));

    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MappingName);
    if (hMapFile == NULL || hMapFile == INVALID_HANDLE_VALUE) {
        MessageBoxA(hWnd, "Unable to open the mapped object", "Map", MB_OK);
        return;
    }

    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 512);
    if (pBuf == NULL) {
        MessageBoxA(hWnd, "Cannot display the projected file", "Map", MB_OK);
        CloseHandle(hMapFile);
        return;
    }

    // Запис у спільну пам'ять
    CopyMemory((PVOID)pBuf, szBuf, sizeof(szBuf));
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    // Сигнал серверу про оновлення даних
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

        CollectClient1Data(hWnd);
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
    wcex.lpszClassName = "Client1MappingClass";
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExA(&wcex);

    HWND hWnd = CreateWindowA("Client1MappingClass", "Client 1",
        WS_OVERLAPPEDWINDOW, 560, 100, 400, 340, NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}