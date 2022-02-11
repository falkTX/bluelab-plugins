sinclude([ac-init.generated])

dnl work around lack of support for sinclude in latest libtoolize
ifelse([
AC_INIT
], [], [])

AM_INIT_AUTOMAKE([tar-ustar])

AM_MAINTAINER_MODE
FW_SUBST_PROTECT(USE_MAINTAINER_MODE)

AC_PROG_CC
AC_LANG(C)
AC_PROG_LIBTOOL

sinclude([configure.ac.local])

FW_TEMPLATE_C
FW_BUILD_AUTOMAKE
AC_OUTPUT
