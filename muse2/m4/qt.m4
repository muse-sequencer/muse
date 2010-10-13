dnl
dnl CONFIGURE_QT([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl \author J.E. Hoffmann <je-h@gmx.net>
dnl
AC_DEFUN([CONFIGURE_QT],
[
  qt_error="no"

  QT_CFLAGS=""
  QT_LIBS=""
  MOC="moc"
  UIC="uic"
  qt_includes="/usr/include/qt"
  qt_libraries=""
  qt_binaries=""

  if test "$qt_error" = "no"; then
    AC_MSG_CHECKING([for QT environment variable QTDIR])
    if test -z "$QTDIR"; then
      AC_MSG_RESULT(no)
      AC_MSG_WARN([

    ***************** WARNING *****************

YOU HAVE NOT SET YOUR 'QTDIR' ENVIRONMENT VARIABLE!!!

This is the source of most people's problems when
configuring muse.  If the configuration fails to find
qt, try setting your QTDIR environment variable to
the directory where qt is installed.

    *******************************************

])
      echo -ne "\a"
      echo -ne "\a"
      echo -ne "\a"
      sleep 7
    else
      AC_MSG_RESULT(yes)
      qt_includes="$QTDIR/include"
      qt_libraries="$QTDIR/lib"
      qt_binaries="$QTDIR/bin"
      MOC="$qt_binaries/moc";
      UIC="$qt_binaries/uic";
    fi
  fi

  muse_qttest="yes"
  AC_ARG_ENABLE(qttest,
  [  --disable-qttest        do not try to compile and run a test libqt program],[
    case "$enableval" in
      "yes")
        muse_qttest="yes"
        ;;
      "no")
        muse_qttest="no"
        ;;
      *)
        AC_MSG_ERROR([must use --enable-qttest(=yes/no) or --disable-qttest])
      ;;
    esac
  ])


  AC_ARG_WITH(qt-prefix,
    [  --with-qt-prefix=PFX    where the root of Qt is installed ],
    [
      qt_includes="$withval/include"
      qt_libraries="$withval/lib"
      qt_binaries="$withval/bin"
      MOC="$qt_binaries/moc";
      UIC="$qt_binaries/uic";
    ])

  AC_ARG_WITH(qt-includes,
    [  --with-qt-includes=DIR  where the Qt includes are installed ],
    [
      qt_includes="$withval"
    ])

  AC_ARG_WITH(qt-libraries,
    [  --with-qt-libraries=DIR where the Qt libraries are installed.],
    [
      qt_libraries="$withval"
    ])

  AC_ARG_WITH(qt-binaries,
    [  --with-qt-binaries=DIR  where the Qt binaries are installed.],
    [
      qt_binaries="$withval"
      MOC="$qt_binaries/moc";
      UIC="$qt_binaries/uic";
    ])

  AC_ARG_WITH(qt-moc,
    [  --with-qt-moc=PROG      where the Qt meta object compiler is installed.],
    [
      MOC="$withval"
    ])

  AC_ARG_WITH(qt-uic,
    [  --with-qt-uic=PROG      where the Qt user interface compiler is installed.],
    [
      UIC="$withval"
    ])

  if test "$qt_error" = "no"; then
    saved_CPPFLAGS="$CPPFLAGS"
    saved_LIBS="$LIBS"
    CPPFLAGS="$saved_CPPFLAGS -I$qt_includes"
    if test -n "$qt_libraries"; then
      LIBS="$saved_LIBS -L$qt_libraries -lqt-mt -lqui"
      QT_LIBS="-L$qt_libraries -lqt-mt -lqui"
    else
      LIBS="$saved_LIBS -lqt-mt -lqui -lm $X11_LIBS"
      QT_LIBS="-lqt-mt -lqui"
    fi
    AC_MSG_CHECKING([for QT includes ($qt_includes)])
    AC_CACHE_VAL(qt_includes_found,
    [
      AC_LANG_PUSH(C++)
      AC_TRY_CPP([#include <qapplication.h>],
                 qt_includes_found=yes, qt_includes_found=no)
      AC_LANG_POP(C++)
      if test "$qt_includes_found" = "yes"; then
        QT_CFLAGS="-I$qt_includes"
        AC_MSG_RESULT(yes)
      else
        qt_error=yes
        AC_MSG_RESULT(no)
      fi
    ])

    AC_MSG_CHECKING([for QT libraries ($qt_libraries)])
    AC_CACHE_VAL(qt_libraries_found,
    [
      AC_LANG_SAVE
      AC_LANG_CPLUSPLUS
      saved_CXXFLAGS="$CXXFLAGS"
      saved_LIBS="$LIBS"
      CXXFLAGS="$QT_CFLAGS $LIBS"
      LIBS="$QT_LIBS $LIBS"


      if test "$muse_qttest" = "yes"; then
          AC_TRY_RUN([
              #include <qapplication.h>
              int main(int argc, char **argv)
              {
                QApplication app(argc, argv, false);
	
              }
            ],[
              AC_MSG_RESULT(yes)
            ],[
              AC_MSG_RESULT(no)
              qt_error="yes"
            ],
            AC_MSG_ERROR([cross compiling unsupported])
          )
      else
        AC_MSG_RESULT([yes (assumed due to --disable-qttest)])
      fi

      LIBS="$saved_LIBS"
      CXXFLAGS="$saved_CXXFLAGS"
      AC_LANG_RESTORE
    ])

    AC_MSG_CHECKING([for QT moc ($MOC)])
    output=`eval "$MOC --help 2>&1 | grep Qt"`
    if test -z "$output"; then
      AC_MSG_RESULT(no)
      qt_error="yes"
    else
      AC_MSG_RESULT(yes)
    fi

    AC_MSG_CHECKING([for QT uic ($UIC)])
    output=`eval "$UIC --help 2>&1 | grep Qt"`
    if test -z "$output"; then
      AC_MSG_RESULT(no)
      qt_error="yes"
    else
      AC_MSG_RESULT(yes)
    fi
    CPPFLAGS="$saved_CPPFLAGS"
    LIBS="$saved_LIBS"
  fi

  if test "$qt_error" = "no"; then
    AC_MSG_CHECKING([for QT version >= $1])
    qt_major_version=`echo $1 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    qt_minor_version=`echo $1 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    qt_micro_version=`echo $1 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    qt_version="$qt_major_version$qt_minor_version$qt_micro_version"

    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    saved_CXXFLAGS="$CXXFLAGS"
    saved_LIBS="$LIBS"
    CXXFLAGS="$QT_CFLAGS $LIBS"
    LIBS="$QT_LIBS $LIBS"

    if test "$muse_qttest" = "yes"; then
        AC_TRY_RUN([
            #include <qglobal.h>
            int main()
            {
            int version = ($qt_major_version << 16)
               + ($qt_minor_version << 8)
               + $qt_micro_version;
              if (QT_VERSION < version) return(1);
              return(0);
            }
          ],[
            AC_MSG_RESULT(yes)
          ],[
            AC_MSG_RESULT(no)
            qt_error="yes"
          ],
          AC_MSG_ERROR([cross compiling unsupported])
        )
    else
      AC_MSG_RESULT([yes (assumed due to --disable-qttest)])
    fi

    LIBS="$saved_LIBS"
    CXXFLAGS="$saved_CXXFLAGS"
    AC_LANG_RESTORE
  fi

  AC_SUBST(QT_CFLAGS)
  AC_SUBST(QT_LIBS)
  AC_SUBST(MOC)
  AC_SUBST(UIC)
  if test "$qt_error" = "no"; then
     ifelse([$2], , :, [$2])
  else
     ifelse([$3], , :, [$3])
  fi
])
