#include <windows.h>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <algorithm>

HWND hList1, hList2, hButton;
std::vector<int> numbers;
std::mutex mtx;
std::condition_variable cv1;
std::condition_variable cv2;

bool generatingNumbers = false;

// Function to generate numbers and add to list box
void GenerateNumbers() {
    for (int i = 1; i <= 100; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        {
            std::unique_lock<std::mutex> lock(mtx);
            numbers.push_back(i);
            std::wstring num = std::to_wstring(i);
            SendMessage(hList1, LB_ADDSTRING, 0, (LPARAM)num.c_str());
            int count = SendMessage(hList1, LB_GETCOUNT, 0, 0);
            SendMessage(hList1, LB_SETTOPINDEX, count - 1, 0);
        }

        if (i % 5 == 0) {
            cv1.notify_one();
        }
    }

    generatingNumbers = false;
    EnableWindow(hButton, TRUE);
}

// Function to print multiples of 5

void PrintMultiples() {
    for (int i = 5; i <= 100; i += 5) {
        std::unique_lock<std::mutex> lock(mtx);
        cv1.wait(lock, [&]() { return std::find(numbers.begin(), numbers.end(), i) != numbers.end(); });
        std::wstring num = std::to_wstring(i);

        SendMessage(hList2, LB_ADDSTRING, 0, (LPARAM)num.c_str());
        int count = SendMessage(hList2, LB_GETCOUNT, 0, 0);
        SendMessage(hList2, LB_SETTOPINDEX, count - 1, 0);

        SendMessage(hList1, LB_ADDSTRING, 0, (LPARAM)num.c_str());
        count = SendMessage(hList1, LB_GETCOUNT, 0, 0);
        SendMessage(hList1, LB_SETTOPINDEX, count - 1, 0);

        if (i % 5 == 0) {
            cv2.notify_one();
        }
    }
}


// Window procedure to handle messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        // Create the first list box
        hList1 = CreateWindowW(L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
            10, 10, 150, 600,
            hwnd, NULL, NULL, NULL);

        // Create the second list box
        hList2 = CreateWindowW(L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
            170, 10, 150, 600,
            hwnd, NULL, NULL, NULL);

        // Create the start button
        hButton = CreateWindowW(L"BUTTON", L"Start",
            WS_CHILD | WS_VISIBLE,
            330, 10, 80, 30,
            hwnd, (HMENU)1, NULL, NULL);
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) {
            if (!generatingNumbers) {
                generatingNumbers = true;
                EnableWindow(hButton, FALSE);
                numbers.clear();
                SendMessage(hList1, LB_RESETCONTENT, 0, 0);
                SendMessage(hList2, LB_RESETCONTENT, 0, 0);
                std::thread generateThread(GenerateNumbers);
                std::thread printThread(PrintMultiples);
                generateThread.detach();
                printThread.detach();
            }
        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"Sample Window Class";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"List Box Example",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 1000,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    if (hwnd == NULL) {
        return 0;
    }
    ShowWindow(hwnd, nCmdShow);
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}