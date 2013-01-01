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

$GCC -I../../include -c blowfish.c

echo EXPORTS > blowfish.def
$NM blowfish.o ../init.o ../fixup.o | grep '^........ [T] _' | sed 's/[^_]*_//' >> blowfish.def

# Link DLL.
$LD --base-file blowfish.base --dll -o blowfish.dll blowfish.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname blowfish.dll --def blowfish.def --base-file\
 blowfish.base --output-exp blowfish.exp
$LD --base-file blowfish.base blowfish.exp --dll -o blowfish.dll blowfish.o\
 ../init.o ../fixup.o $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname blowfish.dll --def blowfish.def --base-file\
 blowfish.base --output-exp blowfish.exp
$LD blowfish.exp --dll -o blowfish.dll blowfish.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12

# Build the blowfishB.a lib to link to:
$DLLTOOL --as=$AS --dllname blowfish.dll --def blowfish.def --output-lib blowfish.a

$RM *.base *.exp *.def
$CP *.dll ..
