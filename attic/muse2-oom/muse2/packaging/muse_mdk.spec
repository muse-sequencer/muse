%define name				muse
%define museversion			0.7.0pre3
%define	release				1thac
%define alsa_version 			0.9.8
%define jackit_version			0.93.11
%define libjack0_version		0.93.11
%define libsndfile1_version		1.0.5
%define ladspa_version			1.12
%define glib2_0_version			2.2.3
%define qtversion   			3.2
%define dssslver 			1.78
%define musever  			0.7.0pre3
%define musedir  			0.7.0pre3
%define capver   			1.0
%define fluidsynth_version   		1.0.3
%define graphviz_version		1.9.0
%define jade_version			1.3.1
%define doxy_version			1.2.17
%define ladccaver			0.4.0

%define	major	0
%define	libname	%mklibname %name %major


Name:				%{name}
Summary:			MusE is a MIDI/Audio sequencer with recording and editing capabilities.
Version:			%{museversion}
Release:			%{release}
URL:				http://muse.sourceforge.net/
Source0:			%{name}-%{musever}.tar.bz2
Source1:			%{name}-ardour-tutorial.tar.bz2
Group:				Sound
BuildRoot:			%{_tmppath}/%{name}-buildroot
License:			GPL
Requires:			libqt3 >= %{qtversion}
Requires:			libsndfile1 >= %{libsndfile1_version}
Requires:			libalsa2 >= %{alsa_version}
Requires:			jackit >= %{jackit_version}
Requires:			libjack0 >= %{libjack0_version}
BuildRequires:			graphviz >= %{graphviz_version}
BuildRequires:			libgraphviz7 >= %{graphviz_version}
BuildRequires:			openjade >= %{jade_version}
BuildRequires:			doxygen >= %{doxy_version}
BuildRequires: 		libfluidsynth1 >= %{fluidsynth_version}
BuildRequires: 		libfluidsynth1-devel >= %{fluidsynth_version}
BuildRequires: 		fluidsynth >= %{fluidsynth_version}
BuildRequires: 		libalsa2-devel >= %{alsa_version}
BuildRequires:			docbook-style-dsssl >= %{dssslver}
BuildRequires:			libjack0-devel >= %{libjack0_version}
BuildRequires:			libsndfile1-devel >= %{libsndfile1_version}
BuildRequires:			libgraphviz7-devel >= %{graphviz_version}
BuildRequires:			libcap1-devel >= %{capver}
BuildRequires:			ladcca >= %{ladccaver}
BuildRequires:			docbook-dtd41-sgml
BuildRequires:			libext2fs2-devel

%description
MusE is a MIDI/Audio sequencer with recording and editing capabilities.
Some Highlights:

    * standard midifile (smf) import-/export
    * organizes songs in tracks and parts which you can arrange
      with the part editor
    * midi editors: pianoroll, drum, list, controller
    * score editor with high quality postscript printer output
    * realtime: editing while playing
    * unlimited number of open editors
    * unlimited undo/redo
    * realtime and step-recording
    * multiple midi devices
    * unlimited number of tracks
    * audio playback/recording 
    * Sync to external devices: MTC/MMC, Midi Clock,
      Master/Slave (currently only partial implemented)
    * LADSPA host
    * ALSA and JACK audio driver
    * uses raw midi devices (ALSA, OSS & serial ports)
    * XML project file
    * project file contains complete app state (session data)
    * Application spanning Cut/Paste Drag/Drop
    * uses C++, QT2 GUI Library, STL
    * GPL Licenced 

%package -n %libname
Summary:	Main libraries for %name.
Group:		System/Libraries

%description -n %{libname}
This package contains the dynamic library of plugins from %name.

%package -n %{libname}-static
Summary:        Static libraries for %name plugins.
Group:		System/Libraries

%description -n %{libname}-static
This package contains the dynamic library of plugins from %name.


Group:          Development/C++
%package -n givertcap
Summary:      Give applications real-time capabilities
Version:      %{capver}
Group:	      Sound
URL:          http://www.tml.hut.fi/~tilmonen/givertcap/

%description -n givertcap
Givertcap is a small Linux application that is used to give other
application real-time capabilities. With the aid of givertcap you can
run real-time applications (audio and video -processing apps for
example) with high priority without running the application as root.

%prep
rm -rf $RPM_BUILD_ROOT

%setup -n %{name}-%{musever} -a 1
#perl -p -i -e 's|version="3.2.3"|version="3.1"||g' widgets/*.ui


%build

export QTDIR=/usr/lib/qt3
export KDEDIR=%_prefix
export LD_LIBRARY_PATH=$QTDIR/lib:$KDEDIR/lib:$LD_LIBRARY_PATH
export PATH=$QTDIR/bin:$KDEDIR/bin:$PATH

%configure2_5x --prefix=%{_prefix} --datadir=%{_datadir} --libdir=%{_libdir} --with-jack \
    --disable-suid-install --disable-suid-build --enable-patchbay --disable-rtcap \
    --with-docbook-stylesheets=/usr/share/sgml/docbook/dsssl-stylesheets-%{dssslver} \
    --enable-arch=i586 --enable-laddca
#    --disable-static --enable-shared --disable-qttest 

%make

%install
rm -rf $RPM_BUILD_ROOT

%makeinstall

mkdir -p $RPM_BUILD_ROOT%_menudir

# (mandrake) menu support
cat << EOF > %{buildroot}%{_menudir}/%{name}
?package(%{name}): longtitle="MusE audio editor" \
command="/usr/bin/muse" title="MusE audio editor"  needs="x11" \
section="Multimedia/Sound" \
icon="sound_section.png"
EOF


%post
%update_menus
/sbin/ldconfig

%postun
%clean_menus
 /sbin/ldconfig

%post -n %{libname} -p /sbin/ldconfig

%postun -n %{libname} -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0755)
%doc AUTHORS COPYING INSTALL ChangeLog NEWS README* SECURITY
%doc html
%{_bindir}/muse
%{_bindir}/grepmidi
%{_libdir}/%name
%{_datadir}/%name
%{_menudir}/*

%files -n %{libname}
%defattr(-,root,root)
%{_libdir}/%name/plugins/*.so*
%{_libdir}/%name/synthi/*.so*

%files -n %{libname}-static
%defattr(-,root,root)
%{_libdir}/%name/plugins/*.la
%{_libdir}/%name/plugins/*.a
%{_libdir}/%name/synthi/*.a
%{_libdir}/%name/synthi/*.la

#%files -n givertcap
#%attr(4755, root, root)
#%{_bindir}/givertcap



%changelog
* Thu May 27 2004 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.7.0pre3-1thac
- Updated to latest version

* Wed May 05 2004 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.7.0pre2-1thac
- Updated to latest version

* Tue Apr 20 2004 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.7.0pre1-1thac
- Updated to latest version

* Tue Apr 20 2004 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.3-2thac
- Rebuilt against Mandrake 10.0 official

* Mon Jan 12 2004 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.3-1thac
- Updated to latest version

* Mon Nov 17 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.2-2thac
- Rebuilt for jackit-0.9.0

* Fri Nov 07 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.2-1thac
- Rebuilt for Mandrake 9.2

* Thu Oct 30 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.1-1thac
- Rebuilt for Mandrake 9.2

* Tue Sep 09 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.1-1thac
- Added muse-softsynth-fix
  on muse 0.6.1, it results in a dead lock when muse starts a softsynth
  without RT.  the patch is an adhoc fix for this problem.
- Added jackit 0.80.0 type patch
- Changed naming to differ from Mandrake cooker

* Fri Jul 25 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.1-1mdk
- Updated to 0.6.1 final built against fluidsynth-1.0.2
- Built with qt-3.1.2 patch

* Sat May 17 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.0-1mdk
- Updated to 0.6.0 final built against fluidsynth-1.0.1

* Sat May 03 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.0pre8.3mdk
- Rebuilt with rpmmacro patch that removes nvidia dependency.

* Tue Apr 22 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.0pre8.2mdk
- Rebuilt against latest version of jackit 0.67.2

* Fri Apr 04 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.0pre8.1mdk
- Updated to latest version

* Wed Mar 26 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.0pre7.2mdk
- Rebuilt for Mandrake 9.1

* Sun Mar 23 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.0pre7.2mdk
- Updated to latest version
- Compiled against jackit-0.62.0 and iiwusynth-0.2.5

* Sun Feb 16 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.0pre5.4mdk
- Recompiled against jackit-0.51.0.

* Fri Feb 14 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.0pre5.3mdk
- Recompiled against libsndfile-1.0.4.

* Sat Feb 08 2003 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.0pre5.2mdk
- Recompiled against jack-0.44,graphviz and i586 optimized.

* Wed Dec 11 2002 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.0pre5.1mdk
- Removed jack dependency again

* Mon Dec 09 2002 Torbjorn Turpeinen <tobbe@nyvalls.se> 0.6.0pre5mdk
- Rebuilt for Mandrake 9.0

* Fri Nov 15 2002 Lenny Cartier <lenny@mandrakesoft.com> 0.6.0-0.pre3.2mdk
- remove jack dependency

* Wed Nov 13 2002 Lenny Cartier <lenny@mandrakesoft.com> 0.6.0-0.pre3.1mdk
- 0.6.0pre3
- clean filelist
- disable-suid-build for now
- from James Gregory <james@james.id.au> :
	- fixed file locations to use macros rather than absolute paths

* Thu Oct  3 2002 James Gregory <james@james.id.au>  0.6.0-0.pre2.2mdk
- First version to build.
