#!/bin/sh
# Make .def file:
export LIBPATH=/usr/local/cygwin-new/i586-pc-cygwin/lib
export LD=/usr/local/cygwin-new/bin/i586-pc-cygwin-ld
export NM=/usr/local/cygwin-new/bin/i586-pc-cygwin-nm
export DLLTOOL=/usr/local/cygwin-new/bin/i586-pc-cygwin-dlltool
export AS=/usr/local/cygwin-new/bin/i586-pc-cygwin-as
export GCC=/usr/local/cygwin-new/bin/i586-pc-cygwin-gcc
CP=cp
RM=rm

$GCC -I../../include -c nap.c
$GCC -I../../include -c md5.c
$GCC -I../../include -c nap_file.c
$GCC -I../../include -c napfunc.c
$GCC -I../../include -c napother.c
$GCC -I../../include -c napsend.c

echo EXPORTS > nap.def
$NM nap.o md5.o nap_file.o napfunc.o napother.o napsend.o ../init.o ../fixup.o | grep '^........ [T] _' | sed 's/[^_]*_//' >> nap.def

# Link DLL.
$LD --base-file nap.base --dll -o nap.dll nap.o md5.o nap_file.o napfunc.o napother.o napsend.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname nap.dll --def nap.def --base-file\
 nap.base --output-exp nap.exp
$LD --base-file nap.base nap.exp --dll -o nap.dll nap.o md5.o nap_file.o napfunc.o napother.o napsend.o \
 ../init.o ../fixup.o $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname nap.dll --def nap.def --base-file\
 nap.base --output-exp nap.exp
$LD nap.exp --dll -o nap.dll nap.o  md5.o nap_file.o napfunc.o napother.o napsend.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12

# Build the napB.a lib to link to:
$DLLTOOL --as=$AS --dllname nap.dll --def nap.def --output-lib nap.a

$RM *.base *.exp *.def
$CP *.dll ..
