// Copyright distributed.net 1997 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "problem.h"
#include "convcsc.h"

#include "csc-common.h"

#if (!defined(lint) && defined(__showids__))
const char * PASTE(csc_1key_driver_,CSC_SUFFIX) (void) {
return "@(#)$Id: csc-1key-driver.cpp,v 1.3.2.10 1999/12/22 01:05:30 trevorh Exp $"; }
#endif

// ------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
s32
PASTE(csc_unit_func_,CSC_SUFFIX)
( RC5UnitWork *unitwork, u32 *timeslice, void *membuff );
}
#endif

s32
PASTE(csc_unit_func_,CSC_SUFFIX)
( RC5UnitWork *unitwork, u32 *timeslice, void *membuff )
{
  // align buffer on a 16-byte boundary
  assert(sizeof(void*) == sizeof(unsigned long));
  char *membuffer = (char*)membuff;
  if( (unsigned long)membuffer & 15 != 0)
    membuffer = (char*)(((unsigned long)(membuffer+15) & ~((unsigned long)15)));

  //ulong key[2][64];
  ulong (*key)[2][64] = (ulong(*)[2][64])membuffer;
  membuffer += sizeof(*key);
  //ulong plain[64];
  ulong (*plain)[64] = (ulong(*)[64])membuffer;
  membuffer += sizeof(*plain);
  //ulong cipher[64];
  ulong (*cipher)[64] = (ulong(*)[64])membuffer;
  membuffer += sizeof(*cipher);

#ifdef CSC_BIT_32
  assert( sizeof(ulong) == 4);
#elif defined(CSC_BIT_64)
  assert( sizeof(ulong) == 8);
#endif

  // convert the starting key from incrementable format
  // to CSC format
  u32 keyhi = unitwork->L0.hi;
  u32 keylo = unitwork->L0.lo;
  convert_key_from_inc_to_csc (&keyhi, &keylo);

  // convert plaintext, cyphertext and key to slice mode
  u32 pp = unitwork->plain.lo;
  u32 cc = unitwork->cypher.lo;
  u32 kk = keylo;
  u32 mask = 1;
  {
  for (int i=0; i<64; i++) {
    (*plain)[i]  = (pp & mask) ? _1 : _0;
    (*cipher)[i] = (cc & mask) ? _1 : _0;
    (*key)[0][i] = (kk & mask) ? _1 : _0;
    if ((u32)(mask <<= 1) == 0) {
      pp = unitwork->plain.hi;
      cc = unitwork->cypher.hi;
      kk = keyhi;
      mask = 1;
    }
  }
  }
  memset( &(*key)[1], 0, sizeof((*key)[1]) );

#if defined( CSC_BIT_32 )
  (*key)[0][csc_bit_order[0]] = 0xAAAAAAAAul;
  (*key)[0][csc_bit_order[1]] = 0xCCCCCCCCul;
  (*key)[0][csc_bit_order[2]] = 0xF0F0F0F0ul;
  (*key)[0][csc_bit_order[3]] = 0xFF00FF00ul;
  (*key)[0][csc_bit_order[4]] = 0xFFFF0000ul;
  #define CSC_BITSLICER_BITS 5
#elif defined( CSC_BIT_64 )
  (*key)[0][csc_bit_order[0]] = CASTNUM64(0xAAAAAAAAAAAAAAAA);
  (*key)[0][csc_bit_order[1]] = CASTNUM64(0xCCCCCCCCCCCCCCCC);
  (*key)[0][csc_bit_order[2]] = CASTNUM64(0xF0F0F0F0F0F0F0F0);
  (*key)[0][csc_bit_order[3]] = CASTNUM64(0xFF00FF00FF00FF00);
  (*key)[0][csc_bit_order[4]] = CASTNUM64(0xFFFF0000FFFF0000);
  (*key)[0][csc_bit_order[5]] = CASTNUM64(0xFFFFFFFF00000000);
  #define CSC_BITSLICER_BITS 6
#endif

  u32 nbits = CSC_BITSLICER_BITS;
  while (*timeslice > (1ul << nbits)) nbits++;

  // Zero out all the bits that are to be varied
  {
  for (u32 i=0; i<nbits-CSC_BITSLICER_BITS; i++)
    (*key)[0][csc_bit_order[i+CSC_BITSLICER_BITS]] = 0;
  }

  // now we iterate, checking 32/64 keys each time
  // we call cscipher_bitslicer()
  ulong result;
  u32 bkey = 0;
  for( ;; ) {
    result = PASTE(cscipher_bitslicer_,CSC_SUFFIX) ( (unsigned long const (* )[64])*key, *plain, *cipher, membuffer );
    if( result )
      break;
    if( ++bkey >= (1ul << (nbits - CSC_BITSLICER_BITS) ) )
      break;
    // Update the key
    u32 i = 0;
    while( !(bkey & (1 << i)) ) i++;
    (*key)[0][csc_bit_order[i+CSC_BITSLICER_BITS]] ^= _1;
  }

  // have we found something ?
  if( result ) {

    // which one is the good key ?
    // search the first bit set to 1 in result
    int numkeyfound;
    for( numkeyfound=0; result != 1; numkeyfound++, result >>= 1);

    // convert winning key from slice mode to CSC format
    // (bits 0..7 should be set to zero)
    keylo = keyhi = 0;
    for( int j=8; j<64; j++ )
      if( j<32 )
	keylo |= (((*key)[0][j] >> numkeyfound) & 1) << j;
      else
	keyhi |= (((*key)[0][j] >> numkeyfound) & 1) << (j-32);

    // convert key from CSC format to incrementable format
    convert_key_from_csc_to_inc( &keyhi, &keylo );

    if( keylo < unitwork->L0.lo )
      *timeslice = unitwork->L0.lo - keylo;
    else
      *timeslice = keylo - unitwork->L0.lo;

    unitwork->L0.lo = keylo;
    unitwork->L0.hi = keyhi;

    return RESULT_FOUND;

  } else {

    *timeslice = (1ul << nbits);
    unitwork->L0.lo += 1 << nbits; // Increment lower 32 bits
    if (unitwork->L0.lo < (u32)(1 << nbits) )
      unitwork->L0.hi++; // Carry to high 32 bits if needed

    return RESULT_NOTHING;
  }
}
