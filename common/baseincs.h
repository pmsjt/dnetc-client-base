// Hey, Emacs, this a -*-C++-*- file !

// Copyright distributed.net 1997-1998 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

// Synchronized with official 1.30

#ifndef __BASEINCS_H__
#define __BASEINCS_H__

#include "cputypes.h"

// --------------------------------------------------------------------------

#if ((CLIENT_OS == OS_AMIGAOS) || (CLIENT_OS == OS_RISCOS))
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>

#if ((CLIENT_OS == OS_AMIGAOS) || (CLIENT_OS == OS_RISCOS))
}
#endif

#if (CLIENT_OS == OS_IRIX)
  #include <limits.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/prctl.h>
  #include <sys/schedctl.h>
  #include <fcntl.h>
#elif (CLIENT_OS == OS_OS2)
  #include <sys/timeb.h>
  #include <conio.h>
  #include <share.h>
  #include <direct.h>
  #include <fcntl.h>
  #include <io.h>
  #include "platforms/os2cli/os2defs.h"
  #include "platforms/os2cli/dod.h"   // needs to be included after Client
  #include "lurk.h"
  #include "platforms/os2cli/os2inst.h" //-install/-uninstall functionality
  #ifndef QSV_NUMPROCESSORS       /* This is only defined in the SMP toolkit */
    #define QSV_NUMPROCESSORS     26
  #endif
#elif (CLIENT_OS == OS_AMIGAOS)
  #include <amiga/amiga.h>
  #include <unistd.h>
  #include <fcntl.h>
#elif (CLIENT_OS == OS_RISCOS)
  extern "C"
  {
    #include <sys/fcntl.h>
    #include <unistd.h>
    #include <stdarg.h>
    #include <machine/endian.h>
    #include <sys/time.h>
    #include <swis.h>
    extern unsigned int ARMident(), IOMDident();
    extern void riscos_clear_screen();
    extern bool riscos_check_taskwindow();
    extern int riscos_find_local_directory(const char *argv0);
    extern char *riscos_localise_filename(const char *filename);
    extern void riscos_upcall_6(void); //yield
    extern int getch();

    #define fileno(f) ((f)->__file)
    #define isatty(f) ((f) == 0)
  }
  extern s32 guiriscos, guirestart;
  extern bool riscos_in_taskwindow;
#elif (CLIENT_OS == OS_VMS)
  #include <fcntl.h>
  #include <types.h>
  #define unlink remove
#elif (CLIENT_OS == OS_SCO)
  #include <fcntl.h>
  #include <sys/time.h>
#elif (CLIENT_OS==OS_WIN32) || (CLIENT_OS==OS_WIN32S) || (CLIENT_OS==OS_WIN16)
  #if (CLIENT_OS == OS_WIN32) || !defined(__WINDOWS_386__)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <winsock.h>      // timeval
  #else
  #include <windows.h>
  #include "w32sock.h"
  #endif  #include <sys/timeb.h>
  #include <process.h>
  #include <conio.h>
  #include <share.h>
  #include <fcntl.h>
  #include <io.h>
  #include "lurk.h"
  #include "w32svc.h"       // service
  #include "w32cons.h"      // console
  #include "w32pre.h"       // prelude
#elif (CLIENT_OS == OS_DOS)
  #include <sys/timeb.h>
  #include <io.h>
  #include <conio.h>
  #include <share.h>
  #include <fcntl.h>
  #include <dos.h> //for drive functions in pathwork.cpp
  #include "platforms/dos/clidos.h"
  #if defined(__WATCOMC__)
    #include <direct.h> //getcwd
  #elif defined(__TURBOC__)
    #include <dir.h>
  #endif
#elif (CLIENT_OS == OS_BEOS)
// nothing  #include <share.h>
  #include <fcntl.h>
#elif (CLIENT_OS == OS_NETWARE)
  #include <sys/time.h> //timeval
  #include <unistd.h> //isatty, chdir, getcwd, access, unlink, chsize, O_...
  #include <conio.h> //ConsolePrintf(), clrscr()
  #include <share.h> //SH_DENYNO
  #include <nwfile.h> //sopen()
  #include <fcntl.h> //O_... constants
  #include "platforms/netware/netware.h" //for stuff in netware.cpp
#elif (CLIENT_OS == OS_SUNOS) || (CLIENT_OS == OS_SOLARIS)
  #include <fcntl.h>
  extern "C" int nice(int);
  extern "C" int gethostname(char *, int);
#elif (CLIENT_OS == OS_LINUX) || (CLIENT_OS == OS_FREEBSD) || (CLIENT_OS==OS_BSDI)
  #include <sys/time.h>
  #include <unistd.h>
  #if (((CLIENT_OS == OS_LINUX) && (__GLIBC__ >= 2)) || (CLIENT_OS==OS_FREEBSD) || (CLIENT_OS==OS_BSDI))
    #include <errno.h> // glibc2 has errno only here
  #endif
  #if ((CLIENT_OS == OS_LINUX) || (CLIENT_OS == OS_FREEBSD))
    #include <sched.h>
  #endif
#elif (CLIENT_OS == OS_NETBSD) && (CLIENT_CPU == CPU_ARM)
  #include <sys/time.h>
#elif (CLIENT_OS == OS_DYNIX)
  #include <unistd.h> // sleep(3c)
  struct timezone
  {
    int  tz_minuteswest;    /* of Greenwich */
    int  tz_dsttime;        /* type of dst correction to apply */
  };
  extern "C" int gethostname(char *, int);
  extern "C" int gettimeofday(struct timeval *, struct timezone *);
#endif

// --------------------------------------------------------------------------

#ifdef max
#undef max
#endif
#define max(a,b)            (((a) > (b)) ? (a) : (b))

#ifdef min
#undef min
#endif
#define min(a,b)            (((a) < (b)) ? (a) : (b))

// --------------------------------------------------------------------------

#endif //__BASEINCS_H__
