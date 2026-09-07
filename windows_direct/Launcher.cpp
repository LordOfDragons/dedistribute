/* 
 * Drag[en]gine Windows Launcher
 *
 * Copyright (C) 2026, DragonDreams GmbH (info@dragondreams.ch)
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
#include "Logger.h"

#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <shellapi.h>
#include <shlobj.h>
#include <string>

Launcher *Launcher::pTheLauncher = nullptr;

Launcher::Launcher(Logger& logger, HINSTANCE hInstance) :
pLogger(logger),
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
    logger.Log("Launcher initialized");
    logger.Log("- Arguments: {}", ToString(ArgsToCmdline(pLaunchArgs)));
}

Launcher::~Launcher(){
	pLogger.Log("Launcher terminated");
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

    pLogger.Log("Reading Launcher.ini '{}'", ToString(pLauncherDirectory + L"\\Launcher.ini"));
    pLauncherIni = std::make_unique<LauncherIni>(pLauncherDirectory + L"\\Launcher.ini");
    pLogger.Log("- File: {}", pLauncherIni->Get("File", "<missing>"));

    std::vector<std::wstring>::iterator argEnd;
    for(argEnd = pLaunchArgs.begin(); argEnd != pLaunchArgs.end(); argEnd++){
        if(*argEnd == L"--"){
            break;
        }
    }

    if(argEnd != pLaunchArgs.end()){
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

std::wstring Launcher::ArgsToCmdline(const std::vector<std::wstring>& args){
    std::vector<std::wstring>::const_iterator iter;
    std::wstring cmdline;

    for(iter = args.cbegin(); iter != args.cend(); iter++){
        const std::wstring &arg = *iter;

        if(iter != args.cbegin()){
            cmdline.push_back(L' ');
        }

        if(!arg.empty() && arg.find_first_of(L" \t\n\v\"") == arg.npos){
            cmdline.append(arg);

        }else{
            cmdline.push_back(L'"');
            
            std::wstring::const_iterator iter2;
            for(iter2 = arg.cbegin(); ; iter2++){
                int countBackslash = 0;
                
                while(iter2 != arg.cend() && *iter2 == L'\\'){
                    iter2++;
                    countBackslash++;
                }
                
                if(iter2 == arg.cend()){
                    // Escape all backslashes, but let the terminating
                    // double quotation mark we add below be interpreted
                    // as a metacharacter.
                    cmdline.append(countBackslash * 2, L'\\');
                    break;

                }else if(*iter2 == L'"'){
                    // Escape all backslashes and the following
                    // double quotation mark.
                    cmdline.append(countBackslash * 2 + 1, L'\\');
                    cmdline.push_back(*iter2);

                }else{
                    // Backslashes aren't special here.
                    cmdline.append(countBackslash, L'\\');
                    cmdline.push_back(*iter2);
                }
            }
            
            cmdline.push_back(L'"');
        }
    }

    return cmdline;
}

void Launcher::pLaunchDelga(){
    if (pLaunchArgs.empty()) {
        if(pLaunchDelgaUsingShellOpen()){
            return;
		}
        //pLogger.LogLastError("Fallback to registry launching.");
        throw std::runtime_error("Failed launching DELGA.");
    }

    if(pLaunchDelgaUsingRegPath()){
        return;
	}
    throw std::runtime_error("Failed launching DELGA.");
}

bool Launcher::pLaunchDelgaUsingShellOpen(){
    const std::wstring path(pLauncherDirectory + L"\\" + ToWString(pLauncherIni->Get("File")));

    SHELLEXECUTEINFO sei = { 0 };
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = nullptr;
    sei.lpVerb = L"open";
    sei.lpFile = path.c_str();
    sei.lpParameters = nullptr;
    sei.lpDirectory = nullptr;
    sei.nShow = SW_SHOW;
    sei.hInstApp = nullptr;

    pLogger.Log("Launching DELGA using Shell Open");
    pLogger.Log("- File: '{}'", ToString(path));
    if (!ShellExecuteEx(&sei)) {
        pLogger.LogLastError("Failed launching DELGA.");
		return false;
    }

    pLogger.Log("Process launched");
    if (sei.hProcess) {
        WaitForSingleObject(sei.hProcess, INFINITE);
        CloseHandle(sei.hProcess);
    }

    pLogger.Log("Process finished");
    return true;
}

bool Launcher::pLaunchDelgaUsingRegPath(){
    const std::wstring path(pLauncherDirectory + L"\\" + ToWString(pLauncherIni->Get("File")));
    
    pLogger.Log("Launching DELGA using registry path");
    wchar_t buffer[MAX_PATH + 1];
    DWORD size = MAX_PATH;
    ZeroMemory(&buffer, sizeof(buffer));

    if (RegGetValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Drag[en]gine", L"PathEngine",
    RRF_RT_REG_SZ, nullptr, buffer, &size) != ERROR_SUCCESS) {
		pLogger.LogLastError("Failed reading Drag[en]gine installation directory from registry.");
        throw std::runtime_error("Failed reading Drag[en]gine installation directory from registry.");
    }
	pLogger.Log("- Drag[en]gine installation directory: '{}'", ToString(buffer));

    std::vector<std::wstring> args;
    args.push_back(std::wstring(buffer) + L"\\Launchers\\Bin\\delauncher-gui.exe");
    args.push_back(path);
    args.insert(args.end(), pLaunchArgs.cbegin(), pLaunchArgs.cend());

    const std::wstring cmdline(ArgsToCmdline(args));

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    ZeroMemory(&pi, sizeof(pi));

	pLogger.Log("- Command line: '{}'", ToString(cmdline));
    if (!CreateProcess(nullptr, (LPWSTR)cmdline.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		pLogger.LogLastError("Creating process failed.");
        throw std::runtime_error("Creating process failed.");
    }

	pLogger.Log("Process launched");
    if (pi.hProcess) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

	pLogger.Log("Process finished");
	return true;
}

/** Entry point. */
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
_In_ LPWSTR    lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);
    UNREFERENCED_PARAMETER(lpCmdLine);

    Logger logger;
    try{
        Launcher launcher(logger, hInstance);
        return launcher.Run();

    }catch(const std::exception &e){
		logger.Log("Failed running launcher: {}", e.what());

        std::wstring message(L"Failed running launcher:\n");
        message += Launcher::ToWString(std::string(e.what()));
        message += L"\n\nPlease Reinstall Game.";
        message += L"\n\nFor Support please provide log file:";
        message += L"\n%LocalAppData%\\DELaunchers\\Logs\\launcher_direct.log";

        MessageBoxW(NULL, message.c_str(), L"Error", MB_OK);
        return -1;
    }
}

