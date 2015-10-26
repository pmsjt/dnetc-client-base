/*
 * Copyright 2008-2010 Vyacheslav Chupyatov <goteam@mail.ru>
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
 *
 * Special thanks for help in testing this core to:
 * Alexander Kamashev, PanAm, Alexei Chupyatov
 *
*/

#include "r72stream-common.h"
#include "r72stream-vc4cng_il.cpp"
#include "r72stream-vc4_bitalign.cpp"

//rc5-72 test
#define P 0xB7E15163
#define Q 0x9E3779B9


#define SHL(x, s) ((u32) ((x) << ((s) & 31)))
#define SHR(x, s) ((u32) ((x) >> (32 - ((s) & 31))))
#define ROTL(x, s) ((u32) (SHL((x), (s)) | SHR((x), (s))))
#define ROTL3(x) ROTL(x, 3)

static s32 rc5_72_unit_func_ansi_ref (RC5_72UnitWork *rc5_72unitwork)
{
  u32 i, j, k;
  u32 A, B;
  u32 S[26];
  u32 L[3];
  u32 kiter = 1;
  while (kiter--)
  {
    L[2] = rc5_72unitwork->L0.hi;
    L[1] = rc5_72unitwork->L0.mid;
    L[0] = rc5_72unitwork->L0.lo;
    for (S[0] = P, i = 1; i < 26; i++)
      S[i] = S[i-1] + Q;
      
    for (A = B = i = j = k = 0;
         k < 3*26; k++, i = (i + 1) % 26, j = (j + 1) % 3)
    {
      A = S[i] = ROTL3(S[i]+(A+B));
      B = L[j] = ROTL(L[j]+(A+B),(A+B));
    }
    A = rc5_72unitwork->plain.lo + S[0];
    B = rc5_72unitwork->plain.hi + S[1];
    for (i=1; i<=12; i++)
    {
      A = ROTL(A^B,B)+S[2*i];
      B = ROTL(B^A,A)+S[2*i+1];
    }
    if (A == rc5_72unitwork->cypher.lo)
    {
      ++rc5_72unitwork->check.count;
      rc5_72unitwork->check.hi  = rc5_72unitwork->L0.hi;
      rc5_72unitwork->check.mid = rc5_72unitwork->L0.mid;
      rc5_72unitwork->check.lo  = rc5_72unitwork->L0.lo;
      if (B == rc5_72unitwork->cypher.hi)
      {
        return RESULT_FOUND;
      }
    }
  }
  return RESULT_NOTHING;
}


static bool init_rc5_72_il4_nand(stream_context_t *cont)
{
  if(cont->coreID!=CORE_IL4N)
    AMDStreamReinitializeDevice(cont);

  if(!cont->active)
  {
    Log("Thread %u: Device is not supported\n", cont->clientDeviceNo);
    return false;
  } else{
    switch(cont->attribs.target) {
    case CAL_TARGET_600:
      cont->domainSizeX=56;
      cont->domainSizeY=56;
      cont->maxIters=128;
      break;
    case CAL_TARGET_610:
      cont->domainSizeX=40;
      cont->domainSizeY=40;
      cont->maxIters=10;
      break;
    case CAL_TARGET_630:
      cont->domainSizeX=24;
      cont->domainSizeY=24;
      cont->maxIters=350;
      break;
    case CAL_TARGET_670:
      cont->domainSizeX=32;
      cont->domainSizeY=32;
      cont->maxIters=300;
      break;
    case CAL_TARGET_7XX:
      //TODO:domainSize
      break;
    case CAL_TARGET_770:
      cont->domainSizeX=600;
      cont->domainSizeY=600;
      cont->maxIters=5;
      break;
    case CAL_TARGET_710:
      //TODO:domainSize
      cont->domainSizeX=80;
      cont->domainSizeY=80;
      cont->maxIters=128;
      break;
    case CAL_TARGET_730:
      cont->domainSizeX=120;
      cont->domainSizeY=120;
      cont->maxIters=30;
      break;
    case 8: //RV870
      cont->domainSizeX=1024;
      cont->domainSizeY=1024;
      cont->maxIters=4;
      break;
    case 9: //RV840
      cont->domainSizeX=656;
      cont->domainSizeY=656;
      cont->maxIters=3;
      break;
    case 17://Barts
      cont->domainSizeX=904;
      cont->domainSizeY=904;
      cont->maxIters=3;
      break;
    case 20: //Tahiti
      cont->domainSizeX=768;
      cont->domainSizeY=768;
      cont->maxIters=1;
      break;
    default:
      cont->domainSizeX=512;
      cont->domainSizeY=512;
      cont->maxIters=1;
      break;
    }
  }

  CALresult result;
  result=calCtxCreate(&cont->ctx, cont->device);
  if (ati_verbose(result, "creating context", cont) != CAL_RESULT_OK)
    return false;

  cont->globalRes0=0;
  if(cont->attribs.target<20)
    if(cont->attribs.memExport) {
      calResAllocRemote2D(&cont->globalRes0, &cont->device, 1, 64,
                        1, CAL_FORMAT_UINT_1, CAL_RESALLOC_GLOBAL_BUFFER);
    }
  

  //-------------------------------------------------------------------------
  // Compiling Device Program
  //-------------------------------------------------------------------------
  result=compileProgram(&cont->ctx,&cont->image,&cont->module0,
                        (CALchar *)il4_bitalign_src, cont->attribs.target, (cont->globalRes0 != 0), cont);

  if ( result!= CAL_RESULT_OK)
    result=compileProgram(&cont->ctx,&cont->image,&cont->module0,
                        (CALchar *)il4_nand_src_g, cont->attribs.target, (cont->globalRes0 != 0), cont);

  if ( result!= CAL_RESULT_OK)
  {
    Log("Core compilation failed. Exiting.\n");
    return false;
  }

  //-------------------------------------------------------------------------
  // Allocating and initializing resources
  //-------------------------------------------------------------------------

  // Input and output resources
  cont->outputRes0=0;
  if(cont->attribs.cachedRemoteRAM>0)
    calResAllocRemote2D(&cont->outputRes0, &cont->device, 1, cont->domainSizeX,
                        cont->domainSizeY, CAL_FORMAT_UINT_1, CAL_RESALLOC_CACHEABLE);

  if(!cont->outputRes0) {
    if(calResAllocRemote2D(&cont->outputRes0, &cont->device, 1, cont->domainSizeX,
                           cont->domainSizeY, CAL_FORMAT_UINT_1, 0)!=CAL_RESULT_OK)
    {
      Log("Failed to allocate output buffer\n");
      return false;
    }
  }

  // Constant resource
  if(calResAllocRemote1D(&cont->constRes0, &cont->device, 1, 3, CAL_FORMAT_UINT_4, 0)!=CAL_RESULT_OK)
  {
    Log("Failed to allocate constants buffer\n");
    return false;
  }

  // Mapping output resource to CPU and initializing values
  // Getting memory handle from resources
  result=calCtxGetMem(&cont->outputMem0, cont->ctx, cont->outputRes0);
  if(result==CAL_RESULT_OK)
    result=calCtxGetMem(&cont->constMem0, cont->ctx, cont->constRes0);
  if(result!=CAL_RESULT_OK)
  {
    Log("Failed to map resources!\n");
    return false;
  }

  // Defining entry point for the module
  result=calModuleGetEntry(&cont->func0, cont->ctx, cont->module0, "main");
  if(result==CAL_RESULT_OK) {
    result=calModuleGetName(&cont->outName0, cont->ctx, cont->module0, "o0");
    if(result==CAL_RESULT_OK)
      result=calModuleGetName(&cont->constName0, cont->ctx, cont->module0, "cb0");
  }
  if(result!=CAL_RESULT_OK)
  {
    Log("Failed to get entry points!\n");
    return false;
  }

  if(cont->globalRes0) {
    result=calCtxGetMem(&cont->globalMem0, cont->ctx, cont->globalRes0);
    if(result==CAL_RESULT_OK) {
      result=calModuleGetName(&cont->globalName0, cont->ctx, cont->module0, "g[]");
      if(result==CAL_RESULT_OK)
        result=calCtxSetMem(cont->ctx, cont->globalName0, cont->globalMem0);
    }
    if(result!=CAL_RESULT_OK)
    {
      Log("Failed to allocate global buffer!\n");
      return false;
    }
  }

  // Setting input and output buffers
  // used in the kernel
  result=calCtxSetMem(cont->ctx, cont->outName0, cont->outputMem0);
  if(result==CAL_RESULT_OK)
    result=calCtxSetMem(cont->ctx, cont->constName0, cont->constMem0);
  if(result!=CAL_RESULT_OK)
  {
    Log("Failed to set buffers!\n");
    return false;
  }

  cont->USEcount=0;
  cont->coreID=CORE_IL4N;

  return true;
}

#ifdef __cplusplus
extern "C" s32 rc5_72_unit_func_il4_nand (RC5_72UnitWork *rc5_72unitwork, u32 *iterations, void *);
#endif

static bool FillConstantBuffer(CALresource res, u32 runsize, u32 iters, u32 rest, float width,
                               RC5_72UnitWork *rc5_72unitwork, u32 keyIncrement)
{
  u32* constPtr = NULL;
  CALuint pitch = 0;

  if(calResMap((CALvoid**)&constPtr, &pitch, res, 0)!=CAL_RESULT_OK)
    return false;

  u32 hi,mid,lo;
  hi=rc5_72unitwork->L0.hi;
  mid=rc5_72unitwork->L0.mid;
  lo=rc5_72unitwork->L0.lo;

  key_incr(&hi,&mid,&lo,keyIncrement*4);

  //cb0[0]					//key_hi,key_mid,key_lo,granularity
  constPtr[0]=hi;
  constPtr[1]=mid;
  constPtr[2]=lo;
  constPtr[3]=runsize*4;

  //cb0[1]					//plain_lo,plain_hi,cypher_lo,cypher_hi
  constPtr[4]=rc5_72unitwork->plain.lo;
  constPtr[5]=rc5_72unitwork->plain.hi;
  constPtr[6]=rc5_72unitwork->cypher.lo;
  constPtr[7]=rc5_72unitwork->cypher.hi;

  //cb0[2]					//iters,rest,width
  constPtr[8]=constPtr[11]=iters;
  constPtr[9]=rest;
  float *f;
  f=(float*)&constPtr[10]; *f=width;

  if(calResUnmap(res)!=CAL_RESULT_OK)
    return false;
  return true;
}

static s32 ReadResultsFromGPU(CALresource res, CALresource globalRes, u32 width, u32 height, RC5_72UnitWork *rc5_72unitwork, u32 *CMC, u32 *iters_done)
{
  u32 *o0, *g0;
  CALuint pitch = 0;
  bool found=true;

  if(globalRes) {
    CALuint result;
    if(calResMap((CALvoid**)&g0, &pitch, globalRes, 0)!=CAL_RESULT_OK)
      return -1;
    result=g0[0];
    g0[0]=0;
    if(calResUnmap(globalRes)!=CAL_RESULT_OK)
      return -1;
    if(result==0)
      found=false;
  }

  if(calResMap((CALvoid**)&o0, &pitch, res, 0)!=CAL_RESULT_OK) {
    return -1;
  }

  u32 last_CMC=0;
  *iters_done=(o0[0]&0x7e000000)>>25;
  if(found)
    for(u32 i=0; i<height; i++) {
      u32 idx=i*pitch;
      for(u32 j=0; j<width; j++) {
        if(o0[idx+j]&0x1ffffff)           //partial match
        {
          u32 output=o0[idx+j];
          u32 CMC_count=(output&0x1ffffff)>>18;
          u32 CMC_iter=(((output>>2)&0x0000ffff)-1)*width*height;
          u32 CMC_hit=(CMC_iter+i*width+j)*4+(output&0x00000003);

          // LogScreen("Partial match found\n");
          u32 hi,mid,lo;
          hi=rc5_72unitwork->L0.hi;
          mid=rc5_72unitwork->L0.mid;
          lo=rc5_72unitwork->L0.lo;

          key_incr(&hi,&mid,&lo,CMC_hit);
          if(last_CMC<=CMC_hit) {
            rc5_72unitwork->check.hi=hi;
            rc5_72unitwork->check.mid=mid;
            rc5_72unitwork->check.lo=lo;
            last_CMC=CMC_hit;
          }

          rc5_72unitwork->check.count+=CMC_count;

          if(output&0x80000000) {            //full match

            rc5_72unitwork->L0.hi=hi;
            rc5_72unitwork->L0.mid=mid;
            rc5_72unitwork->L0.lo=lo;

            calResUnmap(res);

            //full match verification
            RC5_72UnitWork t;
            memcpy(&t,rc5_72unitwork,sizeof(RC5_72UnitWork));
            if(rc5_72_unit_func_ansi_ref(&t)!=RESULT_FOUND)
            {
              Log("WARNING!!! False positive detected!\n");
              Log("Debug info: %x:%x:%x, res=%x\n",hi,mid,lo,output);
              RaiseExitRequestTrigger();
              return -1;
            }

            *CMC=CMC_hit;
            return 1;
          }
        }
      }
    }
  if(calResUnmap(res)!=CAL_RESULT_OK) {
    return -1;
  }
  return 0;
}

s32 rc5_72_unit_func_il4_nand(RC5_72UnitWork *rc5_72unitwork, u32 *iterations, void *)
{
  stream_context_t *cont = stream_get_context(rc5_72unitwork->devicenum);
  RC5_72UnitWork tmp_unit;

  if (cont == NULL)
  {
    RaiseExitRequestTrigger();
    return -1;
  }

  if (cont->coreID!=CORE_IL4N)
  {
    init_rc5_72_il4_nand(cont);
    if(cont->coreID!=CORE_IL4N) {
      RaiseExitRequestTrigger();
      return -1;
    }
  }

  if(checkRemoteConnectionFlag())
  {
    NonPolledUSleep(500*1000);  //sleep 0.5 sec
    *iterations=0;
    return RESULT_WORKING;
  }
  if(cont->coreID==CORE_NONE)
  {
    *iterations=0;
    return RESULT_WORKING;
  }

  memcpy(&tmp_unit, rc5_72unitwork, sizeof(RC5_72UnitWork));

  u32 kiter =(*iterations)/4;

  u32 itersNeeded=kiter;
  u32 width=cont->domainSizeX;
  u32 height=cont->domainSizeY;
  u32 RunSize=width*height;

  CALevent e0 = 0;
  u32 iters0=1;
  u32 rest0=0;

//#define VERBOSE 1

#ifdef VERBOSE
  LogScreen("Tread %u: %u ITERS (%u), maxiters=%u\n",deviceID, kiter,kiter/RunSize,cont->maxIters);
#endif
  double fr_d=HiresTimerGetResolution();
  hirestimer_type cstart, cend;

  //Clear global buffer
  if(cont->globalRes0) {
    u32* gPtr = NULL;
    CALuint pitch = 0;
    if(calResMap((CALvoid**)&gPtr, &pitch, cont->globalRes0, 0)==CAL_RESULT_OK)
    {
      gPtr[0]=0;
      calResUnmap(cont->globalRes0);
    }else
    {
      if(setRemoteConnectionFlag()) {
        *iterations=0;
        return RESULT_WORKING;
      }
      Log("Failed to map global buffer!\n");
      RaiseExitRequestTrigger();
      return -1;          //err
    }
  }

  CALresult result;
  while(itersNeeded) {
    //Make sure there is no overflow in core output
    if(cont->maxIters>65535)
      cont->maxIters=65535;

    iters0=itersNeeded/RunSize;
    if(iters0>=cont->maxIters) {
      iters0=cont->maxIters;
      rest0=RunSize;
    } else  {
      rest0=itersNeeded-iters0*RunSize;
      iters0++;
    }
    itersNeeded-=(iters0-1)*RunSize+rest0;

    //fill constant buffer
    if(!FillConstantBuffer(cont->constRes0,RunSize, iters0, rest0, (float)width, rc5_72unitwork,0))
    {
      if(setRemoteConnectionFlag()) {
        memcpy(rc5_72unitwork, &tmp_unit, sizeof(RC5_72UnitWork));
        *iterations=0;
        return RESULT_WORKING;
      }
      Log("Internal error!\n");
      RaiseExitRequestTrigger();
      return -1;          //err
    }

    CALdomain domain = {0, 0, width, height};
    result=calCtxRunProgram(&e0, cont->ctx, cont->func0, &domain);
    if((result!=CAL_RESULT_OK)&&(result!=CAL_RESULT_PENDING))
    {
      if(setRemoteConnectionFlag()) {
        memcpy(rc5_72unitwork, &tmp_unit, sizeof(RC5_72UnitWork));
        *iterations=0;
        return RESULT_WORKING;
      }
      Log("Error running GPU program\n");
      RaiseExitRequestTrigger();
      return -1;          //err
    }

    // Checking whether the execution of the program is complete or not
    HiresTimerGet(&cstart);

    CALresult result;
    if(isCalCtxWaitForEventsSupported)  //normal case
      result=calCtxWaitForEvents(cont->ctx, &e0, 1, 0);
    else
      while((result=calCtxIsEventDone(cont->ctx, e0)) == CAL_RESULT_PENDING) 
        NonPolledUSleep(15000);  //15ms

    if(result!=CAL_RESULT_OK)
    {
      if(setRemoteConnectionFlag()) {
        memcpy(rc5_72unitwork, &tmp_unit, sizeof(RC5_72UnitWork));
        *iterations=0;
        return RESULT_WORKING;
      }
      Log("Error while waiting for GPU program to finish!\n");
      RaiseExitRequestTrigger();
      return -1;          //err
    }
    HiresTimerGet(&cend);
    double d=HiresTimerDiff(cend, cstart)/fr_d;
    if(isCalCtxWaitForEventsSupported)
    {
      if(d>12.)
        if(cont->maxIters>1)
          cont->maxIters--;
      if(d<8.)
        cont->maxIters++;
    }else
    {
      if(d>16.5)
      {
        u32 delta;
        if(d>60.)
          delta=(u32)(cont->maxIters*0.3f);
        else
          delta=(u32)(cont->maxIters*0.1f);
        if(delta==0)
          delta=1;
        if(delta>=cont->maxIters)
          cont->maxIters=1;
        else
          cont->maxIters-=delta;
      }else
      if(d<16.)
      {
        u32 delta;
        delta=(u32)(cont->maxIters*0.02f);
        if(delta==0)
          delta=1;
        cont->maxIters+=delta;
      }
    }
    //Check the results
    u32 CMC, iters_finished;
    s32 read_res=ReadResultsFromGPU(cont->outputRes0, cont->globalRes0, width, height, rc5_72unitwork, &CMC, &iters_finished);
    if (read_res==1) {
      *iterations -= (kiter*4-CMC);
      return RESULT_FOUND;
    }
    if (read_res<0)
    {
      if(setRemoteConnectionFlag()) {
        memcpy(rc5_72unitwork, &tmp_unit, sizeof(RC5_72UnitWork));
        *iterations=0;
        return RESULT_WORKING;
      }
      Log("Internal error!\n");
      RaiseExitRequestTrigger();
      return -1;
    }
    if(iters_finished!=((iters0-(rest0==0))&0x3f) /*6 lower bits*/)
    {
      // Something bad happend during program execution
      if(cont->USEcount<15)
      {
        Log("Thread %d was terminated in unexpected way. Restarting after a pause...\n", cont->clientDeviceNo);
        NonPolledUSleep(1000000);  //1s
        memcpy(rc5_72unitwork, &tmp_unit, sizeof(RC5_72UnitWork));
        *iterations=0;
        cont->USEcount++;
        return RESULT_WORKING;
      }else
      {
        Log("Too many errors! Giving up.\n");
	Log("Debug info: Expected: %x, got:%x! Iters:%u MAXiters:%d rest:%u\n",iters0-(rest0==0),iters_finished,iters0,cont->maxIters,rest0);
        RaiseExitRequestTrigger();
        return -1;
      }
    }

    cont->USEcount=0;	//Reset Unexpected Stop Error counter
    unsigned itersDone=(iters0-1)*RunSize+rest0;
    kiter-=itersDone;
    key_incr(&rc5_72unitwork->L0.hi,&rc5_72unitwork->L0.mid,&rc5_72unitwork->L0.lo,itersDone*4);
  }

  /* tell the client about the optimal timeslice increment for this core
     (with current parameters) */
  rc5_72unitwork->optimal_timeslice_increment = RunSize*4*cont->maxIters;
  return RESULT_NOTHING;
}
