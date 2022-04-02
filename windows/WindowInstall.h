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

#pragma once

#include "framework.h"

/** Install window. */
class WindowInstall{
private:
	ATOM pWindowClass;
	HWND pWindow;
	HWND pButtonInstall;
	HWND pButtonCancel;
	HICON pIconDragengine;
	bool pDoneInstall;

	static const wchar_t *WindowClassName;


public:
	/** Create splash window. */
	WindowInstall();

	/** Clean up splash window. */
	~WindowInstall();

	/** Show window. */
	void Show();

	/** Icon. */
	inline HICON GetIconDragengine() const{return pIconDragengine;}

	/** Installation done. */
	void InstallationDone();

	/** Installation done. */
	inline bool IsDoneInstall() const{return pDoneInstall;}

private:
	void RegisterWindowClass();
	void CreateInstallWindow();
	void CenterWindow();
};
