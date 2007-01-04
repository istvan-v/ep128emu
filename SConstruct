# vim: syntax=python

compilerFlags = Split('''
    -Wall -W -ansi -pedantic -Wno-long-long -Wshadow -g -O2
''')

fltkConfig = 'fltk-config'

# -----------------------------------------------------------------------------

ep128emuLibEnvironment = Environment()
ep128emuLibEnvironment.Append(CCFLAGS = compilerFlags)
ep128emuLibEnvironment.Append(CPPPATH = ['.', '/usr/local/include'])
ep128emuLibEnvironment.Append(LINKFLAGS = ['-L.'])

ep128emuGUIEnvironment = ep128emuLibEnvironment.Copy()
if not ep128emuGUIEnvironment.ParseConfig(
        '%s --cxxflags --ldflags --libs' % fltkConfig):
    print 'WARNING: could not run fltk-config'
    ep128emuGUIEnvironment.Append(LIBS = ['fltk'])

ep128emuGLGUIEnvironment = ep128emuLibEnvironment.Copy()
if not ep128emuGLGUIEnvironment.ParseConfig(
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
    gldisp.cpp
    snd_conv.cpp
    soundio.cpp
    system.cpp
    tape.cpp
    vm.cpp
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
'''))

# -----------------------------------------------------------------------------

ep128emuEnvironment = ep128emuGLGUIEnvironment.Copy()
ep128emuEnvironment.Append(CPPPATH = ['./z80'])
ep128emuEnvironment.Append(LIBS = ['ep128', 'z80', 'plus4', 'ep128emu'])
if haveDotconf:
    ep128emuEnvironment.Append(LIBS = ['dotconf'])
ep128emuEnvironment.Append(LIBS = ['portaudio', 'sndfile', 'jack', 'asound',
                                   'pthread'])

ep128emu = ep128emuEnvironment.Program('ep128emu', Split('''
    main.cpp
'''))
Depends(ep128emu, ep128Lib)
Depends(ep128emu, z80Lib)
Depends(ep128emu, plus4Lib)
Depends(ep128emu, ep128emuLib)

# -----------------------------------------------------------------------------

tapeeditEnvironment = ep128emuGUIEnvironment.Copy()
tapeeditEnvironment.Append(CPPPATH = ['./tapeutil'])
tapeeditEnvironment.Append(LIBS = ['ep128emu'])
if haveDotconf:
    tapeeditEnvironment.Append(LIBS = ['dotconf'])
tapeeditEnvironment.Append(LIBS = ['sndfile', 'pthread'])

Command(['tapeutil/tapeedit.cpp', 'tapeutil/tapeedit.hpp'],
        'tapeutil/tapeedit.fl',
        'fluid -c -o tapeutil/tapeedit.cpp -h tapeutil/tapeedit.hpp $SOURCES')

tapeedit = tapeeditEnvironment.Program('tapeedit', Split('''
    tapeutil/tapeedit.cpp
    tapeutil/tapeio.cpp
'''))
Depends(tapeedit, ep128emuLib)

