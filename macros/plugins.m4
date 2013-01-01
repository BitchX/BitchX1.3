dnl AC_CHECK_PLUGINS
dnl Copyright (c) David Walluck 1999-2000
dnl All rights reserved.

AC_DEFUN(AC_CHECK_PLUGINS,
[ if test x"$addl_plugins" != x"none"; then
  if test x"`echo $addl_plugins | grep all`" != x""; then
    one_plugins="`sed -n -e '/^#PLUGINS = /s///p' -e '/^#PLUGINS/q' < $srcdir/dll/Makefile.in`"
    two_plugins="`echo "$addl_plugins" | sed 's/all//g'`"
    addl_plugins="$one_plugins $two_plugins"
  fi
  for plugin in $addl_plugins; do
    if test ! -d "$srcdir/dll/$plugin" -o -f "$srcdir/dll/$plugin.c"; then
      AC_MSG_ERROR(plugin directory "$srcdir/dll/$plugin" or file "$srcdir/dll/$plugin.c" not found)
    fi
    # start of sound plugin stuff
    if test x"$checked_sound_plugins" != x"1"; then
      if test x"$plugin" = x"amp" -o x"$plugin" = x"wavplay"; then
        checked_sound_plugins=1
        AC_CHECK_HEADERS(machine/soundcard.h linux/soundcard.h sys/soundcard.h)
        if test x"$ac_cv_header_machine_soundcard_h" = x"no" -a x"$ac_cv_header_linux_soundcard_h" = x"no" -a x"$ac_cv_header_sys_soundcard_h" = x"no"; then
          noamp=1
          nowavplay=1
        fi
        AC_CHECK_FUNC(mlock, , noamp=1)
      fi
      if test x"$noamp" = x"1"; then
        addl_plugins="`echo "$addl_plugins" | sed 's/amp//g'`"
      fi
      if test x"$nowavplay" = x"1"; then
        addl_plugins="`echo "$addl_plugins" | sed 's/wavplay//g'`"
      fi
    fi
    # end of sound plugin stuff
    if test x"$plugin" = x"xmms"; then
      if test x"$checked_xmms_plugin" != x"1"; then
        checked_xmms_plugin=1
        AM_PATH_XMMS(0.9.5.1, AC_CHECK_LIB(c_r, pthread_attr_init, LIBS="$LIBS -lc_r",), addl_plugins="`echo "$addl_plugins" | sed 's/xmms//g'`")
      fi
    fi
    if test x"$plugin" = x"europa"; then
      if test x"$checked_europa_plugin" != x"1"; then
        checked_europa_plugin=1
        AC_CHECK_HEADER(mysql/mysql.h)
        AC_CHECK_LIB(mysqlclient, mysql_query, MYSQL_LIBS="-lmysqlclient", addl_plugins="`echo "$addl_plugins" | sed 's/europa//g'`")
        if test x"$ac_cv_header_mysql_mysql_h" = x"no"; then
          addl_plugins="`echo "$addl_plugins" | sed 's/europa//g'`"
        fi
        AC_SUBST(MYSQL_LIBS)
      fi
    fi
  done
  fi
  PLUGINS="`echo "$addl_plugins" | sed 's/  / /g'`"
  AC_MSG_CHECKING(which plugins to build)
  AC_MSG_RESULT($PLUGINS)
  if test x"$PLUGINS" = x"none"; then
    PLUGINS=""
  fi
  AC_SUBST(PLUGINS)
  AC_DEFINE(HAVE_DLLIB, 1, Define this if you want loadable module support.)
  AH_VERBATIM([WANT_DLL],
  [/*
 * Define this if you have shlib support and want plugin support in BitchX
 * Note: Not all systems support this.
 */
#ifdef HAVE_DLLIB
#define WANT_DLL
#endif])
])
