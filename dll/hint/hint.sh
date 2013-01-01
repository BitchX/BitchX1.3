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

$GCC -I../../include -c hint.c

echo EXPORTS > hint.def
$NM hint.o ../init.o ../fixup.o | grep '^........ [T] _' | sed 's/[^_]*_//' >> hint.def

# Link DLL.
$LD --base-file hint.base --dll -o hint.dll hint.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname hint.dll --def hint.def --base-file\
 hint.base --output-exp hint.exp
$LD --base-file hint.base hint.exp --dll -o hint.dll hint.o\
 ../init.o ../fixup.o $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname hint.dll --def hint.def --base-file\
 hint.base --output-exp hint.exp
$LD hint.exp --dll -o hint.dll hint.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12

# Build the hintB.a lib to link to:
$DLLTOOL --as=$AS --dllname hint.dll --def hint.def --output-lib hint.a
$RM *.base *.exp *.def
$CP *.dll ..
