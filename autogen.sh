#!/bin/sh
# Run this to generate all the initial makefiles, etc.
# Modified for BitchX - modernized for current autotools (2024)
#
# Prerequisites:
#   autoconf >= 2.69
#   pkg-config (for OpenSSL and ncurses detection)
#
# Usage:
#   ./autogen.sh              # configure with defaults
#   ./autogen.sh --enable-ipv6 --with-ssl  # pass args to configure
#   NOCONFIGURE=1 ./autogen.sh  # only regenerate, don't configure

srcdir=$(dirname "$0")
test -z "$srcdir" && srcdir=.
PKG_NAME="BitchX"

DIE=0

# Prefer configure.ac over configure.in (modern convention)
if test -f "$srcdir/configure.ac"; then
  CONFIGURE_INPUT="configure.ac"
elif test -f "$srcdir/configure.in"; then
  CONFIGURE_INPUT="configure.in"
else
  echo "**Error**: No configure.ac or configure.in found in $srcdir"
  exit 1
fi
echo "Using $CONFIGURE_INPUT"

# Check for autoconf
(autoconf --version) </dev/null >/dev/null 2>&1 || {
  echo
  echo "**Error**: You must have 'autoconf' installed to compile $PKG_NAME."
  echo "Install: apt install autoconf (Debian/Ubuntu)"
  echo "         dnf install autoconf (Fedora/RHEL)"
  echo "         brew install autoconf (macOS)"
  DIE=1
}

# Check for autoheader (part of autoconf)
(autoheader --version) </dev/null >/dev/null 2>&1 || {
  echo
  echo "**Error**: 'autoheader' not found (should be part of autoconf)."
  DIE=1
}

# Check for pkg-config (optional but recommended)
(pkg-config --version) </dev/null >/dev/null 2>&1 || {
  echo
  echo "**Warning**: 'pkg-config' not found. SSL and ncurses detection"
  echo "will fall back to manual library checks."
  echo "Install: apt install pkg-config (Debian/Ubuntu)"
  echo "         dnf install pkgconf (Fedora/RHEL)"
  echo "         brew install pkg-config (macOS)"
}

if test "$DIE" -eq 1; then
  exit 1
fi

if test -z "$NOCONFIGURE" && test $# -eq 0; then
  echo "**Note**: Running 'configure' with default arguments."
  echo "Pass --help to see available options, or specify them on the"
  echo "'$0' command line."
  echo
fi

echo "Processing $srcdir..."

cd "$srcdir" || exit 1

# Include macros directory if it exists
aclocalinclude="$ACLOCAL_FLAGS"
if test -d macros; then
  aclocalinclude="$aclocalinclude -I macros"
fi

# Run aclocal if available (needed for pkg-config macros)
if (aclocal --version) </dev/null >/dev/null 2>&1; then
  echo "Running aclocal $aclocalinclude ..."
  aclocal $aclocalinclude 2>/dev/null || true
fi

# Run autoheader to generate include/defs.h.in
echo "Running autoheader..."
autoheader || {
  echo "**Warning**: autoheader failed (non-fatal, continuing)"
}

# Run autoconf to generate configure script
echo "Running autoconf..."
autoconf || {
  echo "**Error**: autoconf failed."
  exit 1
}

cd "$OLDPWD" || exit 1

# Run configure unless NOCONFIGURE is set
if test -z "$NOCONFIGURE"; then
  rm -f "$srcdir/config.cache"
  echo "Running $srcdir/configure $* ..."
  "$srcdir/configure" "$@"
else
  echo "Skipping configure (NOCONFIGURE is set)."
  echo "Run './configure' manually when ready."
fi
