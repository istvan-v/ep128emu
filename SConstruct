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
ep128emuEnvironment.Append(LIBS = ['ep128', 'z80', 'portaudio', 'sndfile',
                                   'jack', 'asound', 'pthread'])

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
'''))

z80lib = z80libEnvironment.StaticLibrary('z80', Split('''
    z80/z80.cpp
    z80/z80funcs2.cpp
'''))

# ep128emu = ep128emuEnvironment.Program('ep128emu', Split('''
#     main.cpp
# '''))
# Depends(ep128emu, ep128lib)
# Depends(ep128emu, z80lib)

