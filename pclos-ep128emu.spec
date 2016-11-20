# Checkout: cvs -z3 -d:pserver:anonymous@ep128emu.cvs.sourceforge.net:/cvsroot/ep128emu co -P ep128emu2

Name:			ep128emu
Version:		2.0.11
Release:		%mkrel 1
Summary:		Enterprise 64/128, ZX Spectrum 48/128, and Amstrad CPC 464/664/6128 emulator
License:		GPLv2+
Group:			Emulators
URL:			https://github.com/istvan-v/ep128emu/
Source0:		%{name}/%{name}-%{version}.tar.xz
Source1:		%{name}-32.png
%define	romname		ep128emu_roms-2.0.10.bin
Source2:		%{romname}
BuildRequires:		libSDL-devel libgtk+2.0_0-devel
BuildRequires:		scons libfltk-devel libportaudio2-devel libsndfile-devel liblua-devel
BuildRoot:		%{_tmppath}/%{name}-%{version}

%description
ep128emu is a portable emulator of the Enterprise 64/128, ZX Spectrum 48/128, and Amstrad CPC 464/664/6128 computers
https://github.com/istvan-v/ep128emu/releases/

%prep
%setup -q 
# -n %{name}/src

%build
scons

%install
rm -rf %{buildroot}
mkdir %{buildroot}

install -d -m 0755 %{buildroot}/%{_datadir}/games/%{name}
#rename to wrap further down the spec
install -m 0755 %{name} %{buildroot}/%{_datadir}/games/%{name}/%{name}-real
install -m 0755 makecfg %{buildroot}/%{_datadir}/games/%{name}/
install -m 0755 tapeedit %{buildroot}/%{_datadir}/games/%{name}/
install -m 0755 %{SOURCE2} %{buildroot}/%{_datadir}/games/%{name}/

#icon
install -d -m 755 %{buildroot}%{_iconsdir}
install -m 644 %{SOURCE1} %{buildroot}%{_iconsdir}/

#xdg menu
install -d -m 755 %{buildroot}%{_datadir}/applications
cat<<EOF>%{buildroot}%{_datadir}/applications/%{name}-cpc.desktop
[Desktop Entry]
Encoding=UTF-8
Name=%{name} - Amstrad CPC emulator
Comment=Amstrad CPC emulator
Comment[da]=Amstrad CPC emulator
Exec=%{_datadir}/games/%{name}/%{name} -cpc
Icon=%{_iconsdir}/%{name}-32.png
Terminal=false
Type=Application
Categories=X-MandrivaLinux-MoreApplications-Emulators;Emulator;
EOF

install -d -m 755 %{buildroot}%{_datadir}/applications
cat<<EOF>%{buildroot}%{_datadir}/applications/%{name}-enterprise.desktop
[Desktop Entry]
Encoding=UTF-8
Name=%{name} - Enterprise emulator
Comment=Enterprise emulator
Comment[da]=Enterprise emulator
Exec=%{_datadir}/games/%{name}/%{name} -ep128
Icon=%{_iconsdir}/%{name}-32.png
Terminal=false
Type=Application
Categories=X-MandrivaLinux-MoreApplications-Emulators;Emulator;
EOF

install -d -m 755 %{buildroot}%{_datadir}/applications
cat<<EOF>%{buildroot}%{_datadir}/applications/%{name}-spectrum.desktop
[Desktop Entry]
Encoding=UTF-8
Name=%{name} - ZX Spectrum emulator
Comment=ZX Spectrum emulator
Comment[da]=ZX Spectrum emulator
Exec=%{_datadir}/games/%{name}/%{name} -zx
Icon=%{_iconsdir}/%{name}-32.png
Terminal=false
Type=Application
Categories=X-MandrivaLinux-MoreApplications-Emulators;Emulator;
EOF



#wrapper to install rom and configure
cat >  %{buildroot}%{_datadir}/games/%{name}/%{name} << EOF
##!/bin/sh

if [ ! -d \$HOME/.%{name} ]; then
 mkdir -p \$HOME/.%{name}/roms
 cp %{_datadir}/games/%{name}/%{romname} \$HOME/.%{name}/roms/
 %{_datadir}/games/%{name}/makecfg
fi
%{_datadir}/games/%{name}/%{name}-real \$@
EOF

chmod 755 %{buildroot}%{_datadir}/games/%{name}/%{name} 

%files
%defattr(-,root,root)
%doc COPYING NEWS README
%{_iconsdir}
%{_datadir}

%clean
rm -rf %{buildroot}

%post
%{update_menus}
%{update_desktop_database}

%postun
%{clean_menus}
%{clean_desktop_database}

%changelog
* Sun Dec 19 2010 MBantz <martin dot bantz at gmail dot com> 2.0.9-1pclos2010
- Added .dsk support for CPC
- Fixed wrapperscript

* Sat Nov 06 2010 MBantz <martin dot bantz at gmail dot com> 2.0.8.1-1pclos2010
- Built for PCLinuxOS

