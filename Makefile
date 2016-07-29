NSI	= installer/ep128emu.nsi
INSTEXE = $(dir $(NSI))/$(shell awk '$$1 == "OutFile" { gsub("[^a-zA-Z0-9_.-]","",$$2);print $$2 }' $(NSI))
EMUEXES = ep128emu.exe makecfg.exe tapeedit.exe
EMUELFS = ep128emu makecfg tapeedit

all:
	@echo "No target"
	@false

clean:
	scons -c .
	rm -fr .sconf_temp
	rm -f .sconsign.dblite config.log $(EMUEXES) $(INSTEXE) $(EMUELFS)

$(EMUEXES):
	scons win32=1

$(EMUELFS):
	scons

$(INSTEXE): $(EMUEXES) $(NSI)
	cd $(dir $(NSI)) && wine /usr/local/nsis/nsis-2.51/makensis.exe $(notdir $(NSI))
	ls -l $(INSTEXE)

installer:
	$(MAKE) $(INSTEXE)

win32:
	scons win32=1

native:
	scons

.PHONY: all win32 clean installer native
