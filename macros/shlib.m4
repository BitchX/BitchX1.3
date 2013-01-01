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
