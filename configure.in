dnl configure.in for BitchX 1.0
dnl Copyright (c) 1999-2001 David Walluck
dnl All rights reserved.

AC_PREREQ(2.59)
AC_REVISION($Revision: 208 $)
AC_INIT
AC_CONFIG_SRCDIR([$srcdir/source/irc.c])
topdir="`pwd`"
AC_SUBST(topdir)
AC_CONFIG_HEADERS(include/defs.h)

dnl Get the version number from source/irc.c.
dnl Maybe it's better to set the VERSION in here instead.
VERSION=`sed -n -e 's/";$//' -e '/	const char irc_version.. = "/s///p' -e '/const char irc_version/q' < $srcdir/source/irc.c 2>/dev/null`
if test x"$VERSION" = x""; then
  VERSION="BitchX-1.0"
fi
_VERSION_="BitchX"
VERSION_NUMBER="`echo "$VERSION" | sed 's/BitchX-//'`"
AC_DEFINE_UNQUOTED(VERSION_NUMBER, "$VERSION_NUMBER", Define BitchX version number here.)
AC_SUBST(VERSION_NUMBER)
echo Welcome to the "$VERSION" configuration
echo

dnl Checks for programs.
dnl _AC_EMXOS2
dnl _AC_CYGWIN
AC_PROG_CC
AC_PROG_INSTALL
AM_SANITY_CHECK
AC_PROG_LN_S
AC_ARG_PROGRAM
AC_CHECK_PROG(MAKE, gmake, gmake)
if test x"$MAKE" = x""; then
  AC_CHECK_PROG(MAKE, make, make)
fi
AC_SUBST(MAKE)
AC_PROG_MAKE_SET
AC_PROG_GCC_TRADITIONAL
AC_ISC_POSIX
AC_AIX
AC_PROG_AWK

dnl Enable -fno-strict-aliasing, if the compiler supports it.
dnl Hopefully this is a temporary measure, until the code can be fixed.
AC_MSG_CHECKING(whether compiler understands -fno-strict-aliasing)
old_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fno-strict-aliasing"
AC_TRY_COMPILE([],[],
  AC_MSG_RESULT(yes),
  AC_MSG_RESULT(no) 
  CFLAGS="$old_CFLAGS")

dnl Checks for libraries.
AC_CHECK_LIB(sun, getpwnam, LIBS="-lsun $LIBS",)
AC_CHECK_LIB(dgc, inet_addr, LIBS="-ldgc $LIBS",)
AC_CHECK_LIB(crypt, crypt, LIBS="-lcrypt $LIBS"; AC_DEFINE(HAVE_CRYPT, 1, Define this if you have a crypt implementation in -lcrypt),)
AC_CHECK_LIB(m, pow, LIBS="-lm $LIBS",)

dnl Checks for header files.
AC_HEADER_DIRENT
AC_CHECK_HEADERS([sys/socket.h sys/time.h sys/fcntl.h fcntl.h sys/file.h netinet/in.h netdb.h sys/un.h sys/filio.h regex.h arpa/nameser.h dirent.h sys/ndir.h sys/dir.h ndir.h])

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
AC_PROG_CC_STDC
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL
AC_HEADER_TIME

AC_CHECK_SIZEOF(unsigned int, 4)
if test x"$ac_cv_sizeof_unsigned_int" = x"4"; then
  AC_DEFINE(UNSIGNED_INT32, 1, Define this if an unsigned long is 32 bits.)
else
  AC_CHECK_SIZEOF(unsigned long, 4)
  if test x"$ac_cv_sizeof_unsigned_long" = x"4"; then
    AC_DEFINE(UNSIGNED_LONG32, 1, Define this if an unsigned int is 32 bits.)
  else
    AC_DEFINE(UNKNOWN_32INT, 1, Define this if you are unsure what is 32 bits.)
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
	AC_DEFINE(HAVE_RESOLV, 1, Define this if you have the resolv library.)
	if test x"$bx_cv_lib_resolver" != x"none required"; then
		LIBS="$bx_cv_lib_resolver $LIBS"
	fi
	dnl This is for getting the DNS servers on Windows
	AC_MSG_CHECKING([for the iphlpapi library])
	OLDLIBS="$LIBS"
	LIBS="-liphlpapi $LIBS"
	AC_LINK_IFELSE([[
	#include <windows.h>
	#include <iphlpapi.h>

	int main()
	{
		ULONG buflen;
		GetNetworkParams(0, &buflen);
		return buflen;
	}]], [
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_LIBIPHLPAPI, 1, [Define to 1 if you have the Windows iphlpapi library])
	],[
	AC_MSG_RESULT(no)
	LIBS="$OLDLIBS"
	])
else
	AC_MSG_WARN([No resolver library found, /nslookup will be disabled.])
fi

dnl Checks for library functions.
AC_FUNC_ALLOCA
if test x"$cross_compiling" != x"yes"; then
  AC_FUNC_GETPGRP
  AC_FUNC_SETPGRP
fi
AC_FUNC_STRFTIME
AC_CHECK_FUNCS(socket gethostname gettimeofday strtoul strlcpy strlcat stpcpy vsnprintf snprintf setsid strerror uname getrusage sysconf getpgid killpg getlogin realpath fchdir getpass fpathconf getpwent setvbuf select mkstemp memmove scandir)
dnl Need these for Solaris
AC_CHECK_FUNC(gethostbyname,,
  AC_CHECK_LIB(nsl, gethostbyname, LIBS="-lnsl $LIBS",))
AC_CHECK_FUNC(connect,,
  AC_CHECK_LIB(socket, connect, LIBS="-lsocket $LIBS",))
AC_CHECK_FUNC(inet_aton, AC_DEFINE(HAVE_INET_ATON, 1, Define this if inet_aton is in the system), AC_CHECK_LIB(resolv, inet_aton, LIBS="-lresolv $LIBS" ; AC_DEFINE(HAVE_INET_ATON, 1, Define this if inet_aton is in the system.),))

dnl Checking this produces a warning from autoconf.
if test x"$cross_compiling" = x"no"; then
  AC_FUNC_SETVBUF_REVERSED
else
  AC_MSG_RESULT(cross-compiling - assuming setvbuf params are not reversed)
fi

dnl Check for non-blocking type. This is from ircii-2.8.2's configure.in.
dnl (c) 1993 Matthew Green
dnl (c) 1999 David Walluck

AC_CACHE_CHECK(for non-blocking type, ac_cv_non_blocking_type,
[ changequote(<<, >>)dnl
<<
precode='#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <signal.h>

typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler);

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
>>
changequote([, ])dnl

code="$precode O_NONBLOCK $postcode"

AC_TRY_RUN($code,
  ac_cv_non_blocking_type="posix",
  code="$precode O_NDELAY $postcode"
  AC_TRY_RUN($code,
    ac_cv_non_blocking_type="bsd",
    code="$precode FIONBIO $postcode"
    AC_TRY_RUN($code,
      ac_cv_non_blocking_type="unknown",
      ac_cv_non_blocking_type="cross-compiling",
    :),
  :),
:)])

case "$ac_cv_non_blocking_type" in
  posix)
    AC_DEFINE(NBLOCK_POSIX, 1, Define this if your non-blocking type is posix.)
    ;;
  bsd)
    AC_DEFINE(NBLOCK_BSD, 1, Define this if your non-blocking type is bsd.)
    ;;
  sysv)
    AC_DEFINE(NBLOCK_SYSV, 1, Define this if your non-blocking type is sysv.)
    ;;
  unknown)
    AC_MSG_WARN(cannot find a working non-blocking system)
    ;;
  cross)
    AC_MSG_WARN(cross-compiling: assuming non-blocking is posix)
    AC_DEFINE(NBLOCK_POSIX, 1, Define this if your non-blocking type is posix.)
    ;;
esac

AH_VERBATIM([NON_BLOCKING_CONNECTS],
[/*
 * Are we doing non-blocking connects? Note: SOCKS support precludes us from
 * using this feature.
 */
#if (defined(NBLOCK_POSIX) || defined(NBLOCK_BSD) || defined(NBLOCK_SYSV)) && !defined(SOCKS)
#define NON_BLOCKING_CONNECTS
#endif])

dnl This is all stolen from perl-4.036's Configure. larry is god.
dnl Well, most of it. Couple of tweaks to make it work better.
dnl This fails on cygwin, and probably emxos2, because the header files aren't
dnl in the standard "/usr/include" location. There's got to be a better way.
AC_CACHE_CHECK(for a list of signal names, ac_cv_signal_names,
[ if test x"$ac_cv_have_decl_sys_siglist" != x"yes"; then
    if test x"$ac_cv_have_decl__sys_siglist" != x"yes"; then
      if test x"$ac_cv_have_decl_strsignal" != x"yes"; then
        if test -f "$srcdir/include/sig.inc"; then
          rm -f $srcdir/include/sig.inc
        fi
  set X `cat $oldincludedir/signal.h $oldincludedir/sys/signal.h \
  $oldincludedir/linux/signal.h 2>&1 | sed 's/^#[ 	]*/#/' | \
  $AWK '$1 ~ /^#define$/ && $2 ~ /^SIG[A-Z0-9]*$/ && $3 ~ /^[1-9][0-9]*$/ {
    [sig[$3] = substr($2,4,20)]
    if (max < $3 && $3 < 60) {
      max = $3
    }
  }

  END {
    for (i=1; i<=max; i++) {
      [if (sig[i] == "")]
        printf "%d", i
      else
        [printf "%s", sig[i]]
      if (i < max)
        printf " "
    }
  printf "\n"
  }
  '`
  shift
  case "$#" in
    0)
      set X `kill -l 2> /dev/null`
      shift
      case "$#" in
        0)
          if test -f /bin/csh; then
            set X `/bin/csh -cf 'kill -l'`
            shift
            case "$#" in
              0)
                set HUP INT QUIT ILL TRAP ABRT EMT FPE KILL BUS SEGV SYS PIPE ALRM TERM
                ;;
            esac
          fi
          ;;
      esac
      ;;
  esac

  sig_name="ZERO $*"

  [ echo "$sig_name" | sed -e 's/$/"};/' -e 's/ /", "/g' -e 's/^/char *sys_siglist[] = { "/' -e 's/};/, NULL};/' > $srcdir/include/sig.inc ]

        if test -f "$srcdir/include/sig.inc"; then
          ac_cv_signal_names="sig.inc"
        else
          ac_cv_signal_names="none"
        fi
      else
        ac_cv_signal_names="strsignal"
      fi 
    else
      ac_cv_signal_names="_sys_siglist"
    fi 
  else
    ac_cv_signal_names="sys_siglist"
  fi
])
if test x"$ac_cv_signal_names" = x"none"; then
  AC_MSG_WARN(something is wrong. Check to see that the file "$srcdir/include/sig.inc" exists.) 
fi

dnl Check for GTK support.
AC_MSG_CHECKING(whether to enable GTK support)
AC_ARG_WITH(gtk,
[  --with-gtk              Enable GTK support],
[ case "$withval" in
    yes)
      AC_MSG_RESULT(yes)
      AM_PATH_GLIB(1.2.0,,AC_MSG_ERROR(AC_MSG_ERROR(cannot find GLIB)))
      AM_PATH_GTK(1.2.0,,AC_MSG_ERROR(AC_MSG_ERROR(cannot fing GTK)), gthread)
      AC_CHECK_HEADER(pthread.h,,AC_MSG_ERROR(cannot find pthread.h))
      AC_CHECK_LIB(pthread, pthread_attr_init, PTHREAD_LIBS="-lpthread",
        AC_CHECK_LIB(pthreads, pthread_attr_init, PTHREAD_LIBS="-lpthreads",
          AC_CHECK_LIB(c_r, pthread_attr_init, PTHREAD_LIBS="-lc_r",
            AC_MSG_ERROR(cannot find pthreads))))
      AM_PATH_GDK_IMLIB(1.9.4, AC_DEFINE(USE_IMLIB, 1, Define this if you want imlib support.), AC_MSG_ERROR(cannot find imlib))
      AM_PATH_GNOME(1.0.0, AC_DEFINE(USE_GNOME, 1, Define this if you want GNOME support.)
      [AC_TRY_RUN([
      int main()
      {
        int major = 1;
        int minor = 0;
      	int micro = 10;

	if ($gnome_major_version > major || $gnome_minor_version > minor || $gnome_micro_version >= micro)
		exit(0);
	exit(1);
      }
      ],[
      if test x"$gnome_micro_version" = x"10"; then
        AC_MSG_ERROR(GNOME $gnome_major_version.$gnome_minor_version.$gnome_micro_version is broken. Please upgrade gnome-libs.)
      fi
      AC_DEFINE(HAVE_NEW_ZVT, 1, Define this if your ZVT is newer than 1.0.10.)], , AC_MSG_WARN(cross-compiling: assuming new ZVT); AC_DEFINE(HAVE_NEW_ZVT, 1, Define this if your ZVT is newer than 1.0.10.))],
      AC_MSG_ERROR(GNOME not found or too old))
      AC_MSG_CHECKING(for ZVT)
      ZVT_LIBS="`$GNOME_CONFIG $gnome_config_args --libs-only-l zvt`"
      if test x"$ZVT_LIBS" != x""; then
        AC_MSG_RESULT(yes)
        AC_DEFINE(USE_ZVT, 1, Define this if you want ZVT support.)
        AC_MSG_CHECKING(whether ZVT depends on gnome-libs)
        AC_TRY_RUN([
        int main()
        {
          int major = 1;
          int minor = 2;
          int micro = 0;

          if ($gnome_major_version >= major && $gnome_minor_version >= minor)
                  exit(0);
          exit(1);
        }],[ AC_MSG_RESULT(no)
             NEED_GNOME=0
         ], [AC_MSG_RESULT(yes)
             NEED_GNOME=1
         ], [AC_MSG_WARN(cross-compiling: assuming yes)
             NEED_GNOME=1
        ])
      else
        AC_MSG_RESULT(no)
        AC_MSG_ERROR(cannot find ZVT)
      fi
      if test x"$NEED_GNOME" = x"1"; then
        G_CFLAGS="$GLIB_CFLAGS $GTK_CFLAGS $GDK_IMLIB_CFLAGS $GNOME_CFLAGS $ZVT_CFLAGS"
        G_LIBS="$GLIB_LIBS $GTK_LIBS $PTHREAD_LIBS $GDK_IMLIB_LIBS $GNOME_LIBS $ZVT_LIBS"
      else
        G_CFLAGS="$GLIB_CFLAGS $GTK_CFLAGS $GDK_IMLIB_CFLAGS $ZVT_CFLAGS"
        G_LIBS="$GLIB_LIBS $GTK_LIBS $PTHREAD_LIBS $GDK_IMLIB_LIBS $ZVT_LIBS"
      fi
      CFLAGS="$CFLAGS $G_CFLAGS"
      LIBS="$G_LIBS $LIBS"
      VERSION="gtk$VERSION"
      _VERSION_="gtk$_VERSION_"
      AC_DEFINE(GTK, 1, Define this if you want GTK support.)
      X_O="X.o"
      AC_SUBST(X_O)
      gtk=1
      GUI_SRCS="gtkbitchx.c"
      ;;
    no)
      AC_MSG_RESULT(no)
      ;;
  esac ],
AC_MSG_RESULT(no)
)

dnl Check for Win32 GUI support.
AC_MSG_CHECKING(whether to enable Win32 GUI support)
AC_ARG_WITH(win,
[  --with-win              Enable Win32 GUI support],
[ case "$withval" in
    yes)
      AC_MSG_RESULT(yes)
      VERSION="Win$VERSION"
      _VERSION_="Win$_VERSION_"
      dnl Here we could just use LDFLAGS="-mwindows", but I prefer this way
      dnl so that we know exactly what's going on.
      LIBS="-luser32 -lgdi32 -Wl,--subsystem,windows $LIBS"
      win32=1
      GUI_SRCS="winbitchx.c"
      ;;
    no)
      AC_MSG_RESULT(no)
      ;;
  esac ],
AC_MSG_RESULT(no)
)

AH_VERBATIM([WIN32],
[/* WIN32 is true if we are building a GUI client under Windows NT */
#if defined(GUI) && defined(WINNT)
#define WIN32 1
#endif])

dnl Check for PM support.
AC_MSG_CHECKING(whether to enable OS/2 PM support)
AC_ARG_WITH(pm,
[  --with-pm               Enable OS/2 PM support],
[ case "$withval" in
    yes)
      AC_MSG_RESULT(yes)
      VERSION="PM$VERSION"
      _VERSION_="PM$_VERSION_"
      AC_DEFINE(__EMXPM__, 1, Define this is you want OS/2 PM support.)
      LIBS="pmbitchx.def pmbitchx.res -ldw $LIBS"
      pm=1
      GUI_SRCS="pmbitchx.c"
      ;;
    no)
      AC_MSG_RESULT(no)
      ;;
  esac ],
AC_MSG_RESULT(no)
)

dnl Check for sound support.
AC_MSG_CHECKING(whether to enable sound support)
AC_ARG_ENABLE(sound,
[  --enable-sound          Enable sound support (Currently requires GUI BitchX)],
[ case "$enableval" in
    yes)
      AC_MSG_RESULT(yes)
      AC_DEFINE(SOUND, 1, Define this is you want sound support via ESD.)
      if test x"$pm" = x"1"; then
        LIBS="-lmmpm $LIBS"
      else
        if test x"$gtk" = x"1"; then
          AM_PATH_ESD(0.2.5, CFLAGS="$CFLAGS $ESD_CFLAGS"; LIBS="$LIBS $ESD_LIBS", AC_MSG_ERROR(AC_MSG_ERROR(cannot find ESD)))
          AM_PATH_AUDIOFILE(0.1.5, CFLAGS="$CFLAGS $AUDIOFILE_CFLAGS"; LIBS="$LIBS $AUDIOFILE_LIBS", AC_MSG_ERROR(AC_MSG_ERROR(cannot find AUDIOFILE)))
        else
          if test x"$win32" = x"1"; then
            : dnl -lmmsystem already included in link
          else
            AC_MSG_RESULT(no)
            AC_MSG_ERROR(sound support can only be enabled in GUI BitchX.)
          fi
        fi
      fi
      ;;
    no)
      AC_MSG_RESULT(no)
      ;;
  esac ],
AC_MSG_RESULT(no)
)

dnl Check for SSL support.
AC_MSG_CHECKING(whether to enable SSL support)
AC_ARG_WITH(ssl,
[[  --with-ssl[=PATH]       Enable SSL support (enabled by default if OpenSSL found)]],
[], [with_ssl=check])

case "$with_ssl" in
    yes)
      AC_MSG_RESULT(yes)
      ;;
    check)
      AC_MSG_RESULT(if available)
      ;;
    no)
      AC_MSG_RESULT(no)
      ;;
    *)
      AC_MSG_RESULT(yes)
      CFLAGS="$CFLAGS -I${with_ssl}/include"
      LDFLAGS="$LDFLAGS -L${with_ssl}/lib"
      with_ssl=yes
      ;;
esac
case "$with_ssl" in
    yes|check)
      AC_CHECK_LIB([crypto], [SSLeay], [], [], [])
      if test x"$ac_cv_lib_crypto_SSLeay" = x"yes"; then
        AC_CHECK_LIB([ssl], [SSL_accept], [], [], [])
      fi
      if test x"$ac_cv_lib_ssl_SSL_accept" = x"yes"; then
        dnl This would be unnecessary if we used HAVE_LIBSSL in the code
        AC_DEFINE(HAVE_SSL, 1, Define this if the system has SSL support.)
      else
        if test x"$with_ssl" = x"yes"; then
          AC_MSG_FAILURE([--with-ssl given, but could not find OpenSSL.])
        else
          AC_MSG_WARN([OpenSSL not found, will not have SSL support.])
        fi
      fi
      ;;
esac

dnl Now that we've checked for PM and GTK support, make sure
dnl that we "#define GUI" and and that we don't build wserv or
dnl scr-bx for PMBitchX or GTKBitchX, and substitue the BitchX
dnl version now that we have the correct one.
if test -n "$gtk" -o -n "$pm" -o -n "$win32"; then
  AC_DEFINE(GUI, 1, Define this if you want GUI support (PM or GTK or Win32).)
  gui=1
else
  if test -z "$CYGWIN" -a -z "$EMXOS2" -a -z "$MINGWIN32"; then
    EXTRAS="wserv scr-bx"
    INSTALL_EXTRAS="installwserv installscr-bx"
  fi
fi
AC_SUBST(VERSION)
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", Define long BitchX version here.)
AC_SUBST(_VERSION_)
AC_DEFINE_UNQUOTED(_VERSION_, "$_VERSION_", Define short BitchX version here.)
AC_SUBST(EXTRAS)
AC_SUBST(INSTALL_EXTRAS)

dnl Check for setupterm/tgetent. *DO NOT* check on CYGWIN or EMXOS2 because
dnl term support is built-in.
if test -z "$CYGWIN" -a -z "$EMXOS2" -a -z "$MINGWIN32"; then
  AC_MSG_CHECKING(whether to use use tgetent or setupterm)
  AC_ARG_WITH(tgetent,
  [  --with-tgetent          Use tgetent (termcap) instead of setupterm (ncurses) ],
  [   AC_MSG_RESULT(tgetent)
      AC_CHECK_LIB(tinfo, tgetent, tinfo=1; LIBS="-ltinfo $LIBS",
        AC_CHECK_LIB(mytinfo, tgetent, tinfo=1; LIBS="-lmytinfo $LIBS",
          AC_CHECK_LIB(termcap, tgetent, termcap=1; LIBS="-ltermcap $LIBS",
            AC_CHECK_LIB(termlib, tgetent, termlib=1; LIBS="-ltermlib $LIBS",
              AC_CHECK_LIB(curses,  tgetent, curses=1; LIBS="-lcurses $LIBS",
              AC_MSG_WARN(cannot find tgetent - trying setupterm)
              AC_CHECK_LIB(ncurses, setupterm, ncurses=1; LIBS="-lncurses $LIBS",
                AC_CHECK_LIB(curses,  setupterm, ncurses=1; LIBS="-lcurses $LIBS",
                  AC_CHECK_LIB(tinfo,   setupterm, tinfo=1; LIBS="-ltinfo $LIBS",
                    AC_MSG_ERROR(cannot find tgetent or setupterm)))))))))],
  [   AC_MSG_RESULT(setupterm)
      AC_CHECK_LIB(ncurses, setupterm, ncurses=1; LIBS="-lncurses $LIBS",
        AC_CHECK_LIB(curses,  setupterm, curses=1; LIBS="-lcurses $LIBS",
          AC_CHECK_LIB(tinfo,   tgetent, tinfo=1; LIBS="-ltinfo $LIBS",
            AC_CHECK_LIB(mytinfo,   tgetent, tinfo=1; LIBS="-lmytinfo $LIBS",
            AC_CHECK_LIB(tinfo,   setupterm, tinfo=1; LIBS="-ltinfo $LIBS",
              AC_MSG_WARN(cannot find setupterm - trying tgetent)
              AC_CHECK_LIB(termlib, tgetent, termlib=1; LIBS="-ltermlib $LIBS",
                AC_CHECK_LIB(termcap, tgetent, termcap=1; LIBS="-ltermcap $LIBS",
                  AC_CHECK_LIB(curses, tgetent, curses=1; LIBS="-lcurses $LIBS",
                    AC_MSG_ERROR(cannot find setupterm or tgetent)))))))))])

  dnl Check for tputs and tparm.
  if test x"$ncurses" = x"1"; then
    AC_CHECK_DECLARATION(tparm, tparm, [tparm( |        |)], ncurses.h, TPARM_DECLARED)
    AC_CHECK_LIB(ncurses, tparm, AC_DEFINE(HAVE_TPARM, 1, Define this if you have the tparm function in an included lib.),)
    AC_CHECK_HEADERS(ncurses.h ncurses/termcap.h termcap.h)
    AC_CHECK_DECLARATION(tputs, tputs, [tputs( |        |)], ncurses/termcap.h, TPUTS_DECLARED)
    AC_CHECK_DECLARATION(tputs, tputs, [tputs( |        |)], termcap.h, TPUTS_DECLARED)
    AC_CHECK_LIB(ncurses, tputs, AC_DEFINE(HAVE_TPUTS, 1, Define this if you have the tputs function in an included lib.),)
  fi
  if test x"$curses" = x"1"; then
    AC_CHECK_HEADERS(curses.h termcap.h)
    AC_CHECK_DECLARATION(tparm, tparm, [tparm( |        |)], curses.h, TPARM_DECLARED)
    AC_CHECK_LIB(curses, tparm, AC_DEFINE(HAVE_TPARM, 1, Define this if you have the tparm function in an included lib.),)
    AC_CHECK_DECLARATION(tputs, tputs, [tputs( |        |)], curses.h, TPUTS_DECLARED)
    AC_CHECK_LIB(curses, tputs, AC_DEFINE(HAVE_TPUTS, 1, Define this if you have the tputs function in an included lib.),)
    AC_CHECK_DECLARATION(tputs, tputs, [tputs( |        |)], termcap.h, TPUTS_DECLARED)
  fi
  if test x"$termcap" = x"1"; then
    AC_CHECK_HEADERS(termcap.h)
    AC_CHECK_DECLARATION(tparm, tparm, [tparm( |        |)], termcap.h, TPARM_DECLARED)
    AC_CHECK_LIB(termcap, tparm, AC_DEFINE(HAVE_TPARM, 1, Define this if you have the tparm function in an included lib.),)
    AC_CHECK_DECLARATION(tputs, tputs, [tputs( |        |)], termcap.h, TPUTS_DECLARED)
    AC_CHECK_LIB(termcap, tputs, AC_DEFINE(HAVE_TPUTS, 1, Define this if you have the tputs function in an included lib.),)
  fi
fi
if test x"$ncurses" = x"1" -o x"$tinfo" = x"1"; then
  AC_DEFINE(HAVE_TERMINFO, 1, Define this if you have terminfo support.)
fi

dnl Check for Tcl.
AC_MSG_CHECKING(whether to enable Tcl support)
AC_ARG_WITH(tcl,
[  --with-tcl              Enable Tcl support],
[ case "$withval" in
    yes)
      AC_MSG_RESULT(yes)
      AC_DEFINE(WANT_TCL, 1, Define this if you want Tcl support.)
      dnl For some reason, panasync's system needs this check.
      AC_CHECK_SHLIB
      AC_CHECK_HEADER(tcl.h, AC_DEFINE(HAVE_TCL_H, 1, Define if you have the <tcl.h> header file.), 
AC_MSG_CHECKING(for alternate tcl.h)
for startdir in  /usr/include /usr/local /opt
do
	for file in `(find $startdir -name tcl.h | sort -r) 2>/dev/null`
	do
        	if test "$TCL_INC" = ""; then
                	genname=`echo $file | grep generic`
                        if test "$genname" = ""; then
                		TCL_INC=`echo $file | sed s@tcl.h@@`
                                TCL_INC="-I$TCL_INC"
                                AC_MSG_RESULT(yes)
		        fi
                fi
	done
done

if test "$TCL_INC" = ""; then      
	AC_MSG_RESULT(no)
	AC_MSG_ERROR(cannot find tcl.h)
fi
CFLAGS="$CFLAGS $TCL_INC")

      dnl A lot of people can't seem to install Tcl correctly, for whatever
      dnl reason, including cygwin.
      AC_CHECK_LIB(tcl, Tcl_SetVar, TCL_LIBS="-ltcl",
AC_MSG_CHECKING(for alternate libtcl)
for startdir in /lib /usr/lib /usr/local /opt
do
	for file in `(find $startdir -name "libtcl*" | sort -r) 2>/dev/null`
	do
        	if test "$TCL_LIBS" = ""; then
	        	for num in 1 2 3 4 5 6 7 8 9
	        	do
				testname=`echo $file | cut -f $num -d / | grep libtcl`
                	        stubname=`echo $testname | grep libtclstub`
                        	if test "$stubname" = ""; then
		                	if test "$testname" != ""; then
	                	                [[tcllib=`echo $testname | sed s@.so.[0-9]@@`]]
        	                	        tcllib=`echo $tcllib | sed s@.so@@`
                	                	tcllib=`echo $tcllib | sed s@.a@@`
	                	                tcllib=`echo $tcllib | sed s@libtcl@tcl@`
                	        	        tclpath=`echo $file | sed s@"$testname"@@`
                                	        TCL_LIBS="-l$tcllib -L$tclpath"
					        AC_MSG_RESULT(yes)
	                                fi
		                fi
        		done
                fi
	done
done)
      
      if test "$TCL_LIBS" = ""; then
	AC_MSG_RESULT(no)
      	AC_MSG_ERROR(cannot find Tcl)
      fi
      LIBS="$TCL_LIBS $LIBS"
      AC_SUBST(TCL_LIBS)
      AC_MSG_CHECKING(for $srcdir/source/tcl.c)
      if test -f "$srcdir/source/tcl.c"; then
        AC_MSG_RESULT(yes)
        TCL_SRCS="\$(srcdir)/tcl.c"
        AC_SUBST(TCL_SRCS)
        TCL_OBJS="tcl.o"
        AC_SUBST(TCL_OBJS)
        dnl If we have tcl.c, then remove tcl.o, or tcl.c may not get 
        dnl compiled
        rm -f source/tcl.o
        dnl To be safe, remove tcl.o from here, too.
        rm -f $srcdir/source/tcl.o
      else
        AC_MSG_RESULT(no)
        TCL_SRCS=""
        AC_SUBST(TCL_SRCS)
        AC_MSG_CHECKING(for source/tcl.o)
        if test -f "source/tcl.o"; then
         dnl tcl.o already here, so don't over-write with tcl-linux.o.
         got_tcl_objs=1
        else
          mkdir source 2>/dev/null
          cp -f $srcdir/tcl-linux.o source/tcl.o 2>/dev/null
          if test -f "source/tcl.o"; then
            got_tcl_objs=1
          fi
        fi
        if test x"$got_tcl_objs" = x"1"; then
          AC_MSG_RESULT(yes)
          TCL_OBJS="tcl.o"
          AC_SUBST(TCL_OBJS)
        else
          AC_MSG_RESULT(no)
          AC_MSG_ERROR(cannot find "$srcdir/source/tcl.c" or "source/tcl.o")
        fi
      fi
      ;;
    no)
      AC_MSG_RESULT(no)
      ;;
  esac ],
AC_MSG_RESULT(no)
)

dnl Work out UNIX mail dir and check for QMAIL support.
AC_MSG_CHECKING(for unix mail directory)
AC_ARG_WITH(maildir,
[  --with-maildir=PATH     Enable QMAIL support],
[ case "$withval" in
    no)
      AC_MSG_RESULT(none)
      ;;
    *)
      AC_MSG_RESULT($withval)
      AC_DEFINE(HAVE_QMAIL, 1, Define this if you want QMAIL support.)
      AC_DEFINE_UNQUOTED(UNIX_MAIL, "$withval", Define your maildir here.)
      ;;
  esac ],
  for mdir in /var/spool/mail /usr/spool/mail /var/mail /usr/mail; do
    if test -d "$mdir"; then
      AC_MSG_RESULT($mdir)
      AC_DEFINE_UNQUOTED(UNIX_MAIL, "$mdir", Define your maildir here.)
      break
    fi
  done

  if test ! -d "$mdir"; then
    AC_MSG_RESULT(none)
  fi
)

dnl Check for default server list.
AC_MSG_CHECKING(for default server list)
AC_ARG_WITH(default-server,
[  --with-default-server=SERVERNAME[:PORT:PASSWORD:NICK:SERVERNETWORK]
                          Connect to SERVERNAME by default
                          Additional servers can be separated by a comma],
[ case "$withval" in
    yes)
      AC_MSG_RESULT(none)
      ;;
    no)
      AC_MSG_RESULT(none)
      ;;
    *)
      addl_servers="`echo "$withval" | sed -e 's/,/ /g'`"
      AC_MSG_RESULT($addl_servers)
      AC_DEFINE_UNQUOTED(DEFAULT_SERVER, "$addl_servers", Define a list of default servers here.)
      ;;
  esac ],
AC_MSG_RESULT(none)
)

AC_MSG_CHECKING(whether to enable IPv6 support)
AC_ARG_ENABLE(ipv6,
[  --enable-ipv6           Enable IPv6 support (Linux only)],
[ case "$enableval" in
    yes)
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
      }]])], [ ac_cv_ipv6_type="native" ], [
        if test -d "/usr/inet6/include"; then
          CFLAGS="$CFLAGS -I/usr/inet6/include"
          LIBS="-L/usr/inet6/lib -linet6 $LIBS"
          ac_cv_ipv6_type="libinet6"
        else
          if test -d "/usr/local/v6/lib"; then
            LIBS="-L/usr/local/v6/lib -linet6 $LIBS"
            ac_cv_ipv6_type="freebsd+kame"
          else
            ac_cv_ipv6_type="none"
          fi
        fi])

      if test x"$ac_cv_ipv6_type" != x"none"; then
        AC_MSG_RESULT(yes ($ac_cv_ipv6_type))
        AC_DEFINE(IPV6, 1, Define this if you want IPV6 support.)
      else
        AC_MSG_RESULT(none found)
      fi
      ;;
    no)
      AC_MSG_RESULT(no)
      ;;
  esac ],
AC_MSG_RESULT(no)
)

dnl Check for SOCKS support.
AC_MSG_CHECKING(whether to enable SOCKS support)
AC_ARG_WITH(socks,
[  --with-socks=4|5        Enable external SOCKS4/SOCKS5 support],
[ case "$withval" in
    yes)
      AC_MSG_ERROR(please specify SOCKS4 or SOCKS5)
      ;;
    no)
      AC_MSG_RESULT(no)
      ;;
    5)
      AC_MSG_RESULT(yes)
      AC_DEFINE(SOCKS, 1, Define this if you want SOCKS support.)
      AC_CHECK_LIB(socks5, SOCKSconnect, LIBS="-lsocks5 $LIBS", AC_MSG_ERROR(cannot find SOCKS5))
      ;;
    4)
      AC_MSG_RESULT(yes)
      AC_DEFINE(SOCKS, 1, Define this if you want SOCKS support.)
      CFLAGS="$CFLAGS -Dconnect=Rconnect -Dgetsockname=Rgetsockname -Dgetpeername=Rgetpeername -Dbind=Rbind -Daccept=Raccept -Dlisten=Rlisten -Dselect=Rselect"
      AC_CHECK_LIB(socks, Rconnect, LIBS="-lsocks $LIBS", AC_MSG_ERROR(cannot find SOCKS4))
      ;;
  esac ],
AC_MSG_RESULT(no)
)

dnl Check for CD-ROM support.
AC_MSG_CHECKING(whether to enable CD-ROM support)
AC_ARG_ENABLE(cdrom,
[  --enable-cdrom          Enable CD-ROM support (tested on Linux only)],
[ case "$enableval" in
    yes)
      case "$(uname -s)" in
        Linux)
          AC_MSG_RESULT(yes)
          CD_SRCS="cdrom.c"
          CD_OBJS="cdrom.o"
          AC_SUBST(CD_SRCS)
          AC_SUBST(CD_OBJS)
          AC_DEFINE(WANT_CD, 1, Define this if you want CD-ROM support.)
          ;;
        *)
          AC_MSG_RESULT(no (cdrom support can currently be enabled on Linux only))
          ;;
      esac
      ;;
    no)
      AC_MSG_RESULT(no)
      ;;
  esac ],
AC_MSG_RESULT(no)
)

dnl Check for dmalloc.
AC_MSG_CHECKING(whether to enable dmalloc debugging support)
AC_ARG_ENABLE(debug,
[  --enable-debug          Enable dmalloc debugging support (console BX only)],
[ case "$enableval" in
    yes)
      if test x"$gui" != x"1"; then
        AC_MSG_RESULT(yes)
        AC_CHECK_LIB(dmalloc, dmalloc_debug, CFLAGS="$CFLAGS -O -g -DMEM_DEBUG" LIBS="-ldmalloc $LIBS", AC_MSG_ERROR(cannot find dmalloc))
      else
        AC_MSG_RESULT(no)
      fi
      ;;
    no)
      AC_MSG_RESULT(no)
      ;;
  esac ],
AC_MSG_RESULT(no)
)

dnl Check for plugin support and which plugins to build.
AC_MSG_CHECKING(whether to enable plugin support)
AC_ARG_WITH(plugins,
[[  --with-plugins[=LIST]   Enable plugin support
                          LIST is a comma separated list of plugins to build
                          If none are specified, all available plugins are built]],
[ case "$withval" in
    yes | all)
      AC_MSG_RESULT(yes)
      AC_CHECK_SHLIB
      AC_CHECK_PLUGIN_SUPPORT
      addl_plugins="`sed -n -e '/^#PLUGINS = /s///p' -e '/^#PLUGINS/q' < $srcdir/dll/Makefile.in`"
      AC_CHECK_PLUGINS
      ;;
    no)
      AC_MSG_RESULT(not building with plugin support - you will not be able to use plugins)
      ;;
    *)
      AC_MSG_RESULT(yes)
      AC_CHECK_SHLIB
      AC_CHECK_PLUGIN_SUPPORT
      addl_plugins="`echo "$withval" | sed -e 's/,/ /g'`"
      AC_CHECK_PLUGINS
      ;;
  esac ],
AC_MSG_RESULT(not building with plugin support - you will not be able to use plugins)
)
if test -z "$SHLIB_LD"; then
  SHLIB_LD="$CC -shared"
fi
if test -z "$SHLIB_SUFFIX"; then
  SHLIB_SUFFIX=".so"
fi
AC_SUBST(SHLIB_CFLAGS)
AC_SUBST(SHLIB_LD)
AC_SUBST(SHLIB_SUFFIX)

dnl Finish up

dnl Add -Wall to our CFLAGS if we're using gcc
if test x"$GCC" = x"yes" -a -z "`echo "$CFLAGS" | grep \\\-Wall`"; then
  CFLAGS="$CFLAGS -Wall"
fi

LIBS="`echo "$LIBS" | sed -e 's/-rdynamic//g'`"

dnl Remove dupes in LIBS and CFLAGS
NEW_LIBS=""
for LIB_IN in $LIBS; do
	for LIB_OUT in $NEW_LIBS; do
		if test x"$LIB_OUT" = x"$LIB_IN"; then
			LIB_IN=""
			break
		fi
	done
	NEW_LIBS="$NEW_LIBS $LIB_IN"
done
LIBS="$NEW_LIBS"

NEW_CFLAGS=""
for CFLAG_IN in $CFLAGS; do
	for CFLAG_OUT in $NEW_CFLAGS; do
		if test x"$CFLAG_OUT" = x"$CFLAG_IN"; then
			CFLAG_IN=""
			break
		fi
	done
	NEW_CFLAGS="$NEW_CFLAGS $CFLAG_IN"
done
CFLAGS="$NEW_CFLAGS"

RM="rm -f"
CP="cp -f"
MV="mv -f"

GZIP_ENV="--best"
BZIP2="-9 --repetitive-best"

dnl Should I keep this or not?
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

dnl Maybe someday we can #define these here
dnl AC_DEFINE_UNQUOTED(DEFAULT_HELP_PATH, "$HELPDIR", BitchX help directory.)
dnl AC_DEFINE_UNQUOTED(IRCLIB, "$IRCLIB", BitchX library path.)
dnl AC_DEFINE_UNQUOTED(IRCPATH, "$IRCPATH", BitchX path.)
dnl AC_DEFINE_UNQUOTED(SHLIB_SUFFIX, "$SHLIB_SUFFIX", Plugin suffix.)
dnl AC_DEFINE_UNQUOTED(PLUGINDIR, "$PLUGINDIR", Plugin directory.)
dnl AC_DEFINE_UNQUOTED(SCRIPT_PATH, "$INSTALL_SCRIPT", Script Directory.)
dnl AC_DEFINE_UNQUOTED(WSERV_PATH, "$INSTALL_WSERV", Wserv path.)
dnl AC_DEFINE_UNQUOTED(TRANSLATION_PATH, "$TRANSLATION_PATH", Translation path.)

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

AC_DEFINE_UNQUOTED(DEFAULT_CTOOLZ_DIR, "$DEFAULT_CTOOLZ_DIR", Default CToolZ directory.)
AC_SUBST(DEFAULT_CTOOLZ_DIR, "$DEFAULT_CTOOLZ_DIR")

AC_DEFINE_UNQUOTED(DEFAULT_MSGLOGFILE, "$DEFAULT_MSGLOGFILE", Default MsgLog file.)
AC_SUBST(DEFAULT_MSGLOGFILE)
AC_DEFINE_UNQUOTED(DEFAULT_BITCHX_HELP_FILE, "$DEFAULT_BITCHX_HELP_FILE", Default BitchX help file.)
AC_SUBST(DEFAULT_BITCHX_HELP_FILE)
AC_DEFINE_UNQUOTED(DEFAULT_SCRIPT_HELP_FILE, "$DEFAULT_SCRIPT_HELP_FILE", Default BitchX script file.)
AC_SUBST(DEFAULT_SCRIPT_HELP_FILE)
AC_DEFINE_UNQUOTED(DEFAULT_BITCHX_KICK_FILE, "$DEFAULT_BITCHX_KICK_FILE", Default BitchX kick file.)
AC_SUBST(DEFAULT_BITCHX_KICK_FILE)
AC_DEFINE_UNQUOTED(DEFAULT_BITCHX_QUIT_FILE, "$DEFAULT_BITCHX_QUIT_FILE", Default BitchX quit file.)
AC_SUBST(DEFAULT_BITCHX_QUIT_FILE)
AC_DEFINE_UNQUOTED(DEFAULT_BITCHX_IRCNAME_FILE, "$DEFAULT_BITCHX_IRCNAME_FILE", Default BitchX ircname file.)
AC_SUBST(DEFAULT_BITCHX_IRCNAME_FILE)

AC_SUBST(HINT_FILE)
AC_SUBST(WINNT_INSTALL)

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
source/Makefile ])
AC_CONFIG_COMMANDS([default],[[ echo timestamp > stamp-h 
]],[[]])
AC_OUTPUT

echo
echo BitchX \(c\) 1996-2012 Colten Edwards et al
echo ----------------------------------------------------------
echo
echo The configuration script has finished. You should look through
echo \"include/config.h\" and make any changes you would like to make.
echo
echo Now type \"$MAKE\" to compile BitchX.
