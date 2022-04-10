/* 
 * Drag[en]gine Game Engine Distribution Launcher
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

#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "Launcher.h"
#include "LauncherIni.h"
#include "Exception.h"
#include "String.h"

Launcher::Launcher(int argc, char **argv) :
pLauncherIni(nullptr)
{
	int i;
	for(i=1; i<argc; i++){
		if(pLaunchArgs.Length() > 0){
			pLaunchArgs += " ";
		}
		pLaunchArgs += argv[i];
	}
}

Launcher::~Launcher(){
	if(pLauncherIni){
		delete pLauncherIni;
	}
}

int Launcher::Run(){
	char cwd[PATH_MAX];
	if(!getcwd(cwd, sizeof(cwd))){
		throw Exception("Failed get working directory");
	}
	pLauncherDirectory = cwd;
	
	if(pLauncherDirectory[-1] != '/'){
		pLauncherDirectory += '/';
	}
	
	pLauncherIni = new LauncherIni(pLauncherDirectory + "Launcher.ini");
	
	pFindInstaller();
	pLaunchDelga();
	return 0;
}

void Launcher::pFindInstaller(){
	DIR * const dir = opendir(pLauncherDirectory.Pointer());
	if(!dir){
		throw Exception("Failed scanning working directory");
	}
	
	while(true){
		struct dirent * const ent = readdir(dir);
		if(!ent){
			break;
		}
		
		if(strlen(ent->d_name) > 30
		&& strncmp(ent->d_name, "install-dragengine-", 19) == 0
		&& strcmp(ent->d_name + strlen(ent->d_name) - 11, "-linux64.sh") == 0){
			pFilenameInstaller = ent->d_name;
			closedir(dir);
			return;
		}
	}
	
	closedir(dir);
	throw Exception("Failed finding dragengine installer");
}

String Launcher::pGetInstallerEngineVersion(){
	return pFilenameInstaller.SubString(19, pFilenameInstaller.Length() - 11);
}

String Launcher::pGetInstalledEngineVersion(){
	FILE * const cmdline = popen("delauncher --version", "r");
	if(!cmdline){
		return String();
	}
	
	char buffer[16];
	memset(buffer, 0, sizeof(buffer));
	if(fgets(buffer, sizeof(buffer), cmdline)){
		char drain[8];
		while(fgets(drain, sizeof(drain), cmdline));
	}
	
	pclose(cmdline);
	
	return String(buffer);
}

bool Launcher::pCompareEngineVersion(const String &a, const String &b){
	const char *ptrA = a.Pointer();
	const char *ptrB = b.Pointer();
	
	while(*ptrA || *ptrB){
		char *deliA = nullptr;
		char *deliB = nullptr;
		
		const int numberA = (int)strtol(ptrA, &deliA, 10);
		const int numberB = (int)strtol(ptrB, &deliB, 10);
		
		if(!deliA || (*deliA && *deliA != '.')){ // not '.' or 0-terminator
			String message("Invalid engine version '");
			message += a;
			message += "'";
			throw Exception(message);
		}
		
		if(!deliB || (*deliB && *deliB != '.')){ // not '.' or 0-terminator
			String message("Invalid engine version '");
			message += b;
			message += "'";
			throw Exception(message);
		}
		
		if(numberA < numberB){
			return -1;
			
		}else if(numberA > numberB){
			return 1;
		}
		
		ptrA = deliA;
		if(*deliA){
			ptrA++;
		}
		
		ptrB = deliB;
		if(*deliB){
			ptrB++;
		}
	}
	
	return 0;
}

bool Launcher::pSystemCanLaunchDelga(){
	// check if system knows how to launch delga files. we are using this extra
	// check instead of just calling xdg-open and checking the return value because
	// xdg-open can block the process with a user-question because it detects the
	// delga file to launch to be a binary (albeit not an executable one). to avoid
	// potential stalling problems the query solution is used. less elegant but
	// the result will certainly never stall the process
	
	FILE * const cmdline = popen("xdg-mime query default application/dragengine-delga", "r");
	if(!cmdline){
		return false;
	}
	
	int count = 0;
	char buffer[8];
	while(fgets(buffer, sizeof(buffer), cmdline)){
		count++;
	}
	pclose(cmdline);
	
	return count > 0;
}

void Launcher::pInstallEngine(){
	printf("Install game engine using installer: %s\n", pFilenameInstaller.Pointer());
	int returnValue;
	
	// try gnome-terminal (ubuntu)
	printf("Trying gnome-terminal\n");
	String cmdline("gnome-terminal --wait -- bash -c \"");
	cmdline += pLauncherDirectory + pFilenameInstaller;
	cmdline += " --yes";
	cmdline += '"';
	
	returnValue = system(cmdline.Pointer());
	if(WIFEXITED(returnValue)){
		printf("Installation Exit Status: %d\n", WEXITSTATUS(returnValue));
		
		switch(WEXITSTATUS(returnValue)){
		case 0:
			return;
			
		case 1: // installer
		case 130: // bash: ctrl+c
			throw Exception("Installation cancelled");
			
		case 127: // bash: command not found
			break;
			
		default:
			throw Exception("Installation failed");
		}
		
	}else if(WIFSIGNALED(returnValue)){
		throw Exception("Installation cancelled");
		
	}else{
		// otherwise command not found
	}
	
	// try xterm
	printf("Trying xterm\n");
	cmdline = "xterm -e \"";
	cmdline += pLauncherDirectory + pFilenameInstaller;
	cmdline += " --yes";
	cmdline += '"';
	
	returnValue = system(cmdline.Pointer());
	if(WIFEXITED(returnValue)){
		printf("Installation Exit Status: %d\n", WEXITSTATUS(returnValue));
		
		switch(WEXITSTATUS(returnValue)){
		case 0:
			return;
			
		case 1: // installer
		case 130: // bash: ctrl+c
			throw Exception("Installation cancelled");
			
		case 127: // bash: command not found
			break;
			
		default:
			throw Exception("Installation failed");
		}
		
	}else if(WIFSIGNALED(returnValue)){
		throw Exception("Installation cancelled");
		
	}else{
		// otherwise command not found
	}
	
	// we found no terminal we know to install the game engine
	throw Exception("No terminal found to run installer");
}

void Launcher::pLaunchDelga(){
	// install the game engine if game engine is not installed (version=0) or not newer
	const String requiredVersion(pGetInstallerEngineVersion());
	const String installedVersion(pGetInstalledEngineVersion());
	
	printf("Required game engine version: %s\n", requiredVersion.Pointer());
	printf("Installed game engine version: %s\n", installedVersion.Pointer());
	
	if(pCompareEngineVersion(requiredVersion, installedVersion) > 0){
		pInstallEngine();
	}
	
	// check again if system knows how to launch delga files. this should
	// return success otherwise installer failed or user aborted it
	//
	// NOTE Ubunut seems to sometimes act up installing mime-types failing to launch
	//      the game engine albeit
// 	if(!pSystemCanLaunchDelga()){
// 		return;
// 	}
	
	// launch delga file. this should work now.
	// 
	// problem is xdg-open supports no command line arguments. we thus have to
	// use dragengine launcher directly. we need in the future a way to set
	// a link "delauncher" which allows the user to select which launcher to
	// open. the launcher is required to support command line of this form:
	// 
	//   delauncher <options> <delga> <arguments>
	// 
	// where <options> can be:
	//   
	//   --profile=profile
	const String pathDelga(pLauncherIni->Get("File"));
	String cmdline;
	int returnValue;
	
	printf("Launch: %s\n", pathDelga.Pointer());
	
	/*
	if(pLaunchArgs.Length() == 0){
		cmdline = "xdg-open \"";
		cmdline += pLauncherDirectory + pathDelga;
		cmdline += '"';
		
		exitCode = system(cmdline);
		switch( exitCode ){
		case 0: // success
		case 130: // bash control+c
			return;
			
		default:
			break;
		}
		
		// someting went wrong. try using delauncher-gui directly
	}
	*/
	
	cmdline = "delauncher-gui \"";
	cmdline += pLauncherDirectory + pathDelga;
	cmdline += "\" ";
	cmdline += pLaunchArgs;
	
	returnValue = system(cmdline);
	if(WIFEXITED(returnValue)){
		switch(WEXITSTATUS(returnValue)){
		case 0: // success
		case 130: // bash: ctrl+c
			return;
			
		default:
			throw Exception("Failed launching");
		}
		
	}else if(WIFSIGNALED(returnValue)){
		// most probably ctrl+c
		
	}else{
		throw Exception("Failed launching");
	}
}

// Entry point
int main( int argc, char **argv ){
	try{
		Launcher launcher(argc, argv);
		return launcher.Run();
		
	}catch(const Exception &e){
		String str("xmessage -center \"Failed running launcher:\n");
		str += e.Message();
		str += '"';
		system(str);
		return 1;
	}
}
