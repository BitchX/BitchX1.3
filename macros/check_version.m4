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
