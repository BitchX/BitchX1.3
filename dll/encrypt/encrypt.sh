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

$GCC -I../../include -c encrypt.c

echo EXPORTS > encrypt.def
$NM encrypt.o ../init.o ../fixup.o | grep '^........ [T] _' | sed 's/[^_]*_//' >> encrypt.def

# Link DLL.
$LD --base-file encrypt.base --dll -o encrypt.dll encrypt.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname encrypt.dll --def encrypt.def --base-file\
 encrypt.base --output-exp encrypt.exp
$LD --base-file encrypt.base encrypt.exp --dll -o encrypt.dll encrypt.o\
 ../init.o ../fixup.o $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname encrypt.dll --def encrypt.def --base-file\
 encrypt.base --output-exp encrypt.exp
$LD encrypt.exp --dll -o encrypt.dll encrypt.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12

# Build the encryptB.a lib to link to:
$DLLTOOL --as=$AS --dllname encrypt.dll --def encrypt.def --output-lib encrypt.a

$RM *.base *.exp *.def
$CP *.dll ..
