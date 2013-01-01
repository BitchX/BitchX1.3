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

$GCC -I../../include -c head.c
$GCC -I../../include -c llist.c
$GCC -I../../include -c possum.c

echo EXPORTS > possum.def
$NM head.o llist.o possum.o ../init.o ../fixup.o | grep '^........ [T] _' | sed 's/[^_]*_//' >> possum.def

# Link DLL.
$LD --base-file possum.base --dll -o possum.dll head.o llist.o possum.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname possum.dll --def possum.def --base-file\
 possum.base --output-exp possum.exp
$LD --base-file possum.base possum.exp --dll -o possum.dll head.o llist.o possum.o\
 ../init.o ../fixup.o $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname possum.dll --def possum.def --base-file\
 possum.base --output-exp possum.exp
$LD possum.exp --dll -o possum.dll head.o llist.o possum.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12

# Build the possumB.a lib to link to:
$DLLTOOL --as=$AS --dllname possum.dll --def possum.def --output-lib possum.a

$RM *.base *.exp *.def
$CP *.dll ..
