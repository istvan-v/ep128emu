# vim: syntax=python

import sys, os

win64CrossCompile = int(ARGUMENTS.get('win64', 0))
mingwCrossCompile = win64CrossCompile or int(ARGUMENTS.get('win32', 0))
linux32CrossCompile = int(ARGUMENTS.get('linux32', 0))
# on Linux, statically linked SDL version >= 1.2.10 that was not built
# without video support (--disable-video) breaks FLTK
disableSDL = int(ARGUMENTS.get('nosdl', 0))
disableLua = int(ARGUMENTS.get('nolua', 0))
buildUtilities = int(ARGUMENTS.get('utils', 1))
enableGLShaders = int(ARGUMENTS.get('glshaders', 1))
enableDebug = int(ARGUMENTS.get('debug', 0))
buildRelease = not enableDebug and int(ARGUMENTS.get('release', 1))
# for mingwCrossCompile, use LuaJIT instead of Lua 5.3
useLuaJIT = int(ARGUMENTS.get('luajit', 0))
cmosZ80 = int(ARGUMENTS.get('z80cmos', 0))
# build with experimental SD card emulation
enableSDExt = int(ARGUMENTS.get('sdext', 1))
# use cURL library in makecfg to download the ROM package
enableCURL = int(ARGUMENTS.get('curl', int(not mingwCrossCompile)))
userFlags = ARGUMENTS.get('cflags', '')
disablePkgConfig = int(ARGUMENTS.get('nopkgconfig',
                                     int(linux32CrossCompile or \
                                         mingwCrossCompile)))
enableBuildCache = int(ARGUMENTS.get('cache', 0))

compilerFlags = ''
if buildRelease:
    if linux32CrossCompile or (mingwCrossCompile and not win64CrossCompile):
        compilerFlags = ' -march=pentium2 -mtune=generic '
if enableDebug and not buildRelease:
    compilerFlags = ' -Wno-long-long -Wshadow -g -O0 ' + compilerFlags
    compilerFlags = ' -Wall -W -ansi -pedantic ' + compilerFlags
else:
    compilerFlags = ' -Wall -O3 ' + compilerFlags
    compilerFlags = compilerFlags + ' -fno-inline-functions '
    compilerFlags = compilerFlags + ' -fomit-frame-pointer -ffast-math '

# -----------------------------------------------------------------------------

# pkgname : [ pkgconfig, [ package_names ],
#             linux_flags, mingw_flags, c_header, cxx_header, optional ]

fltkLibsLinux = '-lfltk -lfltk_images -lfltk_jpeg -lfltk_png'
fltkLibsMinGW = fltkLibsLinux + ' -lz -lcomdlg32 -lcomctl32 -lole32'
fltkLibsMinGW = fltkLibsMinGW + ' -luuid -lws2_32 -lwinmm -lgdi32'
fltkLibsLinux = fltkLibsLinux + ' -lfltk_z -lXinerama -lXext -lXft'
fltkLibsLinux = fltkLibsLinux + ' -lXfixes -lX11 -lfontconfig -ldl'

packageConfigs = {
    'FLTK' : [
        'fltk-config --use-images --cflags --cxxflags --ldflags', [''],
        fltkLibsLinux, fltkLibsMinGW, '', 'FL/Fl.H', 0 ],
    'FLTK-GL' : [
        'fltk-config --use-gl --use-images --cflags --cxxflags --ldflags', [''],
        '-lfltk_gl ' + fltkLibsLinux + ' -lGL',
        '-lfltk_gl ' + fltkLibsMinGW + ' -lopengl32',
        '', 'FL/Fl_Gl_Window.H', 1 ],
    'sndfile' : [
        'pkg-config --silence-errors --cflags --libs',
        ['sndfile'],
        '-lsndfile', '-lsndfile', 'sndfile.h', '', 0 ],
    'PortAudio' : [
        'pkg-config --silence-errors --cflags --libs',
        ['portaudio-2.0', 'portaudio2', 'portaudio'],
        '-lportaudio -lasound -lm -ldl -lpthread -lrt', '-lportaudio -lwinmm',
        'portaudio.h', '', 0 ],
    'Lua' : [
        'pkg-config --silence-errors --cflags --libs',
        ['lua-5.3', 'lua53', 'lua-5.2', 'lua52', 'lua-5.1', 'lua51', 'lua'],
        '-llua', '', 'lua.h', '', 1 ],
    'SDL' : [
        'pkg-config --silence-errors --cflags --libs',
        ['sdl', 'sdl1'],
        '-lSDL', '-lSDL -lwinmm', 'SDL/SDL.h', '', 1],
    'cURL' : [
        'pkg-config --silence-errors --cflags --libs',
        ['libcurl', 'curl'],
        '-lcurl -lssl -lcrypto', '-lcurldll',
        'curl/curl.h', '', 1]
}

def configurePackage(env, pkgName):
    global packageConfigs, disablePkgConfig
    global linux32CrossCompile, mingwCrossCompile, win64CrossCompile
    if not disablePkgConfig:
        for s in packageConfigs[pkgName][1]:
            if not s:
                print 'Checking for package ' + pkgName + '...',
                # hack to work around fltk-config adding unwanted compiler flags
                savedCFlags = env['CCFLAGS']
                savedCXXFlags = env['CXXFLAGS']
            else:
                print 'Checking for package ' + s + '...',
                s = ' ' + s
            try:
                if not env.ParseConfig(packageConfigs[pkgName][0] + s):
                    raise Exception()
                print 'yes'
                if not s:
                    env['CCFLAGS'] = savedCFlags
                    env['CXXFLAGS'] = savedCXXFlags
                return 1
            except:
                print 'no'
                continue
        pkgFound = 0
    else:
        configure = env.Configure()
        if packageConfigs[pkgName][4]:
            pkgFound = configure.CheckCHeader(packageConfigs[pkgName][4])
        else:
            pkgFound = configure.CheckCXXHeader(packageConfigs[pkgName][5])
        configure.Finish()
        if pkgFound:
            env.MergeFlags(
                packageConfigs[pkgName][2 + int(bool(mingwCrossCompile))])
    if not pkgFound:
        if not packageConfigs[pkgName][6]:
            print ' *** error configuring ' + pkgName
            Exit(-1)
        print 'WARNING: package ' + pkgName + ' not found'
        return 0
    return 1

if enableBuildCache:
    CacheDir("./.build_cache")

programNamePrefix = ""
buildingLinuxPackage = 0
if not mingwCrossCompile:
    if sys.platform[:5] == 'linux':
        try:
            instPrefix = os.environ["UB_INSTALLDIR"]
            if instPrefix:
                instPrefix += "/usr"
                buildingLinuxPackage = 1
        except:
            pass
    if not buildingLinuxPackage:
        instPrefix = os.environ["HOME"]
        instShareDir = instPrefix + "/.local/share"
    else:
        instShareDir = instPrefix + "/share"
    instBinDir = instPrefix + "/bin"
    instDataDir = instShareDir + "/ep128emu"
    instPixmapDir = instShareDir + "/pixmaps"
    instDesktopDir = instShareDir + "/applications"
    instROMDir = instDataDir + "/roms"
    instConfDir = instDataDir + "/config"
    instDiskDir = instDataDir + "/disk"
    programNamePrefix = "ep"

oldSConsVersion = 0
try:
    EnsureSConsVersion(0, 97)
except:
    print 'WARNING: using old SCons version'
    oldSConsVersion = 1

def copyEnvironment(env):
    if oldSConsVersion:
        return env.Copy()
    return env.Clone()

ep128emuLibEnvironment = Environment(ENV = { 'PATH' : os.environ['PATH'],
                                             'HOME' : os.environ['HOME'] })
if linux32CrossCompile:
    compilerFlags = ' -m32 ' + compilerFlags
ep128emuLibEnvironment.Append(CCFLAGS = Split(compilerFlags))
ep128emuLibEnvironment.Append(CPPPATH = ['.', './src'])
if userFlags:
    ep128emuLibEnvironment.MergeFlags(userFlags)
if not mingwCrossCompile:
    ep128emuLibEnvironment.Append(CPPPATH = ['/usr/local/include'])
if sys.platform[:6] == 'darwin':
    ep128emuLibEnvironment.Append(CPPPATH = ['/usr/X11R6/include'])
if not linux32CrossCompile:
    linkFlags = ' -L. '
else:
    linkFlags = ' -m32 -L. -L/usr/X11R6/lib '
ep128emuLibEnvironment.Append(LINKFLAGS = Split(linkFlags))
if mingwCrossCompile:
    wordSize = ['32', '64'][int(bool(win64CrossCompile))]
    mingwPrefix = 'C:/mingw' + wordSize
    ep128emuLibEnvironment.Prepend(CCFLAGS = ['-m' + wordSize])
    ep128emuLibEnvironment.Prepend(LINKFLAGS = ['-m' + wordSize])
    ep128emuLibEnvironment.Append(CPPPATH = [mingwPrefix + '/include'])
    if sys.platform[:3] == 'win':
        toolNamePrefix = ''
    elif win64CrossCompile:
        toolNamePrefix = 'x86_64-w64-mingw32-'
    else:
        toolNamePrefix = 'i686-w64-mingw32-'
    ep128emuLibEnvironment['AR'] = toolNamePrefix + 'ar'
    ep128emuLibEnvironment['CC'] = toolNamePrefix + 'gcc'
    ep128emuLibEnvironment['CPP'] = toolNamePrefix + 'cpp'
    ep128emuLibEnvironment['CXX'] = toolNamePrefix + 'g++'
    ep128emuLibEnvironment['LINK'] = toolNamePrefix + 'g++'
    ep128emuLibEnvironment['RANLIB'] = toolNamePrefix + 'ranlib'
    ep128emuLibEnvironment['PROGSUFFIX'] = '.exe'
    packageConfigs['Lua'][3] = '-llua' + ['53', '51'][int(bool(useLuaJIT))]
    ep128emuLibEnvironment.Append(
        CPPPATH = [mingwPrefix
                   + '/include/lua' + ['5.3', '5.1'][int(bool(useLuaJIT))]])
    ep128emuLibEnvironment.Append(LIBS = ['user32', 'kernel32'])
    ep128emuLibEnvironment.Prepend(CCFLAGS = ['-mthreads'])
    ep128emuLibEnvironment.Prepend(LINKFLAGS = ['-mthreads'])
if buildRelease:
    ep128emuLibEnvironment.Append(LINKFLAGS = ['-s'])

ep128emuGUIEnvironment = copyEnvironment(ep128emuLibEnvironment)
if mingwCrossCompile:
    ep128emuGUIEnvironment.Prepend(LINKFLAGS = ['-mwindows'])
else:
    ep128emuGUIEnvironment.Append(LIBS = ['pthread'])
configurePackage(ep128emuGUIEnvironment, 'FLTK')
makecfgEnvironment = copyEnvironment(ep128emuGUIEnvironment)
configurePackage(ep128emuGUIEnvironment, 'sndfile')
tapeeditEnvironment = copyEnvironment(ep128emuGUIEnvironment)
haveLua = 0
haveSDL = 0
if not disableLua:
    haveLua = configurePackage(ep128emuGUIEnvironment, 'Lua')
if not disableSDL:
    haveSDL = configurePackage(ep128emuGUIEnvironment, 'SDL')
configurePackage(ep128emuGUIEnvironment, 'PortAudio')

ep128emuGLGUIEnvironment = copyEnvironment(ep128emuGUIEnvironment)
disableOpenGL = 1
if configurePackage(ep128emuGLGUIEnvironment, 'FLTK-GL'):
    configure = ep128emuGLGUIEnvironment.Configure()
    if configure.CheckCHeader('GL/gl.h'):
        disableOpenGL = 0
        if enableGLShaders:
            if not configure.CheckType('PFNGLCOMPILESHADERPROC',
                                       '#include <GL/gl.h>\n'
                                       + '#include <GL/glext.h>'):
                print 'WARNING: disabling GL shader support'
                enableGLShaders = 0
    configure.Finish()
if sys.platform[:5] == 'linux' and not mingwCrossCompile:
    ep128emuGUIEnvironment.Append(LIBS = ['X11'])
    ep128emuGLGUIEnvironment.Append(LIBS = ['GL', 'X11'])
if disableOpenGL:
    print 'WARNING: OpenGL is not found, only software video will be supported'
    enableGLShaders = 0
    ep128emuGLGUIEnvironment = copyEnvironment(ep128emuGUIEnvironment)
    ep128emuGLGUIEnvironment.Append(CCFLAGS = ['-DDISABLE_OPENGL_DISPLAY'])

ep128emuLibEnvironment['CPPPATH'] = ep128emuGLGUIEnvironment['CPPPATH']

imageLibTestProgram = '''
    #include <FL/Fl.H>
    #include <FL/Fl_Shared_Image.H>
    int main()
    {
      Fl_Shared_Image *tmp = Fl_Shared_Image::get("foo");
      tmp->release();
      return 0;
    }
'''

def imageLibTest(env1, env2, env3, env4):
    # remove unneeded and possibly conflicting libraries added by fltk-config
    for libName in ['jpeg', 'png']:
        if libName in env1['LIBS']:
            tmpEnv = copyEnvironment(env1)
            tmpEnv['LIBS'].remove(libName)
            tmpConfig = tmpEnv.Configure()
            if tmpConfig.TryLink(imageLibTestProgram, '.cpp'):
                env1['LIBS'].remove(libName)
                env2['LIBS'].remove(libName)
                env3['LIBS'].remove(libName)
                env4['LIBS'].remove(libName)
            tmpConfig.Finish()

if not oldSConsVersion:
    imageLibTest(makecfgEnvironment, tapeeditEnvironment,
                 ep128emuGUIEnvironment, ep128emuGLGUIEnvironment)

configure = ep128emuLibEnvironment.Configure()
if configure.CheckType('PaStreamCallbackTimeInfo', '#include <portaudio.h>'):
    havePortAudioV19 = 1
else:
    havePortAudioV19 = 0
    print 'WARNING: using old v18 PortAudio interface'
fltkVersion13 = 0
if configure.CheckCXXHeader('FL/Fl_Cairo.H'):
    fltkVersion13 = 1
else:
    ep128emuLibEnvironment.Append(CPPPATH = ['./Fl_Native_File_Chooser'])
    ep128emuGUIEnvironment.Append(CPPPATH = ['./Fl_Native_File_Chooser'])
    ep128emuGLGUIEnvironment.Append(CPPPATH = ['./Fl_Native_File_Chooser'])
if configure.CheckCHeader('stdint.h'):
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_STDINT_H'])
if sys.platform[:5] == 'linux' and not mingwCrossCompile:
    if configure.CheckCHeader('linux/fd.h'):
        ep128emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_LINUX_FD_H'])

oldLuaVersion = 0
if haveLua:
    if not configure.CheckType('lua_Integer',
                               '#include <lua.h>\n#include <lauxlib.h>'):
        oldLuaVersion = 1
        print 'WARNING: using old Lua 5.0.x API'
configure.Finish()

if not havePortAudioV19:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DUSING_OLD_PORTAUDIO_API'])
if haveSDL:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_SDL_H'])
if haveLua:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_LUA_H'])
    if oldLuaVersion:
        ep128emuLibEnvironment.Append(CCFLAGS = ['-DUSING_OLD_LUA_API'])
if enableGLShaders:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DENABLE_GL_SHADERS'])
if not fltkVersion13:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DFLTK1'])
if cmosZ80:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DZ80_ENABLE_CMOS'])
if enableSDExt:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DENABLE_SDEXT'])

ep128emuGUIEnvironment.MergeFlags(ep128emuLibEnvironment['CCFLAGS'])
ep128emuGLGUIEnvironment.MergeFlags(ep128emuLibEnvironment['CCFLAGS'])
makecfgEnvironment.MergeFlags(ep128emuLibEnvironment['CCFLAGS'])
tapeeditEnvironment.MergeFlags(ep128emuLibEnvironment['CCFLAGS'])

def fluidCompile(flNames):
    cppNames = []
    for flName in flNames:
        if flName.endswith('.fl'):
            cppName = flName[:-3] + '_fl.cpp'
            hppName = flName[:-3] + '_fl.hpp'
            Command([cppName, hppName], flName,
                    'fluid -c -o %s -h %s $SOURCES' % (cppName, hppName))
            cppNames += [cppName]
    return cppNames

ep128emuLibSources = Split('''
    src/bplist.cpp
    src/cfg_db.cpp
    src/compress.cpp
    src/comprlib.cpp
    src/debuglib.cpp
    src/decompm2.cpp
    src/display.cpp
    src/dotconf.c
    src/emucfg.cpp
    src/ep_fdd.cpp
    src/fileio.cpp
    src/fldisp.cpp
    src/gldisp.cpp
    src/guicolor.cpp
    src/joystick.cpp
    src/pngwrite.cpp
    src/script.cpp
    src/snd_conv.cpp
    src/soundio.cpp
    src/system.cpp
    src/tape.cpp
    src/videorec.cpp
    src/vm.cpp
    src/vmthread.cpp
    src/wd177x.cpp
''')
if not fltkVersion13:
    ep128emuLibSources += ['Fl_Native_File_Chooser/Fl_Native_File_Chooser.cxx']
if disableOpenGL:
    ep128emuLibSources.remove('src/gldisp.cpp')
ep128emuLib = ep128emuLibEnvironment.StaticLibrary('ep128emu',
                                                   ep128emuLibSources)

# -----------------------------------------------------------------------------

ep128LibEnvironment = copyEnvironment(ep128emuLibEnvironment)
ep128LibEnvironment.Append(CPPPATH = ['./z80'])
sdextSources = []
if enableSDExt:
    sdextSources = ['src/sdext.cpp']

ep128Lib = ep128LibEnvironment.StaticLibrary('ep128', Split('''
    src/dave.cpp
    src/ep128vm.cpp
    src/ioports.cpp
    src/memory.cpp
    src/nick.cpp
    z80/z80.cpp
    z80/z80funcs2.cpp
    src/epmemcfg.cpp
    src/ide.cpp
    src/snapshot.cpp
''') + sdextSources)

# -----------------------------------------------------------------------------

zx128LibEnvironment = copyEnvironment(ep128emuLibEnvironment)
zx128LibEnvironment.Append(CPPPATH = ['./z80'])

zx128Lib = zx128LibEnvironment.StaticLibrary('zx128', Split('''
    src/ay3_8912.cpp
    src/zx128vm.cpp
    src/zxioport.cpp
    src/zxmemory.cpp
    src/ula.cpp
    src/zx_snap.cpp
'''))

# -----------------------------------------------------------------------------

cpc464LibEnvironment = copyEnvironment(ep128emuLibEnvironment)
cpc464LibEnvironment.Append(CPPPATH = ['./z80'])

cpc464Lib = cpc464LibEnvironment.StaticLibrary('cpc464', Split('''
    src/cpc464vm.cpp
    src/cpcio.cpp
    src/cpcmem.cpp
    src/crtc6845.cpp
    src/cpcvideo.cpp
    src/fdc765.cpp
    src/cpcdisk.cpp
    src/cpc_snap.cpp
'''))

# -----------------------------------------------------------------------------

tvc64LibEnvironment = copyEnvironment(ep128emuLibEnvironment)
tvc64LibEnvironment.Append(CPPPATH = ['./z80'])

tvc64Lib = tvc64LibEnvironment.StaticLibrary('tvc64', Split('''
    src/tvc64vm.cpp
    src/tvcmem.cpp
    src/tvcvideo.cpp
    src/tvc_snap.cpp
'''))

# -----------------------------------------------------------------------------

ep128emuEnvironment = copyEnvironment(ep128emuGLGUIEnvironment)
ep128emuEnvironment.Append(CPPPATH = ['./z80', './gui'])
if haveLua and oldLuaVersion:
    ep128emuEnvironment.Append(LIBS = ['lualib'])
ep128emuEnvironment.Prepend(LIBS = ['ep128', 'zx128', 'cpc464', 'tvc64',
                                    'ep128emu'])

ep128emuSources = ['gui/gui.cpp']
ep128emuSources += fluidCompile(['gui/gui.fl', 'gui/disk_cfg.fl',
                                 'gui/disp_cfg.fl', 'gui/kbd_cfg.fl',
                                 'gui/snd_cfg.fl', 'gui/vm_cfg.fl',
                                 'gui/debug.fl', 'gui/about.fl'])
ep128emuSources += ['gui/debugger.cpp', 'gui/monitor.cpp', 'gui/main.cpp']
if mingwCrossCompile:
    ep128emuResourceObject = ep128emuEnvironment.Command(
        'resource/resource.o',
        ['resource/ep128emu.rc', 'resource/cpc464emu.ico',
         'resource/ep128emu.ico', 'resource/tvc64emu.ico',
         'resource/zx128emu.ico'],
        toolNamePrefix + 'windres -v --use-temp-file '
        + '--preprocessor="gcc.exe -E -xc -DRC_INVOKED" '
        + '-o $TARGET resource/ep128emu.rc')
    ep128emuSources += [ep128emuResourceObject]
ep128emu = ep128emuEnvironment.Program('ep128emu', ep128emuSources)
Depends(ep128emu, ep128Lib)
Depends(ep128emu, zx128Lib)
Depends(ep128emu, cpc464Lib)
Depends(ep128emu, tvc64Lib)
Depends(ep128emu, ep128emuLib)

if sys.platform[:6] == 'darwin':
    Command('ep128emu.app/Contents/MacOS/ep128emu', 'ep128emu',
            'mkdir -p ep128emu.app/Contents/MacOS ; cp -pf $SOURCES $TARGET')

# -----------------------------------------------------------------------------

tapeeditEnvironment.Append(CPPPATH = ['./tapeutil'])
tapeeditEnvironment.Prepend(LIBS = ['ep128emu'])
tapeeditSources = fluidCompile(['tapeutil/tapeedit.fl'])
tapeeditSources += ['tapeutil/tapeio.cpp']
if mingwCrossCompile:
    tapeeditResourceObject = tapeeditEnvironment.Command(
        'resource/te_resrc.o',
        ['resource/tapeedit.rc', 'resource/tapeedit.ico'],
        toolNamePrefix + 'windres -v --use-temp-file '
        + '--preprocessor="gcc.exe -E -xc -DRC_INVOKED" '
        + '-o $TARGET resource/tapeedit.rc')
    tapeeditSources += [tapeeditResourceObject]
tapeedit = tapeeditEnvironment.Program('tapeedit', tapeeditSources)
Depends(tapeedit, ep128emuLib)

if sys.platform[:6] == 'darwin':
    Command('ep128emu.app/Contents/MacOS/tapeedit', 'tapeedit',
            'mkdir -p ep128emu.app/Contents/MacOS ; cp -pf $SOURCES $TARGET')

# -----------------------------------------------------------------------------

if buildUtilities:
    compressLibEnvironment = copyEnvironment(ep128emuLibEnvironment)
    compressLibEnvironment.Append(CPPPATH = ['./util/epcompress/src'])
    compressLibEnvironment.Prepend(LIBS = [ep128emuLib])
    compressLib = compressLibEnvironment.StaticLibrary(
                      'epcompress', Split('''
                          util/epcompress/src/archive.cpp
                          util/epcompress/src/compress0.cpp
                          util/epcompress/src/compress2.cpp
                          util/epcompress/src/compress3.cpp
                          util/epcompress/src/compress5.cpp
                          util/epcompress/src/compress.cpp
                          util/epcompress/src/decompress0.cpp
                          util/epcompress/src/decompress2.cpp
                          util/epcompress/src/decompress3.cpp
                          util/epcompress/src/decompress5.cpp
                          util/epcompress/src/sfxcode.cpp
                          util/epcompress/src/sfxdecomp.cpp
                      '''))
    Depends(compressLib, ep128emuLib)
    epcompressEnvironment = copyEnvironment(compressLibEnvironment)
    epcompressEnvironment.Prepend(LIBS = [compressLib])
    if not mingwCrossCompile:
        epcompressEnvironment.Append(LIBS = ['pthread'])
    epcompress = epcompressEnvironment.Program(
                     'epcompress', ['util/epcompress/src/main.cpp'])
    Depends(epcompress, compressLib)
    dtfEnvironment = copyEnvironment(epcompressEnvironment)
    dtf = dtfEnvironment.Program('dtf', ['util/dtf/dtf.cpp'])
    Depends(dtf, compressLib)
    epimgconvEnvironment = copyEnvironment(makecfgEnvironment)
    epimgconvEnvironment.Append(CPPPATH = ['./util/epcompress/src'])
    epimgconvEnvironment.Prepend(LIBS = [compressLib, ep128emuLib])
    if mingwCrossCompile:
        epimgconvEnvironment['LINKFLAGS'].remove('-mwindows')
    epimgconv = epimgconvEnvironment.Program(
                    'epimgconv', Split('''
                        util/epimgconv/src/attr16.cpp
                        util/epimgconv/src/epimgconv.cpp
                        util/epimgconv/src/imageconv.cpp
                        util/epimgconv/src/img_cfg.cpp
                        util/epimgconv/src/imgwrite.cpp
                        util/epimgconv/src/main.cpp
                        util/epimgconv/src/pixel16_1.cpp
                        util/epimgconv/src/pixel16_2.cpp
                        util/epimgconv/src/pixel256.cpp
                        util/epimgconv/src/pixel2.cpp
                        util/epimgconv/src/pixel4.cpp
                        util/epimgconv/src/tvc_16.cpp
                        util/epimgconv/src/tvc_2.cpp
                        util/epimgconv/src/tvc_4.cpp
                    '''))
    Depends(epimgconv, compressLib)
    Depends(epimgconv, ep128emuLib)

# -----------------------------------------------------------------------------

makecfgEnvironment.Append(CPPPATH = ['./installer'])
makecfgEnvironment.Prepend(LIBS = ['ep128emu'])
if enableCURL:
    if configurePackage(makecfgEnvironment, 'cURL'):
        makecfgEnvironment.Append(CCFLAGS = ['-DMAKECFG_USE_CURL'])
if not mingwCrossCompile:
    if sys.platform[:5] == 'linux':
        for envName in ['DISPLAY', 'XAUTHORITY']:
            if envName in os.environ:
                makecfgEnvironment.Append(ENV = {
                                              envName : os.environ[envName] })

makecfg = makecfgEnvironment.Program(programNamePrefix + 'makecfg',
    ['installer/makecfg.cpp'] + fluidCompile(['installer/mkcfg.fl']))
Depends(makecfg, ep128emuLib)

if sys.platform[:6] == 'darwin':
    Command('ep128emu.app/Contents/MacOS/makecfg', 'makecfg',
            'mkdir -p ep128emu.app/Contents/MacOS ; cp -pf $SOURCES $TARGET')

# -----------------------------------------------------------------------------

if not mingwCrossCompile:
    makecfgEnvironment.Install(instBinDir,
                               [ep128emu, tapeedit, makecfg])
    for prgName in [instBinDir + "/zx128emu", instBinDir + "/cpc464emu",
                    instBinDir + "/tvc64emu"]:
        makecfgEnvironment.Command(prgName, ep128emu,
                                   ['ln -s -f ep128emu "' + prgName + '"'])
    if buildUtilities:
        makecfgEnvironment.Install(instBinDir, [dtf, epcompress, epimgconv])
    makecfgEnvironment.Install(instPixmapDir,
                               ["resource/cpc464emu.png",
                                "resource/ep128emu.png",
                                "resource/tapeedit.png",
                                "resource/tvc64emu.png",
                                "resource/zx128emu.png"])
    makecfgEnvironment.Install(instDesktopDir,
                               ["resource/cpc464emu.desktop",
                                "resource/ep128.desktop",
                                "resource/eptapeedit.desktop",
                                "resource/tvc64emu.desktop",
                                "resource/zx128.desktop"])
    if not buildingLinuxPackage:
        confFileList = [instConfDir + '/EP_Keyboard_HU.cfg',
                        instConfDir + '/EP_Keyboard_US.cfg']
        confFiles = 0
        f = open("./installer/makecfg.cpp")
        for l in f:
          if not confFiles:
            confFiles = "machineConfigs" in l
          elif "};" in l:
            confFiles = None
            break
          elif '"' in l:
            confFileList += [instConfDir + '/'
                             + l[l.find('"') + 1:l.rfind('"')]]
        f.close()
        f = None
        makecfgEnvironment.Command(
            [instROMDir] + [confFileList], [makecfg],
            ['./' + programNamePrefix + 'makecfg -'
             + ['f "', 'c "'][int('DISPLAY' in makecfgEnvironment['ENV'])]
             + instDataDir + '"'])
    else:
        makecfgEnvironment.Command(instROMDir, None, [])
    makecfgEnvironment.Install(instConfDir,
                               ["config/clearkbd.cfg", "config/ep_keys.cfg"])
    makecfgEnvironment.Install(instDiskDir,
                               ["disk/disk.zip", "disk/ide126m.vhd.bz2",
                                "disk/ide189m.vhd.bz2"])
    makecfgEnvironment.Alias("install",
                             [instBinDir, instPixmapDir, instDesktopDir,
                              instDataDir, instROMDir, instConfDir,
                              instDiskDir])

