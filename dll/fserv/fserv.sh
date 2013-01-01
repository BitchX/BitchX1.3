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

$GCC -I../../include -c fserv.c

echo EXPORTS > fserv.def
$NM fserv.o ../init.o ../fixup.o | grep '^........ [T] _' | sed 's/[^_]*_//' >> fserv.def

# Link DLL.
$LD --base-file fserv.base --dll -o fserv.dll fserv.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname fserv.dll --def fserv.def --base-file\
 fserv.base --output-exp fserv.exp
$LD --base-file fserv.base fserv.exp --dll -o fserv.dll fserv.o\
 ../init.o ../fixup.o $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname fserv.dll --def fserv.def --base-file\
 fserv.base --output-exp fserv.exp
$LD fserv.exp --dll -o fserv.dll fserv.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12

# Build the fservB.a lib to link to:
$DLLTOOL --as=$AS --dllname fserv.dll --def fserv.def --output-lib fserv.a

$RM *.base *.exp *.def
$CP *.dll ..
