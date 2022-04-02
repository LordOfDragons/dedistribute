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

#include "resource.h"
#include "Launcher.h"
#include "WindowSplash.h"
#include "WindowInstall.h"
#include "LauncherIni.h"

#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <shellapi.h>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include <chrono>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.ViewManagement.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::ViewManagement;

Launcher *Launcher::pTheLauncher = nullptr;

Launcher::Launcher(HINSTANCE hInstance) :
pInstance(hInstance),
pCancelInstall(false),
pCloseInstall(false),
hAccelTable(nullptr)
{
    pTheLauncher = this;

    // parse command line
    int i, nArgs;
    LPWSTR * const szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if(!szArglist){
        throw std::runtime_error("CommandLineToArgvW failed");
    }
    for(i=1; i<nArgs; i++){
        pLaunchArgs.push_back(szArglist[i]);
    }
    LocalFree(szArglist);
}

Launcher::~Launcher(){
    pTheLauncher = nullptr;
}

int Launcher::Run(){
    //pWindowSplash = std::make_unique<WindowSplash>();
    //return pRunMessageLoop();
    
    wchar_t executablePath[MAX_PATH];
    GetModuleFileNameW(pInstance, executablePath, MAX_PATH);

    pLauncherDirectory = executablePath;
    
    const std::wstring::size_type pathSeparator = pLauncherDirectory.rfind(L'\\');
    if(pathSeparator != std::wstring::npos){
        pLauncherDirectory = pLauncherDirectory.substr(0, pathSeparator);
    }

    pLauncherIni = std::make_unique<LauncherIni>(pLauncherDirectory + L"\\Launcher.ini");

    if(!pIsLaucherInstalled() && !pInstallLauncher()){
        return 1;
    }

    if(!pLaunchArgs.empty() && pLaunchArgs[0] == L"--install-only"){
        return 0;
    }

    pLaunchDelga();
    
    // just long enough for the dialog to show then we can close. if we close
    // too quickly the dialog is not shown. 3 seconds should be enough especially
    // since we show no window and thus are invisible
    SleepEx(3000, true);

    return 0;
    //return pRunMessageLoop();
}

void Launcher::CancelInstall(){
    pCancelInstall = true;
}

void Launcher::RequestInstall(){
    pRequestInstallLauncher();
}

void Launcher::CloseInstall(){
    pCloseInstall = true;
}

std::wstring Launcher::ToWString(const std::string& string){
    const int neededSize = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), (int)string.size(), NULL, 0);
    std::wstring wstring(neededSize, 0);
    MultiByteToWideChar(CP_UTF8, 0, string.c_str(), (int)string.size(), &wstring[0], neededSize);
    return wstring;
}

std::string Launcher::ToString(const std::wstring& string){
    const int neededSize = WideCharToMultiByte(CP_UTF8, 0, string.c_str(), (int)string.size(), NULL, 0, NULL, NULL);
    std::string cstring(neededSize, 0);
    WideCharToMultiByte(CP_UTF8, 0, string.c_str(), (int)string.size(), &cstring[0], neededSize, NULL, NULL);
    return cstring;
}

std::wstring Launcher::UrlEncode(const std::wstring& string){
    // we have to utf8 encode the result
    const std::string utf8(ToString(string));
    std::wstringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    std::string::const_iterator iter;
    for(iter = utf8.cbegin(); iter != utf8.cend(); iter++){
        const std::string::value_type c = *iter;

        if(isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~'){
            escaped << c;
            
        }else{
            escaped << std::uppercase;
            // do not remove the (unsigned char) cast or the result is wrong for UTF8 characters
            escaped << '%' << std::setw(2) << (int)(unsigned char)c;
            escaped << std::nouppercase;
        }
    }

    return escaped.str();
}

int Launcher::pRunMessageLoop(){
    MSG msg;
    HACCEL hAccelTable = LoadAccelerators(pInstance, MAKEINTRESOURCE(IDC_LAUNCHER));

    while(GetMessage(&msg, nullptr, 0, 0)){
        if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

void Launcher::pRunMessageLoopOnce(){
    MSG msg;
    if(!hAccelTable){
        hAccelTable = LoadAccelerators(pInstance, MAKEINTRESOURCE(IDC_LAUNCHER));
    }

    while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)){
        if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

void Launcher::pExitApplication(){
    PostQuitMessage(0);
}

void Launcher::pLaunchDelga(){
    Windows::System::LauncherOptions options;

    //const auto init = options.as<IInitializeWithWindow>();
    //winrt::check_hresult(init->Initialize(hWnd));

    // quite the mess here. if the application is not installed setting TargetApplicationPackageFamilyName
    // causes the popup window to not show the preferred application to install which is bad.
    // if LaunchUriForResultAsync() is used not using TargetApplicationPackageFamilyName causes
    // exception to be thrown and launching does not work. the launcher can though not run anyways
    // because it can not react to activation events (not a winrt application)
    
    //options.TargetApplicationPackageFamilyName(L"DragonDreams.Dragengine.GameEngine_14hw6vre8sh8m");
    options.PreferredApplicationPackageFamilyName(L"DragonDreams.Dragengine.GameEngine_14hw6vre8sh8m");
    options.PreferredApplicationDisplayName(L"Drag[en]gine Game Engine");
    options.DesiredRemainingView(ViewSizePreference::UseNone);
    options.TreatAsUntrusted(false);

    std::wstring uriString{L"delauncher:run?file="};
    uriString += UrlEncode(pLauncherDirectory + L"\\" + ToWString(pLauncherIni->Get("File")));
    
    std::vector<std::wstring>::const_iterator iterArg;
    for(iterArg = pLaunchArgs.cbegin(); iterArg != pLaunchArgs.cend(); iterArg++){
        uriString += L"&argument=";
        uriString += UrlEncode(*iterArg);
    }

    Uri uri{uriString};

    
    const bool result = Windows::System::Launcher::LaunchUriAsync(uri, options).get();
    if(result){

    }else{
        throw std::runtime_error("Failed launching DELGA");
    }
    
    /*
    const Windows::System::LaunchUriResult result =
        Windows::System::Launcher::LaunchUriForResultsAsync(uri, options).get();

    {
    std::wstringstream sstr;
    sstr << L"LaunchUriStatus: ";
    sstr << (int)result.Status();
    OutputDebugString(sstr.str().c_str());
    }

    if(result){

    }else{
        throw std::runtime_error("Failed launching DELGA");
    }
    */
}

bool Launcher::pIsLaucherInstalled(){
    // QueryAppUriSupportAsync does not seem to work
    const Windows::System::LaunchQuerySupportStatus result =
        Windows::System::Launcher::QueryUriSupportAsync(Uri(L"delauncher:"),
            Windows::System::LaunchQuerySupportType::Uri).get();

    return result == Windows::System::LaunchQuerySupportStatus::Available;
}

bool Launcher::pInstallLauncher(){
    std::unique_ptr<WindowInstall> window = std::make_unique<WindowInstall>();
    window->Show();

    pRequestInstallLauncher();

    auto startTime = std::chrono::high_resolution_clock::now();
    const std::chrono::seconds intervalCheck(1);

    while(!pCancelInstall){
        pRunMessageLoopOnce();

        const auto curTime = std::chrono::high_resolution_clock::now();
        if((curTime - startTime) < intervalCheck){
            continue;
        }

        if(pIsLaucherInstalled()){
            //OutputDebugString(L"Launcher installed.\n");
            break;
        }
        //OutputDebugString(L"Launcher not installed. Sleeping.\n");
        startTime = curTime;
    }

    if(pCancelInstall){
        return false;
    }

    window->InstallationDone();

    while(!pCloseInstall){
        pRunMessageLoopOnce();
    }

    return true;
}

void Launcher::pRequestInstallLauncher(){
    Windows::System::LauncherOptions options;
    options.PreferredApplicationPackageFamilyName(L"DragonDreams.Dragengine.GameEngine_14hw6vre8sh8m");
    options.PreferredApplicationDisplayName(L"Drag[en]gine Game Engine");
    options.DesiredRemainingView(ViewSizePreference::UseNone);
    options.TreatAsUntrusted(false);

    if(!Windows::System::Launcher::LaunchUriAsync(Uri(L"delauncher:ready"), options).get()){
        throw std::runtime_error("Failed launching launcher installation");
    }
}

/** Entry point. */
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
_In_ LPWSTR    lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);
    UNREFERENCED_PARAMETER(lpCmdLine);

    winrt::init_apartment();

    try{
        Launcher launcher(hInstance);
        return launcher.Run();

    }catch(const std::exception &e){
        std::wstring message(L"Failed running launcher:\n");
        message += Launcher::ToWString(std::string(e.what()));
        MessageBoxW(NULL, message.c_str(), L"Error", MB_OK);
        return -1;
    }
}

