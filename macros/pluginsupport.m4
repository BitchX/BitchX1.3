dnl AC_CHECK_PLUGIN_SUPPORT
dnl Copyright (c) David Walluck 2000
dnl All rights reserved.

AC_DEFUN(AC_CHECK_PLUGIN_SUPPORT,
[ if test x"$plugins" != x"0"; then
    AC_CHECK_HEADER(dlfcn.h)
    AC_CACHE_CHECK(for system version, ac_cv_system_version,
    [ if test -f "/usr/lib/NextStep/software_version"; then
        system=NEXTSTEP-"`$AWK '/3/,/3/' /usr/lib/NextStep/software_version`"
      else
        system="`uname -s`-`uname -r`"
        if test -z "$system"; then
            AC_MSG_RESULT(unknown \(can't find uname command\))
            system="unknown"
        else
          if test -r "/etc/.relid" -a x"`uname -n`" = x"`uname -s`"; then
            system="MP-RAS-`$AWK '{print $[3]}' /etc/.relid`"
          fi
        fi
      fi
      ac_cv_system_version="$system"
    ])
    dnl I really wish I could ditch all of this in favor of libtool
    case "$ac_cv_system_version" in
      AIX*)
        SHLIB_LD="ld -shared"
        ;;
      BSD/OS-2* | BSD/OS-3*)
        SHLIB_LD="ld -r"
        ;;
      BSD/OS-4*)
        SHLIB_LD="$CC -shared"
        ;;
      HP-UX-*9* | HP-UX-*10* | HP-UX-*11*)
        SHLIB_CFLAGS="+Z"
        SHLIB_LD="ld"
        SHLIB_SUFFIX=".sl"
        ;;
      IRIX*32*)
        SHLIB_LD="ld -shared -32"
        ;;
      IRIX*64*)
        SHLIB_LD="ld -shared -64"
        ;;
      Linux*)
        SHLIB_CFLAGS="-fPIC"
        ;;
      MP-RAS-02*)
        SHLIB_CFLAGS="-KPIC"
        SHLIB_LD="$CC -G"
        ;;
      MP-RAS-*)
        SHLIB_CFLAGS="-KPIC"
        SHLIB_LD="$CC -G"
        LDFLAGS="$LDFLAGS -Wl,-Bexport"
        ;;
      NetBSD*)
        SHLIB_CFLAGS="-fPIC"
        if echo __ELF__ | $CC -E - | grep __ELF__ >/dev/null; then
            SHLIB_LD="ld -Bshareable"
        else
            SHLIB_LD="ld -shared"
        fi
        ;;
      FreeBSD-1*)
        dnl It sucks that I to have to do this.
        AC_MSG_ERROR(sorry, no plugins on $ac_cv_system_version)
        ;;
      FreeBSD-2.2*)
        SHLIB_LD="ld -Bshareable"
        ;;
      FreeBSD-2*)
        SHLIB_LD="ld -Bshareable"
        ;;
      FreeBSD*)
        SHLIB_CFLAGS="-fPIC"
        SHLIB_LD="$CC -shared"
        ;;
      OpenBSD*)
        SHLIB_CFLAGS="-fPIC"
        SHLIB_LD="$CC -shared"
        ;;
      BSDI*)
        SHLIB_CFLAGS="-fPIC"
        SHLIB_LD="$CC -shared"
        ;;
      NEXTSTEP*)
        SHLIB_LD="$CC -nostdlib -r"
        ;;
      OSF1-1.0 | OSF1-1.1 | OSF1-1.2)
        SHLIB_LD="ld -R -export \$(PLUGIN_NAME)"
        ;;
      OSF1-1*)
        SHLIB_CFLAGS="-fpic"
        SHLIB_LD="ld -shared"
        ;;
      OSF1-V*)
        SHLIB_LD="ld -shared -expect_unresolved \*"
        ;;
      OSF3* | OSF4*)
        SHLIB_LD="ld -shared -expect_unresolved \*"
        ;;
      SCO_SV-3.2*)
        SHLIB_CFLAGS="-Kpic -belf"
        SHLIB_LD="ld -G"
        LDFLAGS="ldFLAGS -belf -Wl,-Bexport"
        ;;
      SINIX*5.4*)
        SHLIB_CFLAGS="-K PIC"
        SHLIB_LD="$CC -G"
        ;;
      Solaris*)
        if test x"$GCC" = x"yes"; then
          SHLIB_LD="ld -shared"
        else
          SHLIB_CFLAGS="-KPIC"
          SHLIB_LD="$CC -shared"
        fi
        ;;
      SunOS-4*)
        if test x"$GCC" = x"yes"; then
          SHLIB_LD="ld -shared"
        else
          SHLIB_CFLAGS="-PIC"
          SHLIB_LD="$CC"
        fi
        ;;
      SunOS-5*)
        if test x"$GCC" = x"yes"; then
          SHLIB_LD="ld -shared"
        else
          SHLIB_CFLAGS="-KPIC"
          SHLIB_LD="$CC"
        fi
        ;;
      UNIX_SV* | SYSTEM_V* | DYNIX/ptx*)
        SHLIB_CFLAGS="-KPIC"
        SHLIB_LD="$CC -G"
        hold_ldflags="$LDFLAGS"
        AC_TRY_LINK(, , [
        LDFLAGS="-Wl,-Bexport"
        AC_MSG_RESULT(yes)
        ], [
        AC_MSG_RESULT(no)
        LD_FLAGS="$hold_ldflags"
        ])
        ;;
      CYGWIN*)
        SHLIB_SUFFIX=".dll"
        SHLIB_LD="dllwrap --export-all --output-def \$(PLUGIN_NAME).def --implib lib\$(PLUGIN_NAME).a --driver-name \$(CC)"
        ;;
      OS/2*)
        SHLIB_SUFFIX=".dll"
        SHLIB_LD="$CC -Zdll -Zcrtdll -Zmt"
        ;;
      *)
        AC_MSG_WARN(couldn't determine plugin flags for your operatins system: $ac_cv_system_version. Check to see that "dll/Makefile" is correct and that you can actually build plugins.)
        SHLIB_CFLAGS="-fPIC"
        SHLIB_LD="$CC -shared"
        SHLIB_SUFFIX=".so"
        ;;
    esac
  else
    AC_MSG_RESULT(not building with plugin support - you will not be able to use plugins)
  fi ])
