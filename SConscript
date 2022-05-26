import requests
import os
import shutil

env = Environment()

baseUrl = 'https://github.com/lordofdragons/dragengine/releases'
#engineVersion = requests.get('{}/latest'.format(baseUrl)).url.split('/')[-1][1:]
#print('Latest Drag[en]gine Release Version: {}'.format(engineVersion))

params = Variables(['custom.py']) 
params.Add(StringVariable('version', 'Version', '9999'))
params.Update(env)

distribution = []

pathMSStore = 'distribution/content/microsoft_appstore/VFS/ProgramFilesX64/MyGame'
distribution.extend(env.Install(pathMSStore, 'windows/build/Launcher64.exe'))
distribution.extend(env.Install(pathMSStore, 'data/Launcher.ini'))

pathSteamWorks = 'distribution/content/steamworks'
distribution.extend(env.Install(pathSteamWorks, 'windows_direct/build/Launcher64.exe'))
distribution.extend(env.Install(pathSteamWorks, 'linux/build/launcher64'))
distribution.extend(env.Install(pathSteamWorks, 'data/Launcher.ini'))

def downloadFile(env, target, source):
	path = target[0].abspath
	with open(path, 'wb') as f:
		f.write(requests.get(env['URL']).content)
	if 'SET_EXECUTABLE_BIT' in env and env['SET_EXECUTABLE_BIT']:
		os.chmod(path, 0o755)

def copyWindowsInstallerScript(env, target, source):
	with open(source[0].abspath, 'r') as f:
		content = f.read()
	
	content = content.replace('{VERSION}', env['version'])
	
	with open(target[0].abspath, 'w') as f:
		f.write(content)

def copyReadme(env, target, source):
	with open(source[0].abspath, 'r') as f:
		content = f.read()
	
	content = content.replace('{VERSION}', env['version'])
	
	with open(target[0].abspath, 'w') as f:
		f.write(content)

def createZipFile(env, target, source):
	shutil.make_archive(target[0].abspath, 'zip', source[0].abspath)

distribution.extend(env.Command(
	'{}/install-dragengine-{}-windows64.exe'.format(pathSteamWorks, env['version']),
	'data/Launcher.ini',
	env.Action(downloadFile, 'Download Windows Installer'),
	URL='{0}/download/v{1}/install-dragengine-{1}-windows64.exe'.format(baseUrl, env['version'])))

distribution.extend(env.Command(
	'{}/install-dragengine-{}-linux64.sh'.format(pathSteamWorks, env['version']),
	'data/Launcher.ini',
	env.Action(downloadFile, 'Download Linux Installer'),
	URL='{0}/download/v{1}/install-dragengine-{1}-linux64.sh'.format(baseUrl, env['version']),
	SET_EXECUTABLE_BIT=True))

distribution.extend(env.Command(
	'{}/installscript-dragengine-{}.vdf'.format(pathSteamWorks, env['version']),
	'data/installscript-dragengine.vdf',
	env.Action(copyWindowsInstallerScript, 'SteamWorks Windows Install Script')))

distribution.extend(env.Command(
	'distribution/content/README.md'.format(env['version']),
	'README.md',
	env.Action(copyReadme, 'ReadMe')))

archive = env.Command('distribution/distribute-delga-{}'.format(env['version']),
	'distribution/content', env.Action(createZipFile, 'Archive Distribution'))
env.Depends(archive, distribution)

Default(env.Alias('distribution', archive))
