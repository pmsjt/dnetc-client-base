/*
 * Copyright distributed.net 1997-2002 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
 *
 * $Id: gentests72.cpp,v 1.4 2002/09/25 02:15:46 acidblood Exp $
*/
/**************************************************************************/
/*                                                                        */
/* This code is used to provide test cases for RC5-32/12/9                */
/*     the RSA data security secret key challenge RC5-32/12/9             */
/*                                                                        */
/* The RSA pseudo-contest solution is one of the generated codes          */
/*                                                                        */
/* Written by Tim Charron (tcharron@interlog.com) October 21, 1997        */
/* Modified by D�cio Luiz Gazzoni Filho (acidblood@distributed.net)       */
/*                                                                        */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned long int WORD; /* should be 32 bit = 4 bytes   */
#define w 32			/* Word size in bits		*/
#define r 12			/* number of rounds 		*/
#define b 9			/* Number of Bytes in key  */
//#define c 4			/* Number of words in key */
#define c 3			/* Number of words in key -- upper(b/u), where u=bytes/word */
#define t 26			/* Size of table S=2*(r+1) words*/
WORD S[t];
WORD P=0xb7e15163, Q=0x9e3779b9; /* Magic Constants */

/* Rotation operators. x must be unsigned, to get logical right shift */
#define ROTL(x,y) (((x)<<(y&(w-1))) | ((x)>>(w-(y&(w-1)))))
#define ROTR(x,y) (((x)>>(y&(w-1))) | ((x)<<(w-(y&(w-1)))))

unsigned int Random(); // prototype

void RC5_ENCRYPT(WORD *pt, WORD *ct)    /* 2 WORD input pt/output ct  */
{
WORD i, A=pt[0]+S[0], B=pt[1]+S[1];
for (i=1; i<=r; i++)
  {A=ROTL(A^B,B)+S[2*i];
   B=ROTL(B^A,A)+S[2*i+1];
   }
ct[0]=A; ct[1]=B;
}
/* end of RC5_ENCRYPT */


void RC5_DECRYPT(WORD *ct, WORD *pt) /* 2 WORD input pt/output ct  */
{ WORD i, B=ct[1], A=ct[0];
for (i=r; i>0; i--)
  {B=ROTR(B-S[2*i+1],A)^A;
  A=ROTR(A-S[2*i],B)^B;
  }
pt[1] = B-S[1]; pt[0]=A-S[0];
/* End of RC5_DECRYPT */
}

void RC5_SETUP(unsigned char *K)  /* secret input key K[0...b-1]  */
{ WORD i, j, k, u=w/8, A, B, L[c];
/* Initialize L, then S, then mix key into S */
for (i=b-1, L[c-1]=0; i!=-1; i--) L[i/u]=(L[i/u]<<8)+K[i];
for (S[0]=P, i=1; i<t; i++) S[i] = S[i-1]+Q;
for (A=B=i=j=k=0; k<3*t; k++, i=(i+1)%t, j=(j+1)%c)    /* 3*t > 3*c  */
  {A=S[i]=ROTL(S[i]+(A+B),3);
  B=L[j]=ROTL(L[j]+(A+B),(A+B));
  }
/* End of RC5_SETUP  */
}


void printword(WORD A)
{ WORD k;
  for (k=0;k<w;k+=8)
    printf("%02.2X", (A>>k)&0xFF);
}

void printrevword(WORD A)
{ WORD k;
  printf("0x");
  for (k=0;k<w;k+=8)
    printf("%02.2X", (A>>(24-k))&0xFF);
}

#define TEST_CASE_COUNT 32

int main ()
{
   WORD j, pt[2], ct[2]={0,0};
   unsigned char key[b];
   WORD bigcipher[2],bigplain[2],iv[2];
   WORD seed;

   if (sizeof(WORD)!=4)
      printf("RC5 error: WORD has %d bytes. \n", sizeof(WORD));

   seed = 982; //(WORD) times(NULL);
   srand( seed ); // seed the rng so we always get the same results...

   printf("// RC5-32/12/9 test cases -- generated by gentests72.cpp:\n");
   printf("u32 rc5_72_test_cases[TEST_CASE_COUNT][9] = { // seed = %ld\n", seed);

   for (int testcase = 0; testcase < TEST_CASE_COUNT ; testcase ++) {
      if (testcase == 0) {
         key[0]=0xc9;
         key[1]=0x0c;
         key[2]=0x03;
         key[3]=0x53;
         key[4]=0xc0;
         key[5]=0xd4;
         key[6]=0xe1;
         key[7]=0xfe;
         key[8]=0x85;
         iv[0]=0x1f59ce07;
         iv[1]=0x419a1486;
         bigcipher[0]= 0x562d285a;
         bigcipher[1]= 0x2fb7852a;
      } else {
         key[0]=Random( ) & 0x000000FF;
         key[1]=Random( ) & 0x000000FF;
         key[2]=Random( ) & 0x000000FF;
         key[3]=Random( ) & 0x000000FF;
         key[4]=Random( ) & 0x000000FF;
         key[5]=Random( ) & 0x000000FF;
         key[6]=Random( ) & 0x000000FF;
         key[7]=Random( ) & 0x000000FF;
         key[8]=Random( ) & 0x000000FF;
	 switch (testcase) {
	     case 6: key[1] = 0x00;
	     case 5 :key[2] = 0x00;
	     case 4 :key[3] = 0x00;
	     case 3 :key[4] = 0x00;
	     case 2 :key[5] = 0x00; break;
	 }
         iv[0]=Random( ) & 0xFFFFFFFF;
         iv[1]=Random( ) & 0xFFFFFFFF;
         bigcipher[0]= Random( ) & 0xFFFFFFFF;
         bigcipher[1]= Random( ) & 0xFFFFFFFF;
      }

      printf("  {"); printrevword(key[8]);
         printf(","); printrevword((key[4]<<24) + (key[5]<<16) + (key[6]<<8) + key[7]);
         printf(","); printrevword((key[0]<<24) + (key[1]<<16) + (key[2]<<8) + key[3]); printf(",");
      printrevword(iv[0]); printf(","); printrevword(iv[1]); printf(",");

      // Setup the S / L values...
      RC5_SETUP(key);

      // Decrypt the wordpair...
      ct[0]=bigcipher[0];
      ct[1]=bigcipher[1];
      // decrypt (including the initial vector):
      RC5_DECRYPT( ct, pt );
      bigplain[0]=pt[0]^iv[0];
      bigplain[1]=pt[1]^iv[1];
      // Prior step's cipher becomes the xor vector for the next step (if we were doing more than 1 wordpair)...
      iv[0]=ct[0];
      iv[1]=ct[1];

      // Print the plaintext (hex)...
      printrevword(bigplain[0]);printf(",");printrevword(bigplain[1]);printf(",");
      // Print the ciphertext (hex)...
      printrevword(bigcipher[0]);printf(",");printrevword(bigcipher[1]);
      if (testcase != (TEST_CASE_COUNT-1)) {
         printf("},\n");
      } else {
         printf("}\n");
      }
   }
   printf("};\n");
   return 0;
}

unsigned int Random()
{
  unsigned int tmp;

  tmp = ( ( ( (unsigned int) rand() & 0xFF ) << 24 ) +
          ( ( (unsigned int) rand() & 0xFF ) << 8  ) +
          ( ( (unsigned int) rand() & 0xFF ) << 16 ) +
          ( ( (unsigned int) rand() & 0xFF ) << 0  ) );

  return( tmp );
}
