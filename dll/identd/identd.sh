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

$GCC -I../../include -c identd.c

echo EXPORTS > identd.def
$NM identd.o ../init.o ../fixup.o | grep '^........ [T] _' | sed 's/[^_]*_//' >> identd.def

# Link DLL.
$LD --base-file identd.base --dll -o identd.dll identd.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname identd.dll --def identd.def --base-file\
 identd.base --output-exp identd.exp
$LD --base-file identd.base identd.exp --dll -o identd.dll identd.o\
 ../init.o ../fixup.o $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname identd.dll --def identd.def --base-file\
 identd.base --output-exp identd.exp
$LD identd.exp --dll -o identd.dll identd.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12

# Build the identdB.a lib to link to:
$DLLTOOL --as=$AS --dllname identd.dll --def identd.def --output-lib identd.a

$RM *.base *.exp *.def
$CP *.dll ..
