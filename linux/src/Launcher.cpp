/*
 * MIT License
 *
 * Copyright (C) 2024, DragonDreams GmbH (info@dragondreams.ch)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
#include "File.h"

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
	return pLaunchDelga();
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

Launcher::sEngineInfo Launcher::pGetInstalledEngineInfo(){
	const char *value = getenv("XDG_DATA_DIRS");
	if(value){
		String dataDirs(value);
		int from = 0;
		while(true){
			int to = dataDirs.Find(':', from);
			if(to == -1){
				to = dataDirs.Length();
			}
			
			const String path(dataDirs.SubString(from, to));
			
			String filename(path + "/metainfo/ch.dragondreams.delauncher.metainfo.xml");
			try{
				File file(filename);
				sEngineInfo info{};
				
				while(!file.Eof()){
					const String line(file.ReadLine());
					int tagBegin = line.FindString("<value key=\"DragonDreams::EngineVersion\">");
					if(tagBegin != -1){
						const int valueBegin = tagBegin + 41;
						const int tagEnd = line.FindString("</value>", valueBegin);
						if(tagEnd == -1){
							throw Exception("Invalid metainfo format");
						}
						info.version = line.SubString(valueBegin, tagEnd);
						continue;
					}
					
					tagBegin = line.FindString("<value key=\"DragonDreams::DistroMaintained\">");
					if(tagBegin != -1){
						const int valueBegin = tagBegin + 44;
						const int tagEnd = line.FindString("</value>", valueBegin);
						if(tagEnd == -1){
							throw Exception("Invalid metainfo format");
						}
						info.distroMaintained = line.SubString(valueBegin, tagEnd) == "true";
						continue;
					}
					
					tagBegin = line.FindString("<value key=\"DragonDreams::DistroMaintainedUpdateUrl\">");
					if(tagBegin != -1){
						const int valueBegin = tagBegin + 53;
						const int tagEnd = line.FindString("</value>", valueBegin);
						if(tagEnd == -1){
							throw Exception("Invalid metainfo format");
						}
						info.distroMaintainedUpdateInfoUrl = line.SubString(valueBegin, tagEnd);
						continue;
					}
				}
				
				if(info.version.Length() > 0){
					return info;
				}
				
			}catch(...){
				// file does not exist or can not be read. ignore it
			}
			
			if(to == dataDirs.Length()){
				break;
			}
			from = to + 1;
		}
	}
	
	FILE * const cmdline = popen("delauncher --version", "r");
	if(!cmdline){
		return {};
	}
	
	char buffer[16];
	memset(buffer, 0, sizeof(buffer));
	if(fgets(buffer, sizeof(buffer), cmdline)){
		char drain[8];
		while(fgets(drain, sizeof(drain), cmdline));
	}
	
	pclose(cmdline);
	
	sEngineInfo info{};
	info.version = buffer;
	return info;
}

int Launcher::pCompareEngineVersion(const String &a, const String &b){
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
	String cmdline;
	
	// try x-terminal-emulator (debian)
	printf("Trying x-terminal-emulator\n");
	cmdline = "x-terminal-emulator -e \"";
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
	
	// try gnome-terminal (ubuntu)
	printf("Trying gnome-terminal\n");
	cmdline = "gnome-terminal --wait -- bash -c \"";
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
	
	// try urxvt
	printf("Trying urxvt\n");
	cmdline = "urxvt -e \"";
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
	
	// try rxvt
	printf("Trying rxvt\n");
	cmdline = "rxvt -e \"";
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
	
	// try termit
	printf("Trying termit\n");
	cmdline = "termit -e \"";
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

int Launcher::pLaunchDelga(){
	// install the game engine if game engine is not installed (version=0) or not newer
	const String requiredVersion(pGetInstallerEngineVersion());
	const sEngineInfo installedInfo(pGetInstalledEngineInfo());
	
	printf("Required game engine version: %s\n", requiredVersion.Pointer());
	
	if(installedInfo.distroMaintained){
		printf("Installed game engine version: %s (distro managed)\n", installedInfo.version.Pointer());
		
	}else{
		printf("Installed game engine version: %s\n", installedInfo.version.Pointer());
	}
	
	if(pCompareEngineVersion(requiredVersion, installedInfo.version) > 0){
		if(installedInfo.distroMaintained){
			const int returnValue = system("xmessage -center "
				"'Required version of Drag[en]gine can not be installed\n"
				"since Drag[en]gine is managed by system distro.\n"
				"Please do manual update.'"
				" -button Close:0,'Update Info':2");
			
			if(WIFEXITED(returnValue) && WEXITSTATUS(returnValue) == 2){
				String cmdline("xdg-open '");
				cmdline += installedInfo.distroMaintainedUpdateInfoUrl;
				cmdline += "'";
				system(cmdline);
			}
			return 2;
		}
		
		pInstallEngine();
	}
	
	// launch delga file. this should work now.
	const String pathDelga(pLauncherIni->Get("File"));
	String cmdline;
	int returnValue;
	
	printf("Launch: %s\n", pathDelga.Pointer());
	
	if(pLaunchArgs.Length() > 0){
		// xdg-open supports no passing of extra arguments, so we use delauncher directly
		// if extra arguments are given. this will fail if dragengine is installed using
		// sandbox methods like flatpak. if we want to handle this properly we have to use
		// 'xdg-mime query default application/dragengine-delga' to locate the .desktop
		// file and parse the Exec= line to get the actual command to launch.
		cmdline = "delauncher '";
		cmdline += pLauncherDirectory + pathDelga;
		cmdline += "' ";
		cmdline += pLaunchArgs;
		
	}else{
		cmdline = "xdg-open '";
		cmdline += pLauncherDirectory + pathDelga;
		cmdline += "' ";
	}
	
	returnValue = system(cmdline);
	if(WIFEXITED(returnValue)){
		switch(WEXITSTATUS(returnValue)){
		case 0: // success
		case 130: // bash: ctrl+c
			return 0;
			
		default:
			throw Exception("Failed launching");
		}
		
	}else if(WIFSIGNALED(returnValue)){
		// most probably ctrl+c
		return 1;
		
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
