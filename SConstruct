# vim: syntax=python

import sys

win32CrossCompile = 0
linux32CrossCompile = 0
disableSDL = 0          # set this to 1 on Linux with SDL version >= 1.2.10
disableLua = 0
enableDebug = 0
buildRelease = 0

compilerFlags = ''
if buildRelease:
    if linux32CrossCompile or win32CrossCompile:
        compilerFlags = ' -march=pentium2 -mtune=generic '
if enableDebug and not buildRelease:
    compilerFlags = ' -Wno-long-long -Wshadow -g -O2 ' + compilerFlags
    compilerFlags = ' -Wall -W -ansi -pedantic ' + compilerFlags
else:
    compilerFlags = ' -Wall -O3 ' + compilerFlags
    compilerFlags = compilerFlags + ' -fno-inline-functions '
    compilerFlags = compilerFlags + ' -fomit-frame-pointer -ffast-math '

fltkConfig = 'fltk-config'

# -----------------------------------------------------------------------------

ep128emuLibEnvironment = Environment()
if linux32CrossCompile:
    compilerFlags = ' -m32 ' + compilerFlags
ep128emuLibEnvironment.Append(CCFLAGS = Split(compilerFlags))
ep128emuLibEnvironment.Append(CPPPATH = ['.', './src', '/usr/local/include'])
if sys.platform[:6] == 'darwin':
    ep128emuLibEnvironment.Append(CPPPATH = ['/usr/X11R6/include'])
if not linux32CrossCompile:
    linkFlags = ' -L. '
else:
    linkFlags = ' -m32 -L. -L/usr/X11R6/lib '
ep128emuLibEnvironment.Append(LINKFLAGS = Split(linkFlags))
if win32CrossCompile:
    ep128emuLibEnvironment['AR'] = 'wine C:/MinGW/bin/ar.exe'
    ep128emuLibEnvironment['CC'] = 'wine C:/MinGW/bin/gcc-sjlj.exe'
    ep128emuLibEnvironment['CPP'] = 'wine C:/MinGW/bin/cpp-sjlj.exe'
    ep128emuLibEnvironment['CXX'] = 'wine C:/MinGW/bin/g++-sjlj.exe'
    ep128emuLibEnvironment['LINK'] = 'wine C:/MinGW/bin/g++-sjlj.exe'
    ep128emuLibEnvironment['RANLIB'] = 'wine C:/MinGW/bin/ranlib.exe'
    ep128emuLibEnvironment.Append(LIBS = ['ole32', 'uuid', 'ws2_32',
                                          'gdi32', 'user32', 'kernel32'])
    ep128emuLibEnvironment.Prepend(CCFLAGS = ['-mthreads'])
    ep128emuLibEnvironment.Prepend(LINKFLAGS = ['-mthreads'])

ep128emuGUIEnvironment = ep128emuLibEnvironment.Copy()
if win32CrossCompile:
    ep128emuGUIEnvironment.Prepend(LIBS = ['fltk'])
elif not ep128emuGUIEnvironment.ParseConfig(
        '%s --cxxflags --ldflags' % fltkConfig):
    print 'WARNING: could not run fltk-config'
    ep128emuGUIEnvironment.Append(LIBS = ['fltk'])

ep128emuGLGUIEnvironment = ep128emuLibEnvironment.Copy()
if win32CrossCompile:
    ep128emuGLGUIEnvironment.Prepend(LIBS = ['fltk_gl', 'fltk',
                                             'glu32', 'opengl32'])
elif not ep128emuGLGUIEnvironment.ParseConfig(
        '%s --use-gl --cxxflags --ldflags' % fltkConfig):
    print 'WARNING: could not run fltk-config'
    ep128emuGLGUIEnvironment.Append(LIBS = ['fltk_gl', 'GL'])

ep128emuLibEnvironment['CPPPATH'] = ep128emuGLGUIEnvironment['CPPPATH']

configure = ep128emuLibEnvironment.Configure()
if not configure.CheckCHeader('sndfile.h'):
    print ' *** error: libsndfile 1.0 is not found'
    Exit(-1)
if not configure.CheckCHeader('portaudio.h'):
    print ' *** error: PortAudio is not found'
    Exit(-1)
elif configure.CheckType('PaStreamCallbackTimeInfo', '#include <portaudio.h>'):
    havePortAudioV19 = 1
else:
    havePortAudioV19 = 0
    print 'WARNING: using old v18 PortAudio interface'
if not configure.CheckCXXHeader('FL/Fl.H'):
    if configure.CheckCXXHeader('/usr/include/fltk-1.1/FL/Fl.H'):
        ep128emuLibEnvironment.Append(CPPPATH = ['/usr/include/fltk-1.1'])
    else:
        print ' *** error: FLTK 1.1 is not found'
    Exit(-1)
if not configure.CheckCHeader('GL/gl.h'):
    print ' *** error: OpenGL is not found'
    Exit(-1)
haveDotconf = configure.CheckCHeader('dotconf.h')
if configure.CheckCHeader('stdint.h'):
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_STDINT_H'])
if not disableSDL:
    haveSDL = configure.CheckCHeader('SDL/SDL.h')
else:
    haveSDL = 0
if not disableLua:
    haveLua = configure.CheckCHeader('lua.h')
    haveLua = haveLua and configure.CheckCHeader('lauxlib.h')
    haveLua = haveLua and configure.CheckCHeader('lualib.h')
else:
    haveLua = 0
configure.Finish()

if not havePortAudioV19:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DUSING_OLD_PORTAUDIO_API'])
if haveDotconf:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_DOTCONF_H'])
if haveSDL:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_SDL_H'])
if haveLua:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_LUA_H'])

ep128emuGUIEnvironment['CCFLAGS'] = ep128emuLibEnvironment['CCFLAGS']
ep128emuGUIEnvironment['CXXFLAGS'] = ep128emuLibEnvironment['CXXFLAGS']
ep128emuGLGUIEnvironment['CCFLAGS'] = ep128emuLibEnvironment['CCFLAGS']
ep128emuGLGUIEnvironment['CXXFLAGS'] = ep128emuLibEnvironment['CXXFLAGS']

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

ep128emuLib = ep128emuLibEnvironment.StaticLibrary('ep128emu', Split('''
    src/bplist.cpp
    src/cfg_db.cpp
    src/debuglib.cpp
    src/display.cpp
    src/emucfg.cpp
    src/fileio.cpp
    src/fldisp.cpp
    src/gldisp.cpp
    src/guicolor.cpp
    src/joystick.cpp
    src/script.cpp
    src/snd_conv.cpp
    src/soundio.cpp
    src/system.cpp
    src/tape.cpp
    src/videorec.cpp
    src/vm.cpp
    src/vmthread.cpp
    src/wd177x.cpp
'''))

# -----------------------------------------------------------------------------

ep128LibEnvironment = ep128emuLibEnvironment.Copy()
ep128LibEnvironment.Append(CPPPATH = ['./z80'])

ep128Lib = ep128LibEnvironment.StaticLibrary('ep128', Split('''
    src/dave.cpp
    src/ep128vm.cpp
    src/ioports.cpp
    src/memory.cpp
    src/nick.cpp
'''))

z80LibEnvironment = ep128emuLibEnvironment.Copy()
z80LibEnvironment.Append(CPPPATH = ['./z80'])

z80Lib = z80LibEnvironment.StaticLibrary('z80', Split('''
    z80/z80.cpp
    z80/z80funcs2.cpp
'''))

# -----------------------------------------------------------------------------

ep128emuEnvironment = ep128emuGLGUIEnvironment.Copy()
ep128emuEnvironment.Append(CPPPATH = ['./z80', './gui'])
ep128emuEnvironment.Prepend(LIBS = ['ep128', 'z80', 'ep128emu'])
if haveDotconf:
    ep128emuEnvironment.Append(LIBS = ['dotconf'])
if haveLua:
    ep128emuEnvironment.Append(LIBS = ['lua'])
if haveSDL:
    ep128emuEnvironment.Append(LIBS = ['SDL'])
ep128emuEnvironment.Append(LIBS = ['portaudio', 'sndfile'])
if not win32CrossCompile:
    if sys.platform[:5] == 'linux':
        if not buildRelease:
            ep128emuEnvironment.Append(LIBS = ['jack'])
        ep128emuEnvironment.Append(LIBS = ['asound', 'pthread', 'rt'])
    else:
        ep128emuEnvironment.Append(LIBS = ['pthread'])
else:
    ep128emuEnvironment.Prepend(LINKFLAGS = ['-mwindows'])

ep128emu = ep128emuEnvironment.Program('ep128emu',
    ['gui/gui.cpp']
    + fluidCompile(['gui/gui.fl', 'gui/disk_cfg.fl', 'gui/disp_cfg.fl',
                    'gui/kbd_cfg.fl', 'gui/snd_cfg.fl', 'gui/vm_cfg.fl',
                    'gui/debug.fl', 'gui/about.fl'])
    + ['gui/debugger.cpp', 'gui/main.cpp', 'gui/monitor.cpp'])
Depends(ep128emu, ep128Lib)
Depends(ep128emu, z80Lib)
Depends(ep128emu, ep128emuLib)

if sys.platform[:6] == 'darwin':
    Command('ep128emu.app/Contents/MacOS/ep128emu', 'ep128emu',
            'mkdir -p ep128emu.app/Contents/MacOS ; cp -pf $SOURCES $TARGET')

# -----------------------------------------------------------------------------

tapeeditEnvironment = ep128emuGUIEnvironment.Copy()
tapeeditEnvironment.Append(CPPPATH = ['./tapeutil'])
tapeeditEnvironment.Prepend(LIBS = ['ep128emu'])
if haveDotconf:
    tapeeditEnvironment.Append(LIBS = ['dotconf'])
if haveSDL:
    tapeeditEnvironment.Append(LIBS = ['SDL'])
tapeeditEnvironment.Append(LIBS = ['sndfile'])
if not win32CrossCompile:
    tapeeditEnvironment.Append(LIBS = ['pthread'])
else:
    tapeeditEnvironment.Prepend(LINKFLAGS = ['-mwindows'])

tapeedit = tapeeditEnvironment.Program('tapeedit',
    fluidCompile(['tapeutil/tapeedit.fl']) + ['tapeutil/tapeio.cpp'])
Depends(tapeedit, ep128emuLib)

if sys.platform[:6] == 'darwin':
    Command('ep128emu.app/Contents/MacOS/tapeedit', 'tapeedit',
            'mkdir -p ep128emu.app/Contents/MacOS ; cp -pf $SOURCES $TARGET')

# -----------------------------------------------------------------------------

makecfgEnvironment = ep128emuGUIEnvironment.Copy()
makecfgEnvironment.Append(CPPPATH = ['./installer'])
makecfgEnvironment.Prepend(LIBS = ['ep128emu'])
if haveDotconf:
    makecfgEnvironment.Append(LIBS = ['dotconf'])
if haveSDL:
    makecfgEnvironment.Append(LIBS = ['SDL'])
makecfgEnvironment.Append(LIBS = ['sndfile'])
if not win32CrossCompile:
    makecfgEnvironment.Append(LIBS = ['pthread'])
else:
    makecfgEnvironment.Prepend(LINKFLAGS = ['-mwindows'])

makecfg = makecfgEnvironment.Program('makecfg',
    ['installer/makecfg.cpp'] + fluidCompile(['installer/mkcfg.fl']))
Depends(makecfg, ep128emuLib)

if sys.platform[:6] == 'darwin':
    Command('ep128emu.app/Contents/MacOS/makecfg', 'makecfg',
            'mkdir -p ep128emu.app/Contents/MacOS ; cp -pf $SOURCES $TARGET')

