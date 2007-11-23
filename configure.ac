AC_PREREQ([2.57])

m4_define(cairo_clock_version_major, 0)
m4_define(cairo_clock_version_minor, 3)
m4_define(cairo_clock_version_micro, 4)

AC_INIT([cairo-clock],
	[cairo_clock_version_major().cairo_clock_version_minor().cairo_clock_version_micro()],
	[macslow@bangang.de])

CAIRO_CLOCK_VERSION_MAJOR=cairo_clock_version_major()
CAIRO_CLOCK_VERSION_MINOR=cairo_clock_version_minor()
CAIRO_CLOCK_VERSION_MICRO=cairo_clock_version_micro()
AC_SUBST(CAIRO_CLOCK_VERSION_MAJOR)
AC_SUBST(CAIRO_CLOCK_VERSION_MINOR)
AC_SUBST(CAIRO_CLOCK_VERSION_MICRO)

AM_INIT_AUTOMAKE([1.9])
AC_CONFIG_HEADER([config.h])
AC_PROG_INTLTOOL([0.23])
AM_MAINTAINER_MODE

dnl Check for C compiler
AC_LANG([C])
AC_ISC_POSIX

dnl Checks for various programs
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

dnl Various checks just so we're really sane checking (proposed by AzaToth)
AC_C_VOLATILE

AC_TYPE_MODE_T
AC_TYPE_PID_T

AC_CHECK_HEADERS([ctype.h fcntl.h math.h stdio.h errno.h])

AC_CHECK_FUNCS([dup2])
AC_CHECK_FUNCS([floor])
AC_CHECK_FUNCS([mkdir])
AC_CHECK_FUNCS([rmdir])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strcspn])
AC_CHECK_FUNCS([strncasecmp])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([strtol])

AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STAT

dnl Next four lines is a hack to prevent libtool checking for C++/F77
m4_undefine([AC_PROG_CXX])
m4_defun([AC_PROG_CXX],[])
m4_undefine([AC_PROG_F77])
m4_defun([AC_PROG_F77],[])

dnl Libtool
AC_DISABLE_STATIC
AC_ENABLE_SHARED
AC_PROG_LIBTOOL

dnl intltool
ALL_LINGUAS="da de en_GB es fi fr it ja nl nn pl pt_PT ru sl sv tr zh_CN zh_TW"
AC_SUBST(ALL_LINGUAS)
AM_GLIB_GNU_GETTEXT
GETTEXT_PACKAGE=cairo-clock
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])
AC_SUBST(GETTEXT_PACKAGE)

cairoclocklocaledir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(cairoclocklocaledir)

dnl pkg-config
PKG_PROG_PKG_CONFIG

dnl GLIB
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.8.0 gobject-2.0 gthread-2.0 gmodule-2.0])

dnl GTK
PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.10.0])

dnl GTK Deps+Libglade
PKG_CHECK_MODULES([GTK_DEP],
	[cairo >= 1.2.0
         pango >= 1.10.0
	 pangocairo >= 1.10.0
	 libglade-2.0 >= 2.6.0
	 librsvg-2.0 >= 2.14.0])

dnl CLOCK defines
CLOCK_DEFINES='-DDATA_DIR="\"$(datadir)\"" -DPKGDATA_DIR="\"$(pkgdatadir)\""'
AC_SUBST([CLOCK_DEFINES])

dnl CFLAGS
CFLAGS="$CFLAGS -Wall -pedantic -std=c99 -fno-strict-aliasing -fmessage-length=0 -D_FORTIFY_SOURCE=2"

AC_CONFIG_FILES([
	Makefile
	desktop/Makefile
	pixmaps/Makefile
	glade/Makefile
	src/Makefile
	man/Makefile
	po/Makefile.in
	themes/Makefile
	themes/fdo/Makefile
	themes/simple/Makefile
	themes/default/Makefile
	themes/glassy/Makefile
	themes/gremlin/Makefile
	themes/gremlin-24/Makefile
	themes/zen/Makefile
	themes/antique/Makefile
	themes/radium/Makefile
	themes/funky/Makefile
	themes/default-24/Makefile
	themes/ipulse/Makefile
	themes/ipulse-24/Makefile
	themes/quartz-24/Makefile
	themes/radium-24/Makefile
	themes/silvia/Makefile
	themes/silvia-24/Makefile
	themes/simple-24/Makefile
	themes/tango/Makefile
])

AC_OUTPUT

