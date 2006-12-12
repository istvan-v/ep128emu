# vim: syntax=python

compilerFlags = Split('''
    -Wall -W -ansi -pedantic -Wno-long-long -Wshadow -g -O2
''')

ep128emuLibEnvironment = Environment()
ep128emuLibEnvironment.Append(CCFLAGS = compilerFlags)
ep128emuLibEnvironment.Append(CPPPATH = ['.'])

ep128emuLib = ep128emuLibEnvironment.StaticLibrary('ep128emu', Split('''
    bplist.cpp
    cfg_db.cpp
    display.cpp
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

ep128LibEnvironment = Environment()
ep128LibEnvironment.Append(CCFLAGS = compilerFlags)
ep128LibEnvironment.Append(CPPPATH = ['.', './z80'])

ep128Lib = ep128LibEnvironment.StaticLibrary('ep128', Split('''
    dave.cpp
    ep128vm.cpp
    ioports.cpp
    memory.cpp
    nick.cpp
'''))

z80LibEnvironment = Environment()
z80LibEnvironment.Append(CCFLAGS = compilerFlags)
z80LibEnvironment.Append(CPPPATH = ['.', './z80'])

z80Lib = z80LibEnvironment.StaticLibrary('z80', Split('''
    z80/z80.cpp
    z80/z80funcs2.cpp
'''))

ep128emuEnvironment = Environment()
ep128emuEnvironment.Append(CCFLAGS = compilerFlags)
ep128emuEnvironment.Append(CPPPATH = ['.', './z80'])
ep128emuEnvironment.Append(LINKFLAGS = ['-L.'])
ep128emuEnvironment.Append(LIBS = ['ep128', 'z80', 'ep128emu',
                                   'fltk_gl', 'GL',
                                   'portaudio', 'sndfile', 'jack', 'asound',
                                   'pthread'])

ep128emu = ep128emuEnvironment.Program('ep128emu', Split('''
    main.cpp
'''))
Depends(ep128emu, ep128Lib)
Depends(ep128emu, z80Lib)
Depends(ep128emu, ep128emuLib)

# -----------------------------------------------------------------------------

tapeeditEnvironment = Environment()
tapeeditEnvironment.Append(CCFLAGS = compilerFlags)
tapeeditEnvironment.Append(CPPPATH = ['.', './tapeutil'])
tapeeditEnvironment.Append(LINKFLAGS = ['-L.'])
tapeeditEnvironment.Append(LIBS = ['ep128emu', 'fltk', 'sndfile', 'pthread'])

Command(['tapeutil/tapeedit.cpp', 'tapeutil/tapeedit.hpp'],
        'tapeutil/tapeedit.fl',
        'fluid -c -o tapeutil/tapeedit.cpp -h tapeutil/tapeedit.hpp $SOURCES')

tapeedit = tapeeditEnvironment.Program('tapeedit', Split('''
    tapeutil/tapeedit.cpp
    tapeutil/tapeio.cpp
'''))
Depends(tapeedit, ep128emuLib)

# -----------------------------------------------------------------------------

plus4LibEnvironment = Environment()
plus4LibEnvironment.Append(CCFLAGS = compilerFlags)
plus4LibEnvironment.Append(CPPPATH = ['.', './plus4'])

plus4Lib = plus4LibEnvironment.StaticLibrary('plus4', Split('''
    plus4/cpu.cpp
    plus4/memory.cpp
    plus4/plus4vm.cpp
    plus4/render.cpp
    plus4/ted_api.cpp
    plus4/ted_init.cpp
    plus4/ted_main.cpp
    plus4/ted_read.cpp
    plus4/ted_write.cpp
'''))

plus4Environment = Environment()
plus4Environment.Append(CCFLAGS = compilerFlags)
plus4Environment.Append(CPPPATH = ['.', './plus4'])
plus4Environment.Append(LINKFLAGS = ['-L.'])
plus4Environment.Append(LIBS = ['plus4', 'ep128emu',
                                'fltk_gl', 'GL',
                                'portaudio', 'sndfile', 'jack', 'asound',
                                'pthread'])

plus4 = plus4Environment.Program('plus4emu', Split('''
    p4main.cpp
'''))
Depends(plus4, plus4Lib)
Depends(plus4, ep128emuLib)

