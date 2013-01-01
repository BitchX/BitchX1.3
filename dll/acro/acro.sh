#!/bin/sh
# Make .def file:
export LIBPATH=/usr/local/cygwin-new/i586-pc-cygwin/lib
export LD=/usr/local/cygwin-new/bin/i586-pc-cygwin-ld
export NM=/usr/local/cygwin-new/bin/i586-pc-cygwin-nm
export DLLTOOL=/usr/local/cygwin-new/bin/i586-pc-cygwin-dlltool
export AS=/usr/local/cygwin-new/bin/i586-pc-cygwin-as
export GCC=/usr/local/cygwin-new/bin/i586-pc-cygwin-gcc
RM=rm
CP=cp

$GCC -I../../include -c acro.c

echo EXPORTS > acro.def
$NM acro.o ../init.o ../fixup.o | grep '^........ [T] _' | sed 's/[^_]*_//' >> acro.def

# Link DLL.
$LD --base-file acro.base --dll -o acro.dll acro.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname acro.dll --def acro.def --base-file\
 acro.base --output-exp acro.exp
$LD --base-file acro.base acro.exp --dll -o acro.dll acro.o\
 ../init.o ../fixup.o $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname acro.dll --def acro.def --base-file\
 acro.base --output-exp acro.exp
$LD acro.exp --dll -o acro.dll acro.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12

# Build the acroB.a lib to link to:
$DLLTOOL --as=$AS --dllname acro.dll --def acro.def --output-lib acro.a
$RM *.base *.exp *.def
$CP *.dll ..
