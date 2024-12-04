using namespace std;

#include <windows.h>
#include <commdlg.h>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>



#define IDM_OPEN 1001

//-----------------------------------
struct Info 
{
    std::vector<COLORREF>* pixels;
    int height, width;
    int fXMin, fXMax, fYMin, fYMax;
    COLORREF backgroundColor;
};
//-----------------------------------

//-----------------------------------
bool BMPFileOpener(
    HWND hWnd, 
    wchar_t** selectedFile
) 
{
    OPENFILENAME file;
    wchar_t FILE[MAX_PATH] = L"";

    ZeroMemory(&file, sizeof(file));
    file.lStructSize = sizeof(file);
    file.hwndOwner = hWnd;
    file.lpstrFilter = L"BMP Files\0*.bmp\0";
    file.lpstrFile = FILE;
    file.lpstrTitle = L"Выберите файл";
    file.nMaxFile = sizeof(FILE);
    file.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&file))
    {
        *selectedFile = _wcsdup(FILE);
        return true;
    }
    return false;
}
//-----------------------------------

//-----------------------------------
bool Color(
    COLORREF color1,
    COLORREF color2,
    int tolerance) 
{
    int red1 = GetRValue(color1), green1 = GetGValue(color1), blue1 = GetBValue(color1);
    int red2 = GetRValue(color2), green2 = GetGValue(color2), blue2 = GetBValue(color2);
    return (abs(red1 - red2) <= tolerance && abs(green1 - green2) <= tolerance && abs(blue1 - blue2) <= tolerance);
}
//-----------------------------------

//-----------------------------------
Info* Pixel(
    const wchar_t* filename
) 
{
    ifstream file(filename, ios::binary);
    if (!file.is_open())
    {
        return nullptr;
    }

    char header[54];
    file.read(header, 54);

    int width = *(int*)&header[18];
    int height = *(int*)&header[22];
    int Length = ((width * 3 + 3) & ~3); 
    char* Data = new char[Length];

    vector<COLORREF>* Pixels = new vector<COLORREF>();
    COLORREF backgroundColor;

    for (int y = 0; y < height; y++) 
    {
        file.read(Data, Length);
        for (int x = 0; x < width; x++) 
        {
            int i = x * 3;
            COLORREF color = RGB((unsigned char)Data[i + 2], (unsigned char)Data[i + 1], (unsigned char)Data[i]);
            if (y == 0 && x == 0)
            {
                backgroundColor = color;
            }
         
            Pixels->push_back(color);
        }
    }

    delete[] Data;

    Info* info = new Info{ Pixels, height, width, INT_MAX, 0, INT_MAX, 0, backgroundColor };

    for (int y = 0; y < height; y++) 
    {
        for (int x = 0; x < width; x++) 
        {
            COLORREF color = Pixels->at(y * width + x);
            if (!Color(color, backgroundColor, 10)) 
            {
                info->fXMin = min(info->fXMin, x);
                info->fXMax = max(info->fXMax, x);
                info->fYMin = min(info->fYMin, y);
                info->fYMax = max(info->fYMax, y);
            }
        }
    }

    return info;
}
//-----------------------------------

//-----------------------------------
void Image(
    const Info* imageInfo, 
    HDC hdcWindow, 
    HWND hwnd) 
{
    int fWidth = imageInfo->fXMax - imageInfo->fXMin + 1;
    int fHeight = imageInfo->fYMax - imageInfo->fYMin + 1;

    
    HBRUSH Brush = CreateSolidBrush(RGB(255, 255, 255)); ///белый
    RECT Rect;
    GetClientRect(hwnd, &Rect);
    FillRect(hdcWindow, &Rect, Brush);
    DeleteObject(Brush);

    ///---Вычисление положения фигуры(Справа снизу)--
    int RightX = (Rect.right - fWidth / 2);
    int Bot = Rect.bottom;

    int X = RightX - fWidth / 2;
    int Y = Bot - fHeight;

    for (int x = 0; x < fWidth; x++) 
    {
        for (int y = 0; y < fHeight; y++) 
        {
            int fX = x;
            int fY = fHeight - 1 - y; 

            COLORREF color = imageInfo->pixels->at((imageInfo->fYMin + fY) * imageInfo->width + imageInfo->fXMin + fX);
            if (!Color(color, imageInfo->backgroundColor, 10)) 
            {
                SetPixel(hdcWindow, X + x, Y + y, color);
            }
        }
    }
}
//-----------------------------------

//-----------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static wchar_t* selected = nullptr;
    static Info* info = nullptr;

    switch (message) 
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDM_OPEN) 
        {
            if (BMPFileOpener(hwnd, &selected)) 
            {
                if (info)
                {
                    delete info;
                }

                info = Pixel(selected);

                if (info) 
                {
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
        }
        break;

    case WM_PAINT: 
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (info) 
        {
            Image(info, hdc, hwnd);
        }

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_CLOSE:
        if (info)
        {
            delete info;
        }
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
//-----------------------------------

//-----------------------------------
int WINAPI WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine, 
    int nCmdShow) 
{
    const wchar_t CLASS_NAME[] = L"WindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Отображение BMP-файлов",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd)
    {
        return 0;
    }

    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_OPEN, L"Выбрать  файл");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"Открыть файл");
    SetMenu(hwnd, hMenu);

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
//-----------------------------------