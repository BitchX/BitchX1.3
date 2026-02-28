dnl configure.ac for BitchX 1.3
dnl Modernized for current systems (2024)
dnl
dnl Changes from legacy configure.in:
dnl   - Renamed to configure.ac (autoconf 2.50+ convention)
dnl   - Uses PKG_CHECK_MODULES for OpenSSL and ncurses
dnl   - IPv6 enabled by default (was opt-in, Linux-only)
dnl   - Replaced deprecated macros (AC_TRY_COMPILE, etc.)
dnl   - Removed references to obsolete platforms (DG/UX, etc.)
dnl   - Added modern compiler warning flags
dnl   - Added --enable-hardening for security flags
dnl   - Uses AC_CONFIG_HEADERS (plural)
dnl   - Original configure.in preserved for reference

AC_PREREQ([2.69])
AC_INIT([BitchX], [1.3], [bitchx-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR([source/irc.c])
AC_CONFIG_HEADERS([include/defs.h])
AC_CONFIG_AUX_DIR([.])

topdir="`pwd`"
AC_SUBST(topdir)

dnl Get the version number from source/irc.c.
VERSION=`sed -n -e 's/";$//' -e '/	const char irc_version.. = "/s///p' -e '/const char irc_version/q' < $srcdir/source/irc.c 2>/dev/null`
if test x"$VERSION" = x""; then
  VERSION="BitchX-1.3"
fi
_VERSION_="BitchX"
VERSION_NUMBER="`echo "$VERSION" | sed 's/BitchX-//'`"
AC_DEFINE_UNQUOTED(VERSION_NUMBER, "$VERSION_NUMBER", [BitchX version number.])
AC_SUBST(VERSION_NUMBER)
echo "Welcome to the $VERSION configuration"
echo

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_ARG_PROGRAM
AC_CHECK_PROG(MAKE, gmake, gmake)
if test x"$MAKE" = x""; then
  AC_CHECK_PROG(MAKE, make, make)
fi
AC_SUBST(MAKE)
AC_PROG_MAKE_SET
AC_PROG_AWK

dnl Enable -fno-strict-aliasing
AC_MSG_CHECKING([whether compiler understands -fno-strict-aliasing])
old_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fno-strict-aliasing"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no])
   CFLAGS="$old_CFLAGS"])

dnl Security hardening flags (opt-in)
AC_MSG_CHECKING([whether to enable hardening flags])
AC_ARG_ENABLE([hardening],
  [AS_HELP_STRING([--enable-hardening], [Enable compiler security hardening flags])],
  [enable_hardening=$enableval], [enable_hardening=no])
if test x"$enable_hardening" = x"yes"; then
  AC_MSG_RESULT([yes])
  HARDENING_CFLAGS=""
  for flag in -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE; do
    old_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $flag"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],
      [HARDENING_CFLAGS="$HARDENING_CFLAGS $flag"],
      [])
    CFLAGS="$old_CFLAGS"
  done
  CFLAGS="$CFLAGS $HARDENING_CFLAGS"
  dnl Linker flags
  for flag in -pie -Wl,-z,relro -Wl,-z,now; do
    old_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS $flag"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([],[])],
      [],
      [LDFLAGS="$old_LDFLAGS"])
  done
else
  AC_MSG_RESULT([no])
fi

dnl Checks for libraries.
AC_CHECK_LIB(crypt, crypt, LIBS="-lcrypt $LIBS"; AC_DEFINE(HAVE_CRYPT, 1, [Define if you have crypt in -lcrypt.]),)
AC_CHECK_LIB(m, pow, LIBS="-lm $LIBS",)

dnl Checks for header files.
AC_CHECK_HEADERS([sys/socket.h sys/time.h sys/fcntl.h fcntl.h sys/file.h \
  netinet/in.h netdb.h sys/un.h sys/filio.h regex.h arpa/nameser.h \
  dirent.h sys/ndir.h sys/dir.h ndir.h])

AC_CHECK_HEADERS([net/if.h], [], [],
[AC_INCLUDES_DEFAULT
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

AC_CHECK_HEADERS([arpa/inet.h resolv.h], [], [],
[AC_INCLUDES_DEFAULT
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_NAMESER_H
#include <arpa/nameser.h>
#endif
])

dnl Checks for declarations.
AC_CHECK_DECLARATION(stpcpy, stpcpy, stpcpy, string.h, STPCPY_DECLARED)
AC_CHECK_DECLARATION(getpgid, getpgid, [" getpgid"], unistd.h, GETPGID_DECLARED)
AC_CHECK_DECLARATION(killpg, killpg, killpg, signal.h, KILLPG_DECLARED)
AC_CHECK_DECLARATION(getpass, getpass, getpass, unistd.h, GETPASS_DECLARED)
AC_CHECK_DECLARATION(errno, errno, [int?( |	)_?_?errno], errno.h, ERRNO_DECLARED)
AC_CHECK_DECLARATION(struct linger, struct_linger, [struct( |	)*linger], sys/socket.h, STRUCT_LINGER_DECLARED)
AC_CHECK_DECLARATION(sun_len, sun_len, sun_len, sys/un.h, HAVE_SUN_LEN)
AC_CHECK_DECLARATION(bcopy, bcopy, bcopy, string.h, BCOPY_DECLARED)

AC_CHECK_DECLS([strsignal, sys_siglist, _sys_siglist], , ,
[AC_INCLUDES_DEFAULT
#include <signal.h>])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

AC_CHECK_SIZEOF([unsigned int], 4)
if test x"$ac_cv_sizeof_unsigned_int" = x"4"; then
  AC_DEFINE(UNSIGNED_INT32, 1, [Define if unsigned int is 32 bits.])
else
  AC_CHECK_SIZEOF([unsigned long], 4)
  if test x"$ac_cv_sizeof_unsigned_long" = x"4"; then
    AC_DEFINE(UNSIGNED_LONG32, 1, [Define if unsigned long is 32 bits.])
  else
    AC_DEFINE(UNKNOWN_32INT, 1, [Define if unsure what is 32 bits.])
  fi
fi

dnl Check for resolver library (used by /NSLOOKUP)
AC_CACHE_CHECK([library required for resolver functions], [bx_cv_lib_resolver],
[
OLDLIBS="$LIBS"
bx_cv_lib_resolver="not found"
for reslib in '' '-lresolv' '-lbind' '-lsocket'
do
	LIBS="$reslib $OLDLIBS"
	AC_LINK_IFELSE([AC_LANG_SOURCE([
	#if !defined( __APPLE__ )
	  #if !defined( BIND_4_COMPAT )
	    #define BIND_4_COMPAT
	  #endif
	#else
	  #if !defined( BIND_8_COMPAT )
	    #define BIND_8_COMPAT
	  #endif
	#endif
	AC_INCLUDES_DEFAULT
	#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
	#endif
	#ifdef HAVE_ARPA_NAMESER_H
	#include <arpa/nameser.h>
	#endif
	#ifdef HAVE_RESOLV_H
	#include <resolv.h>
	#endif

	int main()
	{
		return res_mkquery(0, 0, 0, 0, 0, 0, 0, 0, 0);
	}])], [
	if test x"$reslib" != x; then
		bx_cv_lib_resolver="$reslib"
	else
		bx_cv_lib_resolver="none required"
	fi
	break], [])
done
LIBS="$OLDLIBS"
])
if test x"$bx_cv_lib_resolver" != x"not found"; then
	AC_DEFINE(HAVE_RESOLV, 1, [Define if you have the resolv library.])
	if test x"$bx_cv_lib_resolver" != x"none required"; then
		LIBS="$bx_cv_lib_resolver $LIBS"
	fi
fi

dnl Checks for library functions.
AC_FUNC_ALLOCA
if test x"$cross_compiling" != x"yes"; then
  AC_FUNC_GETPGRP
  AC_FUNC_SETPGRP
fi
AC_CHECK_FUNCS([socket gethostname gettimeofday strtoul strlcpy strlcat \
  stpcpy vsnprintf snprintf setsid strerror uname getrusage sysconf \
  getpgid killpg getlogin realpath fchdir getpass fpathconf getpwent \
  setvbuf select mkstemp memmove scandir getaddrinfo getnameinfo \
  inet_pton inet_ntop setenv])

dnl Need these for Solaris/illumos
AC_CHECK_FUNC(gethostbyname,,
  AC_CHECK_LIB(nsl, gethostbyname, LIBS="-lnsl $LIBS",))
AC_CHECK_FUNC(connect,,
  AC_CHECK_LIB(socket, connect, LIBS="-lsocket $LIBS",))
AC_CHECK_FUNC(inet_aton, AC_DEFINE(HAVE_INET_ATON, 1, [Define if inet_aton is available.]),
  AC_CHECK_LIB(resolv, inet_aton, LIBS="-lresolv $LIBS"; AC_DEFINE(HAVE_INET_ATON, 1, [Define if inet_aton is available.]),))

dnl Check for non-blocking type.
AC_CACHE_CHECK([for non-blocking type], [ac_cv_non_blocking_type],
[
precode='#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <signal.h>

typedef void (*sighandler_t)(int);

int alarmed(void)
{
	exit(1);
}

int main()
{
	char b[12], x[32];
	int f, l = sizeof(x);
	f = socket(AF_INET, SOCK_DGRAM, 0);
	if (f >= 0 && (fcntl(f, F_SETFL,'

postcode=') != -1))
	{
		signal(SIGALRM, (sighandler_t)alarmed);
		alarm(3);
		recvfrom(f, b, 12, 0, (struct sockaddr *)x, &l);
		alarm(0);
		exit(0);
	}
	exit(1);
}'

if test x"$cross_compiling" = x"yes"; then
  ac_cv_non_blocking_type=O_NONBLOCK
else
  for type in O_NONBLOCK O_NDELAY FIONBIO; do
    AC_RUN_IFELSE([AC_LANG_SOURCE([$precode $type $postcode])],
      [ac_cv_non_blocking_type=$type; break], [], [])
  done
fi
if test x"$ac_cv_non_blocking_type" = x""; then
  ac_cv_non_blocking_type=O_NONBLOCK
fi
])
AC_DEFINE_UNQUOTED(NBLOCK_TYPE, $ac_cv_non_blocking_type, [Non-blocking I/O type.])

dnl ============================
dnl GUI support options
dnl ============================

dnl Check for Win32 GUI support.
AC_MSG_CHECKING([whether to enable Win32 GUI support])
AC_ARG_WITH([win],
  [AS_HELP_STRING([--with-win], [Enable Win32 GUI support])],
  [case "$withval" in
    yes)
      AC_MSG_RESULT([yes])
      VERSION="Win$VERSION"
      _VERSION_="Win$_VERSION_"
      LIBS="-luser32 -lgdi32 -lws2_32 -lcomctl32 -ldwmapi -luxtheme -lshcore -liphlpapi -Wl,--subsystem,windows $LIBS"
      win32=1
      GUI_SRCS="winbitchx.c"
      ;;
    no)
      AC_MSG_RESULT([no])
      ;;
  esac],
  [AC_MSG_RESULT([no])])

AH_VERBATIM([WIN32],
[/* WIN32 is true if we are building a GUI client under Windows NT */
#if defined(GUI) && defined(WINNT)
#define WIN32 1
#endif])

dnl Check for sound support.
AC_MSG_CHECKING([whether to enable sound support])
AC_ARG_ENABLE([sound],
  [AS_HELP_STRING([--enable-sound], [Enable sound support (requires GUI)])],
  [case "$enableval" in
    yes)
      if test -n "$gtk"; then
        AC_MSG_RESULT([yes])
        AC_CHECK_LIB(esd, esd_play_stream, LIBS="-lesd $LIBS"; AC_DEFINE(SOUND, 1, [Define for sound support.]),
          AC_MSG_RESULT([no])
          AC_MSG_ERROR([sound support requires libesd]))
      elif test x"$win32" = x"1"; then
        AC_MSG_RESULT([yes])
        AC_DEFINE(SOUND, 1, [Define for sound support.])
      else
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([sound support requires GUI BitchX])
      fi
      ;;
    no)
      AC_MSG_RESULT([no])
      ;;
  esac],
  [AC_MSG_RESULT([no])])

dnl ============================
dnl SSL support (auto-detect via pkg-config)
dnl ============================
AC_MSG_CHECKING([whether to enable SSL support])
AC_ARG_WITH([ssl],
  [AS_HELP_STRING([--with-ssl@<:@=PATH@:>@], [Enable SSL support (auto-detected by default)])],
  [], [with_ssl=check])

case "$with_ssl" in
  yes)
    AC_MSG_RESULT([yes])
    ;;
  check)
    AC_MSG_RESULT([auto-detecting])
    ;;
  no)
    AC_MSG_RESULT([no])
    ;;
  *)
    AC_MSG_RESULT([yes ($with_ssl)])
    CFLAGS="$CFLAGS -I${with_ssl}/include"
    LDFLAGS="$LDFLAGS -L${with_ssl}/lib"
    with_ssl=yes
    ;;
esac

case "$with_ssl" in
  yes|check)
    dnl Try pkg-config first (modern approach)
    ssl_found=no
    if test x"$PKG_CONFIG" != x""; then
      PKG_CHECK_MODULES([OPENSSL], [openssl >= 1.1.0], [
        CFLAGS="$CFLAGS $OPENSSL_CFLAGS"
        LIBS="$OPENSSL_LIBS $LIBS"
        AC_DEFINE(HAVE_SSL, 1, [Define if SSL support is available.])
        ssl_found=yes
      ], [ssl_found=no])
    fi
    dnl Fallback to manual detection
    if test x"$ssl_found" = x"no"; then
      AC_CHECK_LIB([crypto], [ERR_get_error], [], [], [])
      AS_IF([test x"$ac_cv_lib_crypto_ERR_get_error" = x"yes"],
        [AC_CHECK_LIB([ssl], [SSL_accept], [], [], [])])
      AS_IF([test x"$ac_cv_lib_ssl_SSL_accept" != x"yes"],
        [AS_IF([test x"$with_ssl" = x"yes"],
          [AC_MSG_FAILURE([--with-ssl given, but could not find OpenSSL.])],
          [AC_MSG_WARN([OpenSSL not found, building without SSL support.])])],
        [AC_DEFINE(HAVE_SSL, 1, [Define if SSL support is available.])])
    fi
    ;;
esac

dnl Now that we've checked for GUI support, define GUI macro
if test -n "$gtk" -o -n "$pm" -o -n "$win32"; then
  AC_DEFINE(GUI, 1, [Define for GUI support (PM or GTK or Win32).])
  gui=1
else
  if test -z "$CYGWIN" -a -z "$EMXOS2" -a -z "$MINGWIN32"; then
    EXTRAS="wserv scr-bx"
    INSTALL_EXTRAS="installwserv installscr-bx"
  fi
fi
AC_SUBST(VERSION)
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [BitchX version string.])
AC_SUBST(_VERSION_)
AC_DEFINE_UNQUOTED(_VERSION_, "$_VERSION_", [Short BitchX version.])
AC_SUBST(EXTRAS)
AC_SUBST(INSTALL_EXTRAS)

dnl ============================
dnl Terminal library detection
dnl ============================
dnl Check for setupterm/tgetent. Skip on CYGWIN/EMXOS2/MINGWIN32.
if test -z "$CYGWIN" -a -z "$EMXOS2" -a -z "$MINGWIN32"; then
  AC_MSG_CHECKING([whether to use tgetent or setupterm])
  AC_ARG_WITH([tgetent],
    [AS_HELP_STRING([--with-tgetent], [Use tgetent (termcap) instead of setupterm (ncurses)])],
    [use_tgetent=yes], [use_tgetent=no])

  if test x"$use_tgetent" = x"yes"; then
    AC_MSG_RESULT([tgetent])
    AC_CHECK_LIB(tinfo, tgetent, tinfo=1; LIBS="-ltinfo $LIBS",
      AC_CHECK_LIB(ncurses, tgetent, ncurses=1; LIBS="-lncurses $LIBS",
        AC_CHECK_LIB(termcap, tgetent, termcap=1; LIBS="-ltermcap $LIBS",
          AC_CHECK_LIB(curses, tgetent, curses=1; LIBS="-lcurses $LIBS",
            AC_MSG_ERROR([cannot find tgetent])))))
  else
    AC_MSG_RESULT([setupterm])
    dnl Try pkg-config for ncurses first (modern approach)
    ncurses_found=no
    if test x"$PKG_CONFIG" != x""; then
      PKG_CHECK_MODULES([NCURSESW], [ncursesw], [
        CFLAGS="$CFLAGS $NCURSESW_CFLAGS"
        LIBS="$NCURSESW_LIBS $LIBS"
        ncurses=1
        ncurses_found=yes
        AC_DEFINE(HAVE_NCURSESW, 1, [Define if ncursesw (wide char) is available.])
      ], [
        PKG_CHECK_MODULES([NCURSES_PC], [ncurses], [
          CFLAGS="$CFLAGS $NCURSES_PC_CFLAGS"
          LIBS="$NCURSES_PC_LIBS $LIBS"
          ncurses=1
          ncurses_found=yes
        ], [ncurses_found=no])
      ])
    fi
    dnl Fallback to manual detection
    if test x"$ncurses_found" = x"no"; then
      AC_CHECK_LIB(ncursesw, setupterm, ncurses=1; LIBS="-lncursesw $LIBS";
        AC_DEFINE(HAVE_NCURSESW, 1, [Define if ncursesw is available.]),
        AC_CHECK_LIB(ncurses, setupterm, ncurses=1; LIBS="-lncurses $LIBS",
          AC_CHECK_LIB(curses, setupterm, curses=1; LIBS="-lcurses $LIBS",
            AC_CHECK_LIB(tinfo, tgetent, tinfo=1; LIBS="-ltinfo $LIBS",
              AC_MSG_ERROR([cannot find setupterm or tgetent. Install ncurses-dev.])))))
    fi
  fi

  dnl Check for tputs and tparm.
  if test x"$ncurses" = x"1"; then
    AC_CHECK_HEADERS([ncurses.h ncurses/termcap.h termcap.h])
    AC_CHECK_LIB(ncurses, tparm, AC_DEFINE(HAVE_TPARM, 1, [Define if tparm is available.]),)
    AC_CHECK_LIB(ncurses, tputs, AC_DEFINE(HAVE_TPUTS, 1, [Define if tputs is available.]),)
  fi
  if test x"$curses" = x"1"; then
    AC_CHECK_HEADERS([curses.h termcap.h])
    AC_CHECK_LIB(curses, tparm, AC_DEFINE(HAVE_TPARM, 1, [Define if tparm is available.]),)
    AC_CHECK_LIB(curses, tputs, AC_DEFINE(HAVE_TPUTS, 1, [Define if tputs is available.]),)
  fi
  if test x"$termcap" = x"1"; then
    AC_CHECK_HEADERS([termcap.h])
    AC_CHECK_LIB(termcap, tparm, AC_DEFINE(HAVE_TPARM, 1, [Define if tparm is available.]),)
    AC_CHECK_LIB(termcap, tputs, AC_DEFINE(HAVE_TPUTS, 1, [Define if tputs is available.]),)
  fi
fi
if test x"$ncurses" = x"1" -o x"$tinfo" = x"1"; then
  AC_DEFINE(HAVE_TERMINFO, 1, [Define if terminfo support is available.])
fi

dnl ============================
dnl IPv6 support (enabled by default)
dnl ============================
AC_MSG_CHECKING([whether to enable IPv6 support])
AC_ARG_ENABLE([ipv6],
  [AS_HELP_STRING([--disable-ipv6], [Disable IPv6 support (enabled by default)])],
  [enable_ipv6=$enableval], [enable_ipv6=yes])

if test x"$enable_ipv6" = x"yes"; then
  AC_LINK_IFELSE([AC_LANG_SOURCE([[
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>

    int main()
    {
      struct sockaddr_in6 sin6;
      sin6.sin6_family = AF_INET6;
      sin6.sin6_addr = in6addr_any;
      return sin6.sin6_addr.s6_addr[0];
    }]])],
    [AC_MSG_RESULT([yes (native)])
     AC_DEFINE(IPV6, 1, [Define for IPv6 support.])],
    [AC_MSG_RESULT([no (system lacks IPv6 support)])
     AC_MSG_WARN([IPv6 not available on this system, continuing without it.])])
else
  AC_MSG_RESULT([disabled])
fi

dnl ============================
dnl Tcl support (optional)
dnl ============================
AC_MSG_CHECKING([whether to enable Tcl support])
AC_ARG_WITH([tcl],
  [AS_HELP_STRING([--with-tcl], [Enable Tcl scripting support])],
  [case "$withval" in
    yes)
      AC_MSG_RESULT([yes])
      AC_DEFINE(WANT_TCL, 1, [Define for Tcl support.])
      AC_CHECK_HEADER(tcl.h, AC_DEFINE(HAVE_TCL_H, 1, [Define if tcl.h exists.]),
        [AC_MSG_ERROR([cannot find tcl.h])])
      AC_CHECK_LIB(tcl, Tcl_SetVar, TCL_LIBS="-ltcl",
        [AC_MSG_ERROR([cannot find Tcl library])])
      LIBS="$TCL_LIBS $LIBS"
      AC_SUBST(TCL_LIBS)
      if test -f "$srcdir/source/tcl.c"; then
        TCL_SRCS="\$(srcdir)/tcl.c"
        TCL_OBJS="tcl.o"
      fi
      AC_SUBST(TCL_SRCS)
      AC_SUBST(TCL_OBJS)
      ;;
    no)
      AC_MSG_RESULT([no])
      ;;
  esac],
  [AC_MSG_RESULT([no])])

dnl ============================
dnl UNIX mail directory
dnl ============================
AC_MSG_CHECKING([for unix mail directory])
AC_ARG_WITH([maildir],
  [AS_HELP_STRING([--with-maildir=PATH], [Set mail directory path])],
  [case "$withval" in
    no)
      AC_MSG_RESULT([none])
      ;;
    *)
      AC_MSG_RESULT([$withval])
      AC_DEFINE(HAVE_QMAIL, 1, [Define for QMAIL support.])
      AC_DEFINE_UNQUOTED(UNIX_MAIL, "$withval", [Mail directory.])
      ;;
  esac],
  [for mdir in /var/spool/mail /var/mail /usr/spool/mail /usr/mail; do
    if test -d "$mdir"; then
      AC_MSG_RESULT([$mdir])
      AC_DEFINE_UNQUOTED(UNIX_MAIL, "$mdir", [Mail directory.])
      break
    fi
  done
  if test ! -d "$mdir"; then
    AC_MSG_RESULT([none])
  fi])

dnl ============================
dnl Default server list
dnl ============================
AC_MSG_CHECKING([for default server list])
AC_ARG_WITH([default-server],
  [AS_HELP_STRING([--with-default-server=SERVER], [Set default IRC server (host:port:pass:nick:network)])],
  [case "$withval" in
    yes|no)
      AC_MSG_RESULT([none])
      ;;
    *)
      addl_servers="`echo "$withval" | sed -e 's/,/ /g'`"
      AC_MSG_RESULT([$addl_servers])
      AC_DEFINE_UNQUOTED(DEFAULT_SERVER, "$addl_servers", [Default server list.])
      ;;
  esac],
  [AC_MSG_RESULT([none])])

dnl ============================
dnl SOCKS proxy support
dnl ============================
AC_MSG_CHECKING([whether to enable SOCKS support])
AC_ARG_WITH([socks],
  [AS_HELP_STRING([--with-socks=4|5], [Enable SOCKS4 or SOCKS5 proxy support])],
  [case "$withval" in
    5)
      AC_MSG_RESULT([SOCKS5])
      AC_DEFINE(SOCKS, 1, [Define for SOCKS support.])
      AC_CHECK_LIB(socks5, SOCKSconnect, LIBS="-lsocks5 $LIBS",
        [AC_MSG_ERROR([cannot find SOCKS5 library])])
      ;;
    4)
      AC_MSG_RESULT([SOCKS4])
      AC_DEFINE(SOCKS, 1, [Define for SOCKS support.])
      CFLAGS="$CFLAGS -Dconnect=Rconnect -Dgetsockname=Rgetsockname -Dgetpeername=Rgetpeername -Dbind=Rbind -Daccept=Raccept -Dlisten=Rlisten -Dselect=Rselect"
      AC_CHECK_LIB(socks, Rconnect, LIBS="-lsocks $LIBS",
        [AC_MSG_ERROR([cannot find SOCKS4 library])])
      ;;
    no)
      AC_MSG_RESULT([no])
      ;;
    *)
      AC_MSG_ERROR([please specify --with-socks=4 or --with-socks=5])
      ;;
  esac],
  [AC_MSG_RESULT([no])])

dnl ============================
dnl Plugin support
dnl ============================
AC_MSG_CHECKING([whether to enable plugin support])
AC_ARG_WITH([plugins],
  [AS_HELP_STRING([--with-plugins@<:@=LIST@:>@], [Enable plugin support (comma-separated list, or 'all')])],
  [case "$withval" in
    yes|all)
      AC_MSG_RESULT([yes (all)])
      AC_CHECK_SHLIB
      AC_CHECK_PLUGIN_SUPPORT
      addl_plugins="`sed -n -e '/^#PLUGINS = /s///p' -e '/^#PLUGINS/q' < $srcdir/dll/Makefile.in`"
      AC_CHECK_PLUGINS
      ;;
    no)
      AC_MSG_RESULT([no])
      ;;
    *)
      AC_MSG_RESULT([yes ($withval)])
      AC_CHECK_SHLIB
      AC_CHECK_PLUGIN_SUPPORT
      addl_plugins="`echo "$withval" | sed -e 's/,/ /g'`"
      AC_CHECK_PLUGINS
      ;;
  esac],
  [AC_MSG_RESULT([no])])

if test -z "$SHLIB_LD"; then
  SHLIB_LD="$CC -shared"
fi
if test -z "$SHLIB_SUFFIX"; then
  SHLIB_SUFFIX=".so"
fi
AC_SUBST(SHLIB_CFLAGS)
AC_SUBST(SHLIB_LD)
AC_SUBST(SHLIB_SUFFIX)

dnl ============================
dnl Final setup
dnl ============================

dnl Add -Wall for gcc, plus additional modern warnings
if test x"$GCC" = x"yes"; then
  if test -z "`echo "$CFLAGS" | grep '\-Wall'`"; then
    CFLAGS="$CFLAGS -Wall"
  fi
  dnl Suppress noisy warnings in legacy code
  CFLAGS="$CFLAGS -Wno-unused-variable -Wno-pointer-sign"
fi

LIBS="`echo "$LIBS" | sed -e 's/-rdynamic//g'`"

dnl Remove duplicates in LIBS and CFLAGS
NEW_LIBS=""
for LIB_IN in $LIBS; do
  dup=no
  for LIB_OUT in $NEW_LIBS; do
    if test x"$LIB_OUT" = x"$LIB_IN"; then
      dup=yes
      break
    fi
  done
  if test x"$dup" = x"no"; then
    NEW_LIBS="$NEW_LIBS $LIB_IN"
  fi
done
LIBS="$NEW_LIBS"

RM="rm -f"
CP="cp -f"
MV="mv -f"

GZIP_ENV="--best"
BZIP2="-9"

if test -n "$EMXOS2" -o -n "$MINGWIN32"; then
  bindir="\$(IRCLIB)"
  INSTALL_IRC="\$(IRCLIB)/\$(_VERSION_)\$(EXEEXT)"
  IRCLIB="\${prefix}/BitchX"
  IRCPATH="\$(DEFAULT_CTOOLZ_DIR):\$(DEFAULT_CTOOLZ_DIR)/plugins:.:\$(PLUGINDIR):\$(INSTALL_SCRIPT):\$(IRCLIB)"
  TRANSLATION_PATH="\$(IRCLIB)/translation"
  HELPDIR="\$(IRCLIB)/help"
  INSTALL_WSERV="\$(IRCLIB)/wserv\$(EXEEXT)"
  INSTALL_SCRBX="\$(IRCLIB)/scr-bx\$(EXEEXT)"
  INSTALL_SCRIPT="\$(IRCLIB)/script"
  PLUGINDIR="\$(IRCLIB)/plugins"
  DEFAULT_CTOOLZ_DIR="/PROGRA~1/BitchX"
  DEFAULT_MSGLOGFILE="BitchX.awy"
  DEFAULT_BITCHX_HELP_FILE="BitchX.hlp"
  DEFAULT_SCRIPT_HELP_FILE="BitchX.hlp"
  DEFAULT_BITCHX_KICK_FILE="BitchX.kck"
  DEFAULT_BITCHX_QUIT_FILE="BitchX.qt"
  DEFAULT_BITCHX_IRCNAME_FILE="BitchX.nam"
  WINNT_INSTALL="\$(INSTALL_DATA) \$(top_srcdir)/doc/BitchX.ico \$(DESTDIR)\$(IRCLIB); \$(INSTALL_DATA) \$(top_srcdir)/doc/BitchX.bat \$(DESTDIR)\$(IRCLIB); \$(INSTALL_DATA) \$(top_srcdir)/doc/bx-rc \$(DESTDIR)\$(IRCLIB); \$(MV) \$(DESTDIR)\$(IRCLIB)/\$(VERSION)\$(EXEEXT) \$(DESTDIR)\$(IRCLIB)/\$(_VERSION_)\$(EXEEXT); \$(RM) \$(DESTDIR)\$(IRCLIB)/\$(_VERSION_).old"
  HINT_FILE="BitchX.hnt"
  mandir="\$(IRCLIB)"
else
  INSTALL_IRC="\${bindir}/\$(_VERSION_)\$(EXEEXT)"
  IRCLIB="\${libdir}/bx"
  IRCPATH="\$(DEFAULT_CTOOLZ_DIR):\$(DEFAULT_CTOOLZ_DIR)/plugins:.:\$(PLUGINDIR):\$(INSTALL_SCRIPT):\$(IRCLIB)"
  TRANSLATION_PATH="\$(IRCLIB)/translation"
  HELPDIR="\$(IRCLIB)/help"
  INSTALL_WSERV="\$(IRCLIB)/wserv\$(EXEEXT)"
  INSTALL_SCRBX="\${bindir}/scr-bx\$(EXEEXT)"
  INSTALL_SCRIPT="\$(IRCLIB)/script"
  PLUGINDIR="\$(IRCLIB)/plugins"
  DEFAULT_CTOOLZ_DIR="~/.BitchX"
  DEFAULT_MSGLOGFILE="BitchX.away"
  DEFAULT_BITCHX_HELP_FILE="BitchX.help"
  DEFAULT_SCRIPT_HELP_FILE="BitchX.help"
  DEFAULT_BITCHX_KICK_FILE="BitchX.kick"
  DEFAULT_BITCHX_QUIT_FILE="BitchX.quit"
  DEFAULT_BITCHX_IRCNAME_FILE="BitchX.ircnames"
  HINT_FILE="BitchX.hints"
fi

INCLUDES="-I. -I\$(topdir)/include -I\$(top_srcdir)/include -I\$(srcdir) -I\$(srcdir)/include"

AC_SUBST(RM)
AC_SUBST(CP)
AC_SUBST(MV)
AC_SUBST(GZIP_ENV)
AC_SUBST(BZIP2)
AC_SUBST(INSTALL_IRC)
AC_SUBST(IRCLIB)
AC_SUBST(IRCPATH)
AC_SUBST(TRANSLATION_PATH)
AC_SUBST(HELPDIR)
AC_SUBST(INSTALL_WSERV)
AC_SUBST(INSTALL_SCRBX)
AC_SUBST(INSTALL_SCRIPT)
AC_SUBST(PLUGINDIR)
AC_SUBST(INCLUDES)
AC_SUBST(GUI_SRCS)
AC_SUBST(HAVE_INET_ATON)

AC_DEFINE_UNQUOTED(DEFAULT_CTOOLZ_DIR, "$DEFAULT_CTOOLZ_DIR", [Default CToolZ directory.])
AC_SUBST(DEFAULT_CTOOLZ_DIR, "$DEFAULT_CTOOLZ_DIR")

AC_DEFINE_UNQUOTED(DEFAULT_MSGLOGFILE, "$DEFAULT_MSGLOGFILE", [Default message log file.])
AC_SUBST(DEFAULT_MSGLOGFILE)
AC_DEFINE_UNQUOTED(DEFAULT_BITCHX_HELP_FILE, "$DEFAULT_BITCHX_HELP_FILE", [Default help file.])
AC_SUBST(DEFAULT_BITCHX_HELP_FILE)
AC_DEFINE_UNQUOTED(DEFAULT_SCRIPT_HELP_FILE, "$DEFAULT_SCRIPT_HELP_FILE", [Default script help.])
AC_SUBST(DEFAULT_SCRIPT_HELP_FILE)
AC_DEFINE_UNQUOTED(DEFAULT_BITCHX_KICK_FILE, "$DEFAULT_BITCHX_KICK_FILE", [Default kick file.])
AC_SUBST(DEFAULT_BITCHX_KICK_FILE)
AC_DEFINE_UNQUOTED(DEFAULT_BITCHX_QUIT_FILE, "$DEFAULT_BITCHX_QUIT_FILE", [Default quit file.])
AC_SUBST(DEFAULT_BITCHX_QUIT_FILE)
AC_DEFINE_UNQUOTED(DEFAULT_BITCHX_IRCNAME_FILE, "$DEFAULT_BITCHX_IRCNAME_FILE", [Default ircname file.])
AC_SUBST(DEFAULT_BITCHX_IRCNAME_FILE)

AC_SUBST(HINT_FILE)
AC_SUBST(WINNT_INSTALL)

dnl Check for pkg-config (used by SSL and ncurses detection above)
PKG_PROG_PKG_CONFIG

AC_CONFIG_FILES([
Makefile
bx-conf/Makefile
doc/BitchX.bat
doc/Makefile
dll/Makefile
dll/abot/Makefile
dll/acro/Makefile
dll/aim/Makefile
dll/aim/toc/Makefile
dll/amp/Makefile
dll/arcfour/Makefile
dll/autocycle/Makefile
dll/blowfish/Makefile
dll/cavlink/Makefile
dll/cdrom/Makefile
dll/encrypt/Makefile
dll/europa/Makefile
dll/fserv/Makefile
dll/hint/Makefile
dll/identd/Makefile
dll/nap/Makefile
dll/nicklist/Makefile
dll/pkga/Makefile
dll/possum/Makefile
dll/qbx/Makefile
dll/qmail/Makefile
dll/wavplay/Makefile
dll/xmms/Makefile
source/Makefile])
AC_OUTPUT

echo
echo "BitchX (c) 1996-2024 Colten Edwards et al"
echo "----------------------------------------------------------"
echo
echo "Configuration summary:"
echo "  Version:      $VERSION"
echo "  C compiler:   $CC"
echo "  CFLAGS:       $CFLAGS"
echo "  SSL support:  $(test x"$ac_cv_lib_ssl_SSL_accept" = x"yes" -o x"$ssl_found" = x"yes" && echo yes || echo no)"
echo "  IPv6 support: $enable_ipv6"
echo "  Plugins:      $(test -n "$addl_plugins" && echo yes || echo no)"
echo
echo "Now type \"$MAKE\" to compile BitchX."
