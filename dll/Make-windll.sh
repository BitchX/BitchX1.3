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

$GCC -c init.cc 
$GCC -c fixup.cc
$GCC -I../include -c pkga.c 

echo EXPORTS > pkga.def
$NM pkga.o init.o fixup.o | grep '^........ [T] _' | sed 's/[^_]*_//' >> pkga.def

# Link DLL.
$LD --base-file pkga.base --dll -o pkga.dll pkga.o init.o fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname pkga.dll --def pkga.def --base-file\
 pkga.base --output-exp pkga.exp
$LD --base-file pkga.base pkga.exp --dll -o pkga.dll pkga.o\
 init.o fixup.o $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname pkga.dll --def pkga.def --base-file\
 pkga.base --output-exp pkga.exp
$LD pkga.exp --dll -o pkga.dll pkga.o init.o fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12

# Build the pkgaB.a lib to link to:
$DLLTOOL --as=$AS --dllname pkga.dll --def pkga.def --output-lib pkga.a

$RM *.base *.def *.exp
(cd abot; sh ./abot.sh)
(cd acro; sh ./acro.sh)
(cd blowfish; sh ./blowfish.sh)
(cd possum; sh ./possum.sh)
(cd hint; sh ./hint.sh)
(cd cavlink; sh ./cavlink.sh)
(cd identd; sh ./identd.sh)
(cd encrypt; sh ./encrypt.sh)
(cd fserv; sh ./fserv.sh)
(cd nap; sh ./nap.sh)

