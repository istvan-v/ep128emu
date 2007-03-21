# vim: syntax=python

import sys

win32CrossCompile = 0

compilerFlags = Split('''
    -Wall -W -ansi -pedantic -Wno-long-long -Wshadow -g -O2
''')

fltkConfig = 'fltk-config'

# -----------------------------------------------------------------------------

ep128emuLibEnvironment = Environment()
ep128emuLibEnvironment.Append(CCFLAGS = compilerFlags)
ep128emuLibEnvironment.Append(CPPPATH = ['.', '/usr/local/include'])
ep128emuLibEnvironment.Append(LINKFLAGS = ['-L.'])
if win32CrossCompile:
    ep128emuLibEnvironment['AR'] = 'wine D:/MinGW/bin/ar.exe'
    ep128emuLibEnvironment['CC'] = 'wine D:/MinGW/bin/gcc.exe'
    ep128emuLibEnvironment['CPP'] = 'wine D:/MinGW/bin/cpp.exe'
    ep128emuLibEnvironment['CXX'] = 'wine D:/MinGW/bin/g++.exe'
    ep128emuLibEnvironment['LINK'] = 'wine D:/MinGW/bin/g++.exe'
    ep128emuLibEnvironment['RANLIB'] = 'wine D:/MinGW/bin/ranlib.exe'
    ep128emuLibEnvironment.Append(LIBS = ['ole32', 'uuid', 'ws2_32',
                                          'gdi32', 'user32', 'kernel32'])
    ep128emuLibEnvironment.Prepend(CCFLAGS = ['-mthreads'])
    ep128emuLibEnvironment.Prepend(LINKFLAGS = ['-mthreads'])

ep128emuGUIEnvironment = ep128emuLibEnvironment.Copy()
if win32CrossCompile:
    ep128emuGUIEnvironment.Prepend(LIBS = ['fltk'])
elif not ep128emuGUIEnvironment.ParseConfig(
        '%s --cxxflags --ldflags --libs' % fltkConfig):
    print 'WARNING: could not run fltk-config'
    ep128emuGUIEnvironment.Append(LIBS = ['fltk'])

ep128emuGLGUIEnvironment = ep128emuLibEnvironment.Copy()
if win32CrossCompile:
    ep128emuGLGUIEnvironment.Prepend(LIBS = ['fltk_gl', 'glu32', 'opengl32'])
elif not ep128emuGLGUIEnvironment.ParseConfig(
        '%s --use-gl --cxxflags --ldflags --libs' % fltkConfig):
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
configure.Finish()

if not havePortAudioV19:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DUSING_OLD_PORTAUDIO_API'])
if haveDotconf:
    ep128emuLibEnvironment.Append(CCFLAGS = ['-DHAVE_DOTCONF_H'])

ep128emuGUIEnvironment['CCFLAGS'] = ep128emuLibEnvironment['CCFLAGS']
ep128emuGUIEnvironment['CXXFLAGS'] = ep128emuLibEnvironment['CXXFLAGS']
ep128emuGLGUIEnvironment['CCFLAGS'] = ep128emuLibEnvironment['CCFLAGS']
ep128emuGLGUIEnvironment['CXXFLAGS'] = ep128emuLibEnvironment['CXXFLAGS']

ep128emuLib = ep128emuLibEnvironment.StaticLibrary('ep128emu', Split('''
    bplist.cpp
    cfg_db.cpp
    display.cpp
    emucfg.cpp
    fileio.cpp
    fldisp.cpp
    gldisp.cpp
    snd_conv.cpp
    soundio.cpp
    system.cpp
    tape.cpp
    vm.cpp
    vmthread.cpp
    wd177x.cpp
'''))

# -----------------------------------------------------------------------------

ep128LibEnvironment = ep128emuLibEnvironment.Copy()
ep128LibEnvironment.Append(CPPPATH = ['./z80'])

ep128Lib = ep128LibEnvironment.StaticLibrary('ep128', Split('''
    dave.cpp
    ep128vm.cpp
    ioports.cpp
    memory.cpp
    nick.cpp
'''))

z80LibEnvironment = ep128emuLibEnvironment.Copy()
z80LibEnvironment.Append(CPPPATH = ['./z80'])

z80Lib = z80LibEnvironment.StaticLibrary('z80', Split('''
    z80/disasm.cpp
    z80/z80.cpp
    z80/z80funcs2.cpp
'''))

# -----------------------------------------------------------------------------

plus4LibEnvironment = ep128emuLibEnvironment.Copy()
plus4LibEnvironment.Append(CPPPATH = ['./plus4'])

plus4Lib = plus4LibEnvironment.StaticLibrary('plus4', Split('''
    plus4/cia8520.cpp
    plus4/cpu.cpp
    plus4/cpuoptbl.cpp
    plus4/disasm.cpp
    plus4/memory.cpp
    plus4/plus4vm.cpp
    plus4/render.cpp
    plus4/ted_api.cpp
    plus4/ted_init.cpp
    plus4/ted_main.cpp
    plus4/ted_read.cpp
    plus4/ted_write.cpp
    plus4/vc1541.cpp
    plus4/vc1581.cpp
    plus4/via6522.cpp
'''))

# -----------------------------------------------------------------------------

residLibEnvironment = plus4LibEnvironment.Copy()
residLibEnvironment.Append(CPPPATH = ['./plus4/resid'])

residLib = residLibEnvironment.StaticLibrary('resid', Split('''
    plus4/resid/envelope.cpp
    plus4/resid/extfilt.cpp
    plus4/resid/filter.cpp
    plus4/resid/pot.cpp
    plus4/resid/sid.cpp
    plus4/resid/version.cpp
    plus4/resid/voice.cpp
    plus4/resid/wave6581_PS_.cpp
    plus4/resid/wave6581_PST.cpp
    plus4/resid/wave6581_P_T.cpp
    plus4/resid/wave6581__ST.cpp
    plus4/resid/wave8580_PS_.cpp
    plus4/resid/wave8580_PST.cpp
    plus4/resid/wave8580_P_T.cpp
    plus4/resid/wave8580__ST.cpp
    plus4/resid/wave.cpp
'''))

# -----------------------------------------------------------------------------

ep128emuEnvironment = ep128emuGLGUIEnvironment.Copy()
ep128emuEnvironment.Append(CPPPATH = ['./z80', './gui'])
ep128emuEnvironment.Prepend(LIBS = ['ep128', 'z80', 'plus4', 'resid',
                                    'ep128emu'])
if haveDotconf:
    ep128emuEnvironment.Append(LIBS = ['dotconf'])
ep128emuEnvironment.Append(LIBS = ['portaudio', 'sndfile'])
if not win32CrossCompile:
    if sys.platform[:5] == 'linux':
        ep128emuEnvironment.Append(LIBS = ['jack', 'asound', 'pthread', 'rt'])
    else:
        ep128emuEnvironment.Append(LIBS = ['pthread'])
else:
    ep128emuEnvironment.Prepend(LINKFLAGS = ['-mwindows'])

Command(['gui/gui_fl.cpp', 'gui/gui_fl.hpp'], 'gui/gui.fl',
        'fluid -c -o gui/gui_fl.cpp -h gui/gui_fl.hpp $SOURCES')
Command(['gui/disk_cfg.cpp', 'gui/disk_cfg.hpp'], 'gui/disk_cfg.fl',
        'fluid -c -o gui/disk_cfg.cpp -h gui/disk_cfg.hpp $SOURCES')
Command(['gui/disp_cfg.cpp', 'gui/disp_cfg.hpp'], 'gui/disp_cfg.fl',
        'fluid -c -o gui/disp_cfg.cpp -h gui/disp_cfg.hpp $SOURCES')
Command(['gui/snd_cfg.cpp', 'gui/snd_cfg.hpp'], 'gui/snd_cfg.fl',
        'fluid -c -o gui/snd_cfg.cpp -h gui/snd_cfg.hpp $SOURCES')
Command(['gui/vm_cfg.cpp', 'gui/vm_cfg.hpp'], 'gui/vm_cfg.fl',
        'fluid -c -o gui/vm_cfg.cpp -h gui/vm_cfg.hpp $SOURCES')
Command(['gui/debug_fl.cpp', 'gui/debug_fl.hpp'], 'gui/debug.fl',
        'fluid -c -o gui/debug_fl.cpp -h gui/debug_fl.hpp $SOURCES')
Command(['gui/about_fl.cpp', 'gui/about_fl.hpp'], 'gui/about.fl',
        'fluid -c -o gui/about_fl.cpp -h gui/about_fl.hpp $SOURCES')

ep128emu = ep128emuEnvironment.Program('ep128emu', Split('''
    gui/gui.cpp
    gui/gui_fl.cpp
    gui/disk_cfg.cpp
    gui/disp_cfg.cpp
    gui/snd_cfg.cpp
    gui/vm_cfg.cpp
    gui/debug_fl.cpp
    gui/about_fl.cpp
    gui/main.cpp
'''))
Depends(ep128emu, ep128Lib)
Depends(ep128emu, z80Lib)
Depends(ep128emu, plus4Lib)
Depends(ep128emu, residLib)
Depends(ep128emu, ep128emuLib)

# -----------------------------------------------------------------------------

tapeeditEnvironment = ep128emuGUIEnvironment.Copy()
tapeeditEnvironment.Append(CPPPATH = ['./tapeutil'])
tapeeditEnvironment.Prepend(LIBS = ['ep128emu'])
if haveDotconf:
    tapeeditEnvironment.Append(LIBS = ['dotconf'])
tapeeditEnvironment.Append(LIBS = ['sndfile'])
if not win32CrossCompile:
    tapeeditEnvironment.Append(LIBS = ['pthread'])
else:
    tapeeditEnvironment.Prepend(LINKFLAGS = ['-mwindows'])

Command(['tapeutil/tapeedit.cpp', 'tapeutil/tapeedit.hpp'],
        'tapeutil/tapeedit.fl',
        'fluid -c -o tapeutil/tapeedit.cpp -h tapeutil/tapeedit.hpp $SOURCES')

tapeedit = tapeeditEnvironment.Program('tapeedit', Split('''
    tapeutil/tapeedit.cpp
    tapeutil/tapeio.cpp
'''))
Depends(tapeedit, ep128emuLib)

tapconvEnvironment = ep128emuLibEnvironment.Copy()
tapconvEnvironment.Prepend(LIBS = ['ep128emu'])
tapconv = tapconvEnvironment.Program('tapconv', ['plus4/util/tapconv.cpp'])
Depends(tapconv, ep128emuLib)

# -----------------------------------------------------------------------------

makecfgEnvironment = ep128emuGUIEnvironment.Copy()
makecfgEnvironment.Append(CPPPATH = ['./installer'])
makecfgEnvironment.Prepend(LIBS = ['ep128emu'])
if haveDotconf:
    makecfgEnvironment.Append(LIBS = ['dotconf'])
makecfgEnvironment.Append(LIBS = ['sndfile'])
if not win32CrossCompile:
    makecfgEnvironment.Append(LIBS = ['pthread'])
else:
    makecfgEnvironment.Prepend(LINKFLAGS = ['-mwindows'])

Command(['installer/mkcfg_fl.cpp', 'installer/mkcfg_fl.hpp'],
        'installer/mkcfg.fl',
        'fluid -c -o installer/mkcfg_fl.cpp -h installer/mkcfg_fl.hpp $SOURCES')

makecfg = makecfgEnvironment.Program('makecfg', Split('''
    installer/makecfg.cpp
    installer/mkcfg_fl.cpp
'''))
Depends(makecfg, ep128emuLib)

