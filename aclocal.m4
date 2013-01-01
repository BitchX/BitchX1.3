

# Configure paths for AUDIOFILE
# Bertrand Guiheneuf 98-10-21
# stolen from esd.m4 in esound :
# Manish Singh    98-9-30
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_AUDIOFILE([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for AUDIOFILE, and define AUDIOFILE_CFLAGS and AUDIOFILE_LIBS
dnl
AC_DEFUN(AM_PATH_AUDIOFILE,
[dnl
dnl Get the cflags and libraries from the audiofile-config script
dnl
AC_ARG_WITH(audiofile-prefix,[  --with-audiofile-prefix=PFX
                          Prefix where AUDIOFILE is installed (optional)],
            audiofile_prefix="$withval", audiofile_prefix="")
AC_ARG_WITH(audiofile-exec-prefix,[  --with-audiofile-exec-prefix=PFX
                          Exec prefix where AUDIOFILE is installed (optional)],
            audiofile_exec_prefix="$withval", audiofile_exec_prefix="")
AC_ARG_ENABLE(audiofiletest, [  --disable-audiofiletest
                          Do not try to compile and run a test AUDIOFILE program],
		    , enable_audiofiletest=yes)

  if test x$audiofile_exec_prefix != x ; then
     audiofile_args="$audiofile_args --exec-prefix=$audiofile_exec_prefix"
     if test x${AUDIOFILE_CONFIG+set} != xset ; then
        AUDIOFILE_CONFIG=$audiofile_exec_prefix/bin/audiofile-config
     fi
  fi
  if test x$audiofile_prefix != x ; then
     audiofile_args="$audiofile_args --prefix=$audiofile_prefix"
     if test x${AUDIOFILE_CONFIG+set} != xset ; then
        AUDIOFILE_CONFIG=$audiofile_prefix/bin/audiofile-config
     fi
  fi

  AC_PATH_PROG(AUDIOFILE_CONFIG, audiofile-config, no)
  min_audiofile_version=ifelse([$1], ,0.2.5,$1)
  AC_MSG_CHECKING(for AUDIOFILE - version >= $min_audiofile_version)
  no_audiofile=""
  if test "$AUDIOFILE_CONFIG" = "no" ; then
    no_audiofile=yes
  else
    AUDIOFILE_LIBS=`$AUDIOFILE_CONFIG $audiofileconf_args --libs`
    AUDIOFILE_CFLAGS=`$AUDIOFILE_CONFIG $audiofileconf_args --cflags`
    audiofile_major_version=`$AUDIOFILE_CONFIG $audiofile_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    audiofile_minor_version=`$AUDIOFILE_CONFIG $audiofile_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    audiofile_micro_version=`$AUDIOFILE_CONFIG $audiofile_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_audiofiletest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $AUDIOFILE_CFLAGS"
      LIBS="$LIBS $AUDIOFILE_LIBS"
dnl
dnl Now check if the installed AUDIOFILE is sufficiently new. (Also sanity
dnl checks the results of audiofile-config to some extent
dnl
      rm -f conf.audiofiletest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <audiofile.h>

char*
my_strdup (char *str)
{
  char *new_str;

  if (str)
    {
      new_str = malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;

  return new_str;
}

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.audiofiletest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_audiofile_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_audiofile_version");
     exit(1);
   }

   if (($audiofile_major_version > major) ||
      (($audiofile_major_version == major) && ($audiofile_minor_version > minor)) ||
      (($audiofile_major_version == major) && ($audiofile_minor_version == minor) && ($audiofile_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'audiofile-config --version' returned %d.%d.%d, but the minimum version\n", $audiofile_major_version, $audiofile_minor_version, $audiofile_micro_version);
      printf("*** of AUDIOFILE required is %d.%d.%d. If audiofile-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If audiofile-config was wrong, set the environment variable AUDIOFILE_CONFIG\n");
      printf("*** to point to the correct copy of audiofile-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_audiofile=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_audiofile" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$AUDIOFILE_CONFIG" = "no" ; then
       echo "*** The audiofile-config script installed by AUDIOFILE could not be found"
       echo "*** If AUDIOFILE was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the AUDIOFILE_CONFIG environment variable to the"
       echo "*** full path to audiofile-config."
     else
       if test -f conf.audiofiletest ; then
        :
       else
          echo "*** Could not run AUDIOFILE test program, checking why..."
          CFLAGS="$CFLAGS $AUDIOFILE_CFLAGS"
          LIBS="$LIBS $AUDIOFILE_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <audiofile.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding AUDIOFILE or finding the wrong"
          echo "*** version of AUDIOFILE. If it is not finding AUDIOFILE, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means AUDIOFILE was incorrectly installed"
          echo "*** or that you have moved AUDIOFILE since it was installed. In the latter case, you"
          echo "*** may want to edit the audiofile-config script: $AUDIOFILE_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     AUDIOFILE_CFLAGS=""
     AUDIOFILE_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(AUDIOFILE_CFLAGS)
  AC_SUBST(AUDIOFILE_LIBS)
  rm -f conf.audiofiletest
])
dnl AC_CHKDECL(pretty, unpretty, regexp, header)
dnl Copyright (c) David Walluck 2000
dnl All rights reserved.

AC_DEFUN(AC_CHECK_DECLARATION, [
  AC_CACHE_CHECK(for $1 in $4, ac_cv_$2_declared, [
    AC_EGREP_HEADER($3, $4, ac_cv_$2_declared="yes", ac_cv_$2_declared="no",)
  ])
 if test x"$ac_cv_$2_declared" = x"yes"; then
   AC_DEFINE($5, 1, Define this if $2 is declared in $4.)
 fi
])
dnl AC_CHECK_VERSION(UPPER, lower, pretty, version)
dnl Copyright (c) David Walluck 1999-2000
dnl All rights reserved.
dnl Parts of this taken from mc configure script.
dnl Released under the GPLv2.

AC_DEFUN(AC_CHECK_VERSION,
[ AC_CACHE_CHECK(for $3 version, ac_cv_$3_version,
[ $1_version="unknown"
cat > conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
#include <$2.h>
#undef VERSION
VERSION:$1_VERSION
EOF
  if (eval "$ac_cpp conftest.$ac_ext") 2>&AS_MESSAGE_LOG_FD([]) | egrep "VERSION:" >conftest.out 2>&1; then
changequote(,)dnl
    $2_version="`cat conftest.out | sed -e 's/^[^"]*"//' -e 's/".*//'`"
changequote([,])dnl
  fi
  rm -rf conftest*
  ac_cv_$3_version="$$2_version"
])])
# Configure paths for ESD
# Manish Singh    98-9-30
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_ESD([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for ESD, and define ESD_CFLAGS and ESD_LIBS
dnl
AC_DEFUN(AM_PATH_ESD,
[dnl
dnl Get the cflags and libraries from the esd-config script
dnl
AC_ARG_WITH(esd-prefix,[  --with-esd-prefix=PFX   Prefix where ESD is installed (optional)],
            esd_prefix="$withval", esd_prefix="")
AC_ARG_WITH(esd-exec-prefix,[  --with-esd-exec-prefix=PFX
                          Exec prefix where ESD is installed (optional)],
            esd_exec_prefix="$withval", esd_exec_prefix="")
AC_ARG_ENABLE(esdtest, [  --disable-esdtest       Do not try to compile and run a test ESD program],
		    , enable_esdtest=yes)

  if test x$esd_exec_prefix != x ; then
     esd_args="$esd_args --exec-prefix=$esd_exec_prefix"
     if test x${ESD_CONFIG+set} != xset ; then
        ESD_CONFIG=$esd_exec_prefix/bin/esd-config
     fi
  fi
  if test x$esd_prefix != x ; then
     esd_args="$esd_args --prefix=$esd_prefix"
     if test x${ESD_CONFIG+set} != xset ; then
        ESD_CONFIG=$esd_prefix/bin/esd-config
     fi
  fi

  AC_PATH_PROG(ESD_CONFIG, esd-config, no)
  min_esd_version=ifelse([$1], ,0.2.7,$1)
  AC_MSG_CHECKING(for ESD - version >= $min_esd_version)
  no_esd=""
  if test "$ESD_CONFIG" = "no" ; then
    no_esd=yes
  else
    ESD_CFLAGS=`$ESD_CONFIG $esdconf_args --cflags`
    ESD_LIBS=`$ESD_CONFIG $esdconf_args --libs`

    esd_major_version=`$ESD_CONFIG $esd_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    esd_minor_version=`$ESD_CONFIG $esd_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    esd_micro_version=`$ESD_CONFIG $esd_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_esdtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $ESD_CFLAGS"
      LIBS="$LIBS $ESD_LIBS"
dnl
dnl Now check if the installed ESD is sufficiently new. (Also sanity
dnl checks the results of esd-config to some extent
dnl
      rm -f conf.esdtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esd.h>

char*
my_strdup (char *str)
{
  char *new_str;

  if (str)
    {
      new_str = malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;

  return new_str;
}

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.esdtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_esd_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_esd_version");
     exit(1);
   }

   if (($esd_major_version > major) ||
      (($esd_major_version == major) && ($esd_minor_version > minor)) ||
      (($esd_major_version == major) && ($esd_minor_version == minor) && ($esd_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'esd-config --version' returned %d.%d.%d, but the minimum version\n", $esd_major_version, $esd_minor_version, $esd_micro_version);
      printf("*** of ESD required is %d.%d.%d. If esd-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If esd-config was wrong, set the environment variable ESD_CONFIG\n");
      printf("*** to point to the correct copy of esd-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_esd=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_esd" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$ESD_CONFIG" = "no" ; then
       echo "*** The esd-config script installed by ESD could not be found"
       echo "*** If ESD was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the ESD_CONFIG environment variable to the"
       echo "*** full path to esd-config."
     else
       if test -f conf.esdtest ; then
        :
       else
          echo "*** Could not run ESD test program, checking why..."
          CFLAGS="$CFLAGS $ESD_CFLAGS"
          LIBS="$LIBS $ESD_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <esd.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding ESD or finding the wrong"
          echo "*** version of ESD. If it is not finding ESD, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means ESD was incorrectly installed"
          echo "*** or that you have moved ESD since it was installed. In the latter case, you"
          echo "*** may want to edit the esd-config script: $ESD_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     ESD_CFLAGS=""
     ESD_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(ESD_CFLAGS)
  AC_SUBST(ESD_LIBS)
  rm -f conf.esdtest
])
# Configure paths for GLIB
# Owen Taylor     97-11-3

dnl AM_PATH_GLIB([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GLIB, and define GLIB_CFLAGS and GLIB_LIBS, if "gmodule" or
dnl gthread is specified in MODULES, pass to glib-config
dnl
AC_DEFUN(AM_PATH_GLIB,
[dnl
dnl Get the cflags and libraries from the glib-config script
dnl
AC_ARG_WITH(glib-prefix,[  --with-glib-prefix=PFX  Prefix where GLIB is installed (optional)],
            glib_config_prefix="$withval", glib_config_prefix="")
AC_ARG_WITH(glib-exec-prefix,[  --with-glib-exec-prefix=PFX
                          Exec prefix where GLIB is installed (optional)],
            glib_config_exec_prefix="$withval", glib_config_exec_prefix="")
AC_ARG_ENABLE(glibtest, [  --disable-glibtest      Do not try to compile and run a test GLIB program],
		    , enable_glibtest=yes)

  if test x$glib_config_exec_prefix != x ; then
     glib_config_args="$glib_config_args --exec-prefix=$glib_config_exec_prefix"
     if test x${GLIB_CONFIG+set} != xset ; then
        GLIB_CONFIG=$glib_config_exec_prefix/bin/glib-config
     fi
  fi
  if test x$glib_config_prefix != x ; then
     glib_config_args="$glib_config_args --prefix=$glib_config_prefix"
     if test x${GLIB_CONFIG+set} != xset ; then
        GLIB_CONFIG=$glib_config_prefix/bin/glib-config
     fi
  fi

  for module in . $4
  do
      case "$module" in
         gmodule)
             glib_config_args="$glib_config_args gmodule"
         ;;
         gthread)
             glib_config_args="$glib_config_args gthread"
         ;;
      esac
  done

  AC_PATH_PROG(GLIB_CONFIG, glib-config, no)
  min_glib_version=ifelse([$1], ,0.99.7,$1)
  AC_MSG_CHECKING(for GLIB - version >= $min_glib_version)
  no_glib=""
  if test "$GLIB_CONFIG" = "no" ; then
    no_glib=yes
  else
    GLIB_CFLAGS=`$GLIB_CONFIG $glib_config_args --cflags`
    GLIB_LIBS=`$GLIB_CONFIG $glib_config_args --libs`
    glib_config_major_version=`$GLIB_CONFIG $glib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    glib_config_minor_version=`$GLIB_CONFIG $glib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    glib_config_micro_version=`$GLIB_CONFIG $glib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_glibtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GLIB_CFLAGS"
      LIBS="$LIBS $GLIB_LIBS"
dnl
dnl Now check if the installed GLIB is sufficiently new. (Also sanity
dnl checks the results of glib-config to some extent
dnl
      rm -f conf.glibtest
      AC_TRY_RUN([
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

int
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.glibtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_glib_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_glib_version");
     exit(1);
   }

  if ((glib_major_version != $glib_config_major_version) ||
      (glib_minor_version != $glib_config_minor_version) ||
      (glib_micro_version != $glib_config_micro_version))
    {
      printf("\n*** 'glib-config --version' returned %d.%d.%d, but GLIB (%d.%d.%d)\n",
             $glib_config_major_version, $glib_config_minor_version, $glib_config_micro_version,
             glib_major_version, glib_minor_version, glib_micro_version);
      printf ("*** was found! If glib-config was correct, then it is best\n");
      printf ("*** to remove the old version of GLIB. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If glib-config was wrong, set the environment variable GLIB_CONFIG\n");
      printf("*** to point to the correct copy of glib-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    }
  else if ((glib_major_version != GLIB_MAJOR_VERSION) ||
	   (glib_minor_version != GLIB_MINOR_VERSION) ||
           (glib_micro_version != GLIB_MICRO_VERSION))
    {
      printf("*** GLIB header files (version %d.%d.%d) do not match\n",
	     GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     glib_major_version, glib_minor_version, glib_micro_version);
    }
  else
    {
      if ((glib_major_version > major) ||
        ((glib_major_version == major) && (glib_minor_version > minor)) ||
        ((glib_major_version == major) && (glib_minor_version == minor) && (glib_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GLIB (%d.%d.%d) was found.\n",
               glib_major_version, glib_minor_version, glib_micro_version);
        printf("*** You need a version of GLIB newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GLIB is always available from ftp://ftp.gtk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the glib-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GLIB, but you can also set the GLIB_CONFIG environment to point to the\n");
        printf("*** correct copy of glib-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_glib=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_glib" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$GLIB_CONFIG" = "no" ; then
       echo "*** The glib-config script installed by GLIB could not be found"
       echo "*** If GLIB was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the GLIB_CONFIG environment variable to the"
       echo "*** full path to glib-config."
     else
       if test -f conf.glibtest ; then
        :
       else
          echo "*** Could not run GLIB test program, checking why..."
          CFLAGS="$CFLAGS $GLIB_CFLAGS"
          LIBS="$LIBS $GLIB_LIBS"
          AC_TRY_LINK([
#include <glib.h>
#include <stdio.h>
],      [ return ((glib_major_version) || (glib_minor_version) || (glib_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GLIB or finding the wrong"
          echo "*** version of GLIB. If it is not finding GLIB, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
          echo "***"
          echo "*** If you have a RedHat 5.0 system, you should remove the GTK package that"
          echo "*** came with the system with the command"
          echo "***"
          echo "***    rpm --erase --nodeps gtk gtk-devel" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GLIB was incorrectly installed"
          echo "*** or that you have moved GLIB since it was installed. In the latter case, you"
          echo "*** may want to edit the glib-config script: $GLIB_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GLIB_CFLAGS=""
     GLIB_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GLIB_CFLAGS)
  AC_SUBST(GLIB_LIBS)
  rm -f conf.glibtest
])
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
# Configure paths for GTK+
# Owen Taylor     97-11-3

dnl AM_PATH_GTK([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GTK, and define GTK_CFLAGS and GTK_LIBS
dnl
AC_DEFUN(AM_PATH_GTK,
[dnl
dnl Get the cflags and libraries from the gtk-config script
dnl
AC_ARG_WITH(gtk-prefix,[  --with-gtk-prefix=PFX   Prefix where GTK is installed (optional)],
            gtk_config_prefix="$withval", gtk_config_prefix="")
AC_ARG_WITH(gtk-exec-prefix,[  --with-gtk-exec-prefix=PFX
                          Exec prefix where GTK is installed (optional)],
            gtk_config_exec_prefix="$withval", gtk_config_exec_prefix="")
AC_ARG_ENABLE(gtktest, [  --disable-gtktest       Do not try to compile and run a test GTK program],
		    , enable_gtktest=yes)

  for module in . $4
  do
      case "$module" in
         gthread)
             gtk_config_args="$gtk_config_args gthread"
         ;;
      esac
  done

  if test x$gtk_config_exec_prefix != x ; then
     gtk_config_args="$gtk_config_args --exec-prefix=$gtk_config_exec_prefix"
     if test x${GTK_CONFIG+set} != xset ; then
        GTK_CONFIG=$gtk_config_exec_prefix/bin/gtk-config
     fi
  fi
  if test x$gtk_config_prefix != x ; then
     gtk_config_args="$gtk_config_args --prefix=$gtk_config_prefix"
     if test x${GTK_CONFIG+set} != xset ; then
        GTK_CONFIG=$gtk_config_prefix/bin/gtk-config
     fi
  fi

  AC_PATH_PROG(GTK_CONFIG, gtk-config, no)
  min_gtk_version=ifelse([$1], ,0.99.7,$1)
  AC_MSG_CHECKING(for GTK - version >= $min_gtk_version)
  no_gtk=""
  if test "$GTK_CONFIG" = "no" ; then
    no_gtk=yes
  else
    GTK_CFLAGS=`$GTK_CONFIG $gtk_config_args --cflags`
    GTK_LIBS=`$GTK_CONFIG $gtk_config_args --libs`
    gtk_config_major_version=`$GTK_CONFIG $gtk_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gtk_config_minor_version=`$GTK_CONFIG $gtk_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gtk_config_micro_version=`$GTK_CONFIG $gtk_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_gtktest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GTK_CFLAGS"
      LIBS="$LIBS $GTK_LIBS"
dnl
dnl Now check if the installed GTK is sufficiently new. (Also sanity
dnl checks the results of gtk-config to some extent
dnl
      rm -f conf.gtktest
      AC_TRY_RUN([
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

int
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.gtktest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_gtk_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_gtk_version");
     exit(1);
   }

  if ((gtk_major_version != $gtk_config_major_version) ||
      (gtk_minor_version != $gtk_config_minor_version) ||
      (gtk_micro_version != $gtk_config_micro_version))
    {
      printf("\n*** 'gtk-config --version' returned %d.%d.%d, but GTK+ (%d.%d.%d)\n",
             $gtk_config_major_version, $gtk_config_minor_version, $gtk_config_micro_version,
             gtk_major_version, gtk_minor_version, gtk_micro_version);
      printf ("*** was found! If gtk-config was correct, then it is best\n");
      printf ("*** to remove the old version of GTK+. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If gtk-config was wrong, set the environment variable GTK_CONFIG\n");
      printf("*** to point to the correct copy of gtk-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    }
#if defined (GTK_MAJOR_VERSION) && defined (GTK_MINOR_VERSION) && defined (GTK_MICRO_VERSION)
  else if ((gtk_major_version != GTK_MAJOR_VERSION) ||
	   (gtk_minor_version != GTK_MINOR_VERSION) ||
           (gtk_micro_version != GTK_MICRO_VERSION))
    {
      printf("*** GTK+ header files (version %d.%d.%d) do not match\n",
	     GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     gtk_major_version, gtk_minor_version, gtk_micro_version);
    }
#endif /* defined (GTK_MAJOR_VERSION) ... */
  else
    {
      if ((gtk_major_version > major) ||
        ((gtk_major_version == major) && (gtk_minor_version > minor)) ||
        ((gtk_major_version == major) && (gtk_minor_version == minor) && (gtk_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GTK+ (%d.%d.%d) was found.\n",
               gtk_major_version, gtk_minor_version, gtk_micro_version);
        printf("*** You need a version of GTK+ newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GTK+ is always available from ftp://ftp.gtk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the gtk-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GTK+, but you can also set the GTK_CONFIG environment to point to the\n");
        printf("*** correct copy of gtk-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_gtk=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_gtk" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$GTK_CONFIG" = "no" ; then
       echo "*** The gtk-config script installed by GTK could not be found"
       echo "*** If GTK was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the GTK_CONFIG environment variable to the"
       echo "*** full path to gtk-config."
     else
       if test -f conf.gtktest ; then
        :
       else
          echo "*** Could not run GTK test program, checking why..."
          CFLAGS="$CFLAGS $GTK_CFLAGS"
          LIBS="$LIBS $GTK_LIBS"
          AC_TRY_LINK([
#include <gtk/gtk.h>
#include <stdio.h>
],      [ return ((gtk_major_version) || (gtk_minor_version) || (gtk_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GTK or finding the wrong"
          echo "*** version of GTK. If it is not finding GTK, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
          echo "***"
          echo "*** If you have a RedHat 5.0 system, you should remove the GTK package that"
          echo "*** came with the system with the command"
          echo "***"
          echo "***    rpm --erase --nodeps gtk gtk-devel" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GTK was incorrectly installed"
          echo "*** or that you have moved GTK since it was installed. In the latter case, you"
          echo "*** may want to edit the gtk-config script: $GTK_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GTK_CFLAGS=""
     GTK_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GTK_CFLAGS)
  AC_SUBST(GTK_LIBS)
  rm -f conf.gtktest
])
# Configure paths for IMLIB
# Frank Belew     98-8-31
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_IMLIB([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for IMLIB, and define IMLIB_CFLAGS and IMLIB_LIBS
dnl
AC_DEFUN(AM_PATH_IMLIB,
[dnl
dnl Get the cflags and libraries from the imlib-config script
dnl
AC_ARG_WITH(imlib-prefix,[  --with-imlib-prefix=PFX Prefix where IMLIB is installed (optional)],
            imlib_prefix="$withval", imlib_prefix="")
AC_ARG_WITH(imlib-exec-prefix,[  --with-imlib-exec-prefix=PFX
                          Exec prefix where IMLIB is installed (optional)],
            imlib_exec_prefix="$withval", imlib_exec_prefix="")
AC_ARG_ENABLE(imlibtest, [  --disable-imlibtest    Do not try to compile and run a test IMLIB program],
		    , enable_imlibtest=yes)

  if test x$imlib_exec_prefix != x ; then
     imlib_args="$imlib_args --exec-prefix=$imlib_exec_prefix"
     if test x${IMLIB_CONFIG+set} != xset ; then
        IMLIB_CONFIG=$imlib_exec_prefix/bin/imlib-config
     fi
  fi
  if test x$imlib_prefix != x ; then
     imlib_args="$imlib_args --prefix=$imlib_prefix"
     if test x${IMLIB_CONFIG+set} != xset ; then
        IMLIB_CONFIG=$imlib_prefix/bin/imlib-config
     fi
  fi

  AC_PATH_PROG(IMLIB_CONFIG, imlib-config, no)
  min_imlib_version=ifelse([$1], ,1.9.5,$1)
  AC_MSG_CHECKING(for IMLIB - version >= $min_imlib_version)
  no_imlib=""
  if test "$IMLIB_CONFIG" = "no" ; then
    no_imlib=yes
  else
    IMLIB_CFLAGS=`$IMLIB_CONFIG $imlibconf_args --cflags`
    IMLIB_LIBS=`$IMLIB_CONFIG $imlibconf_args --libs`

    imlib_major_version=`$IMLIB_CONFIG $imlib_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    imlib_minor_version=`$IMLIB_CONFIG $imlib_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    imlib_micro_version=`$IMLIB_CONFIG $imlib_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_imlibtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $IMLIB_CFLAGS"
      LIBS="$LIBS $IMLIB_LIBS"
dnl
dnl Now check if the installed IMLIB is sufficiently new. (Also sanity
dnl checks the results of imlib-config to some extent
dnl
      rm -f conf.imlibtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Imlib.h>

char*
my_strdup (char *str)
{
  char *new_str;

  if (str)
    {
      new_str = malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;

  return new_str;
}

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.imlibtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_imlib_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_imlib_version");
     exit(1);
   }

    if (($imlib_major_version > major) ||
        (($imlib_major_version == major) && ($imlib_minor_version > minor)) ||
	(($imlib_major_version == major) && ($imlib_minor_version == minor) &&
	($imlib_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'imlib-config --version' returned %d.%d, but the minimum version\n", $imlib_major_version, $imlib_minor_version);
      printf("*** of IMLIB required is %d.%d. If imlib-config is correct, then it is\n", major, minor);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If imlib-config was wrong, set the environment variable IMLIB_CONFIG\n");
      printf("*** to point to the correct copy of imlib-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_imlib=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_imlib" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$IMLIB_CONFIG" = "no" ; then
       echo "*** The imlib-config script installed by IMLIB could not be found"
       echo "*** If IMLIB was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the IMLIB_CONFIG environment variable to the"
       echo "*** full path to imlib-config."
     else
       if test -f conf.imlibtest ; then
        :
       else
          echo "*** Could not run IMLIB test program, checking why..."
          CFLAGS="$CFLAGS $IMLIB_CFLAGS"
          LIBS="$LIBS $IMLIB_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <Imlib.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding IMLIB or finding the wrong"
          echo "*** version of IMLIB. If it is not finding IMLIB, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means IMLIB was incorrectly installed"
          echo "*** or that you have moved IMLIB since it was installed. In the latter case, you"
          echo "*** may want to edit the imlib-config script: $IMLIB_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     IMLIB_CFLAGS=""
     IMLIB_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(IMLIB_CFLAGS)
  AC_SUBST(IMLIB_LIBS)
  rm -f conf.imlibtest
])

# Check for gdk-imlib
AC_DEFUN(AM_PATH_GDK_IMLIB,
[dnl
dnl Get the cflags and libraries from the imlib-config script
dnl
AC_ARG_WITH(imlib-prefix,[  --with-imlib-prefix=PFX Prefix where IMLIB is installed (optional)],
            imlib_prefix="$withval", imlib_prefix="")
AC_ARG_WITH(imlib-exec-prefix,[  --with-imlib-exec-prefix=PFX
                          Exec prefix where IMLIB is installed (optional)],
            imlib_exec_prefix="$withval", imlib_exec_prefix="")
AC_ARG_ENABLE(imlibtest, [  --disable-imlibtest     Do not try to compile and run a test IMLIB program],
		    , enable_imlibtest=yes)

  if test x$imlib_exec_prefix != x ; then
     imlib_args="$imlib_args --exec-prefix=$imlib_exec_prefix"
     if test x${IMLIB_CONFIG+set} != xset ; then
        IMLIB_CONFIG=$imlib_exec_prefix/bin/imlib-config
     fi
  fi
  if test x$imlib_prefix != x ; then
     imlib_args="$imlib_args --prefix=$imlib_prefix"
     if test x${IMLIB_CONFIG+set} != xset ; then
        IMLIB_CONFIG=$imlib_prefix/bin/imlib-config
     fi
  fi

  AC_PATH_PROG(IMLIB_CONFIG, imlib-config, no)
  min_imlib_version=ifelse([$1], ,1.9.5,$1)
  AC_MSG_CHECKING(for IMLIB - version >= $min_imlib_version)
  no_imlib=""
  if test "$IMLIB_CONFIG" = "no" ; then
    no_imlib=yes
  else
    GDK_IMLIB_CFLAGS=`$IMLIB_CONFIG $imlibconf_args --cflags-gdk`
    GDK_IMLIB_LIBS=`$IMLIB_CONFIG $imlibconf_args --libs-gdk`

    imlib_major_version=`$IMLIB_CONFIG $imlib_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    imlib_minor_version=`$IMLIB_CONFIG $imlib_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    if test "x$enable_imlibtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GDK_IMLIB_CFLAGS"
      LIBS="$LIBS $GDK_IMLIB_LIBS"
dnl
dnl Now check if the installed IMLIB is sufficiently new. (Also sanity
dnl checks the results of imlib-config to some extent
dnl
      rm -f conf.imlibtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <gdk_imlib.h>

int main ()
{
  int major, minor;
  char *tmp_version;

  system ("touch conf.gdkimlibtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_imlib_version");
  if (sscanf(tmp_version, "%d.%d", &major, &minor) != 2) {
     printf("%s, bad version string\n", "$min_imlib_version");
     exit(1);
   }

    if (($imlib_major_version > major) ||
        (($imlib_major_version == major) && ($imlib_minor_version >= minor)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'imlib-config --version' returned %d.%d, but the minimum version\n", $imlib_major_version, $imlib_minor_version);
      printf("*** of IMLIB required is %d.%d. If imlib-config is correct, then it is\n", major, minor);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If imlib-config was wrong, set the environment variable IMLIB_CONFIG\n");
      printf("*** to point to the correct copy of imlib-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_imlib=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_imlib" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$IMLIB_CONFIG" = "no" ; then
       echo "*** The imlib-config script installed by IMLIB could not be found"
       echo "*** If IMLIB was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the IMLIB_CONFIG environment variable to the"
       echo "*** full path to imlib-config."
     else
       if test -f conf.gdkimlibtest ; then
        :
       else
          echo "*** Could not run IMLIB test program, checking why..."
          CFLAGS="$CFLAGS $GDK_IMLIB_CFLAGS"
          LIBS="$LIBS $GDK_IMLIB_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <gdk_imlib.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding IMLIB or finding the wrong"
          echo "*** version of IMLIB. If it is not finding IMLIB, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means IMLIB was incorrectly installed"
          echo "*** or that you have moved IMLIB since it was installed. In the latter case, you"
          echo "*** may want to edit the imlib-config script: $IMLIB_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     IMLIB_CFLAGS=""
     IMLIB_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GDK_IMLIB_CFLAGS)
  AC_SUBST(GDK_IMLIB_LIBS)
  rm -f conf.gdkimlibtest
])
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
#
# Check to make sure that the build environment is sane.
#

AC_DEFUN(AM_SANITY_CHECK,
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])
dnl AC_CHECK_SHLIB
dnl Copyright (c) David Walluck 1999
dnl All rights reserved.

AC_DEFUN(AC_CHECK_SHLIB,
[
  AC_CHECK_LIB(dl, dlopen, LIBS="-ldl $LIBS", [
    AC_CHECK_FUNC(dlopen,, [
      AC_CHECK_LIB(dld, dld_link, LIBS="-ldld $LIBS", [
        AC_CHECK_LIB(dld, shl_load, LIBS="$LIBS -ldld", AC_MSG_WARN(Cannot find dlopen.) [
        ])
      ])
    ])
  ])
])
# CFLAGS and library paths for XMMS
# written 15 December 1999 by Ben Gertzfield <che@debian.org>

dnl Usage:
dnl AM_PATH_XMMS([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl
dnl Example:
dnl AM_PATH_XMMS(0.9.5.1, , AC_MSG_ERROR([*** XMMS >= 0.9.5.1 not installed - please install first ***]))
dnl
dnl Defines XMMS_CFLAGS, XMMS_LIBS, XMMS_DATA_DIR, XMMS_PLUGIN_DIR, 
dnl XMMS_VISUALIZATION_PLUGIN_DIR, XMMS_INPUT_PLUGIN_DIR, 
dnl XMMS_OUTPUT_PLUGIN_DIR, XMMS_GENERAL_PLUGIN_DIR, XMMS_EFFECT_PLUGIN_DIR,
dnl and XMMS_VERSION for your plugin pleasure.
dnl

dnl XMMS_TEST_VERSION(AVAILABLE-VERSION, NEEDED-VERSION [, ACTION-IF-OKAY [, ACTION-IF-NOT-OKAY]])
AC_DEFUN(XMMS_TEST_VERSION, [

# Determine which version number is greater. Prints 2 to stdout if	
# the second number is greater, 1 if the first number is greater,	
# 0 if the numbers are equal.						
									
# Written 15 December 1999 by Ben Gertzfield <che@debian.org>		
# Revised 15 December 1999 by Jim Monty <monty@primenet.com>		
									
    AC_PROG_AWK
    xmms_got_version=[` $AWK '						\
BEGIN {									\
    print vercmp(ARGV[1], ARGV[2]);					\
}									\
									\
function vercmp(ver1, ver2,    ver1arr, ver2arr,			\
                               ver1len, ver2len,			\
                               ver1int, ver2int, len, i, p) {		\
									\
    ver1len = split(ver1, ver1arr, /\./);				\
    ver2len = split(ver2, ver2arr, /\./);				\
									\
    len = ver1len > ver2len ? ver1len : ver2len;			\
									\
    for (i = 1; i <= len; i++) {					\
        p = 1000 ^ (len - i);						\
        ver1int += ver1arr[i] * p;					\
        ver2int += ver2arr[i] * p;					\
    }									\
									\
    if (ver1int < ver2int)						\
        return 2;							\
    else if (ver1int > ver2int)						\
        return 1;							\
    else								\
        return 0;							\
}' $1 $2`]								

    if test $xmms_got_version -eq 2; then 	# failure
	ifelse([$4], , :, $4)			
    else  					# success!
	ifelse([$3], , :, $3)
    fi
])

AC_DEFUN(AM_PATH_XMMS,
[
AC_ARG_WITH(xmms-prefix,[  --with-xmms-prefix=PFX  Prefix where XMMS is installed (optional)],
	xmms_config_prefix="$withval", xmms_config_prefix="")
AC_ARG_WITH(xmms-exec-prefix,[  --with-xmms-exec-prefix=PFX Exec prefix where XMMS is installed (optional)],
	xmms_config_exec_prefix="$withval", xmms_config_exec_prefix="")

if test x$xmms_config_exec_prefix != x; then
    xmms_config_args="$xmms_config_args --exec-prefix=$xmms_config_exec_prefix"
    if test x${XMMS_CONFIG+set} != xset; then
	XMMS_CONFIG=$xmms_config_exec_prefix/bin/xmms-config
    fi
fi

if test x$xmms_config_prefix != x; then
    xmms_config_args="$xmms_config_args --prefix=$xmms_config_prefix"
    if test x${XMMS_CONFIG+set} != xset; then
  	XMMS_CONFIG=$xmms_config_prefix/bin/xmms-config
    fi
fi

AC_PATH_PROG(XMMS_CONFIG, xmms-config, no)
min_xmms_version=ifelse([$1], ,0.9.5.1, $1)

if test "$XMMS_CONFIG" = "no"; then
    no_xmms=yes
else
    XMMS_CFLAGS=`$XMMS_CONFIG $xmms_config_args --cflags`
    XMMS_LIBS=`$XMMS_CONFIG $xmms_config_args --libs`
    XMMS_VERSION=`$XMMS_CONFIG $xmms_config_args --version`
    XMMS_DATA_DIR=`$XMMS_CONFIG $xmms_config_args --data-dir`
    XMMS_PLUGIN_DIR=`$XMMS_CONFIG $xmms_config_args --plugin-dir`
    XMMS_VISUALIZATION_PLUGIN_DIR=`$XMMS_CONFIG $xmms_config_args \
					--visualization-plugin-dir`
    XMMS_INPUT_PLUGIN_DIR=`$XMMS_CONFIG $xmms_config_args --input-plugin-dir`
    XMMS_OUTPUT_PLUGIN_DIR=`$XMMS_CONFIG $xmms_config_args --output-plugin-dir`
    XMMS_EFFECT_PLUGIN_DIR=`$XMMS_CONFIG $xmms_config_args --effect-plugin-dir`
    XMMS_GENERAL_PLUGIN_DIR=`$XMMS_CONFIG $xmms_config_args --general-plugin-dir`

    XMMS_TEST_VERSION($XMMS_VERSION, $min_xmms_version, ,no_xmms=version)
fi

AC_MSG_CHECKING(for XMMS - version >= $min_xmms_version)

if test "x$no_xmms" = x; then
    AC_MSG_RESULT(yes)
    ifelse([$2], , :, [$2])
else
    AC_MSG_RESULT(no)

    if test "$XMMS_CONFIG" = "no" ; then
	echo "*** The xmms-config script installed by XMMS could not be found."
      	echo "*** If XMMS was installed in PREFIX, make sure PREFIX/bin is in"
	echo "*** your path, or set the XMMS_CONFIG environment variable to the"
	echo "*** full path to xmms-config."
    else
	if test "$no_xmms" = "version"; then
	    echo "*** An old version of XMMS, $XMMS_VERSION, was found."
	    echo "*** You need a version of XMMS newer than $min_xmms_version."
	    echo "*** The latest version of XMMS is always available from"
	    echo "*** http://www.xmms.org/"
	    echo "***"

            echo "*** If you have already installed a sufficiently new version, this error"
            echo "*** probably means that the wrong copy of the xmms-config shell script is"
            echo "*** being found. The easiest way to fix this is to remove the old version"
            echo "*** of XMMS, but you can also set the XMMS_CONFIG environment to point to the"
            echo "*** correct copy of xmms-config. (In this case, you will have to"
            echo "*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf"
            echo "*** so that the correct libraries are found at run-time)"
	fi
    fi
    XMMS_CFLAGS=""
    XMMS_LIBS=""
    ifelse([$3], , :, [$3])
fi
AC_SUBST(XMMS_CFLAGS)
AC_SUBST(XMMS_LIBS)
AC_SUBST(XMMS_VERSION)
AC_SUBST(XMMS_DATA_DIR)
AC_SUBST(XMMS_PLUGIN_DIR)
AC_SUBST(XMMS_VISUALIZATION_PLUGIN_DIR)
AC_SUBST(XMMS_INPUT_PLUGIN_DIR)
AC_SUBST(XMMS_OUTPUT_PLUGIN_DIR)
AC_SUBST(XMMS_GENERAL_PLUGIN_DIR)
AC_SUBST(XMMS_EFFECT_PLUGIN_DIR)
])
