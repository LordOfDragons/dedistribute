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

#pragma once

#include <fstream>
#include <format>

class Logger{
private:
	std::ofstream pLogFile;

public:
	Logger();
	~Logger();

	/** Log a message. */
	void Log(const std::string& message);

	/** Log a formatted message. */
	template<typename... Args>
	void Log(std::string_view format, Args&&... args) {
		try {
			auto formatted = std::vformat(format, std::make_format_args(args...));
			Log(formatted);
		}
		catch (const std::exception& e) {
			Log(std::string("Format error: ") + e.what());
		}
	}

	/** Log a wide string message. */
	void Log(const std::wstring& message) {
		Log(ToString(message));
	}

	/** Log message with last windows error. */
	void LogLastError(const std::string& context);

	static std::string ToString(const std::wstring& string);
};
