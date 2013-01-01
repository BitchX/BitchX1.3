# Make .def file:
export LIBPATH=/usr/local/cygwin-new/i586-pc-cygwin/lib
export LD=/usr/local/cygwin-new/bin/i586-pc-cygwin-ld
export NM=/usr/local/cygwin-new/bin/i586-pc-cygwin-nm
export DLLTOOL=/usr/local/cygwin-new/bin/i586-pc-cygwin-dlltool
export AS=/usr/local/cygwin-new/bin/i586-pc-cygwin-as
export GCC=/usr/local/cygwin-new/bin/i586-pc-cygwin-gcc

$GCC -I../../include -c cavlink.c

echo EXPORTS > cavlink.def
$NM cavlink.o ../init.o ../fixup.o | grep '^........ [T] _' | sed 's/[^_]*_//' >> cavlink.def

# Link DLL.
$LD --base-file cavlink.base --dll -o cavlink.dll cavlink.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname cavlink.dll --def cavlink.def --base-file\
 cavlink.base --output-exp cavlink.exp
$LD --base-file cavlink.base cavlink.exp --dll -o cavlink.dll cavlink.o\
 ../init.o ../fixup.o $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12
$DLLTOOL --as=$AS --dllname cavlink.dll --def cavlink.def --base-file\
 cavlink.base --output-exp cavlink.exp
$LD cavlink.exp --dll -o cavlink.dll cavlink.o ../init.o ../fixup.o\
 $LIBPATH/libcygwin.a $LIBPATH/libkernel32.a -e _dll_entry@12

# Build the cavlinkB.a lib to link to:
$DLLTOOL --as=$AS --dllname cavlink.dll --def cavlink.def --output-lib cavlink.a
