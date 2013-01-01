#ifdef OS_AIX
  #include "audioIO_AIX.c"
#endif

#ifdef OS_Linux
  #include "audioIO_Linux.c"
#endif

#ifdef OS_BSD
  #include "audioIO_Linux.c"
#endif

#if defined(OS_IRIX) || defined(OS_IRIX64)
  #include "audioIO_IRIX.c"
#endif

#ifdef OS_HPUX
  #include "audioIO_HPUX.c"
#endif

#ifdef OS_SunOS
  #include "audioIO_SunOS.c"
#endif

#ifdef __BEOS__
  #include "audioIO_BeOS.c"
#endif
