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
#include "LauncherIni.h"

#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <shellapi.h>
#include <iostream>
#include <string>

Launcher *Launcher::pTheLauncher = nullptr;

Launcher::Launcher(HINSTANCE hInstance) :
pInstance(hInstance)
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
    wchar_t executablePath[MAX_PATH];
    GetModuleFileNameW(pInstance, executablePath, MAX_PATH);

    pLauncherDirectory = executablePath;
    
    const std::wstring::size_type pathSeparator = pLauncherDirectory.rfind(L'\\');
    if(pathSeparator != std::wstring::npos){
        pLauncherDirectory = pLauncherDirectory.substr(0, pathSeparator);
    }

    pLauncherIni = std::make_unique<LauncherIni>(pLauncherDirectory + L"\\Launcher.ini");

    std::vector<std::wstring>::iterator argEnd;
    for(argEnd = pLaunchArgs.begin(); argEnd != pLaunchArgs.end(); argEnd++){
        if(*argEnd == L"--"){
            break;
        }
    }

    if(argEnd != pLaunchArgs.cend()){
        pLaunchArgs.erase(pLaunchArgs.begin(), argEnd + 1);
    }
    
    pLaunchDelga();

    return 0;
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

void Launcher::pLaunchDelga(){
    const std::wstring path(pLauncherDirectory + L"\\" + ToWString(pLauncherIni->Get("File")));

    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = nullptr;
    sei.lpVerb = L"open";
    sei.lpFile = path.c_str();
    sei.lpParameters = L"";
    sei.lpDirectory = nullptr;
    sei.nShow = SW_SHOW;
    sei.hInstApp = nullptr;

    if(!ShellExecuteEx(&sei)){
        throw std::runtime_error("Failed launching DELGA");
    }

    if(sei.hProcess){
        WaitForSingleObject(sei.hProcess, INFINITE);
        CloseHandle(sei.hProcess);
    }
}

/** Entry point. */
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
_In_ LPWSTR    lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);
    UNREFERENCED_PARAMETER(lpCmdLine);

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

