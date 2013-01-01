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

$GCC -I../../include -c autobot.c

echo EXPORTS > autobot.def
$NM autobot.o ../init.o ../fixup.o | grep '^........ [T] _' | sed 's/[^_]*_//' >> autobot.def

# Link DLL.
$LD --base-file autobot.base --dll -o autobot.dll autobot.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname autobot.dll --def autobot.def --base-file\
 autobot.base --output-exp autobot.exp
$LD --base-file autobot.base autobot.exp --dll -o autobot.dll autobot.o\
 ../init.o ../fixup.o $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname autobot.dll --def autobot.def --base-file\
 autobot.base --output-exp autobot.exp
$LD autobot.exp --dll -o autobot.dll autobot.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12

# Build the autobotB.a lib to link to:
$DLLTOOL --as=$AS --dllname autobot.dll --def autobot.def --output-lib autobot.a

$RM *.base *.exp *.def
$CP *.dll ..
