dnl Configure paths for GNOME
dnl David Walluck   99-9-24

dnl AM_PATH_GNOME([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for GNOME, and define GNOME_CFLAGS and GNOME_LIBS
dnl
AC_DEFUN(AM_PATH_GNOME,
[dnl
dnl Get the cflags and libraries from the gnome-config script
dnl
AC_ARG_WITH(gnome-prefix,[  --with-gnome-prefix=PFX Prefix where GNOME is installed (optional)],
            gnome_prefix="$withval", gnome_prefix="")
AC_ARG_WITH(gnome-exec-prefix,[  --with-gnome-exec-prefix=PFX
                          Exec prefix where GNOME is installed (optional)],
            gnome_exec_prefix="$withval", gnome_exec_prefix="")
AC_ARG_ENABLE(gnometest, [  --disable-gnometest     Do not try to compile and run a test GNOME program],
		    , enable_gnometest=yes)

  if test x$gnome_exec_prefix != x ; then
     gnome_args="$gnome_args --exec-prefix=$gnome_exec_prefix"
     if test x${GNOME_CONFIG+set} != xset ; then
        GNOME_CONFIG=$gnome_exec_prefix/bin/gnome-config
     fi
  fi
  if test x$gnome_prefix != x ; then
     gnome_args="$gnome_args --prefix=$gnome_prefix"
     if test x${GNOME_CONFIG+set} != xset ; then
        GNOME_CONFIG=$gnome_prefix/bin/gnome-config
     fi
  fi

  AC_PATH_PROG(GNOME_CONFIG, gnome-config, no)
  min_gnome_version=ifelse([$1], ,0.2.7,$1)
  AC_MSG_CHECKING(for GNOME - version >= $min_gnome_version)
  no_gnome=""
  if test "$GNOME_CONFIG" = "no" ; then
    no_gnome=yes
  else
    GNOME_CFLAGS=`$GNOME_CONFIG $gnomeconf_args gnome --cflags`
    GNOME_LIBS=`$GNOME_CONFIG $gnomeconf_args gnome --libs`

    gnome_major_version=`$GNOME_CONFIG $gnome_args --version | \
           sed 's/gnome-libs //' | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gnome_minor_version=`$GNOME_CONFIG $gnome_args --version | \
           sed 's/gnome-libs //' | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gnome_micro_version=`$GNOME_CONFIG $gnome_config_args --version | \
           sed 's/gnome-libs //' | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_gnometest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GNOME_CFLAGS"
      LIBS="$LIBS $GNOME_LIBS"
dnl
dnl Now check if the installed GNOME is sufficiently new. (Also sanity
dnl checks the results of gnome-config to some extent
dnl
      rm -f conf.gnometest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <gnome.h>

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.gnometest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_gnome_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_gnome_version");
     exit(1);
   }

   if (($gnome_major_version > major) ||
      (($gnome_major_version == major) && ($gnome_minor_version > minor)) ||
      (($gnome_major_version == major) && ($gnome_minor_version == minor) && ($gnome_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'gnome-config --version' returned %d.%d.%d, but the minimum version\n", $gnome_major_version, $gnome_minor_version, $gnome_micro_version);
      printf("*** of GNOME required is %d.%d.%d. If gnome-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If gnome-config was wrong, set the environment variable GNOME_CONFIG\n");
      printf("*** to point to the correct copy of gnome-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_gnome=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_gnome" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$GNOME_CONFIG" = "no" ; then
       echo "*** The gnome-config script installed by GNOME could not be found"
       echo "*** If GNOME was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the GNOME_CONFIG environment variable to the"
       echo "*** full path to gnome-config."
     else
       if test -f conf.gnometest ; then
        :
       else
          echo "*** Could not run GNOME test program, checking why..."
          CFLAGS="$CFLAGS $GNOME_CFLAGS"
          LIBS="$LIBS $GNOME_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <gnome.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GNOME or finding the wrong"
          echo "*** version of GNOME. If it is not finding GNOME, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GNOME was incorrectly installed"
          echo "*** or that you have moved GNOME since it was installed. In the latter case, you"
          echo "*** may want to edit the gnome-config script: $GNOME_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GNOME_CFLAGS=""
     GNOME_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GNOME_CFLAGS)
  AC_SUBST(GNOME_LIBS)
  rm -f conf.gnometest
])
