NSI	= installer/ep128emu.nsi
INSTEXE = $(dir $(NSI))/$(shell awk '$$1 == "OutFile" { gsub("[^a-zA-Z0-9_.-]","",$$2);print $$2 }' $(NSI))
EMUEXES = ep128emu.exe makecfg.exe tapeedit.exe
EMUELFS = ep128emu epmakecfg tapeedit

IPREFIX		= /usr/local
BINDIR		= $(IPREFIX)/bin
DATADIR		= $(IPREFIX)/share/ep128
PIXMAPDIR	= $(IPREFIX)/share/pixmaps
DESKTOPDIR	= $(IPREFIX)/share/applications

all:
	@echo "*** No target" >&2
	@false

clean:
	scons -c .
	rm -fr .sconf_temp
	rm -f .sconsign.dblite config.log $(EMUEXES) $(INSTEXE) $(EMUELFS)

distclean:
	$(MAKE) clean

$(EMUEXES):
	scons win32=1

$(EMUELFS):
	scons

$(INSTEXE): $(EMUEXES) $(NSI)
	cd $(dir $(NSI)) && wine /usr/local/nsis/nsis-2.51/makensis.exe $(notdir $(NSI))
	ls -l $(INSTEXE)

win32installer:
	$(MAKE) $(INSTEXE)

install:
	strip $(EMUELFS)
	mkdir -p $(BINDIR) $(PIXMAPDIR) $(DESKTOPDIR) $(DATADIR)/roms $(DATADIR)/disk $(DATADIR)/config
	$(MAKE) -C resource
	cp resource/*.png $(PIXMAPDIR)/
	cp resource/*.desktop $(DESKTOPDIR)/
	cp config/* $(DATADIR)/config/
	cp disk/* $(DATADIR)/disk/
	cp roms/*.rom $(DATADIR)/roms/
	cp ep128emu tapeedit resource/zx128emu resource/cpc464emu $(BINDIR)/
	cp resource/makecfg-wrapper $(BINDIR)/makecfg-ep128emu
	cp epmakecfg $(BINDIR)/makecfg-ep128emu.bin
	chmod +x $(BINDIR)/makecfg-ep128emu.bin $(BINDIR)/makecfg-ep128emu $(BINDIR)/ep128emu $(BINDIR)/tapeedit $(BINDIR)/zx128emu $(BINDIR)/cpc464emu

win32:
	scons win32=1

native:
	scons

.PHONY: all win32 clean distclean win32installer native install
