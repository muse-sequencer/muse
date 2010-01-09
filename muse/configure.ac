dnl --------------------------------------------------
dnl configure.ac for the MusE
dnl --------------------------------------------------

AC_INIT(MusE, 1.0)
AC_CONFIG_SRCDIR(muse/app.cpp)
AM_INIT_AUTOMAKE(1.0)
AM_CONFIG_HEADER(config.h)
### AM_MAINTAINER_MODE
AM_ACLOCAL_INCLUDE(m4)

dnl ***** Check for programs *****
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_LN_S

###############
### DocBook ###
###############
PGAC_PROG_JADE
if test -n "$JADE"; then
  PGAC_CHECK_DOCBOOK(4.1)
  if test "$pgac_cv_check_docbook" = "yes"; then
    PGAC_PATH_DOCBOOK_STYLESHEETS
    if test -n "$DOCBOOKSTYLE"; then
      DOCBOOKTARGETS="book1.html"
    else
      AC_MSG_WARN([DocBook stylesheets not found or not usable - documentation will not be built])
    fi
  else
    AC_MSG_WARN([DocBook 4.1 DTD not found or not usable - documentation will not be built])
  fi
else
  AC_MSG_WARN([jade not found - documentation will not be built])
fi
AC_SUBST(DOCBOOKTARGETS)

###############
### doxygen ###
###############
AC_ARG_VAR(DOXYGEN, Doxygen)
AC_PATH_PROG(DOXYGEN, doxygen, no)

################
### graphviz ###
################
AC_ARG_VAR(DOT, The "dot" program from graphviz)
AC_PATH_PROG(DOT, dot, no)
if test "$DOT" != "no"; then
  HAVEDOT=YES
  DOTPATH=$( dirname "$DOT" )
else
  HAVEDOT=NO
fi
AC_SUBST(HAVEDOT)
AC_SUBST(DOTPATH)

############
### perl ###
############
AC_ARG_VAR(PERL, Perl)
AC_PATH_PROG(PERL, perl, no)
if test "$PERL" == "no"; then
  AC_MSG_WARN([you don't have perl installed!  wierdo!  (and doxygen might not like this)])
fi

dnl ***** Check for libraries *****

#########
### X ###
#########
AC_PATH_XTRA
if test "$X_DISPLAY_MISSING" != ""; then
  AC_MSG_ERROR([X required])
fi



##########
### QT ###
##########
CONFIGURE_QT(3.2.0, , AC_MSG_ERROR([need qt >= 3.2.0]))
if test "$muse_qttest" = "yes"; then
  AC_LANG_PUSH([C++])
  muse_save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $QT_CFLAGS"
  muse_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $QT_CFLAGS"
  muse_save_LDFLAGS="$LDFLAGS"
  LDFLAGS="$LDFLAGS $QT_LIBS"
  AC_CHECK_LIB(qt-mt, qt_selection_property, [
    true
  ],[
    AC_MSG_ERROR([

  MusE requires thread enabled QT (qt-mt) >= 3.2.0

  You can either compile your own version of QT (making sure you enable
  threading!) or install the appropriate package for your distribution.
])
  ])
  CFLAGS="$muse_save_CFLAGS"
  CXXFLAGS="$muse_save_CXXFLAGS"
  LDFLAGS="$muse_save_LDFLAGS"
  unset muse_save_CFLAGS
  unset muse_save_CXXFLAGS
  unset muse_save_LDFLAGS
  AC_LANG_POP([C++])
fi

############
### ALSA ###
############
AM_PATH_ALSA(0.9.0,
  true,
  AC_MSG_ERROR([latest alsa (0.9.x) is required])
)

#################
### fluidsynth ###
#################
LIBFLUIDSYNTH=
muse_enable_fluidlib="yes"
synth_fluidsynth="fluidsynth"
synth_fluid="fluid"
AC_ARG_ENABLE(fluidsynth,[  --disable-fluidsynth    disable fluidsynth softsynth plugin],[
  case "$enableval" in
    "yes")
      synth_fluidsynth="fluidsynth"
      synth_fluid="fluid"
      ;;
    "no")
      muse_enable_fluidlib="no"
      synth_fluidsynth=""
      synth_fluid=""
      ;;
    *)
      AC_MSG_ERROR([must use --enable-fluidsynth(=yes/no) or --disable-fluidsynth])
      ;;
  esac
])

AM_CONDITIONAL(ENABLEFLUIDSYNTH, test "$muse_enable_fluidlib" = "yes")
PKG_CHECK_MODULES(Fluidsynth, fluidsynth,
  [FLUIDSYNTHDIRS="fluid fluidsynth"; AC_DEFINE(ENABLEFLUIDSYNTH, 1, [Compile fluidsynth])],
  [AC_MSG_WARN([libfluidsynth not found; disabling fluidsynth plugins]) ;
  muse_enable_fluidlib="no"]
  )
AC_SUBST(FLUIDSYNTHDIRS)
AC_SUBST(synth_fluidsynth)
AC_SUBST(synth_fluid)
if test "$muse_enable_fluidlib" = "no"; then
  synth_fluidsynth=""
  synth_fluid=""
fi


###############
### sndfile ###
###############
PKG_CHECK_MODULES(SNDFILE, sndfile >= 1.0.0,
  true,
  AC_MSG_ERROR([need libsndfile >= 1.0.0])
)

###############
### libsamplerate ###
###############
PKG_CHECK_MODULES(SAMPLERATE, samplerate >= 0.1.0,
  true,
  AC_MSG_ERROR([need libsamplerate])
)

###############
### libuuid ###
###############
AC_CHECK_HEADER(uuid/uuid.h, UUID_FOUND="yes", UUID_FOUND="no")
if test x$UUID_FOUND = xno; then
  AC_MSG_ERROR([Header file uuid.h, from the uuid-dev (sometimes in e2fsprogs) package, is required])
fi

AC_CHECK_LIB(uuid, uuid_generate, UUID_FOUND="yes", UUID_FOUND="no")
if test x$UUID_FOUND = xno; then
  AC_MSG_ERROR([libuuid (sometimes in e2fsprogs package) is required])
fi
UUID_LIBS="-luuid"
AC_SUBST(UUID_CFLAGS)
AC_SUBST(UUID_LIBS)

## PKG_CHECK_MODULES(UUID, uuid >= 1.0,
##   true,
##   AC_MSG_ERROR([need libuuid >= 1.0 from e2fsprogs package])
## )

############
### JACK ###
############

PKG_CHECK_MODULES(JACK,
    jack >= 0.98.0,
    true,
    AC_MSG_ERROR([need JACK >= 0.98.0])
  )

##############
### LASH ###
##############
muse_enable_lash="no"
AC_ARG_ENABLE(lash,
  [  --enable-lash           enable LASH support],[
  case "$enableval" in
    "yes")
      muse_enable_lash="yes"
      ;;
    "no")
      muse_enable_lash="no"
      ;;
    *)
      AC_MSG_ERROR([must use --enable-lash(=yes/no) or --disable-lash])
      ;;
  esac
])

if test "$muse_enable_lash" = "yes"; then
  PKG_CHECK_MODULES(LASH, lash-1.0 >= 0.2, LASH_FOUND="yes", LASH_FOUND="no")
  AC_SUBST(LASH_CFLAGS)
  AC_SUBST(LASH_LIBS)

  if test "$LASH_FOUND" = "yes"; then
    AC_DEFINE(HAVE_LASH, 1, [whether or not we are supporting lash])
    LASH_VERSION=$( pkg-config --modversion lash-1.0 )
    AC_DEFINE_UNQUOTED(LASH_VERSION, "$LASH_VERSION", [The version of lash we're compiling against])
  fi
else
  AC_MSG_WARN([LASH support is disabled])
  LASH_FOUND="no"
fi
AM_CONDITIONAL(HAVE_LASH, test "$LASH_FOUND" = "yes")


dnl ***** Check for header files *****

dnl ***** Check for options *****

#################
### givertcap ###
#################

GIVERTCAP=
report_rtcap="no"
AC_ARG_ENABLE(rtcap,
  [  --enable-rtcap (deprecated) use Tommi Ilmonen's "givertcap" to give
                          realtime capabilities],[
  case "$enableval" in
    "yes")
      AC_DEFINE(RTCAP, 1, [Use givertcap])
      MUSECXXFLAGS="$MUSECXXFLAGS -DRTCAP"
      GIVERTCAP="givertcap"

      AC_CHECK_HEADER(sys/capability.h,
               report_rtcap="yes",
               AC_MSG_ERROR([Missing sys/capability.h]))
      ;;
    "no")
      ;;
    *)
      AC_MSG_ERROR([must use --enable-rtcap(=yes/no) or --disable-rtcap])
  esac
])
AM_CONDITIONAL(RTCAP, test "$report_rtcap" = "yes")
AC_SUBST(GIVERTCAP)


####################
### suid-install ###
####################

SUIDINSTALL="no"
AC_ARG_ENABLE(suid-install,
  [  --enable-suid-install (deprecated) install MusE setuid root (with kernel 2.6, use realtime-lsm instead)],[
  case "$enableval" in
    "yes")
      SUIDINSTALL="yes"
      ;;
    "no")
      ;;
    *)
      AC_MSG_ERROR([must use --enable-suid-install(=yes/no) or --disable-suid-install])
      ;;
  esac
])
AC_SUBST(SUIDINSTALL)

##################
### suid-build ###
##################
SUIDBUILD="no"
AC_ARG_ENABLE(suid-build,
  [  --enable-suid-build (deprecated) build MusE setuid root(with kernel 2.6, use realtime-lsm instead)],[
  case "$enableval" in
    "yes")
      SUIDBUILD="yes"
      ;;
    "no")
      SUIDBUILD="no"
      ;;
    *)
      AC_MSG_ERROR([must use --enable-suid-build(=yes/no) or --disable-suid-build])
      ;;
  esac
])
AC_SUBST(SUIDBUILD)


########################
### doxygen-treeview ###
########################
DOXYGEN_TREEVIEW="YES"
report_doxy_treeview="yes";
AC_ARG_ENABLE(doxy-treeview,
  [  --disable-doxy-treeview stop Doxygen from generating java-ified
                          treeview menus in its HTML output],[
  case "$enableval" in
    "yes")
      ;;
    "no")
      DOXYGEN_TREEVIEW="NO"
      report_doxy_treeview="no";
      ;;
    *)
      AC_MSG_ERROR([must use --enable-doxy-treeview(=yes/no) or --disable-doxy-treeview])
      ;;
  esac
])
AC_SUBST(DOXYGEN_TREEVIEW)

################
### patchbay ###
################
###PATCHBAY="no"
###AC_SUBST(PATCHBAY)
###AM_CONDITIONAL(PATCHBAY, test "$PATCHBAY" == "yes")

## -DQT_NO_CAST_ASCII
MUSECXXFLAGS="-g -fno-exceptions -Wall -W -D_GNU_SOURCE -D_REENTRANT \
  -DQT_CLEAN_NAMESPACE -DQT_NO_COMPAT \
  -I\$(top_srcdir) -I\$(top_srcdir)/muse/widgets \$(QT_CFLAGS)"

#MUSEOPTCXXFLAGS="-O6 -fomit-frame-pointer -ffast-math -fstrength-reduce \
#-funroll-loops"
#MUSEOPTCXXFLAGS="-O2"
MUSEOPTCXXFLAGS="-O3 -fomit-frame-pointer -ffast-math -fstrength-reduce -funroll-loops"

# -fmove-all-movables

################
### optimize ###
################
report_optimise="no"
AC_ARG_ENABLE(optimize,
  [  --enable-optimize       ask the compiler for its best optimizations (possibly only i386)],[
    case "$enableval" in
      "yes")
        report_optimise="yes"
        CXXFLAGS="$( echo "$CXXFLAGS" | sed "s/-O.[ ]*//" )"
        MUSECXXFLAGS="$MUSECXXFLAGS $MUSEOPTCXXFLAGS"
        ;;
      "no")
        ;;
      *)
        AC_MSG_ERROR([must use --enable-optimize(=yes/no) or --disable-optimize])
        ;;
    esac
  ])

#############
### debug ###
#############
report_debug="no"
AC_ARG_ENABLE(debug,
  [  --enable-debug          disable optimizations],[
    case "$enableval" in
      "yes")
        report_debug="yes"
        CXXFLAGS=""
        ;;
      "no")
        ;;
      *)
        AC_MSG_ERROR([must use --enable-debug(=yes/no) or --disable-debug])
        ;;
    esac
  ])

###########################
### PCH                 ###
### precompiled headers ###
###########################

AC_ARG_ENABLE(pch,
  [  --enable-pch            enable precompiled headers (gcc 3.4 required)],[
    case "$enableval" in
      "yes")
        MUSECXXFLAGS="$MUSECXXFLAGS -Winvalid-pch"
        PCH="yes"
        AC_SUBST(PCH)
        AC_DEFINE(PCH, 1, [whether or not we use precompiled headers])
        ;;
      "no")
        ;;
      *)
        AC_MSG_ERROR([must use --enable-pch(=yes/no) or --disable-pch])
        ;;
    esac
  ])
AM_CONDITIONAL(PCH, test "$PCH" = "yes")

############
### arch ###
############

report_march="none"
AC_ARG_ENABLE(arch,
[  --enable-arch=ARCH      use -march=ARCH and -mtune=ARCH argument to CXX
                          choose from any supported gcc architecture such as:  
                          386, 486, 586, 686, pentium, pentiumpro, k6, athlon...],[
    case "$enableval" in
      "no")
        ;;
      *)
        report_march="$enableval"
        MUSECXXFLAGS="$MUSECXXFLAGS -mtune=$enableval -march=$enableval"
        ;;
    esac
  ])

###########################
### SSE                 ###
### SSE optimizations   ###
###########################

report_sse="no"
AC_ARG_ENABLE(sse,
  [  --enable-sse            enable SSE optimizations],[
    case "$enableval" in
      "yes")
        ## MUSECXXFLAGS="$MUSECXXFLAGS -Winvalid-pch"
        report_sse="yes"
        USE_SSE="yes"
        AC_SUBST(USE_SSE)
        AC_DEFINE(USE_SSE, 1, [whether or not to enable SSE])
        ;;
      "no")
        ;;
      *)
        AC_MSG_ERROR([must use --enable-sse(=yes/no) or --disable-sse])
        ;;
    esac
  ])
AM_CONDITIONAL(USE_SSE, test "$USE_SSE" = "yes")

##########
# PYTHON #
##########
# python detection borrowed from libvirt (http://libvirt.org)
AC_ARG_WITH([python],
[  --enable-python              Build python bindings (on)],[],[enable_python=yes])

PYTHON_VERSION=
PYTHON_INCLUDES=
if test "$enable_python" != "no" ; then
    if test -x "$enable_python/bin/python"
    then
        AC_MSG_NOTICE(Found python in $enable_python/bin/python)
        PYTHON="$enable_python/bin/python"
        enable_python=yes
    else
        if test -x "$enable_python"
        then
            AC_MSG_NOTICE(Found python in $enable_python)
            PYTHON="$enable_python"
            enable_python=yes
        else
            if test -x "$PYTHON"
            then
                AC_MSG_NOTICE(Found python in environment PYTHON=$PYTHON)
                enable_python=yes
            fi
        fi
    fi

    if test "$enable_python" == "yes" ; then
        AM_PATH_PYTHON(,, [:])

        if test "$PYTHON" != : ; then
            PYTHON_CONFIG="$PYTHON-config"

            if test -x "$PYTHON_CONFIG"
            then
                PYTHON_INCLUDES=`$PYTHON_CONFIG --includes`
                PYTHON_LIB=`$PYTHON_CONFIG --libs`
            else
                if test -r $PYTHON_EXEC_PREFIX/include/python$PYTHON_VERSION/Python.h
                then
                    PYTHON_INCLUDES=-I$PYTHON_EXEC_PREFIX/include/python$PYTHON_VERSION
                else
                    if test -r $prefix/include/python$PYTHON_VERSION/Python.h
                    then
                        PYTHON_INCLUDES=-I$prefix/include/python$PYTHON_VERSION
                    else
                        if test -r /usr/include/python$PYTHON_VERSION/Python.h
                        then
                            PYTHON_INCLUDES=-I/usr/include/python$PYTHON_VERSION
                        else
                            AC_MSG_NOTICE([Could not find python$PYTHON_VERSION/Python.h, disabling bindings])
                            enable_python=no
                        fi
                    fi
                fi
            fi
        else
            AC_MSG_NOTICE([Could not find python interpreter, disabling bindings])
            enable_python=no
        fi
    else
        AC_MSG_NOTICE([Could not find python in $enable_python, disabling bindings])
        enable_python=no
    fi
fi
AM_CONDITIONAL(ENABLE_PYTHON, test "$enable_python" = "yes")
AC_SUBST([PYTHON_VERSION])
AC_SUBST([PYTHON_INCLUDES])
AC_SUBST([PYTHON_LIB])

#AC_ARG_ENABLE(python,
#enable_python="yes"
# [  --disable-python         disable python bindings for MusE],[
#   case "$enableval" in
#     "no")
#       enable_python="no"
#       ;;
#     *)
#       AC_MSG_ERROR([must use --enable-python(=yes/no) or --disable-python])
#       ;;
#   esac
# ])
#
#if test "$enable_python" = "yes"; then
#    AM_PATH_PYTHON
#    PYTHON_INCLUDE_DIR=""
#    if test -n "$PYTHON_INCLUDES" ; then
#        PYTHON_INCLUDE_DIR=$PYTHON_INCLUDES
#        python_bindings="yes"
#    else
#        python_bindings="no - devel pkg not found"
#    fi
#fi
#AM_CONDITIONAL(ENABLE_PYTHON, test "$python_bindings" = "yes")

###################
### vst plugins ###
###################

vst_possible=no
VST="no"
AC_ARG_ENABLE(vst,[  --enable-vst (deprecated, use dssi) build VST/win support. (disabled by default) ],
        [ if test x$enable_vst = xyes ; then
            vst_requested=yes;
        fi
        ]
)

if test x$vst_requested = xyes ; then
      PKG_CHECK_MODULES(FST,libfst, [ vst_possible=yes ], [ vst_possible=no ])
fi

if test x$vst_possible = xyes -a x$vst_requested = xyes ; then
      AC_MSG_CHECKING([support for VST/win plugins])
      VST_CFLAGS="$FST_CFLAGS"
      VST_LIBS="$FST_LIBS"
      VST="yes"
      AC_DEFINE(VST_SUPPORT, 1, [support for VST/win plugins])
fi

AM_CONDITIONAL(VST_SUPPORT, test x$vst_possible = xyes -a x$vst_requested = xyes)


###################
### dssi plugins ###
###################

dssi_requested=no
LO_LIB_FOUND="no"
DSSI_H_FOUND="no"
DSSI="no"
AC_ARG_ENABLE(dssi,[  --enable-dssi           build DSSI + DSSI-Vst support. (disabled by default) ],
        [ if test x$enable_dssi = xyes ; then
            dssi_requested=yes;
        fi
        ]
)


if test x$dssi_requested = xyes ; then
      AC_MSG_CHECKING([support for DSSI + DSSI-Vst plugins])
##      AC_CHECK_LIB(lo, lo_send, LO_LIB_FOUND="yes", LO_LIB_FOUND="no")
##      PKG_CHECK_MODULES(LO, lo >= 0.2, LO_LIB_FOUND="yes", LO_LIB_FOUND="no")
      PKG_CHECK_MODULES(LO,liblo, [ LO_LIB_FOUND=yes ], [ LO_LIB_FOUND=no ])
      if test x$LO_LIB_FOUND = xno; then
        AC_MSG_ERROR([Library liblo from The Lightweight OSC Implementation is required])
      fi
      
      AC_CHECK_HEADER(dssi.h, DSSI_FOUND="yes", DSSI_FOUND="no")
##      PKG_CHECK_MODULES(DSSI [ DSSI_FOUND=yes ], [ DSSI_FOUND=no ])
      if test x$DSSI_FOUND = xno; then
      AC_MSG_ERROR([Header file dssi.h from DSSI (Disposable Soft Synth Interface) is required])
      fi
fi

if test x$LO_LIB_FOUND = xyes -a x$DSSI_FOUND = xyes ; then
      LO_CFLAGS="$LO_CFLAGS"
      ##LO_LIBS="$LO_LIBS"
      LO_LIBS="-llo"
      DSSI="yes"
      AC_SUBST(LO_CFLAGS)
      AC_SUBST(LO_LIBS)
      AC_DEFINE(DSSI_SUPPORT, 1, [support for DSSI + DSSI-Vst plugins])
fi

AM_CONDITIONAL(DSSI_SUPPORT, test x$LO_LIB_FOUND = xyes -a x$DSSI_FOUND = xyes)

## LO_LIBS="-llo"
## AC_SUBST(LO_CFLAGS)
## AC_SUBST(LO_LIBS)

## PKG_CHECK_MODULES(LO, lo >= 0.23,
##   true,
##   AC_MSG_ERROR([need liblo >= 0.23 ])
## )




######################################


AC_DEFINE(HAVE_AUDIO, 1, [Make audio-specific code])
AM_CONDITIONAL(HAVE_AUDIO, test "yes" = "yes")
AC_SUBST(MUSECXXFLAGS)

if test -n "$GIVERTCAP"; then
      SUIDINSTALL="no"
      SUIDBUILD="no"
      fi


AC_OUTPUT([
Makefile
m4/Makefile
al/Makefile
doc/Makefile
xpm/Makefile
demos/Makefile
grepmidi/Makefile
packaging/Makefile
muse/Makefile
muse/widgets/Makefile
muse/master/Makefile
muse/midiedit/Makefile
muse/arranger/Makefile
muse/liste/Makefile
muse/driver/Makefile
muse/waveedit/Makefile
muse/ctrl/Makefile
muse/instruments/Makefile
muse/mixer/Makefile
muse/cliplist/Makefile
muse/marker/Makefile
muse/mplugins/Makefile
muse/remote/Makefile
share/Makefile
share/drummaps/Makefile
share/html/Makefile
share/locale/Makefile
share/wallpapers/Makefile
share/instruments/Makefile
share/plugins/Makefile
share/templates/Makefile
share/pybridge/Makefile
share/scripts/Makefile
synti/Makefile
synti/libsynti/Makefile
synti/fluidsynth/Makefile
synti/fluid/Makefile
synti/organ/Makefile
synti/s1/Makefile
synti/vam/Makefile
synti/deicsonze/Makefile
synti/deicsonze2/Makefile
synti/simpledrums/Makefile
lib/Makefile
lib/synthi/Makefile
lib/plugins/Makefile
plugins/Makefile
plugins/freeverb/Makefile
plugins/pandelay/Makefile
plugins/doublechorus/Makefile
Doxyfile
utils/Makefile
])

######## Show a configuration report ########
if test -n "$JADE"; then
  jade_report="$JADE";
else
  jade_report="not present"
fi
AC_MSG_NOTICE([

  MusE configured

  LASH support:                  $LASH_FOUND
  DSSI support:                  $DSSI
  FluidSynth:                    $muse_enable_fluidlib

  jade:                          $jade_report
  doxygen:                       $DOXYGEN
  graphviz:                      $DOT
  perl:                          $PERL
  treeviews in doxygen
  html output:                   $report_doxy_treeview

  C++ compiler:                  $CXX
  optimizing:                    $report_optimise
  debug:                         $report_debug
  optimise for arch:             $report_march
  SSE optimizations              $report_sse
  python bindings:               $enable_python

  installation prefix:           $prefix

  Deprecated:
  ---------------------------------------------------
  using rtcap:                   $report_rtcap
  setuid root install:           $SUIDINSTALL
  setuid root build:             $SUIDBUILD
  VST/win support:               $VST

])

