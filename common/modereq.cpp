// Copyright distributed.net 1997-1998 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.
//
/* This file contains functions for getting/setting/clearing
   "mode" request from GUI menus and the like. Client::Run() 
   will clear/run the modes when appropriate.
*/    
//
// $Log: modereq.cpp,v $
// Revision 1.13  1998/12/08 05:48:59  dicamillo
// For MacOS GUI client, add calls to create and destroy benchmark display.
//
// Revision 1.12  1998/11/26 22:13:06  cyp
// client::WriteFullConfig() is now client::WriteConfig(1)
//
// Revision 1.11  1998/11/26 07:20:36  cyp
// Changed Fetch()/Flush()/Update() to use new BufferUpdate() method.
//
// Revision 1.10  1998/11/15 11:00:17  remi
// Moved client->SelectCore() for -test and -benchmark* from cmdline.cpp to
// modereq.cpp and told it to not be quiet.
//
// Revision 1.9  1998/11/10 23:01:28  silby
// Fixed a & that should've been a && - was breaking updates.
//
// Revision 1.8  1998/11/10 21:37:47  cyp
// added support for -forceunlock.
//
// Revision 1.7  1998/11/08 19:03:21  cyp
// -help (and invalid command line options) are now treated as "mode" requests.
//
// Revision 1.6  1998/11/08 01:01:47  silby
// Buncha hacks to get win32gui to compile, lots of cleanup to do.
//
// Revision 1.5  1998/11/03 16:08:31  cyp
// config mode changed so that it isn't affected by active command line options.
//
// Revision 1.4  1998/11/02 04:46:08  cyp
// Added check for user break after each mode is processed. Added code to
// automatically trip a restart after mode processing (for use with config).
//
// Revision 1.3  1998/10/11 02:45:20  cyp
// Added &= ~(MODEREQ_CONFIG) that I forgot.
//
// Revision 1.2  1998/10/11 00:40:11  cyp
// Added MODEREQ_CONFIG.
//
// Revision 1.1  1998/10/08 20:49:41  cyp
// Created.
//
#if (!defined(lint) && defined(__showids__))
const char *modereq_cpp(void) {
return "@(#)$Id: modereq.cpp,v 1.13 1998/12/08 05:48:59 dicamillo Exp $"; }
#endif

#include "client.h"   //client class
#include "triggers.h" //CheckExitRequestTrigger() [used by bench stuff]
#include "logstuff.h" //LogScreen() [used by update/fetch/flush stuff]
#include "modereq.h"  //our constants
#include "triggers.h" //RaiseRestartRequestTrigger/CheckExitRequestTriggerNoIO
#include "console.h"  //Clear the screen after config if restarting

#include "disphelp.h" //"mode" DisplayHelp()
#include "cpucheck.h" //"mode" DisplayProcessorInformation()
#include "cliident.h" //"mode" CliIdentifyModules();
#include "selftest.h" //"mode" SelfTest()
#include "bench.h"    //"mode" Benchmark()
#include "buffwork.h" //"mode" UnlockBuffer()
#include "buffupd.h"  //"mode" BufferUpdate() flags
#if (CLIENT_OS == OS_MACOS) && defined(MAC_GUI)
  #include "baseincs.h" // client-specific includes
#endif

/* --------------------------------------------------------------- */

static struct
{
  int isrunning;
  int reqbits;
  const char *filetounlock;
  const char *helpoption;
} modereq = {0,0,(const char *)0,(const char *)0};

/* --------------------------------------------------------------- */

int ModeReqSetArg(int mode, void *arg )
{
  if (mode == MODEREQ_UNLOCK)
    {
    ModeReqSet(MODEREQ_UNLOCK);
    modereq.filetounlock = (const char *)arg;
    return 0;
    }
  if (mode == MODEREQ_CMDLINE_HELP)
    {
    ModeReqSet(MODEREQ_CMDLINE_HELP);
    modereq.helpoption = (const char *)arg;
    return 0;
    }
  return -1;
}  
  
/* --------------------------------------------------------------- */

int ModeReqIsSet(int modemask)
{
  return ((modereq.reqbits & modemask) != 0);
}

/* --------------------------------------------------------------- */

int ModeReqSet(int modemask)
{
  if (modemask == -1)
    modemask = MODEREQ_ALL;
  int oldmask = (modereq.reqbits & modemask);
  modereq.reqbits |= modemask;
  return oldmask;
}

/* --------------------------------------------------------------- */

int ModeReqClear(int modemask)
{
  int oldmask;
  if (modemask == -1)
    {
    oldmask = modereq.reqbits;
    modereq.reqbits = 0;
    }
  else
    {
    modemask &= MODEREQ_ALL;
    oldmask = (modereq.reqbits & modemask);
    modereq.reqbits ^= (modereq.reqbits & modemask);
    }
  return oldmask;
}

/* --------------------------------------------------------------- */

int ModeReqIsRunning(void)
{
  return (modereq.isrunning != 0);
}

/* --------------------------------------------------------------- */

int ModeReqRun(Client *client)
{
  int retval = 0;
  
  if (++modereq.isrunning == 1)
    {
    int restart = ((modereq.reqbits & MODEREQ_RESTART)!=0);
    modereq.reqbits &= ~MODEREQ_RESTART;
    
    while ((modereq.reqbits & MODEREQ_ALL)!=0)
      {
      unsigned int bits = modereq.reqbits;
    
      if ((bits & (MODEREQ_BENCHMARK_DES | MODEREQ_BENCHMARK_RC5)) != 0)
        {
        if (client)
          {
          client->SelectCore( 0 /* not quietly */ );
          u32 benchsize = (1L<<23); /* long bench: 8388608 instead of 100000000 */
          if ((bits & (MODEREQ_BENCHMARK_QUICK))!=0)
            benchsize = (1L<<20); /* short bench: 1048576 instead of 10000000 */
          if ( !CheckExitRequestTriggerNoIO() && (bits&MODEREQ_BENCHMARK_RC5)!=0) {
          	#if (CLIENT_OS == OS_MACOS) && defined(MAC_GUI)
         	  MakeGUIThread(0, 0);
          	#endif

            Benchmark( 0, benchsize, client->cputype );

          	#if (CLIENT_OS == OS_MACOS) && defined(MAC_GUI)
         	  DestroyGUIThread(0);
          	#endif
            }
          if ( !CheckExitRequestTriggerNoIO() && (bits&MODEREQ_BENCHMARK_DES)!=0) {
          	#if (CLIENT_OS == OS_MACOS) && defined(MAC_GUI)
         	  MakeGUIThread(1, 0);
          	#endif

            Benchmark( 1, benchsize, client->cputype );

          	#if (CLIENT_OS == OS_MACOS) && defined(MAC_GUI)
         	  DestroyGUIThread(0);
          	#endif
            }
          }
        retval |= (modereq.reqbits & (MODEREQ_BENCHMARK_DES | 
                 MODEREQ_BENCHMARK_RC5 | MODEREQ_BENCHMARK_QUICK ));
        modereq.reqbits &= ~(MODEREQ_BENCHMARK_DES | 
                 MODEREQ_BENCHMARK_RC5 | MODEREQ_BENCHMARK_QUICK );
        }
      if ((bits & MODEREQ_CMDLINE_HELP) != 0)
        {
        DisplayHelp(modereq.helpoption);
        modereq.helpoption = (const char *)0;
        modereq.reqbits &= ~(MODEREQ_CMDLINE_HELP);        
        retval |= (MODEREQ_CMDLINE_HELP);
        }
      if ((bits & (MODEREQ_CONFIG | MODEREQ_CONFRESTART)) != 0)
        {
#if !((CLIENT_OS==OS_WIN32) && defined(NEEDVIRTUALMETHODS))
        // configure is awkward with the GUI at the moment
        Client *newclient = new Client;
        if (!newclient)
          LogScreen("Unable to configure. (Insufficient memory)");
        else
          {
          int i, nodestroy = 0;
          for (i=0;client->inifilename[i];i++)
            newclient->inifilename[i]=client->inifilename[i];
          newclient->inifilename[i]=0;  
          if ( newclient->ReadConfig() ) /* ini missing */
            {
            delete newclient;
            newclient = client;
            nodestroy = 1;
            }
          if ( newclient->Configure() == 1 )
            newclient->WriteConfig(1); //full new build
          if (!nodestroy)
            delete newclient;
          if ((bits & MODEREQ_CONFRESTART) != 0)
            restart = 1;
          retval |= (bits & (MODEREQ_CONFIG|MODEREQ_CONFRESTART));
          }
#endif
        modereq.reqbits &= ~(MODEREQ_CONFIG|MODEREQ_CONFRESTART);
        }
      if ((bits & (MODEREQ_FETCH | MODEREQ_FLUSH))!=0)
        {
        if (client)
          {
          int domode = 0;
          domode  = ((bits & (MODEREQ_FETCH))?(BUFFERUPDATE_FETCH):(0));
          domode |= ((bits & (MODEREQ_FLUSH))?(BUFFERUPDATE_FLUSH):(0));
          domode = client->BufferUpdate( domode, 1 /* verbose */ );
          if (domode & BUFFERUPDATE_FETCH)             retval|=MODEREQ_FETCH;
          if (domode & BUFFERUPDATE_FLUSH)             retval|=MODEREQ_FLUSH;
          if (domode!=0 && (bits & MODEREQ_FFORCE)!=0) retval|=MODEREQ_FFORCE;
          }
        modereq.reqbits &= ~(MODEREQ_FETCH | MODEREQ_FLUSH | MODEREQ_FFORCE);
        }
      if ((bits & MODEREQ_IDENT)!=0)    
        {
        CliIdentifyModules();
        modereq.reqbits &= ~(MODEREQ_IDENT);
        retval |= (MODEREQ_IDENT);
        }
      if ((bits & MODEREQ_UNLOCK)!=0)
        {
        if (modereq.filetounlock)
          {
          UnlockBuffer(modereq.filetounlock);
          modereq.filetounlock = (const char *)0;
          }
        modereq.reqbits &= ~(MODEREQ_UNLOCK);
        retval |= (MODEREQ_UNLOCK);
        }
      if ((bits & MODEREQ_CPUINFO)!=0)
        {
        DisplayProcessorInformation(); 
        modereq.reqbits &= ~(MODEREQ_CPUINFO);
        retval |= (MODEREQ_CPUINFO);
        }
      if ((bits & MODEREQ_TEST)!=0)
        {
        if (client)
          {
          client->SelectCore( 0 /* not quietly */ );
          if ( SelfTest(0, client->cputype ) > 0 ) 
            SelfTest(1, client->cputype );
          }
        retval |= (MODEREQ_TEST);
        modereq.reqbits &= ~(MODEREQ_TEST);
        }
      if (CheckExitRequestTriggerNoIO())
        {
        restart = 0;
        break;
        }
      } //end while
    
    if (restart)
      RaiseRestartRequestTrigger();
    } //if (++isrunning == 1)

  modereq.isrunning--;
  return retval;
}

