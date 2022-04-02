/* 
 * Drag[en]gine Windows Launcher
 *
 * Copyright (C) 2022, Roland Plüss (roland@rptd.ch)
 * 
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either 
 * version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "WindowInstall.h"
#include "Launcher.h"
#include "Resource.h"
#include <stdexcept>

#define BTN_INSTALL     1000
#define BTN_CANCEL      1001


static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
    switch(message){
    case WM_COMMAND:
        switch(LOWORD(wParam)){
        case BTN_INSTALL:
            if(((WindowInstall*)GetWindowLongPtr(hWnd, GWLP_USERDATA))->IsDoneInstall()){
                Launcher::Get().CloseInstall();
            }else{
                Launcher::Get().RequestInstall();
            }
            break;

        case BTN_CANCEL:
            if(((WindowInstall*)GetWindowLongPtr(hWnd, GWLP_USERDATA))->IsDoneInstall()){
                Launcher::Get().CloseInstall();
            }else{
                Launcher::Get().CancelInstall();
            }
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_PAINT:{
        WindowInstall &window = *((WindowInstall*)GetWindowLongPtr(hWnd, GWLP_USERDATA));
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        DrawIconEx(hdc, 25, 18, window.GetIconDragengine(), 64, 64, 0, nullptr, DI_NORMAL); 
        EndPaint(hWnd, &ps);
        }
        break;

    case WM_CLOSE:
        if(((WindowInstall*)GetWindowLongPtr(hWnd, GWLP_USERDATA))->IsDoneInstall()){
            Launcher::Get().CloseInstall();
        }else{
            Launcher::Get().CancelInstall();
        }
        return 0; // do not close

    case WM_DESTROY:
        break;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

const wchar_t *WindowInstall::WindowClassName = L"LauncherWindowInstall";

WindowInstall::WindowInstall() :
pWindowClass(INVALID_ATOM),
pWindow(nullptr),
pButtonInstall(nullptr),
pButtonCancel(nullptr),
pIconDragengine(nullptr),
pDoneInstall(false)
{
	RegisterWindowClass();
    CreateInstallWindow();
}

WindowInstall::~WindowInstall(){
    if(pWindow){
        DestroyWindow(pWindow);
    }
}

void WindowInstall::Show(){
    CenterWindow();
    ShowWindow(pWindow, SW_SHOWDEFAULT);
    UpdateWindow(pWindow);
}

void WindowInstall::InstallationDone(){
    pDoneInstall = true;
    SendMessage(pButtonInstall, WM_SETTEXT, 0, (LPARAM)_T("Done"));
    EnableWindow(pButtonCancel, FALSE);
    InvalidateRect(pButtonCancel, nullptr, TRUE);
}

void WindowInstall::RegisterWindowClass()
{
    const HINSTANCE hInstance = Launcher::Get().GetInstance();
	WNDCLASSEXW wcex;

    ZeroMemory(&wcex, sizeof(wcex));
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAUNCHER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = WindowClassName;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    pWindowClass = RegisterClassExW(&wcex);
    if(pWindowClass == INVALID_ATOM){
        throw std::runtime_error("RegisterClassExW failed");
    }
}

void WindowInstall::CreateInstallWindow(){
    DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER;
    RECT boundary{0, 0, 350, 100};
    if(!AdjustWindowRect(&boundary, windowStyle, FALSE)){
        throw std::runtime_error("AdjustWindowRect failed");
    }

    const HINSTANCE hInstance = Launcher::Get().GetInstance();
	pWindow = CreateWindowW(WindowClassName, L"Install Drag[en]gine Game Engine",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER,
        CW_USEDEFAULT, CW_USEDEFAULT, boundary.right - boundary.left,
        boundary.bottom - boundary.top, nullptr, nullptr, hInstance, nullptr);
    
    if(pWindow == NULL){
        throw std::runtime_error("CreateWindowW failed");
    }

    SetWindowLongPtr(pWindow, GWLP_USERDATA, (LONG_PTR)this);
    
    pIconDragengine = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_LAUNCHER),
        IMAGE_ICON, 64, 64, LR_DEFAULTCOLOR | LR_LOADTRANSPARENT);

    pButtonInstall = CreateWindow(L"BUTTON", L"Install",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        115, 25, 100, 50, pWindow, (HMENU)BTN_INSTALL, hInstance, nullptr);

    pButtonCancel = CreateWindow(L"BUTTON", L"Cancel",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        225, 25, 100, 50, pWindow, (HMENU)BTN_CANCEL, hInstance, nullptr);
}

void WindowInstall::CenterWindow(){
    RECT rectWindow;
    GetWindowRect(pWindow, &rectWindow);
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    const int windowWidth = rectWindow.right - rectWindow.left;
    const int windowHeight = rectWindow.bottom - rectWindow.top;
    const int x = (screenWidth - windowWidth) / 2;
    const int y = (screenHeight - windowHeight) / 2;
    MoveWindow(pWindow, x, y, windowWidth, windowHeight, FALSE);
}
