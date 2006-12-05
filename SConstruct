# vim: syntax=python

compilerFlags = Split('''
    -Wall -W -ansi -pedantic -Wno-long-long -Wshadow -g -O2
''')

ep128libEnvironment = Environment()
ep128libEnvironment.Append(CCFLAGS = compilerFlags)
ep128libEnvironment.Append(CPPPATH = ['.', './z80'])

ep128emuEnvironment = Environment()
ep128emuEnvironment.Append(CCFLAGS = compilerFlags)
ep128emuEnvironment.Append(CPPPATH = ['.', './z80'])
ep128emuEnvironment.Append(LINKFLAGS = ['-L.'])
ep128emuEnvironment.Append(LIBS = ['ep128', 'z80',
                                   'fltk_gl', 'GL',
                                   'portaudio', 'sndfile', 'jack', 'asound',
                                   'pthread'])

z80libEnvironment = Environment()
z80libEnvironment.Append(CCFLAGS = compilerFlags)
z80libEnvironment.Append(CPPPATH = ['.', './z80'])

ep128lib = ep128libEnvironment.StaticLibrary('ep128', Split('''
    bplist.cpp
    cfg_db.cpp
    dave.cpp
    ep128vm.cpp
    fileio.cpp
    gldisp.cpp
    ioports.cpp
    memory.cpp
    nick.cpp
    system.cpp
    tape.cpp
'''))

z80lib = z80libEnvironment.StaticLibrary('z80', Split('''
    z80/z80.cpp
    z80/z80funcs2.cpp
'''))

ep128emu = ep128emuEnvironment.Program('ep128emu', Split('''
    main.cpp
'''))
Depends(ep128emu, ep128lib)
Depends(ep128emu, z80lib)

# -----------------------------------------------------------------------------

tapeeditEnvironment = Environment()
tapeeditEnvironment.Append(CCFLAGS = compilerFlags)
tapeeditEnvironment.Append(CPPPATH = ['.', './tapeutil'])
tapeeditEnvironment.Append(LINKFLAGS = ['-L.'])
tapeeditEnvironment.Append(LIBS = ['ep128', 'fltk', 'sndfile', 'pthread'])

tapeedit = tapeeditEnvironment.Program('tapeedit', Split('''
    tapeutil/tapeedit.cpp
    tapeutil/tapeio.cpp
'''))

Command(['tapeutil/tapeedit.cpp', 'tapeutil/tapeedit.hpp'],
        'tapeutil/tapeedit.fl',
        'fluid -c -o tapeutil/tapeedit.cpp -h tapeutil/tapeedit.hpp $SOURCES')

# -----------------------------------------------------------------------------

plus4libEnvironment = Environment()
plus4libEnvironment.Append(CCFLAGS = compilerFlags)
plus4libEnvironment.Append(CPPPATH = ['.', './plus4'])

plus4lib = plus4libEnvironment.StaticLibrary('plus4', Split('''
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
plus4Environment.Append(LIBS = ['plus4', 'ep128',
                                'fltk_gl', 'GL',
                                'portaudio', 'sndfile', 'jack', 'asound',
                                'pthread'])

plus4 = plus4Environment.Program('plus4emu', Split('''
    p4main.cpp
'''))
Depends(plus4, plus4lib)
Depends(plus4, ep128lib)

