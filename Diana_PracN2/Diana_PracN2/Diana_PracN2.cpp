#include <windows.h>
#include <commdlg.h>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>

using namespace std;

// Глобальные переменные
#define IDM_OPEN 1001

struct ImageData {
    std::vector<COLORREF>* pixels;
    int height, width;
    int minX, maxX, minY, maxY;
    COLORREF bgColor;
};

bool SelectBMPFile(HWND hWnd, wchar_t** filePath) {
    OPENFILENAME fileDialog;
    wchar_t selectedFile[MAX_PATH] = L"";

    ZeroMemory(&fileDialog, sizeof(fileDialog));
    fileDialog.lStructSize = sizeof(fileDialog);
    fileDialog.hwndOwner = hWnd;
    fileDialog.lpstrFilter = L"BMP Files\0*.bmp\0";
    fileDialog.lpstrFile = selectedFile;
    fileDialog.lpstrTitle = L"Выберите BMP файл";
    fileDialog.nMaxFile = sizeof(selectedFile);
    fileDialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&fileDialog)) {
        *filePath = _wcsdup(selectedFile);
        return true;
    }
    return false;
}

bool CompareColors(COLORREF color1, COLORREF color2, int tolerance) {
    int r1 = GetRValue(color1), g1 = GetGValue(color1), b1 = GetBValue(color1);
    int r2 = GetRValue(color2), g2 = GetGValue(color2), b2 = GetBValue(color2);
    return (abs(r1 - r2) <= tolerance && abs(g1 - g2) <= tolerance && abs(b1 - b2) <= tolerance);
}

ImageData* LoadBMP(const wchar_t* filePath) {
    ifstream file(filePath, ios::binary);
    if (!file.is_open()) return nullptr;

    char header[54];
    file.read(header, 54);

    int width = *(int*)&header[18];
    int height = *(int*)&header[22];
    int rowSize = ((width * 3 + 3) & ~3); // Учет padding
    char* rowData = new char[rowSize];

    vector<COLORREF>* pixels = new vector<COLORREF>();
    COLORREF bgColor;

    for (int y = 0; y < height; ++y) {
        file.read(rowData, rowSize);
        for (int x = 0; x < width; ++x) {
            int index = x * 3;
            COLORREF color = RGB((unsigned char)rowData[index + 2], (unsigned char)rowData[index + 1], (unsigned char)rowData[index]);
            if (y == 0 && x == 0) bgColor = color; // Первый пиксель — цвет фона
            pixels->push_back(color);
        }
    }

    delete[] rowData;

    ImageData* imgData = new ImageData{ pixels, height, width, INT_MAX, 0, INT_MAX, 0, bgColor };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            COLORREF color = pixels->at(y * width + x);
            if (!CompareColors(color, bgColor, 10)) {
                imgData->minX = min(imgData->minX, x);
                imgData->maxX = max(imgData->maxX, x);
                imgData->minY = min(imgData->minY, y);
                imgData->maxY = max(imgData->maxY, y);
            }
        }
    }

    return imgData;
}

void DisplayImage(const ImageData* imgData, HDC hdc, HWND hwnd) {
    int figWidth = imgData->maxX - imgData->minX + 1;
    int figHeight = imgData->maxY - imgData->minY + 1;

    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    RECT rect;
    GetClientRect(hwnd, &rect);
    FillRect(hdc, &rect, whiteBrush);
    DeleteObject(whiteBrush);

    int posX = 0;
    int posY = rect.bottom - figHeight;

    for (int x = 0; x < figWidth; ++x) {
        for (int y = 0; y < figHeight; ++y) {
            int flippedX = x;
            int flippedY = figHeight - 1 - y;

            COLORREF color = imgData->pixels->at((imgData->minY + flippedY) * imgData->width + imgData->minX + flippedX);
            if (!CompareColors(color, imgData->bgColor, 10)) {
                SetPixel(hdc, posX + x, posY + y, color);
            }
        }
    }
}

void ShowStartupMessage() {
    MessageBox(
        NULL,
        L"Выполнила Федотова Диана Алексеевна НМТ-323901\nФигура: Семиугольник\nМестоположение: X: Слева, Y: Внизу\nЦвет: Фиолетовый",
        L"Информация",
        MB_OK | MB_ICONINFORMATION
    );
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static wchar_t* selectedFile = nullptr;
    static ImageData* imgData = nullptr;

    switch (msg) {
    case WM_CREATE:
        ShowStartupMessage();
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDM_OPEN) {
            if (SelectBMPFile(hwnd, &selectedFile)) {
                if (imgData) delete imgData;
                imgData = LoadBMP(selectedFile);
                if (imgData) {
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (imgData) {
            DisplayImage(imgData, hdc, hwnd);
        }
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_CLOSE:
        if (imgData) delete imgData;
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"CustomWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"BMP Диана НМТ-323901",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    HMENU menu = CreateMenu();
    HMENU subMenu = CreatePopupMenu();
    AppendMenu(subMenu, MF_STRING, IDM_OPEN, L"Открыть BMP файл");
    AppendMenu(menu, MF_STRING | MF_POPUP, (UINT_PTR)subMenu, L"Файл");
    SetMenu(hwnd, menu);

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
