# $Header: /cvsroot/lmuse/muse/m4/docbook.m4,v 1.1.1.1 2003/10/27 18:51:13 wschweer Exp $

# PGAC_PROG_JADE
# --------------
AC_DEFUN([PGAC_PROG_JADE],
[AC_CHECK_PROGS([JADE], [openjade jade])])


# PGAC_PROG_NSGMLS
# ----------------
AC_DEFUN([PGAC_PROG_NSGMLS],
[AC_CHECK_PROGS([NSGMLS], [onsgmls nsgmls])])


# PGAC_CHECK_DOCBOOK(VERSION)
# ---------------------------
AC_DEFUN([PGAC_CHECK_DOCBOOK],
[AC_REQUIRE([PGAC_PROG_NSGMLS])
AC_CACHE_CHECK([for DocBook V$1], [pgac_cv_check_docbook],
[cat >conftest.sgml <<EOF
<!doctype book PUBLIC "-//OASIS//DTD DocBook V$1//EN">
<book>
 <title>test</title>
 <chapter>
  <title>random</title>
   <sect1>
    <title>testsect</title>
    <para>text</para>
  </sect1>
 </chapter>
</book>
EOF

${NSGMLS-false} -s conftest.sgml 1>&5 2>&1
if test $? -eq 0; then
  pgac_cv_check_docbook=yes
else
  pgac_cv_check_docbook=no
fi
rm -f conftest.sgml])

have_docbook=$pgac_cv_check_docbook
AC_SUBST([have_docbook])
])
# PGAC_CHECK_DOCBOOK


# PGAC_PATH_DOCBOOK_STYLESHEETS
# -----------------------------
AC_DEFUN([PGAC_PATH_DOCBOOK_STYLESHEETS], [
AC_MSG_CHECKING([for DocBook stylesheets])
AC_ARG_WITH(docbook-stylesheets,
  [  --with-docbook-stylesheets=DIR  use DIR/html/docbook.dsl],
  [muse_docbook_prefix="$withval"])
AC_CACHE_VAL([pgac_cv_path_stylesheets], [
if test -n "$muse_docbook_prefix"; then
  if test -r "$muse_docbook_prefix/html/docbook.dsl" \
     && test -r "$muse_docbook_prefix/print/docbook.dsl"; then
    pgac_cv_path_stylesheets="$muse_docbook_prefix"
  fi
fi
if test -z "$pgac_cv_path_stylesheets"; then
  if test -n "$DOCBOOKSTYLE"; then
    if test -r "$DOCBOOKSTYLE/html/docbook.dsl" \
       && test -r "$DOCBOOKSTYLE/print/docbook.dsl"; then
      pgac_cv_path_stylesheets="$DOCBOOKSTYLE"
    fi
  fi
fi
if test -z "$pgac_cv_path_stylesheets"; then
  for pgac_prefix in /usr /usr/local /opt; do
    for pgac_infix in share lib; do
      for pgac_postfix in \
        sgml/stylesheets/nwalsh-modular \
        sgml/stylesheets/docbook \
        sgml/docbook/dsssl/modular \
	sgml/docbook/stylesheet/dsssl/modular
      do
        pgac_candidate=$pgac_prefix/$pgac_infix/$pgac_postfix
        if test -r "$pgac_candidate/html/docbook.dsl" \
           && test -r "$pgac_candidate/print/docbook.dsl"
        then
          pgac_cv_path_stylesheets=$pgac_candidate
          break 3
        fi
      done
    done
  done
fi
])
DOCBOOKSTYLE=$pgac_cv_path_stylesheets
AC_SUBST([DOCBOOKSTYLE])
if test -n "$DOCBOOKSTYLE"; then
  AC_MSG_RESULT([$DOCBOOKSTYLE])
else
  AC_MSG_RESULT(no)
fi]) # PGAC_PATH_DOCBOOK_STYLESHEETS
