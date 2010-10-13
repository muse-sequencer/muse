#!/bin/sh

AUTOMAKE_REQ=1.7
AUTOCONF_REQ=2.54
LIBTOOL_REQ=2.2.10
PKG_CONFIG_REQ=0.1

lessthan () {
  ver1="$1"
  ver2="$2"

  major1=$( echo $ver1 | sed "s/^\([0-9]*\)\..*/\1/");
  minor1=$( echo $ver1 | sed "s/^[^\.]*\.\([0-9]*\).*/\1/" );
  major2=$( echo $ver2 | sed "s/^\([0-9]*\)\..*/\1/");
  minor2=$( echo $ver2 | sed "s/^[^\.]*\.\([0-9]*\).*/\1/" );
  test "$major1" -lt "$major2" || test "$minor1" -lt "$minor2";
}

morethan () {
  ver1="$1"
  ver2="$2"

  major1=$( echo $ver1 | sed "s/^\([0-9]*\)\..*/\1/");
  minor1=$( echo $ver1 | sed "s/^[^\.]*\.\([0-9]*\).*/\1/" );
  major2=$( echo $ver2 | sed "s/^\([0-9]*\)\..*/\1/");
  minor2=$( echo $ver2 | sed "s/^[^\.]*\.\([0-9]*\).*/\1/" );
  test "$major2" -lt "$major1" || test "$minor2" -lt "$minor1";
}

echo -n "automake version: "
amver=$( automake --version | head -1 | sed "s/.* //" );
echo -n "$amver"

lessthan $amver $AUTOMAKE_REQ
if test $? = 0; then
  echo " (not ok)"
  echo "
####################################################################
#########################  WARNING  ################################
####################################################################

                 You need automake >= ${AUTOMAKE_REQ}!


"
  sleep 1;
else
  echo " (ok)"
fi

echo -n "autoconf version: "
acver=$( autoconf --version | head -1 | sed "s/.* //" );
echo -n "$acver"
lessthan $acver $AUTOCONF_REQ
if test $? = 0; then
  echo " (not ok)"
  echo "
####################################################################
#########################  WARNING  ################################
####################################################################

                  You need autoconf >= ${AUTOCONF_REQ}!


"
  sleep 1;
else
  echo " (ok)"
fi

if [ ! -f `which libtool` ] ; then
  echo "No libtool installed"
  exit 1
fi
echo -n "libtool version: "
ltver=$( libtool --version | cut -d ' ' -f 4 | head -1 );
echo -n "$ltver"
#lessthan $ltver $LIBTOOL_REQ
#if test $? = 0; then
#  echo " (not ok)"
#  echo "
#####################################################################
##########################  WARNING  ################################
#####################################################################
#
#                  You need libtool < ${LIBTOOL_REQ}!
#
#
#"
#else
    echo " (ok)"
    sleep 1;
#  fi


echo -n "pkg-config: "
pkg_config="$( which pkg-config )"
if test -z "$pkg_config"; then
  echo "(not found)"
  echo "
####################################################################
#########################  WARNING  ################################
####################################################################

                    You need pkg-config!


"
else
  echo "$pkg_config"
  echo -n "pkg-config version: "
  pcver=$( pkg-config --version )
  echo -n "$pcver"
  lessthan $pcver $PKG_CONFIG_REQ
  if test $? = 0; then
    echo " (not ok)"
    echo "
####################################################################
#########################  WARNING  ################################
####################################################################

               You need pkg-config >= ${PKG_CONFIG_REQ}!


"
  else
    echo " (ok)"
    sleep 1;
  fi
fi

###  && libtoolize -f

echo -n "generating build system.."
libtoolize -f \
  && echo -n "." && aclocal -I m4 \
  && echo -n "." && autoheader \
  && echo -n "." && automake -a --include-deps \
  && echo -n "." && autoconf && echo "done" \
  && echo "

  You may now run configure

  Eg: ./configure --enable-maintainer-mode \\
      --disable-doxy-treeview --enable-optimize
"

