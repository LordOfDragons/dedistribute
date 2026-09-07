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

#include <shlobj.h>
#include <ctime>
#include <windows.h>

#include "Logger.h"

Logger::Logger(){
    wchar_t localAppData[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        return;
    }

    std::wstring logDir(localAppData);
    logDir += L"\\DELaunchers\\Logs";

    // Create directory if it doesn't exist
    CreateDirectoryW(logDir.c_str(), NULL);

    std::wstring logPath(logDir + L"\\launcher_direct.log");

    pLogFile.open(logPath, std::ios::trunc);
}

Logger::~Logger(){
    if (pLogFile.is_open()) {
        pLogFile.flush();
        pLogFile.close();
    }
}

void Logger::Log(const std::string& message) {
    if (pLogFile.is_open()) {
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);

        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &timeinfo);

        pLogFile << "[" << timestamp << "] " << message << "\n";
        pLogFile.flush();
    }
}

void Logger::LogLastError(const std::string& message) {
    DWORD errorCode = GetLastError();
    if (errorCode == 0) {
        Log("{}: No error information available.", message);
        return;
    }

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    if (size > 0 && messageBuffer) {
        std::string errorMessage(messageBuffer, size);
        if (!errorMessage.empty() && errorMessage.back() == '\n') {
            errorMessage.pop_back();
        }
        if (!errorMessage.empty() && errorMessage.back() == '\r') {
            errorMessage.pop_back();
        }

        Log("{}: {} [{}]", message, errorMessage, errorCode);

        LocalFree(messageBuffer);

    } else {
        Log("{}: Unable to retrieve error message [{}]", message, errorCode);
    }
}

std::string Logger::ToString(const std::wstring& string) {
    if (string.empty()) {
        return std::string();
    }

    const int neededSize = WideCharToMultiByte(CP_UTF8, 0, string.c_str(), (int)string.size(), NULL, 0, NULL, NULL);
    std::string cstring(neededSize, 0);
    WideCharToMultiByte(CP_UTF8, 0, string.c_str(), (int)string.size(), &cstring[0], neededSize, NULL, NULL);
    return cstring;
}
