#include <windows.h>
#include <stdio.h>

#define ID_FILEMAPPING_START 32771
#define ID_FILEMAPPING_STOP  32772
#define ID_FILEMAPPING_READ  32773
#define IDT_CHECK_EVENT      1002

static HWND hwndEdit;
static HANDLE hMapFile = NULL;
static HANDLE hEvent = NULL;
static const char* MappingName = "myFileMapping";
static const char* EventName = "myFileMappingEvent";

char mess[4096] = "Laboratorna robota 6\r\n";
char szBuf[512];

void AppendText(HWND hEdit, const char* text) {
    strcat_s(mess, text);
    SetWindowTextA(hEdit, mess);
    SendMessageA(hEdit, EM_SETSEL, (WPARAM)strlen(mess), (LPARAM)strlen(mess));
    SendMessageA(hEdit, EM_SCROLLCARET, 0, 0);
}

void ReadFromMapping() {
    if (hMapFile == NULL) return;

    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (pBuf == NULL) {
        MessageBoxA(NULL, "Submission of the projected file is impossible", "Map", MB_OK);
        return;
    }

    AppendText(hwndEdit, (LPCSTR)pBuf);
    AppendText(hwndEdit, "\r\n------------------------------------\r\n");
    UnmapViewOfFile(pBuf);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        HMENU hMenu = CreateMenu();
        HMENU hSubMenu = CreatePopupMenu();
        AppendMenuA(hSubMenu, MF_STRING, ID_FILEMAPPING_START, "Start");
        AppendMenuA(hSubMenu, MF_STRING, ID_FILEMAPPING_READ, "Read");
        AppendMenuA(hSubMenu, MF_STRING, ID_FILEMAPPING_STOP, "Stop");
        AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, "FileMapping");
        SetMenu(hWnd, hMenu);

        hwndEdit = CreateWindowA("EDIT", mess,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 10, 500, 420, hWnd, NULL, ((LPCREATESTRUCTA)lParam)->hInstance, NULL);
        break;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case ID_FILEMAPPING_START:
            if (hMapFile != NULL) {
                AppendText(hwndEdit, "The server is alredy running!\r\n");
                break;
            }
            hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 512, MappingName);
            if (hMapFile == NULL || hMapFile == INVALID_HANDLE_VALUE) {
                MessageBoxA(hWnd, "Unable to create memory mapped object", "Map", MB_OK);
                hMapFile = NULL;
                break;
            }

            // Створення іменованої події для автоматичного виявлення змін (Завдання "з зірочкою")
            hEvent = CreateEventA(NULL, FALSE, FALSE, EventName);
            SetTimer(hWnd, IDT_CHECK_EVENT, 200, NULL);

            AppendText(hwndEdit, "\r\nServer FileMapping started\r\n");
            break;

        case ID_FILEMAPPING_READ:
            ReadFromMapping();
            break;

        case ID_FILEMAPPING_STOP:
            if (hMapFile != NULL) {
                KillTimer(hWnd, IDT_CHECK_EVENT);
                CloseHandle(hMapFile);
                hMapFile = NULL;
                if (hEvent) {
                    CloseHandle(hEvent);
                    hEvent = NULL;
                }
                AppendText(hwndEdit, "FileMapping server stopped\r\n");
            }
            break;
        }
        break;
    }

                   // Автоматичне виявлення сигналу від клієнтів ("з зірочкою")
    case WM_TIMER: {
        if (wParam == IDT_CHECK_EVENT && hEvent != NULL) {
            if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0) {
                ReadFromMapping();
            }
        }
        break;
    }

    case WM_DESTROY:
        if (hMapFile != NULL) CloseHandle(hMapFile);
        if (hEvent != NULL) CloseHandle(hEvent);
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
    wcex.lpszClassName = "ServerMappingClass";
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExA(&wcex);

    HWND hWnd = CreateWindowA("ServerMappingClass", "Server (FileMapping)",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 540, 500, NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}