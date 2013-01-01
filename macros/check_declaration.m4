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
